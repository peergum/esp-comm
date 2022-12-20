/**
 * @file WebServer.h
 * @author Phil Hilger (phil@peergum.com)
 * @brief
 * @version 0.1
 * @date 2022-02-11
 *
 * @copyright (c) 2022, PeerGum
 *
 */

#ifndef __SERVER_H
#define __SERVER_H

#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_http_server.h"
#include "Config.h"
#include "FileHandler.h"
#include "FirmwareUpdater.h"
#include "ConfigHandler.h"
#include "cJSON.h"

#define MAX_URIS 20

typedef enum {
  FILE_HANDLER,
  CONFIG_HANDLER,
  FIRMWARE_HANDLER,
} HandlerType;

typedef struct {
  const char* name;
  HandlerType handler;
  int numMethods;
  httpd_method_t methods[5];
} Api;

typedef struct {
  const char* rootPath;    // HTML_PATH;
  const char* imagePath;   // IMAGE_PATH;
  const char* stylePath;   // STYLE_PATH;
  const char* scriptPath;  // SCRIPT_PATH;
  const char* apiPath;     // API_PATH;
  int numAliases;
  Alias* aliases;
  int numApis;
  Api* apis;
  int numImages;
  const char** images;
  int numScripts;
  const char** scripts;
  int numStyles;
  const char** styles;
} WebServerConfig;

class WebServer {
 public:
  WebServer(WebServerConfig& serverConfig, Config &config, bool useFAT = false);
  void start(void);
  void stop(void);
  bool addAliases(void);
  bool addApis(const char* path);
  bool addUris(const char* elements[], int count, const char* path);
  void addUriHandler(httpd_uri_t* uri);
  bool isRunning(void);

 private:
  httpd_handle_t server;
  httpd_config_t webConfig = HTTPD_DEFAULT_CONFIG();
  httpd_uri_t* uris[MAX_URIS];
  char paths[MAX_URIS][40];
  int uriCount = 0;
  bool bRunning = false;
  WebServerConfig serverConfig;
  ConfigHandler configHandler;
  FileHandler fileHandler;
  FirmwareUpdater firmwareUpdater;
};

#endif