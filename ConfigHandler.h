/**
 * @file ConfigHandler.h
 * @author Phil Hilger (phil@peergum.com)
 * @brief
 * @version 0.1
 * @date 2022-02-11
 *
 * @copyright (c) 2022, PeerGum
 *
 */

#ifndef __CONFIGHANDLER_H
#define __CONFIGHANDLER_H

#include "esp_http_server.h"
#include "Config.h"
#include "common.h"

class ConfigHandler: protected Config {
public:
    ConfigHandler(Config &config);
    esp_err_t handler(httpd_req* req);
    // void setSetupDoneCB(void (*_setupDoneCB)());

   private:
    const char *_name;
    const char *_type;
    // void (*setupDoneCB)();
};

#endif