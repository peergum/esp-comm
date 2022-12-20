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
#include "UPnP.h"
#include "arpa/inet.h"
#include "esp_netif.h"
#include "netdb.h"

/* FreeRTOS event group to signal when we are connected*/

/* The event group allows multiple bits for each event, but we only care about
 * two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAILED_BIT BIT1
#define AP_CONNECTED_BIT BIT2
#define AP_DISCONNECTED_BIT BIT3

static const char *TAG = "Wifi";

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

void wifiTask(void *arg) {
  wifi.init();
  wifi.scanSTA();
  wifi.start(WIFI_MODE_AP);
  while (true) {
    vTaskDelay(pdMS_TO_TICKS(50));
  }
  wifi.stop();
}

static void wifiEventHandler(void *arg, esp_event_base_t event_base,
                             int32_t event_id, void *event_data) {
  wifi.event_handler(arg, event_base, event_id, event_data);
}

void ssidList(int &num, char ***labels, char ***values) {
  wifi.ssidList(num, labels, values);
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

void Wifi::setStartCB(void (*cb)()) { _startCB = cb; }

void Wifi::setStopCB(void (*cb)()) { _stopCB = cb; }

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
  _authMode = authMode;
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

  _wifiEventGroup = xEventGroupCreate();

  esp_event_handler_instance_t instance_any_id;
  esp_event_handler_instance_t instance_got_ip;
  ESP_ERROR_CHECK(esp_event_handler_instance_register(
      WIFI_EVENT, ESP_EVENT_ANY_ID, wifiEventHandler, NULL, &instance_any_id));
  ESP_ERROR_CHECK(esp_event_handler_instance_register(
      IP_EVENT, IP_EVENT_STA_GOT_IP, wifiEventHandler, NULL, &instance_got_ip));

  wifi_init_config_t wifiConfig = WIFI_INIT_CONFIG_DEFAULT();
  return (ESP_OK == esp_wifi_init(&wifiConfig));
}

/**
 * @brief start wifi
 *
 * @return true
 * @return false
 */
bool Wifi::start(wifi_mode_t mode) {
  wifiAPConfig.ap.ssid_len = 0;
  wifiAPConfig.ap.channel = 0;
  wifiAPConfig.ap.authmode = _apAuthMode;
  wifiAPConfig.ap.max_connection = WIFI_MAX_AP_CONNECTIONS;

  strcpy((char *)wifiAPConfig.ap.password, _apPasswd);
  strcpy((char *)wifiAPConfig.ap.ssid, _apSSID);

  wifiSTAConfig.sta.scan_method = WIFI_FAST_SCAN;
  wifiSTAConfig.sta.channel = 0;
  wifiSTAConfig.sta.sort_method = WIFI_CONNECT_AP_BY_SIGNAL;
  wifiSTAConfig.sta.threshold.rssi = WIFI_MIN_RSSI;
  wifiSTAConfig.sta.threshold.authmode = _authMode;
  wifiSTAConfig.sta.bssid_set = 0;

  // wifiSTAConfig.sta.authmode = _authMode;
  strcpy((char *)wifiSTAConfig.sta.ssid, _SSID);
  strcpy((char *)wifiSTAConfig.sta.password, _passwd);

  esp_wifi_set_mode(mode);
  esp_wifi_set_config(WIFI_IF_AP, &wifiAPConfig);
  esp_wifi_set_config(WIFI_IF_STA, &wifiSTAConfig);

  if (ESP_OK != esp_wifi_start()) {
    return false;
  }
  ESP_LOGI(TAG, "wifi started.");

  _startCB();  // short callback function

  return true;
}

bool Wifi::isSTAConnected() {
  return xEventGroupGetBits(_wifiEventGroup) & WIFI_CONNECTED_BIT;
}

bool Wifi::isSTAFailed() {
  return xEventGroupGetBits(_wifiEventGroup) & WIFI_FAILED_BIT;
}

bool Wifi::isAPConnected() {
  return xEventGroupGetBits(_wifiEventGroup) & AP_CONNECTED_BIT;
}

