/**
 * @file ConfigHandler.h
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