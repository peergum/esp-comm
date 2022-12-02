/**
 * @file Wifi.cpp
 * @author Phil Hilger (phil@peergum.com)
 * @brief
 * @version 0.1
 * @date 2022-07-15
 *
 * Copyright (c) 2022 Peerjuice
 *
 */

#include "Wifi.h"
// #include <string>
#include <cstring>
#include "esp_log.h"

/* FreeRTOS event group to signal when we are connected*/

/* The event group allows multiple bits for each event, but we only care about
 * two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

static const char *TAG = "Wifi";

static EventGroupHandle_t wifi_event_group;

/**
 * @brief
 *
 * @param url
 * @param protocol (max 20 chars)
 * @param hostname (max 253 chars)
 * @param port (max 5 chars)
 * @param path (max 2048 chars)
 */
void parseUrl(const char *url, char *protocol, char *hostname, char *port,
              char *path) {
  if (!strstr(url, "://")) {
    return;
  }
  int state = 0;
  bool hostmode = true;
  protocol[0] = hostname[0] = path[0] = port[0] = 0;
  for (int i = 0, j = 0; i < strlen(url); i++) {
    switch (url[i]) {
      case ':':
        if (state == 0) {
          // first occurrence of :
          state++;
          sprintf(port, "%d", protocolToPort(protocol));
        } else {
          // second occurrence of :
          hostmode = false;
        }
        j = 0;
        break;
      case '/':
        if (state > 0 && state < 4) {
          // after initial protocol
          state++;
          j = 0;
        }
        if (state == 4) {
          path[j++] = url[i];
          path[j] = 0;
        }
        break;
      default:
        if (state == 0 && j < 20) {
          // protocol part
          protocol[j++] = url[i];
          protocol[j] = 0;
        } else if (state == 3) {
          // host and port part
          if (hostmode && j < 253) {
            hostname[j++] = url[i];
            hostname[j] = 0;
          } else if (!hostmode && j < 5) {
            port[j++] = url[i];
            port[j] = 0;
          }
        } else if (j < 2049) {
          // uri part
          path[j++] = url[i];
          path[j] = 0;
        }
        break;
    }
  }
  ESP_LOGD(TAG, "Extracted from url [%s]:", url);
  ESP_LOGD(TAG, "proto: [%s]", protocol);
  ESP_LOGD(TAG, "host: [%s]", hostname);
  ESP_LOGD(TAG, "port: [%s]", port);
  ESP_LOGD(TAG, "path: [%s]", path);
}

/**
 * @brief resolve hostname and port to IP
 *
 * @param hostname
 * @param port
 * @param ip
 * @return true
 * @return false
 */
bool resolve(const char *hostname, IPAddress &ip) {
  // *** TO DO: resolve IPv6  // check if not already an IP address
  if (ip.fromChar(hostname).isValid()) {
    return true;
  }

  struct hostent *server;
  server = gethostbyname(hostname);
  if (server == NULL) {
    ESP_LOGD(TAG, "could not get host from dns: %d", errno);
    return false;
  }
  ip = IPAddress((const uint8_t *)(server->h_addr_list[0]));
  ESP_LOGI(TAG, "DNS lookup succeeded. IP=%s", ip.toChar());
  return true;
}

/**
 * @brief convert string protocol to number
 *
 * @param protocol
 * @return uint16_t
 */
uint16_t protocolToPort(const char *protocol) {
  if (!strcmp(protocol, "http")) {
    return 80U;
  } else if (!strcmp(protocol, "https")) {
    return 443U;
  }
  return 0;
}

/**
 * @brief Construct a new Wifi:: Wifi object
 *
 */
Wifi::Wifi() {}

/**
 * @brief Destroy the Wifi:: Wifi object
 *
 */
Wifi::~Wifi() {}

/**
 * @brief set STA parameters
 *
 * @param ssid
 * @param passwd
 * @param authMode
 */
void Wifi::setSTA(const char *ssid, const char *passwd,
                  const wifi_auth_mode_t authMode) {
  strcpy(_SSID, ssid);
  strcpy(_passwd, passwd);
  // _authMode = authMode;
}

/**
 * @brief set AP parameters
 *
 * @param ssid
 * @param passwd
 * @param authMode
 */
void Wifi::setAP(const char *ssid, const char *passwd,
                 const wifi_auth_mode_t authMode) {
  strcpy(_apSSID, ssid);
  strcpy(_apPasswd, passwd);
  _apAuthMode = authMode;
}

