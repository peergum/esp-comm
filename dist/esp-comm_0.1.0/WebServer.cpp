/**
 * @file WebServer.cpp
 * @author Phil Hilger (phil@peergum.com)
 * @brief
 * @version 0.1
 * @date 2022-02-11
 *
 * @copyright (c) 2022, PeerGum
 *
 */

#include "WebServer.h"

#define WEBSERVER_DEBUG ESP_LOG_DEBUG

// extern Config config;

static const char* TAG = "WebServer";

// esp_err_t handler(httpd_req_t* req) {
//     return fileHandler.handler(req);
// }

static FileHandler* pFileHandler = NULL;
static FirmwareUpdater* pFirmwareUpdater = NULL;
static ConfigHandler* pConfigHandler = NULL;

static esp_err_t fileHandlerCaller(httpd_req_t* req) {
  return pFileHandler->handler(req);
}
static esp_err_t firmwareUpdaterCaller(httpd_req_t* req) {
  return pFirmwareUpdater->handler(req);
}
static esp_err_t configHandlerCaller(httpd_req_t* req) {
  return pConfigHandler->handler(req);
}

WebServer::WebServer(WebServerConfig& serverConfig, Config &config, bool useFAT)
    : serverConfig(serverConfig),
      configHandler(config),
      fileHandler(serverConfig.numAliases, serverConfig.aliases, useFAT) {
  esp_log_level_set(TAG, WEBSERVER_DEBUG);
  pFileHandler = &fileHandler;
  pFirmwareUpdater = &firmwareUpdater;
  pConfigHandler = &configHandler;
}

bool WebServer::addUris(const char* elements[], int count, const char* path) {
  for (int i = 0; i < count && uriCount < MAX_URIS; i++) {
    uris[uriCount] = (httpd_uri_t*)malloc(sizeof(httpd_uri_t));
    if (!uris[uriCount]) {
      ESP_LOGE(TAG, "Memory Low Error");
      return false;
    }
    sprintf(paths[uriCount], "%s/%s", path, elements[i]);
    uris[uriCount]->uri = (const char*)&paths[uriCount];
    uris[uriCount]->method = HTTP_GET;
    uris[uriCount]->handler = fileHandlerCaller;
    uris[uriCount]->user_ctx = (void*)path;
    ESP_LOGD(TAG, "Adding URI %s", paths[uriCount]);
    addUriHandler(uris[uriCount]);
  }
  return true;
}

bool WebServer::addAliases(void) {
  ESP_LOGD(TAG, "%d aliases", serverConfig.numAliases);
  for (int i = 0; i < serverConfig.numAliases && uriCount < MAX_URIS; i++) {
    uris[uriCount] = (httpd_uri_t*)malloc(sizeof(httpd_uri_t));
    if (!uris[uriCount]) {
      ESP_LOGE(TAG, "Memory Low Error");
      return false;
    }
    strcpy(paths[uriCount], serverConfig.aliases[i].uri);
    uris[uriCount]->uri = (const char*)&paths[uriCount];
    uris[uriCount]->method = serverConfig.aliases[i].method;
    uris[uriCount]->handler = fileHandlerCaller;
    uris[uriCount]->user_ctx = NULL;
    ESP_LOGD(TAG, "Adding Alias %s", paths[uriCount]);
    addUriHandler(uris[uriCount]);
  }
  return true;
}

bool WebServer::addApis(const char* path) {
  for (int i = 0; i < serverConfig.numApis && uriCount < MAX_URIS; i++) {
    for (int j = 0; j < serverConfig.apis[i].numMethods && uriCount < MAX_URIS;
         j++) {
      uris[uriCount] = (httpd_uri_t*)malloc(sizeof(httpd_uri_t));
      if (!uris[uriCount]) {
        ESP_LOGE(TAG, "Memory Low Error");
        return false;
      }
      sprintf(paths[uriCount], "%s/%s", path, serverConfig.apis[i].name);
      uris[uriCount]->uri = (const char*)&paths[uriCount];
      uris[uriCount]->uri = (const char*)&paths[uriCount];
      uris[uriCount]->method = serverConfig.apis[i].methods[j];
      switch (serverConfig.apis[i].handler) {
        case CONFIG_HANDLER:
          uris[uriCount]->handler = configHandlerCaller;
          break;
        case FIRMWARE_HANDLER:
          uris[uriCount]->handler = firmwareUpdaterCaller;
          break;
        case FILE_HANDLER:
          [[fallthrough]];
        default:
          uris[uriCount]->handler = fileHandlerCaller;
          break;
      };
      uris[uriCount]->user_ctx = (void*)path;
      ESP_LOGD(TAG, "Adding URI %s (method %d)", paths[uriCount],
               serverConfig.apis[i].methods[j]);
      addUriHandler(uris[uriCount]);
    }
  }
  return true;
}

void WebServer::start(void) {
  webConfig.lru_purge_enable = true;
  webConfig.max_uri_handlers = MAX_URIS;
  ESP_LOGI(TAG, "Starting server on port: '%d'", webConfig.server_port);
  if (httpd_start(&server, &webConfig) == ESP_OK) {
    uriCount = 0;
    ESP_LOGI(TAG, "Registering URI handlers");
    addAliases();
    addApis(serverConfig.apiPath);
    addUris(serverConfig.scripts, serverConfig.numScripts,
            serverConfig.scriptPath);
    addUris(serverConfig.styles, serverConfig.numStyles,
            serverConfig.stylePath);
    addUris(serverConfig.images, serverConfig.numImages,
            serverConfig.imagePath);
    bRunning = true;
    ESP_LOGI(TAG, "WebServer started.");
    return;
  }

  ESP_LOGI(TAG, "Error starting server!");
  return;
}

void WebServer::stop(void) {
  httpd_stop(server);
  server = NULL;
  bRunning = false;
}

void WebServer::addUriHandler(httpd_uri_t* _uri) {
  uris[uriCount++] = _uri;
  httpd_register_uri_handler(server, _uri);
}

bool WebServer::isRunning(void) { return bRunning; }
