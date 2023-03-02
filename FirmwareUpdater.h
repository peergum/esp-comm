/**
 * @file FirmwareUpdater.h
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

#ifndef __FIRMWAREUPDATER_H
#define __FIRMWAREUPDATER_H

#include "esp_http_server.h"

#ifndef MIN
#define MIN(a, b) ((a < b) ? a : b)
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief decode uploaded data for firmware binary
 *
 * @param content
 * @param ret
 * @param firmwareData
 * @param size
 * @return true
 * @return false
 */
bool parseData(uint8_t* buffer, int ret, uint8_t** firmwareData, size_t* size);

/**
 * @brief initialize parser state
 *
 */
void initParser(void);

#ifdef __cplusplus
}
#endif

class FirmwareUpdater {
public:
  /**
   * @brief Construct a new Firmware Updater object
   *
   */
  FirmwareUpdater();

  /**
   * @brief Handles form file upload and firmware update
   *
   * @param req
   * @return esp_err_t
   */
  esp_err_t handler(httpd_req* req);

private:
};

#endif