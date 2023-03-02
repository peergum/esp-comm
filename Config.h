/**
 * @file Config.h
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

#ifndef __CONFIG_H
#define __CONFIG_H

#define DEFAULT_SCAN_LIST_SIZE 16

#include "cJSON.h"
#include <cstdio>
#include <cinttypes>
#include <cstring>
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"

typedef enum {
  CONFIG_UNKNOWN,  //<! in case parameter is not defined
  CONFIG_INT,
  CONFIG_BOOL,
  CONFIG_CHAR,
  CONFIG_LIST,
  CONFIG_INFO,  // just show, never save
} ConfigType;

typedef struct {
  int id;
  const ConfigType type;
  const char* name;
  int value;
  char* cValue;
  const char* cDefault;
  void (*listLoader)(int& count, char*** labels, char*** values);
  int listCnt;
  char** listLabels;
  char** listValues;
} ConfigItem;

class Config {
 public:
  Config(ConfigItem* items, int numItems, void (*setupDoneCB)() = NULL);

  /**
   *  @brief  Get the config data, and create a json object from it.
   *          It can be used to update the webpage from the stored data.
   */
  cJSON* getJson(void);

  /**
   *  @brief  Get a json object and save it as the new config
   *          It can be used to update the stored data from the webpage.
   *
   *  @param  _json The json object to save as the new config
   */
  void postJson(cJSON* json);

  /**
   *  \brief Get the config values from the NVS.
   */
  void getFromNVS();

  /**
   *  \brief Write the config values to NVS.
   */
  void putToNVS();

  /**
   *  \brief  Get the value of one config item by it's name.
   *
   *  \param  The name of the config item as defined in ConfigItem struct.
   */
  uint32_t getValByName(const char* name);

  /**
   * @brief set a config option to given value
   *
   * @param name
   * @param value
   */
  void setStringByName(const char* name, const char* value);

  /**
   *  \brief  Get the value of one config item by it's name.
   *
   *  \param  The name of the config item as defined in ConfigItem struct.
   */
  const char* getStringByName(const char* name);

  /**
   * @brief set a config option to given value
   *
   * @param name
   * @param value
   */
  void setValByName(const char* name, int value);

  /**
   * @brief confirms name is a config option by returning type
   *
   * @param name
   * @return ConfigType if found
   * @return CONFIG_UNKNOWN otherwise
   */
  ConfigType configItemType(const char* name);

  void setSetupDoneCB(void (*setupDoneCB)());

 protected:
  void (*_setupDoneCB)() = NULL;

 private:
  // void getAPList(int* numAP, wifi_ap_record_t** scannedAP);

  // const ConfigItem* _items;
  ConfigItem* _items;
  int _numItems;
  // uint16_t ap_count = 0;
  // uint16_t num_ap = DEFAULT_SCAN_LIST_SIZE;
  // wifi_ap_record_t ap_info[DEFAULT_SCAN_LIST_SIZE];
};

#endif

// [EOF]
