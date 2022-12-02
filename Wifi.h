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
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "IPAddress.h"
#include "netdb.h"

#define WIFI_MIN_RSSI -107

typedef enum {
  INIT,
  DISCONNECTED,
  CONNECTED,
} WifiStatus;

extern bool resolve(const char *hostname, IPAddress &ip);
extern void parseUrl(const char *url, char *protocol, char *hostname,
                     char *port, char *path);
extern uint16_t protocolToPort(const char *protocol);

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
             const wifi_auth_mode_t authmode);
  void setSTA(const char *ssid, const char *passwd,
                    const wifi_auth_mode_t authmode);
  IPAddress &localIP(void);
  IPAddress &gatewayIP(void);

  bool start(void);
  bool stop(void);
  WifiStatus status(void);

  static void event_handler(void *arg, esp_event_base_t event_base,
                                  int32_t event_id, void *event_data);


 private:
  static Wifi *_instance;

  WifiStatus _status = INIT;
  char _SSID[32] = CONFIG_ESP_WIFI_SSID;  // to connect to your local AP
  char _passwd[64] = CONFIG_ESP_WIFI_PASSWORD;
  char _apSSID[32] = CONFIG_ESP_WIFI_SSID; // ESP AP
  char _apPasswd[64] = CONFIG_ESP_WIFI_PASSWORD;
  // wifi_auth_mode_t _authMode = WIFI_AUTH_WPA2_WPA3_PSK;
  wifi_auth_mode_t _apAuthMode = WIFI_AUTH_WPA2_PSK;
  int _retry_num = 0;
  IPAddress _localIP;
  IPAddress _gatewayIP;
};

extern Wifi wifi;

#endif  // __WIFI_H_