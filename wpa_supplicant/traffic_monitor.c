/*
 * WPA Supplicant - traffic monitor
 * Copyright(c) 2013, Intel Corporation. All rights reserved.
 *
 * this software may be distributed under the terms of the BSD licence.
 * See README for more details.
 */

#include "includes.h"
#include "common.h"
#include "eloop.h"
#include "drivers/driver.h"
#include "wpa_supplicant_i.h"
#include "driver_i.h"
#include "ap/sta_info.h"
#include "ap/hostapd.h"
#include "utils/os.h"
#include "ap/ap_drv_ops.h"
#include "ibss_rsn.h"
#include "traffic_monitor.h"

/*
 * This function returns int because its used as a callback for
 * ap_for_each_sta. It always returns zero.
 */
static int update_sta_stats(struct hostapd_data *hapd, struct sta_info *sta,
			    void *ctx)
{
	struct hostap_sta_driver_data data;

	if (hostapd_drv_read_sta_data(hapd, &data, sta->addr)) {
		sta->traffic_data.updated = FALSE;
	} else {
		sta->traffic_data.rx_bytes = data.rx_bytes;
		sta->traffic_data.tx_bytes = data.tx_bytes;
		sta->traffic_data.updated = TRUE;
	}
	return 0;
}

static void update_ap_stats(struct hostapd_iface *hapd)
{
	size_t i;

	for (i = 0; i < hapd->num_bss; ++i)
		ap_for_each_sta(hapd->bss[i], update_sta_stats, NULL);
}

static void update_infra_stats(struct wpa_supplicant *wpa_s)
{
	struct hostap_sta_driver_data data;

	if (wpa_drv_pktcnt_poll(wpa_s, &data)) {
		wpa_s->traffic_data.updated = FALSE;
		return;
	}
	wpa_s->traffic_data.rx_bytes = data.rx_bytes;
	wpa_s->traffic_data.tx_bytes = data.tx_bytes;
	os_get_time(&wpa_s->traffic_data.last_check);
	wpa_s->traffic_data.updated = TRUE;
}

static void update_peer_stats(struct wpa_supplicant *wpa_s,
			     struct ibss_rsn_peer *peer)
{
	struct hostap_sta_driver_data data;

	if (!wpa_s->driver->read_sta_data)
		return;
	if (wpa_s->driver->read_sta_data(wpa_s->drv_priv, &data, peer->addr)) {
		peer->traffic_data.updated = FALSE;
		return;
	}
	peer->traffic_data.rx_bytes = data.rx_bytes;
	peer->traffic_data.tx_bytes = data.tx_bytes;
	peer->traffic_data.updated = TRUE;
}

static void update_ibss_stats(struct wpa_supplicant *wpa_s)
{
	struct ibss_rsn_peer *peer;

	if (!wpa_s->ibss_rsn)
		return;
	for (peer = wpa_s->ibss_rsn->peers; peer; peer = peer->next)
		update_peer_stats(wpa_s, peer);
}

static void update_iface_stats(struct wpa_supplicant *wpa_s)
{
	if (wpa_s->current_ssid == NULL)
		return;
	switch (wpa_s->current_ssid->mode) {
	case WPAS_MODE_INFRA:
		update_infra_stats(wpa_s);
		break;
	case WPAS_MODE_IBSS:
		update_ibss_stats(wpa_s);
		break;
	case WPAS_MODE_AP:
	case WPAS_MODE_P2P_GO:
		update_ap_stats(wpa_s->ap_iface);
		break;
	case WPAS_MODE_P2P_GROUP_FORMATION:
		break;
	}
}

/*
 * This function returns int because its used as a callback for
 * ap_for_each_sta. It always returns zero.
 */
static int get_sta_stats_and_update(struct hostapd_data *hapd,
				struct sta_info *sta, void *ctx)
{
	struct traffic_info last = sta->traffic_data;

	update_sta_stats(hapd, sta, NULL);
	if (sta->traffic_data.updated && last.updated)
		sta->traffic_data.bytes_over_interval =
			sta->traffic_data.rx_bytes - last.rx_bytes
			+ sta->traffic_data.tx_bytes - last.tx_bytes;
	return 0;
}

static void get_infra_stats_and_update(struct wpa_supplicant *wpa_s)
{
	struct traffic_info last = wpa_s->traffic_data;

	update_infra_stats(wpa_s);
	if (!wpa_s->traffic_data.updated || !last.updated)
		return;
	wpa_s->traffic_data.bytes_over_interval =
		wpa_s->traffic_data.rx_bytes - last.rx_bytes
		+ wpa_s->traffic_data.tx_bytes - last.tx_bytes;
}

static void get_peer_stats_and_update(struct wpa_supplicant *wpa_s,
				     struct ibss_rsn_peer *peer)
{
	struct traffic_info last = peer->traffic_data;

	update_peer_stats(wpa_s, peer);
	if (!peer->traffic_data.updated || !last.updated)
		return;
	peer->traffic_data.bytes_over_interval =
		peer->traffic_data.rx_bytes - last.rx_bytes
		+ peer->traffic_data.tx_bytes - last.tx_bytes;
}

