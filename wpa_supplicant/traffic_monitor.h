/*
 * WPA Supplicant - traffic monitor
 * Copyright(c) 2013, Intel Corporation. All rights reserved.
 *
 * This software may be distributed under the terms of the BSD licence.
 * See README for more details.
 */

#ifndef TRAFFIC_MONITOR_H
#define TRAFFIC_MONITOR_H
#include "utils/os.h"
#include "wpa_supplicant_i.h"
#include "utils/traffic_stats.h"

/*
 * start_traffic_stats - Start sampling traffic statistics with specified
 * interval.
 * if already started the new request will be ignored and the previous request
 * will continue to execute.
 * @global: %wpa_supplicant global data structure
 * @sec: number of seconds in interval length
 * @usec: number of microseconds in interval length
 * Returns: 0 on success, -1 on failure, 1 if already started
 */
int start_traffic_stats(struct wpa_global *global, os_time_t sec,
			os_time_t usec);

/*
 * stop_traffic_stats - Stop sampling traffic statistics
 * @global: %wpa_supplicant global data structure
 */
void stop_traffic_stats(struct wpa_global *global);

/*
 * get_traffic_stats - Get overall bytes sent or recieved on all active
 * interfaces during the last sample interval that finished.
 * Interfaces or stations (in AP/GO mode) that disconnect before the call
 * to this function are disregarded, because the result should be an indicator
 * for future traffic. New connected interfaces and stations are included.
 * @global: %wpa_supplicant global data structure
 * @stats: buffer to return the stats
 * Returns: 0 on success, -1 on failure.
 */
int get_traffic_stats(struct wpa_global *global, struct traffic_stats *stats);

/*
 * traffic_stats_reset - reset bytes counters
 * @info: pointer to traffic_info to reset
 */
void traffic_stats_reset(struct traffic_info *info);
#endif
