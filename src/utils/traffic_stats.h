/*
 * WPA Supplicant - traffic monitor
 * Copyright(c) 2013, Intel Corporation. All rights reserved.
 *
 * This software may be distributed under the terms of the BSD licence.
 * See README for more details.
 */

#ifndef TRAFFIC_STATS_H
#define TRAFFIC_STATS_H

#include "os.h"
#include "common/defs.h"

/*
 * struct traffic_info - traffic data for traffic monitor
 */
struct traffic_info {
	unsigned long rx_bytes, tx_bytes;
	unsigned long bytes_over_interval;
	struct os_time last_check;
	Boolean updated;	/* TRUE if  this info is valid */
};

/*
 * struct traffic_stats - traffic statistics collected by traffic monitor
 */
struct traffic_stats {
	unsigned long bytes_over_interval;
	struct os_time timestamp;
};
#endif
