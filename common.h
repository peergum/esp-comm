/**
 * @file common.h
 * @author Phil Hilger (phil@peergum.com)
 * @brief 
 * @version 0.1
 * @date 2022-05-31
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#ifndef __COMMON_H
#define __COMMON_H

#ifndef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif

#define FAT_MOUNT_POINT "/fat"
#define WIFI_SCAN_LIST_SIZE 16
#define WIFI_MAXIMUM_RETRY 3
#define WIFI_MAX_AP_CONNECTIONS 4

#endif // __COMMON_H