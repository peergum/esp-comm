#include <stdio.h>

#include "Wifi.h"
#include "TCPClient.h"
#include "UDPClient.h"
#include "UPnP.h"
#include "esp_err.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <cstring>
#include "esp_log.h"

#define LISTEN_PORT 8088      // http://<IP>:<LISTEN_PORT>/?name=<your string>
#define LEASE_DURATION 36000  // seconds
#define FRIENDLY_NAME "Blah"  // this name will appear in your router port forwarding
                  // section

#ifdef __cplusplus
extern "C" {
void app_main(void);
}
#endif

static const char *TAG = "Main";

uint8_t buffer[8192];
TaskHandle_t xWifiTask;

TCPClient tcpClient;
UDPClient udpClient;

extern void wifiTask(void *arg);

void app_main(void) {
  esp_log_level_set("*", ESP_LOG_INFO);
  esp_log_level_set("UPnP", ESP_LOG_DEBUG);

  // Initialize NVS
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
      ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);


  xTaskCreate(wifiTask, "wifi", 8192, NULL, 1, &xWifiTask);

}
