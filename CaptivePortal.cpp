/**
 * @file CaptivePortal.cpp
 * @author Phil Hilger (phil@peergum.com)
 * @brief 
 * @version 0.1
 * @date 2023-03-02
 * 
 * CAN-talk. A library for microcontrollers that allows decent comms
 * over a CAN bus.
 * 
 * Copyright (C) 2023, PeerGum
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https: //www.gnu.org/licenses/>.
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