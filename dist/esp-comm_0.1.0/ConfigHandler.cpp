/**
 * @file ConfigHandler.cpp
 * @author Phil Hilger (phil@peergum.com)
 * @brief
 * @version 0.1
 * @date 2022-02-11
 *
 * @copyright (c) 2022, PeerGum
 *
 */

#include "ConfigHandler.h"
#include "esp_log.h"
#include <stdio.h>
#include <string.h>

static const char* TAG = "ConfHdlr";

ConfigHandler::ConfigHandler(Config &config): Config(config) {
}

esp_err_t ConfigHandler::handler(httpd_req_t* req) {
  /* Destination buffer for content of HTTP POST request.
   * httpd_req_recv() accepts char* only, but content could
   * as well be any binary data (needs type casting).
   * In case of string data, null termination will be absent, and
   * content length would give length of string */
  char content[1000];

  switch (req->method) {
    case HTTP_GET: {
      cJSON* currentConfig = getJson();
      char* json = cJSON_Print(currentConfig);
      ESP_LOGD(TAG, "returning config");
      return httpd_resp_send(req, json, strlen(json));
    }
    case HTTP_POST: {
      /* Truncate if content length larger than the buffer */
      size_t recv_size = MIN(req->content_len, sizeof(content));

      int ret = httpd_req_recv(req, content, recv_size);
      if (ret <= 0) { /* 0 return value indicates connection closed */
        /* Check if timeout occurred */
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
          /* In case of timeout one can choose to retry calling
           * httpd_req_recv(), but to keep it simple, here we
           * respond with an HTTP 408 (Request Timeout) error */
          httpd_resp_send_408(req);
        }
        /* In case of error, returning ESP_FAIL will
         * ensure that the underlying socket is closed */
        return ESP_FAIL;
      }

      cJSON* newConfig = cJSON_Parse(content);
      if (newConfig) {
        cJSON* param = newConfig->child;
        for (; param; param = param->next) {
          ESP_LOGD(TAG, "Param: %s", param->string);
          switch (configItemType(param->string)) {
            case CONFIG_UNKNOWN:
              ESP_LOGE(TAG, "Configuration option [%s] unknown.",
                       param->string);
              break;
            case CONFIG_CHAR:
              [[fallthrough]];
            case CONFIG_LIST:
              setStringByName(param->string, param->valuestring);
              break;
            default:
              setValByName(param->string, param->valueint);
              break;
          }
        }
        putToNVS();
        /* Send a simple response */
        const char resp[] = "Saved";
        ESP_LOGI(TAG, "setup done = %p", _setupDoneCB);
        if (_setupDoneCB != NULL) {
          ESP_LOGD(TAG, "Calling setupDone");
          _setupDoneCB();
        }
        httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
        return ESP_OK;
      } else {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, NULL);
        return ESP_FAIL;
      }
    } break;
    default:
      httpd_resp_send_err(req, HTTPD_405_METHOD_NOT_ALLOWED, NULL);
      return ESP_FAIL;
  }
}