static void get_ibss_stats_and_update(struct wpa_supplicant *wpa_s)
{
	struct ibss_rsn_peer *peer;

	if (!wpa_s->ibss_rsn)
		return;
	for (peer = wpa_s->ibss_rsn->peers; peer; peer = peer->next)
		get_peer_stats_and_update(wpa_s, peer);
	os_get_time(&wpa_s->traffic_data.last_check);
	wpa_s->traffic_data.updated = TRUE;
}

static void get_ap_stats_and_update(struct wpa_supplicant *wpa_s)
{
	size_t i;

	for (i = 0; i < wpa_s->ap_iface->num_bss; ++i)
		ap_for_each_sta(wpa_s->ap_iface->bss[i],
				get_sta_stats_and_update, NULL);
	os_get_time(&wpa_s->traffic_data.last_check);
	wpa_s->traffic_data.updated = TRUE;
}

static void get_iface_stats_and_update(struct wpa_supplicant *wpa_s)
{
	if (wpa_s->current_ssid == NULL)
		return;
	switch (wpa_s->current_ssid->mode) {
	case WPAS_MODE_INFRA:
		get_infra_stats_and_update(wpa_s);
		break;
	case WPAS_MODE_IBSS:
		get_ibss_stats_and_update(wpa_s);
		break;
	case WPAS_MODE_AP:
	case WPAS_MODE_P2P_GO:
		get_ap_stats_and_update(wpa_s);
		break;
	case WPAS_MODE_P2P_GROUP_FORMATION:
		break;
	}
}

/*
 * This function returns int because its used as a callback for
 * ap_for_each_sta. It always returns zero.
 */
static int add_sta_bytes(struct hostapd_data *hapd,
			  struct sta_info *sta, void *ctx)
{
	unsigned long *total = ctx;

	if (sta->traffic_data.updated)
		*total += sta->traffic_data.bytes_over_interval;
	return 0;
}

static unsigned long get_ap_bytes(struct wpa_supplicant *iface)
{
	size_t i;
	unsigned long total = 0;

	for (i = 0; i < iface->ap_iface->num_bss; ++i)
		ap_for_each_sta(iface->ap_iface->bss[i], add_sta_bytes,
				&total);
	return total;
}

static unsigned long get_ibss_bytes(struct wpa_supplicant *wpa_s)
{
	struct ibss_rsn_peer *peer;
	unsigned long total = 0;

	if (!wpa_s->ibss_rsn)
		return 0;
	for (peer = wpa_s->ibss_rsn->peers; peer; peer = peer->next)
		if (peer->traffic_data.updated == TRUE)
			total += peer->traffic_data.bytes_over_interval;
	return total;
}

static void traffic_stats_timeout(void *eloop_ctx, void *timeout_ctx)
{
	struct wpa_supplicant *iface;
	struct wpa_global *global = eloop_ctx;

	if (!global)
		return;
	for (iface = global->ifaces; iface; iface = iface->next)
		get_iface_stats_and_update(iface);

	eloop_register_timeout(global->traffic_sample_interval.sec,
			       global->traffic_sample_interval.usec,
			       traffic_stats_timeout, global, NULL);
}

int start_traffic_stats(struct wpa_global *global, os_time_t sec,
			os_time_t usec)
{
	struct wpa_supplicant *iface;

	if (!global || (sec == 0 && usec == 0))
		return -1;
	if (eloop_is_timeout_registered(traffic_stats_timeout, global, NULL))
		return 1;
	global->traffic_sample_interval.sec = sec;
	global->traffic_sample_interval.usec = usec;
	for (iface = global->ifaces; iface; iface = iface->next)
		update_iface_stats(iface);
	eloop_register_timeout(sec, usec, traffic_stats_timeout, global, NULL);
	return 0;
}

void stop_traffic_stats(struct wpa_global *global)
{
	eloop_cancel_timeout(traffic_stats_timeout, global, NULL);
}

int get_traffic_stats(struct wpa_global *global, struct traffic_stats *stats)
{
	struct wpa_supplicant *iface;

	if (!eloop_is_timeout_registered(traffic_stats_timeout, global, NULL))
		return -1;
	stats->bytes_over_interval = 0;
	for (iface = global->ifaces; iface; iface = iface->next) {
		if (iface->current_ssid == NULL ||
		    !iface->traffic_data.updated)
			continue;
		stats->timestamp = iface->traffic_data.last_check;
		switch (iface->current_ssid->mode) {
		case WPAS_MODE_INFRA:
			stats->bytes_over_interval += iface->
				traffic_data.bytes_over_interval;
			break;
		case WPAS_MODE_IBSS:
			stats->bytes_over_interval += get_ibss_bytes(iface);
			break;
		case WPAS_MODE_AP:
		case WPAS_MODE_P2P_GO:
			stats->bytes_over_interval += get_ap_bytes(iface);
			break;
		case WPAS_MODE_P2P_GROUP_FORMATION:
			break;
		}
	}
	return 0;
}

void traffic_stats_reset(struct traffic_info *info)
{
	os_memset(info, 0, sizeof(struct traffic_info));
	os_get_time(&info->last_check);
	info->updated = TRUE;
}