bool Wifi::isAPDisconnected() {
  return xEventGroupGetBits(_wifiEventGroup) & AP_DISCONNECTED_BIT;
}

void Wifi::event_handler(void *arg, esp_event_base_t event_base,
                         int32_t event_id, void *event_data) {
  // if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
  //   esp_wifi_connect();
  // } else if (event_base == WIFI_EVENT &&
  //            event_id == WIFI_EVENT_STA_DISCONNECTED) {
  //   if (_retry_num < CONFIG_ESP_MAXIMUM_RETRY) {
  //     esp_wifi_connect();
  //     _retry_num++;
  //     ESP_LOGI(TAG, "retry to connect to the AP");
  //   } else {
  //     xEventGroupSetBits(wifi_event_group, WIFI_FAIL_BIT);
  //   }
  //   ESP_LOGI(TAG, "connect to the AP fail");
  //   _wifistate = DISCONNECTED;
  // } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
  //   ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
  //   _localIP = event->ip_info.ip.addr;
  //   _gatewayIP = event->ip_info.gw.addr;
  //   ESP_LOGI(TAG, "got ip: %s gw: %s", _localIP.toChar(),
  //            _gatewayIP.toChar());
  //   _retry_num = 0;
  //   xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
  //   _wifistate = CONNECTED;
  // }
  if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
    esp_wifi_connect();
    _retry_num = 0;
  } else if (event_base == WIFI_EVENT &&
             (event_id == WIFI_EVENT_STA_DISCONNECTED ||
              event_id == WIFI_EVENT_STA_BEACON_TIMEOUT)) {
    _wifiState = DISCONNECTED;
    if (_retry_num < WIFI_MAXIMUM_RETRY) {
      esp_wifi_connect();
      _retry_num++;
      ESP_LOGI(TAG, "retry to connect to the AP");
    } else {
      xEventGroupSetBits(_wifiEventGroup, WIFI_FAILED_BIT);
      _wifiState = WIFI_INCORRECT;
    }
    ESP_LOGI(TAG, "connect to the AP fail");
  } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
    ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
    ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
    _retry_num = 0;
    xEventGroupSetBits(_wifiEventGroup, WIFI_CONNECTED_BIT);
    _wifiState = CONNECTED;
  } else if (event_base == WIFI_EVENT &&
             event_id == WIFI_EVENT_AP_STACONNECTED) {
    wifi_event_ap_staconnected_t *event =
        (wifi_event_ap_staconnected_t *)event_data;
    ESP_LOGI(TAG, "station " MACSTR " join, AID=%d", MAC2STR(event->mac),
             event->aid);
    xEventGroupSetBits(_wifiEventGroup, AP_CONNECTED_BIT);
    _wifiState = AP_CONNECTED;
  } else if (event_base == WIFI_EVENT &&
             event_id == WIFI_EVENT_AP_STADISCONNECTED) {
    wifi_event_ap_stadisconnected_t *event =
        (wifi_event_ap_stadisconnected_t *)event_data;
    ESP_LOGI(TAG, "station " MACSTR " leave, AID=%d", MAC2STR(event->mac),
             event->aid);
    xEventGroupSetBits(_wifiEventGroup, AP_DISCONNECTED_BIT);
    _wifiState = AP_DISCONNECTED;
  } else {
    ESP_LOGW(TAG, "Event base: %s, event_id = %ld", event_base, event_id);
  }
}

