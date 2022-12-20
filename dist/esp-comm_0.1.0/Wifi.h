/**
 * @file Wifi.h
 * @author Phil Hilger (phil@peergum.com)
 * @brief
 * @version 0.1
 * @date 2022-07-15
 *
 * Copyright (c) 2022 Peerjuice
 *
 */

#ifndef __WIFI_H_
#define __WIFI_H_

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "IPAddress.h"
#include "netdb.h"
#include "esp_mac.h"
#include "SoftTimer.h"
#include "common.h"
#include "sntp.h"

#define WIFI_MIN_RSSI -107

typedef enum {
  INIT,
  WIFI_INCORRECT,
  DISCONNECTED,
  CONNECTED,
  AP_CONNECTED,
  AP_DISCONNECTED
} WifiStatus;

extern bool resolve(const char *hostname, IPAddress &ip);
extern void parseUrl(const char *url, char *protocol, char *hostname,
                     char *port, char *path);
extern uint16_t protocolToPort(const char *protocol);
extern void ssidList(int &num, char ***labels, char ***values);

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

  void event_handler(void *arg, esp_event_base_t event_base,
                            int32_t event_id, void *event_data);

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
};

extern Wifi wifi;

#endif  // __WIFI_H_