bool Wifi::init() {
  ESP_ERROR_CHECK(esp_netif_init());

  ESP_ERROR_CHECK(esp_event_loop_create_default());
  esp_netif_create_default_wifi_sta();
  esp_netif_create_default_wifi_ap();

  wifi_event_group = xEventGroupCreate();

  esp_event_handler_instance_t instance_any_id;
  esp_event_handler_instance_t instance_got_ip;
  ESP_ERROR_CHECK(esp_event_handler_instance_register(
      WIFI_EVENT, ESP_EVENT_ANY_ID, &instance().event_handler, NULL,
      &instance_any_id));
  ESP_ERROR_CHECK(esp_event_handler_instance_register(
      IP_EVENT, IP_EVENT_STA_GOT_IP, &instance().event_handler, NULL,
      &instance_got_ip));

  wifi_init_config_t wifiConfig = WIFI_INIT_CONFIG_DEFAULT();
  return (ESP_OK == esp_wifi_init(&wifiConfig));
}

/**
 * @brief start wifi
 *
 * @return true
 * @return false
 */
bool Wifi::start() {
  wifi_config_t wifiAPConfig = {.ap = {
                                    .ssid_len = (uint8_t)strlen(_apSSID),
                                    .channel = 6,
                                    .authmode = _apAuthMode,
                                }};
  memcpy(wifiAPConfig.ap.password, _apPasswd, 64);
  memcpy(wifiAPConfig.ap.ssid, _apSSID, 32);

  wifi_config_t wifiSTAConfig = {.sta = {
                                     .scan_method = WIFI_FAST_SCAN,
                                     .channel = 0,
                                     .sort_method = WIFI_CONNECT_AP_BY_SIGNAL,
                                     .threshold = {
                                      .rssi = WIFI_MIN_RSSI,
                                      .authmode = WIFI_AUTH_WPA2_PSK,
                                     },
                                 }};
  // wifiSTAConfig.sta.authmode = _authMode;
  memcpy(wifiSTAConfig.sta.ssid, _SSID, 32);
  memcpy(wifiSTAConfig.sta.password, _passwd, 64);

  if (_SSID[0] && _apSSID[0]) {
    esp_wifi_set_mode(WIFI_MODE_APSTA);
    esp_wifi_set_config(WIFI_IF_AP, &wifiAPConfig);
    esp_wifi_set_config(WIFI_IF_STA, &wifiSTAConfig);
  } else if (_SSID[0]) {
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifiSTAConfig);
  } else if (_apSSID[0]) {
    esp_wifi_set_mode(WIFI_MODE_AP);
    esp_wifi_set_config(WIFI_IF_AP, &wifiAPConfig);
  } else {
    ESP_LOGE(TAG, "Neither SSID or AP SSID defined");
    return false;
  }

  if (ESP_OK != esp_wifi_start()) {
    return false;
  }
  ESP_LOGI(TAG, "wifi started.");

  /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or
   * connection failed for the maximum number of re-tries (WIFI_FAIL_BIT). The
   * bits are set by event_handler() (see above) */
  EventBits_t bits =
      xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                          pdFALSE, pdFALSE, portMAX_DELAY);

  /* xEventGroupWaitBits() returns the bits before the call returned, hence we
   * can test which event actually happened. */
  if (bits & WIFI_CONNECTED_BIT) {
    ESP_LOGI(TAG, "connected to ap SSID:%s password:%s", _apSSID, _apPasswd);
  } else if (bits & WIFI_FAIL_BIT) {
    ESP_LOGW(TAG, "Failed to connect to SSID:%s, password:%s", _apSSID,
             _apPasswd);
  } else {
    ESP_LOGE(TAG, "UNEXPECTED EVENT");
  }
  return true;
}

void Wifi::event_handler(void *arg, esp_event_base_t event_base,
                         int32_t event_id, void *event_data) {
  if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
    esp_wifi_connect();
  } else if (event_base == WIFI_EVENT &&
             event_id == WIFI_EVENT_STA_DISCONNECTED) {
    if (instance()._retry_num < CONFIG_ESP_MAXIMUM_RETRY) {
      esp_wifi_connect();
      instance()._retry_num++;
      ESP_LOGI(TAG, "retry to connect to the AP");
    } else {
      xEventGroupSetBits(wifi_event_group, WIFI_FAIL_BIT);
    }
    ESP_LOGI(TAG, "connect to the AP fail");
    instance()._status = DISCONNECTED;
  } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
    ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
    instance()._localIP = event->ip_info.ip.addr;
    instance()._gatewayIP = event->ip_info.gw.addr;
    ESP_LOGI(TAG, "got ip: %s gw: %s", instance()._localIP.toChar(),
             instance()._gatewayIP.toChar());
    instance()._retry_num = 0;
    xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
    instance()._status = CONNECTED;
  }
}

bool Wifi::stop() { return (ESP_OK == esp_wifi_stop()); }

IPAddress &Wifi::localIP() { return instance()._localIP; }

IPAddress &Wifi::gatewayIP() { return instance()._gatewayIP; }

WifiStatus Wifi::status() { return instance()._status; }

Wifi *Wifi::_instance = nullptr;

Wifi wifi;