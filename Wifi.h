/**
 * @file Wifi.h
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

#ifndef __WIFI_H_
#define __WIFI_H_

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "IPAddress.h"
#include "SoftTimer.h"
#include "common.h"
#include "utils.h"
#include "UPnP.h"
#include "EspNow.h"

#define WIFI_MIN_RSSI -107

typedef enum {
  INIT,
  WIFI_INCORRECT,
  DISCONNECTED,
  CONNECTED,
  AP_CONNECTED,
  AP_DISCONNECTED
} WifiStatus;

#ifdef __cplusplus
extern "C" {
#endif

extern bool resolve(const char *hostname, IPAddress &ip);
extern void parseUrl(const char *url, char *protocol, char *hostname,
                     char *port, char *path);
extern uint16_t protocolToPort(const char *protocol);
extern void ssidList(int &num, char ***labels, char ***values);

#ifdef __cplusplus
}
#endif

class Wifi {
 public:
  Wifi();
  ~Wifi();

  static Wifi &instance() {
    if (!_instance) {
      _instance = new Wifi();
    }
    return *_instance;
  }

  bool init();
  void setConfig(Config &config);
  void setAP(const char *ssid, const char *passwd,
             const wifi_auth_mode_t authmode = WIFI_AUTH_WPA2_PSK);
  void setSTA(const char *ssid, const char *passwd,
              const wifi_auth_mode_t authmode = WIFI_AUTH_WPA2_PSK);

  void setStartCB(void (*cb)(void));
  void setStopCB(void (*cb)(void));

  IPAddress &localIP(void);
  IPAddress &gatewayIP(void);

  bool start(wifi_mode_t mode = WIFI_MODE_APSTA);
  bool stop(void);
  void scanSTA(void);
  void switchToAP(void);
  void switchToSTA(void);

  void ssidList(int &cnt, char ***labels, char ***values);

  void initialize_sntp(void);

  bool isSTAConnected();
  bool isSTAFailed();
  bool isAPConnected();
  bool isAPDisconnected();

  WifiStatus status(void);

  void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id,
                     void *event_data);

  void addPortMappingConfig(int rulePort, const char *ruleProtocol,
                            int ruleLeaseDuration,
                            const char *ruleFriendlyName);
  void checkUPnPMappings(void);

  void startEspNow();
  Config *config;

 private:
  static Wifi *_instance;
  EventGroupHandle_t _wifiEventGroup;

  WifiStatus _wifiState = INIT;
  char _SSID[32] = CONFIG_ESP_WIFI_SSID;  // to connect to your local AP
  char _passwd[64] = CONFIG_ESP_WIFI_PASSWORD;
  char _apSSID[32] = CONFIG_ESP_WIFI_AP_SSID;  // ESP AP
  char _apPasswd[64] = CONFIG_ESP_WIFI_AP_PASSWORD;
  wifi_auth_mode_t _authMode = WIFI_AUTH_WPA_WPA2_PSK;
  wifi_auth_mode_t _apAuthMode = WIFI_AUTH_WPA_WPA2_PSK;
  int _retry_num = 0;
  IPAddress _localIP;
  IPAddress _gatewayIP;
  SoftTimer _apTimer;
  bool _wifiStarted = false;
  bool _wifiAPMode = false;
  bool _wifiSTAReconfigured = false;
  wifi_config_t wifiSTAConfig;
  wifi_config_t wifiAPConfig;
  uint16_t _apNum = WIFI_SCAN_LIST_SIZE;
  uint16_t _apCount = 0;
  wifi_ap_record_t _apInfo[WIFI_SCAN_LIST_SIZE];
  void (*_startCB)(void) = NULL;
  void (*_stopCB)(void) = NULL;
  UPnP upnp;
  SoftTimer upnpTimer;
  bool newMapping = false;
  int mappingTestCnt = 0;
  EspNow *espNow = NULL;
};

extern Wifi wifi;

#endif  // __WIFI_H_