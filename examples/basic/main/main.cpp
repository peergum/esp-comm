#include <stdio.h>

#include "wifi.h"
#include "TCPClient.h"
#include "UDPClient.h"
#include "UPnP.h"
#include "esp_err.h"
#include "nvs_flash.h"
#include "freertos/freeRTOS.h"
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
UPnP uPnP(20000);

void wifiTask(void *arg) {
  assert(wifi.init());
  assert(wifi.start());
  // while (wifi.status() != WifiStatus::CONNECTED) {
  //   vTaskDelay(pdMS_TO_TICKS(1000));
  // }

  // assert(tcpClient.connect(IPAddress(192,168,2,1),80));
  // size_t len = sizeof(buffer);
  // const char *test = "HTTP/1.1 GET http:192.179.2.1\r\n\r\n";
  // int ret = tcpClient.sendAndReceive((uint8_t *)test, strlen(test), buffer, len);
  // while (ret>0) {
  //   len = sizeof(buffer);
  //   ret = tcpClient.read(buffer, len);
  // }
  // tcpClient.close();

  portMappingResult portMappingAdded;
  uPnP.addPortMappingConfig(wifi.localIP(), LISTEN_PORT, RULE_PROTOCOL_TCP,
                                LEASE_DURATION, FRIENDLY_NAME);
  while (true) {
    portMappingAdded = uPnP.commitPortMappings();

    if (portMappingAdded == SUCCESS || portMappingAdded == ALREADY_MAPPED) {
      break;
    }
    // for debugging, you can see this in your router too under forwarding or
    // UPnP
    // uPnP.printAllPortMappings();
    ESP_LOGE(TAG,"This was printed because adding the required port mapping failed");
    delay(30000);  // 30 seconds before trying again
  }

  uPnP.printAllPortMappings();

  // udpClient.begin(IPAddress(192, 168, 2, 1), 53);
  // len = sizeof(buffer);
  // const char *test2 = "Message from ESP32";
  // udpClient.beginPacket();
  // udpClient.write(test2, strlen(test2));
  // udpClient.endPacket();
  // udpClient.stop();
  vTaskDelete(xWifiTask);
}

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