void Wifi::initialize_sntp(void) {
  ESP_LOGI(TAG, "Initializing SNTP");
  sntp_setoperatingmode(SNTP_OPMODE_POLL);

/*
 * If 'NTP over DHCP' is enabled, we set dynamic pool address
 * as a 'secondary' server. It will act as a fallback server in case that
 * address provided via NTP over DHCP is not accessible
 */
#if LWIP_DHCP_GET_NTP_SRV && SNTP_MAX_SERVERS > 1
  sntp_setservername(1, "pool.ntp.org");

#if LWIP_IPV6 && \
    SNTP_MAX_SERVERS > 2  // statically assigned IPv6 address is also possible
  ip_addr_t ip6;
  if (ipaddr_aton("2a01:3f7::1", &ip6)) {  // ipv6 ntp source "ntp.netnod.se"
    sntp_setserver(2, &ip6);
  }
#endif /* LWIP_IPV6 */

#else /* LWIP_DHCP_GET_NTP_SRV && (SNTP_MAX_SERVERS > 1) */
  // otherwise, use DNS address from a pool
  sntp_setservername(0, "pool.ntp.org");

  sntp_setservername(1,
                     "pool.ntp.org");  // set the secondary NTP server (will be
                                       // used only if SNTP_MAX_SERVERS > 1)
#endif

  // sntp_set_time_sync_notification_cb(time_sync_notification_cb);
#ifdef CONFIG_SNTP_TIME_SYNC_METHOD_SMOOTH
  sntp_set_sync_mode(SNTP_SYNC_MODE_SMOOTH);
#endif
  sntp_init();

  ESP_LOGI(TAG, "List of configured NTP servers:");

  for (uint8_t i = 0; i < SNTP_MAX_SERVERS; ++i) {
    if (sntp_getservername(i)) {
      ESP_LOGI(TAG, "server %d: %s", i, sntp_getservername(i));
    } else {
      // we have either IPv4 or IPv6 address, let's print it
      char buff[IP6ADDR_STRLEN_MAX];
      ip_addr_t const *ip = sntp_getserver(i);
      if (ipaddr_ntoa_r(ip, buff, IP6ADDR_STRLEN_MAX) != NULL)
        ESP_LOGI(TAG, "server %d: %s", i, buff);
    }
  }
}

void Wifi::scanSTA() {
  if (_wifiStarted) {
    esp_wifi_stop();
  }

  wifiSTAConfig.sta.ssid[0] = 0;
  wifiSTAConfig.sta.password[0] = 0;
  wifiSTAConfig.sta.scan_method = WIFI_ALL_CHANNEL_SCAN;
  wifiSTAConfig.sta.channel = 0;
  wifiSTAConfig.sta.sort_method = WIFI_CONNECT_AP_BY_SECURITY;
  wifiSTAConfig.sta.threshold.rssi = WIFI_MIN_RSSI;
  wifiSTAConfig.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifiSTAConfig));
  ESP_ERROR_CHECK(esp_wifi_start());

  _apCount = 0;
  memset(_apInfo, 0, sizeof(_apInfo));

  esp_wifi_scan_start(NULL, true);
  // vTaskDelay(pdMS_TO_TICKS(3000));
  esp_wifi_scan_stop();

  ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&_apNum, _apInfo));
  ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&_apCount));
  ESP_LOGI(TAG, "Total APs scanned = %u", _apCount);
  for (int i = 0; (i < WIFI_SCAN_LIST_SIZE) && (i < _apCount); i++) {
    ESP_LOGI(TAG, "SSID \t\t%s", _apInfo[i].ssid);
    ESP_LOGI(TAG, "RSSI \t\t%d", _apInfo[i].rssi);
    ESP_LOGI(TAG, "Auth \t\t%d", _apInfo[i].authmode);
    // if (_apInfo[i].authmode != WIFI_AUTH_WEP) {
    //   print_cipher_type(_apInfo[i].pairwise_cipher,
    //   _apInfo[i].group_cipher);
    // }
    ESP_LOGI(TAG, "Channel \t\t%d\n", _apInfo[i].primary);
  }
  esp_wifi_stop();
}

