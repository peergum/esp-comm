/**
 * @file EspNow.h
 * @author Phil Hilger (phil@peergum.com)
 * @brief 
 * @version 0.1
 * @date 2023-03-02
 * 
 * CAN-talk. A library for microcontrollers that allows decent comms
 * over a CAN bus.
 * 
 * Copyright (C) 2023, PeerGum
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https: //www.gnu.org/licenses/>.
 * 
 */

#ifndef __ESPNOW_H_
#define __ESPNOW_H_

#include "stdbool.h"
#include "stdint.h"
#include "esp_now.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "Config.h"

#define ESPNOW_MAXDELAY 512
#define REMOTE_USB_ESPNOW_ID "RemoteUSB"

#define IS_BROADCAST_ADDR(addr) \
  (memcmp(addr, s_broadcast_mac, ESP_NOW_ETH_ALEN) == 0)

/* ESPNOW can work in both station and softap mode. It is configured in
 * menuconfig. */
#if CONFIG_ESPNOW_WIFI_MODE_STATION
#define ESPNOW_WIFI_MODE WIFI_MODE_STA
#define ESPNOW_WIFI_IF WIFI_IF_STA
#else
#define ESPNOW_WIFI_MODE WIFI_MODE_APSTA
#define ESPNOW_WIFI_IF WIFI_IF_AP
#endif

#define ESPNOW_QUEUE_SIZE 6

typedef enum {
  ESPNOW_SEND_CB,
  ESPNOW_RECV_CB,
} espnow_event_id_t;

typedef struct {
  uint8_t mac_addr[ESP_NOW_ETH_ALEN];
  esp_now_send_status_t status;
} espnow_event_send_cb_t;

typedef struct {
  uint8_t mac_addr[ESP_NOW_ETH_ALEN];
  uint8_t *data;
  int data_len;
} espnow_event_recv_cb_t;

typedef union {
  espnow_event_send_cb_t send_cb;
  espnow_event_recv_cb_t recv_cb;
} espnow_event_info_t;

/* When ESPNOW sending or receiving callback function is called, post event to
 * ESPNOW task. */
typedef struct {
  espnow_event_id_t id;
  espnow_event_info_t info;
} espnow_event_t;

enum {
  ESPNOW_DATA_BROADCAST,
  ESPNOW_DATA_UNICAST,
  ESPNOW_DATA_MAX,
};

/* User defined field of ESPNOW data in this example. */
typedef struct {
  uint8_t type;   // Broadcast or unicast ESPNOW data.
  uint8_t state;  // Indicate that if has received broadcast ESPNOW data or not.
  uint16_t seq_num;  // Sequence number of ESPNOW data.
  uint16_t crc;      // CRC16 value of ESPNOW data.
  uint32_t magic;    // Magic number which is used to determine which device to
                     // send unicast ESPNOW data.
  uint8_t ssid[32];  /**< SSID of target AP. */
  uint8_t password[64]; /**< Password of target AP. */
} __attribute__((packed)) espnow_data_t;

/* Parameters of sending ESPNOW data. */
typedef struct {
  bool unicast;    // Send unicast ESPNOW data.
  bool broadcast;  // Send broadcast ESPNOW data.
  uint8_t state;  // Indicate that if has received broadcast ESPNOW data or not.
  uint32_t magic;   // Magic number which is used to determine which device to
                    // send unicast ESPNOW data.
  uint16_t count;   // Total count of unicast ESPNOW data to be sent.
  uint16_t delay;   // Delay between sending two ESPNOW data, unit: ms.
  int len;          // Length of ESPNOW data to be sent, unit: byte.
  uint8_t *buffer;  // Buffer pointing to ESPNOW data.
  uint8_t dest_mac[ESP_NOW_ETH_ALEN];  // MAC address of destination device.
} espnow_send_param_t;

class EspNow {
public:
  EspNow();
  ~EspNow();
  static void sendCallback(const uint8_t *mac_addr, esp_now_send_status_t status);
  static void receiveCallback(const uint8_t *mac_addr,
                              const uint8_t *data, int len);
  static int data_parse(uint8_t *data, uint16_t data_len, uint8_t *state,
                        uint16_t *seq, int *magic);
  static void data_prepare(espnow_send_param_t *send_param);
  static void task(void *pvParameter);
  static void deinit(espnow_send_param_t *send_param);
  EspNow &instance();
  esp_err_t init(Config *config);

 private:
  static EspNow *_instance;
  Config *config;
  QueueHandle_t s_queue;
  static uint8_t s_broadcast_mac[ESP_NOW_ETH_ALEN];
  static uint16_t s_espnow_seq[ESPNOW_DATA_MAX];
};

#endif  // __ESPNOW_H_