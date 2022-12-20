/**
 * @file CaptivePortal.cpp
 * @author Phil Hilger (phil@peergum.com)
 * @brief
 * @version 0.1
 * @date 2022-07-15
 *
 * Copyright (c) 2022 Peerjuice
 *
 */

#include "CaptivePortal.h"

// esp_err_t configApiHandler(httpd_req* req) { return
// configHandler.handler(req); } esp_err_t updateHandler(httpd_req* req) {
// return firmwareUpdater.handler(req); }

Alias portalAliases[] = {
    {
        .uri = "/",
        .page = "/index.html",
        .method = HTTP_GET,
    },
    {
        .uri = "/config",
        .page = "/config.html",
        .method = HTTP_GET,
    },
    {
        .uri = "/config",
        .page = "/config.html",
        .method = HTTP_POST,
    },
};

Api portalApis[] = {
    {
        .name = "config.json",
        .handler = CONFIG_HANDLER,
        .numMethods = 3,
        .methods = {HTTP_GET, HTTP_POST, HTTP_PUT},
    },
    {
        .name = "update",
        .handler = FIRMWARE_HANDLER,
        .numMethods = 1,
        .methods = {HTTP_POST},
    },
};

const char* images[] = {
    "logo.png",
};

const char* styles[] = {
    "bootstrap.min.css",
    "style.css",
};

const char* scripts[] = {
    "bootstrap.bundle.min.js",
    "jquery-3.5.1.min.js",
    "app.js",
};

WebServerConfig portalConfig = {
    .rootPath = "/",
    .imagePath = "/images",
    .stylePath = "/css",
    .scriptPath = "/js",
    .apiPath = "/api",
    .numAliases = 3,
    .aliases = portalAliases,
    .numApis = 2,
    .apis = portalApis,
    .numImages = 1,
    .images = images,
    .numScripts = 3,
    .scripts = scripts,
    .numStyles = 2,
    .styles = styles,
};

CaptivePortal::CaptivePortal(Config& config, bool useFAT)
    : WebServer(portalConfig, config, useFAT) {
    }

CaptivePortal::~CaptivePortal() {}