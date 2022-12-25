/**
 * @file FirmwareUpdater.cpp
 * @author Phil Hilger (phil@peergum.com)
 * @brief
 * @version 0.1
 * @date 2022-04-08
 *
 * @copyright Copyright (c) 2022
 *
 */

#include "FirmwareUpdater.h"

#include <stdio.h>

#include "esp_flash_partitions.h"
#include "esp_app_format.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_partition.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs.h"
#include "nvs_flash.h"

#define BUFFER_SIZE 4096

static const char *TAG = "FwUpdr";
static bool otaStarted = false, otaFailed = false;
static bool binaryFound = false;
static uint8_t buffer[BUFFER_SIZE];

TaskHandle_t g_hOta;

static void __attribute__((noreturn)) task_fatal_error(void) {
  otaFailed = true;
  ESP_LOGE(TAG, "Exiting task due to fatal error...");
  (void)vTaskDelete(NULL);

  while (1) {
    ;
  }
}

void otaTask(void *pvParameter) {
  esp_err_t err;
  httpd_req *req = (httpd_req *)pvParameter;
  size_t recv_size = MIN(req->content_len, BUFFER_SIZE);

  ESP_LOGD(TAG, "OTA Task starting");

  /* update handle : set by esp_ota_begin(), must be freed via esp_ota_end() */
  esp_ota_handle_t update_handle = 0;
  const esp_partition_t *update_partition = NULL;

  ESP_LOGI(TAG, "Starting OTA");

  const esp_partition_t *configured = esp_ota_get_boot_partition();
  const esp_partition_t *running = esp_ota_get_running_partition();

  if (configured != running) {
    ESP_LOGW(TAG,
             "Configured OTA boot partition at offset 0x%08lx, but running from "
             "offset 0x%08lx",
             configured->address, running->address);
    ESP_LOGW(TAG,
             "(This can happen if either the OTA boot data or preferred boot "
             "image become corrupted somehow.)");
  }
  ESP_LOGI(TAG, "Running partition type %d subtype %d (offset 0x%08lx)",
           running->type, running->subtype, running->address);

  update_partition = esp_ota_get_next_update_partition(NULL);
  assert(update_partition != NULL);
  ESP_LOGI(TAG, "Writing to partition subtype %d at offset 0x%lx",
           update_partition->subtype, update_partition->address);

  int binary_file_length = 0;
  initParser();
  size_t total = 0;
  bool image_header_was_checked = false;
  while (1) {
    int data_read = httpd_req_recv(req, (char *)buffer, recv_size);

    if (data_read < 0 && data_read != HTTPD_SOCK_ERR_TIMEOUT) {
      ESP_LOGE(TAG, "Error: HTTP receive error");
      otaFailed = true;
      task_fatal_error();
    } else if (data_read == 0) {
      break;  // we're done
    } else if (data_read > 0) {
      total += data_read;
      uint8_t *firmwareData = NULL;
      size_t size = 0;
      if (parseData(buffer, data_read, &firmwareData, &size) && size > 0) {
        memcpy(buffer, firmwareData, size);
        data_read = size;
      }
      ESP_LOGD(TAG, "Received %u bytes (total = %u)", data_read, total);
      if (image_header_was_checked == false) {
        esp_app_desc_t new_app_info;
        if (data_read > sizeof(esp_image_header_t) +
                            sizeof(esp_image_segment_header_t) +
                            sizeof(esp_app_desc_t)) {
          // check current version with downloading
          memcpy(&new_app_info,
                 &buffer[sizeof(esp_image_header_t) +
                         sizeof(esp_image_segment_header_t)],
                 sizeof(esp_app_desc_t));
          ESP_LOGI(TAG, "New firmware version: %s", new_app_info.version);

          esp_app_desc_t running_app_info;
          if (esp_ota_get_partition_description(running, &running_app_info) ==
              ESP_OK) {
            ESP_LOGI(TAG, "Running firmware version: %s",
                     running_app_info.version);
          }

          const esp_partition_t *last_invalid_app =
              esp_ota_get_last_invalid_partition();
          esp_app_desc_t invalid_app_info;
          if (esp_ota_get_partition_description(last_invalid_app,
                                                &invalid_app_info) == ESP_OK) {
            ESP_LOGI(TAG, "Last invalid firmware version: %s",
                     invalid_app_info.version);
          }

          // check current version with last invalid partition
          if (last_invalid_app != NULL) {
            if (memcmp(invalid_app_info.version, new_app_info.version,
                       sizeof(new_app_info.version)) == 0) {
              ESP_LOGW(TAG, "New version is the same as invalid version.");
              ESP_LOGW(TAG,
                       "Previously, there was an attempt to launch the "
                       "firmware with %s version, but it failed.",
                       invalid_app_info.version);
              ESP_LOGW(
                  TAG,
                  "The firmware has been rolled back to the previous version.");
              task_fatal_error();
            }
          }

          if (memcmp(new_app_info.version, running_app_info.version,
                     sizeof(new_app_info.version)) == 0) {
            ESP_LOGW(TAG,
                     "Current running version is the same as a new. We will "
                     "not continue the update.");
            task_fatal_error();
          }

          image_header_was_checked = true;

          err = esp_ota_begin(update_partition, 0, &update_handle);
          if (err != ESP_OK) {
            ESP_LOGE(TAG, "esp_ota_begin failed (%s)", esp_err_to_name(err));
            esp_ota_end(update_handle);
            task_fatal_error();
          }
          ESP_LOGI(TAG, "esp_ota_begin succeeded");
        } else {
          ESP_LOGE(TAG, "received package is not fit len");
          esp_ota_end(update_handle);
          task_fatal_error();
        }
      }
      err = esp_ota_write(update_handle, (const void *)buffer, data_read);
      if (err != ESP_OK) {
        esp_ota_end(update_handle);
        task_fatal_error();
      }
      binary_file_length += data_read;
      ESP_LOGD(TAG, "Written image length %d", binary_file_length);
    }
  }
  otaStarted = true;
  ESP_LOGI(TAG, "Total Write binary data length: %d", binary_file_length);

  err = esp_ota_end(update_handle);
  if (err != ESP_OK) {
    if (err == ESP_ERR_OTA_VALIDATE_FAILED) {
      ESP_LOGE(TAG, "Image validation failed, image is corrupted");
    } else {
      ESP_LOGE(TAG, "esp_ota_end failed (%s)!", esp_err_to_name(err));
    }
    task_fatal_error();
  }

  err = esp_ota_set_boot_partition(update_partition);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "esp_ota_set_boot_partition failed (%s)!",
             esp_err_to_name(err));
    task_fatal_error();
  }

  nvs_handle_t h_nvs;
  err = nvs_open("config", NVS_READWRITE, &h_nvs);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
  } else {
    ESP_LOGD(TAG, "NVS open OK.");
    err = nvs_set_u8(h_nvs, "updated", 1);
    nvs_commit(h_nvs);
    nvs_close(h_nvs);
  }
  ESP_LOGI(TAG, "Prepare to restart system!");
  vTaskDelay(pdMS_TO_TICKS(2000));
  esp_restart();
  return;
}