void Wifi::ssidList(int &cnt, char ***pLabels, char ***pValues) {
  if (!*pLabels) {
    *pLabels = (char **)malloc(WIFI_SCAN_LIST_SIZE * sizeof(char *));
    memset((void *)*pLabels, 0, WIFI_SCAN_LIST_SIZE * sizeof(char *));
  }
  if (!*pValues) {
    *pValues = (char **)malloc(WIFI_SCAN_LIST_SIZE * sizeof(char *));
    memset((void *)*pValues, 0, WIFI_SCAN_LIST_SIZE * sizeof(char *));
  }
  char temp[50];
  char **values = *(pValues);
  char **labels = *(pLabels);
  cnt = 0;
  for (int i = 0; i < _apCount; i++) {
    if (!_apInfo[i].ssid[0] || _apInfo[i].ssid[32]!=0) {
      continue;
    }
    values[cnt] = (char *)(_apInfo[i].ssid);
    // if (labels[i]) {
    //   free(labels[i]);
    // }
    sprintf(temp, "%s [%ddB]", _apInfo[i].ssid, _apInfo[i].rssi);
    labels[cnt] = strdup(temp);
    ESP_LOGI(TAG, "ptr #%d = %s -> %s", cnt, labels[cnt], values[cnt]);
    // labels[i] = (char *)malloc(strlen(temp) + 1);
    // strcpy(labels[i], temp);
    cnt++;
  }
}

void Wifi::switchToSTA() {
  if (_wifiStarted) {
    esp_wifi_stop();
  }

  strcpy((char *)wifiSTAConfig.sta.ssid, _SSID);
  strcpy((char *)wifiSTAConfig.sta.password, _passwd);
  wifiSTAConfig.sta.channel = 0;

  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifiSTAConfig));
  ESP_ERROR_CHECK(esp_wifi_start());

  _wifiStarted = true;
  _wifiAPMode = false;
  _wifiSTAReconfigured = false;
  ESP_LOGI(TAG, "Switched to STA mode");
}

void Wifi::switchToAP() {
  if (_wifiStarted) {
    esp_wifi_stop();
  }
  // memset(wifiSTAConfig.ap.ssid, 0, sizeof(wifiSTAConfig.ap.ssid));
  // strncpy((char *)wifiSTAConfig.ap.ssid, DEFAULT_AP_SSID,
  //         sizeof(wifiSTAConfig.ap.ssid));
  // memset(wifiSTAConfig.ap.password, 0, sizeof(wifiSTAConfig.ap.password));
  // strncpy((char *)wifiSTAConfig.ap.password, DEFAULT_AP_PASS,
  //         sizeof(wifiSTAConfig.ap.password));
  strcpy((char *)wifiAPConfig.ap.ssid, _apSSID);
  strcpy((char *)wifiAPConfig.ap.password, _apPasswd);
  wifiAPConfig.ap.channel = 0;
  wifiAPConfig.ap.authmode = _apAuthMode;
  wifiAPConfig.ap.ssid_len = 0;
  wifiAPConfig.ap.max_connection = WIFI_MAX_AP_CONNECTIONS;

  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifiAPConfig));
  ESP_ERROR_CHECK(esp_wifi_start());

  // _apCount = 0;
  // memset(_apInfo, 0, sizeof(_apInfo));

  // esp_wifi_scan_start(NULL, true);

  // ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&number, _apInfo));
  // ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&_apCount));
  // ESP_LOGI(TAG, "Total APs scanned = %u", _apCount);
  // for (int i = 0; (i < WIFI_SCAN_LIST_SIZE) && (i < _apCount); i++) {
  //   ESP_LOGI(TAG, "SSID \t\t%s", _apInfo[i].ssid);
  //   ESP_LOGI(TAG, "RSSI \t\t%d", _apInfo[i].rssi);
  //   ESP_LOGI(TAG, "Auth \t\t%d", _apInfo[i].authmode);
  //   // if (_apInfo[i].authmode != WIFI_AUTH_WEP) {
  //   //   print_cipher_type(_apInfo[i].pairwise_cipher,
  //   _apInfo[i].group_cipher);
  //   // }
  //   ESP_LOGI(TAG, "Channel \t\t%d\n", _apInfo[i].primary);
  // }

  _wifiStarted = true;
  _wifiAPMode = true;
  // _apTimer.initTimer();
  ESP_LOGI(TAG, "Switched to AP mode");
}

bool Wifi::stop() {
  if (ESP_OK != esp_wifi_stop()) {
    return false;
  }
  _stopCB();
  return true;
}

IPAddress &Wifi::localIP() { return _localIP; }

IPAddress &Wifi::gatewayIP() { return _gatewayIP; }

WifiStatus Wifi::status() { return _wifiState; }

Wifi wifi;