bool parseData(uint8_t *buffer, int ret, uint8_t **firmwareData, size_t *size) {
  if (binaryFound) {
    *firmwareData = buffer;
    *size = (size_t)ret;
    return true;
  }

  char *content = (char *)buffer;
  if (strstr(content,
             "Content-Disposition: form-data; name=\"firmware\"; filename=")) {
    char *firmwareStart = strstr(content, "\r\n\r\n");
    if (firmwareStart) {
      ESP_LOGD(TAG, "Found firmware start");
      *firmwareData = (uint8_t *)firmwareStart + 4;
      *size = (size_t)ret - (*firmwareData - buffer);
      binaryFound = true;
    }
  }

  return binaryFound;
}

void initParser() { binaryFound = false; }

//
// FirmwareUpdater Class
//

FirmwareUpdater::FirmwareUpdater() {}

esp_err_t FirmwareUpdater::handler(httpd_req *req) {
  switch (req->method) {
    // case HTTP_GET: {
    //   cJSON* currentConfig = getJson();
    //   char* json = cJSON_Print(currentConfig);
    //   ESP_LOGD(TAG, "returning config");
    //   return httpd_resp_send(req, json, strlen(json));
    // }
    case HTTP_POST: {
      ESP_LOGD(TAG, "Receiving %u bytes", req->content_len);

      ESP_LOGD(TAG, "Free heap size = %lu (internal = %lu)",
               esp_get_free_heap_size(), esp_get_free_internal_heap_size());
      otaStarted = false;
      otaFailed = false;
      if (xTaskCreate(otaTask, "OTA", 5 * 1024, (void *)req, 20, &g_hOta) ==
          pdTRUE) {
        // if (g_hOta) {
        // esp_task_wdt_add(g_hOta);
        /* Send a simple response */
        while (!otaStarted && !otaFailed) {
          vTaskDelay(pdMS_TO_TICKS(100));
        }
      }
      if (otaStarted && !otaFailed) {
        const char resp[] = "OK";
        httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
        return ESP_OK;
      }
      const char resp[] = "FAIL";
      httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
      return ESP_OK;
    } break;
    default:
      httpd_resp_send_err(req, HTTPD_405_METHOD_NOT_ALLOWED, NULL);
      return ESP_FAIL;
  }
}
