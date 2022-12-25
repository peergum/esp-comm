/**
 * @file Config.cpp
 * @author Phil Hilger (phil@peergum.com)
 * @brief 
 * @version 0.1
 * @date 2022-12-05
 * 
 * @copyright (c) 2022, PeerGum
 * 
 */

#include "Config.h"
#include "esp_log.h"
#include "utils.h"

static const char* TAG = "Config";

#define MIN(a, b) ((a < b) ? a : b)

Config::Config(ConfigItem* items, int numItems, void (*setupDoneCB)()) {
  _items = items;
  _numItems = numItems;
  _setupDoneCB = setupDoneCB;
  for (int i = 0; i < _numItems; i++) {
    switch (_items[i].type) {
      case CONFIG_CHAR:
        if (_items[i].cDefault) {
          _items[i].cValue = (char*)malloc(strlen(_items[i].cDefault) + 1);
          strcpy(_items[i].cValue, _items[i].cDefault);
        }
        break;
      case CONFIG_LIST:
        _items[i].listCnt = 0;
        _items[i].listLabels = NULL;
        _items[i].listValues = NULL;
        _items[i].cValue = strdup("");
        break;
      default:
        break;
    }
  }
}

cJSON *Config::getJson(void) {
  cJSON* config = cJSON_CreateObject();

  for (int i = 0; i < _numItems; i++) {
    cJSON* object;
    ConfigItem item = *(_items + i);
    ESP_LOGD(TAG, "Config item #%d type %d", i, item.type);
    switch (item.type) {
      case CONFIG_INT:
        object = cJSON_CreateNumber(item.value);
        cJSON_AddItemToObject(config, item.name, object);
        break;
      case CONFIG_CHAR:
        object = cJSON_CreateString(item.cValue ? item.cValue : "");
        cJSON_AddItemToObject(config, item.name, object);
        break;
      case CONFIG_LIST: {
        object = cJSON_CreateArray();
        item.listLoader(item.listCnt, &(item.listLabels), &(item.listValues));
        for (int j = 0; j < item.listCnt; j++) {
          cJSON* element = cJSON_CreateObject();
          cJSON* label = cJSON_CreateString(urlEncode(item.listLabels[j]));
          cJSON* value = cJSON_CreateString(item.listValues[j]);
          cJSON* selected =
              cJSON_CreateBool(!strcmp(item.cValue, item.listValues[j]));
          cJSON_AddItemToObject(element, "label", label);
          cJSON_AddItemToObject(element, "value", value);
          cJSON_AddItemToObject(element, "selected", selected);
          cJSON_AddItemToArray(object, element);
        }
        cJSON_AddItemToObject(config, item.name, object);
      } break;
      default:
        break;
    }
  }
  ESP_LOGD(TAG, "json: %s", cJSON_Print(config));
  return config;
}

void Config::postJson(cJSON* _json) {
  cJSON* data = cJSON_CreateObject();                         // Create the cJSON object

  for (int i = 0; i < _numItems; i++) {                // Loop trough the possible config items
    if (cJSON_HasObjectItem(_json,_items[i].name)) {          // Check if the name is found in the JSON
      data = cJSON_GetObjectItem(_json, _items[i].name);      // Get the data fro JSON
      if (cJSON_IsNumber(data)) {                             // Check if the data is a number
        _items[i].value = data->valueint;                     // Save the data to the config container
        //printf ("%s was found. Value: %d\n",_items[i].name,data->valueint);
      } else {
        _items[i].cValue = strdup(data->valuestring);
      }
    }
  }
}

void Config::getFromNVS() {
  nvs_handle_t h_nvs;
  esp_err_t err;
  uint32_t data;
  char cData[256];
  size_t len;

  err = nvs_open("config", NVS_READWRITE, &h_nvs);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
  } else {
    ESP_LOGD(TAG, "NVS open OK.");
    for (int i = 0; i < _numItems; i++) {
      len = 255;
      switch (_items[i].type) {
        case CONFIG_CHAR:
        case CONFIG_INFO:
        case CONFIG_LIST:
          err = nvs_get_blob(h_nvs, _items[i].name, cData, &len);
          switch (err) {
            case ESP_OK:
              ESP_LOGD(TAG, "Item [%s] loaded from NVS. Value: %.*s",
                       _items[i].name, len, cData);
              if (_items[i].cValue) {
                free(_items[i].cValue);
              }
              _items[i].cValue = (char*)malloc(len + 1);
              memcpy((void *)_items[i].cValue, (void *)cData, MIN(len, 255));
              _items[i].cValue[MIN(len, 255)] = 0;
              break;
            case ESP_ERR_NVS_NOT_FOUND:
              ESP_LOGD(TAG, "Item [%s] not initialized in NVS.",
                       _items[i].name);
              break;
            default:
              ESP_LOGE(TAG, "Error (%s) reading!", esp_err_to_name(err));
          }
          break;
        default:
          err = nvs_get_u32(h_nvs, _items[i].name, &data);
          switch (err) {
            case ESP_OK:
              ESP_LOGD(TAG, "Item [%s] loaded from NVS. Value: %lu",
                       _items[i].name, data);
              _items[i].value = data;
              break;
            case ESP_ERR_NVS_NOT_FOUND:
              ESP_LOGD(TAG, "Item [%s] not initialized in NVS.",
                       _items[i].name);
              break;
            default:
              ESP_LOGE(TAG, "Error (%s) reading!", esp_err_to_name(err));
          }
          break;
      }
    }
    nvs_close(h_nvs);
  }
}

void Config::putToNVS() {
  nvs_handle_t h_nvs;
  esp_err_t err;

  err = nvs_open("config", NVS_READWRITE, &h_nvs);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
  } else {
    ESP_LOGD(TAG, "NVS open OK.");
    for (int i = 0; i < _numItems; i++) {
      switch (_items[i].type) {
        case CONFIG_LIST:
          [[fallthrough]];
        case CONFIG_CHAR:
          err = nvs_set_blob(h_nvs, _items[i].name, _items[i].cValue,
                             strlen(_items[i].cValue));
          switch (err) {
            case ESP_OK:
              ESP_LOGD(TAG, "Item [%s] written to NVS. Value: %s",
                       _items[i].name, _items[i].cValue);
              break;
            default:
              ESP_LOGE(TAG, "Error (%s) writing!", esp_err_to_name(err));
              break;
          }
          break;
        case CONFIG_INFO:
            // pure information for display only (not saved)
            break;
        default:
          err = nvs_set_u32(h_nvs, _items[i].name, _items[i].value);
          switch (err) {
            case ESP_OK:
              ESP_LOGD(TAG, "Item [%s] written to NVS. Value: %d",
                       _items[i].name, _items[i].value);
              break;
            default:
              ESP_LOGE(TAG, "NVS write error (%s)!", esp_err_to_name(err));
              break;
          }
          break;
      }
    }
    nvs_commit(h_nvs);
    nvs_close(h_nvs);
  }
}

uint32_t Config::getValByName (const char* name) {
  for (int i = 0; i < _numItems; i++) {
    if (!strcmp(_items[i].name, name)) {
       return _items[i].value;
    }
  }
  ESP_LOGE(TAG, "Config name [%s] not found in configuration structure!", name);
  return 0;
}

void Config::setValByName(const char* name, int value) {
  for (int i = 0; i < _numItems; i++) {
    if (!strcmp(_items[i].name, name)) {
      _items[i].value = value;
      ESP_LOGI(TAG, "Config name [%s] value set to [%d]",
               name,value);
      return;
    }
  }
  ESP_LOGE(TAG, "Config name [%s] not found in configuration structure!", name);
  return;
}

const char* Config::getStringByName(const char* name) {
  for (int i = 0; i < _numItems; i++) {
    if (!strcmp(_items[i].name, name) && _items[i].cValue) {
      ESP_LOGI(TAG, "Config name [%s] has value [%s]", name, _items[i].cValue);
      return (const char *)_items[i].cValue;
    }
  }
  ESP_LOGE(TAG, "Config name [%s] not found in configuration structure!", name);
  return "";
}

void Config::setStringByName(const char* name, const char* value) {
  for (int i = 0; i < _numItems; i++) {
    if (!strcmp(_items[i].name, name)) {
      if (_items[i].cValue) {
        free(_items[i].cValue);
      }
      _items[i].cValue = (char *)malloc(strlen(value)+1);
      if (_items[i].cValue) {
        strcpy(_items[i].cValue, value);
        ESP_LOGI(TAG, "Config name [%s] value set to [%s]", name, value);
      } else {
        ESP_LOGE(TAG, "Couldn't allocate memory to save config [%s]", name);
      }
      return;
    }
  }
  ESP_LOGE(TAG, "Config name [%s] not found in configuration structure!", name);
  return;
}

ConfigType Config::configItemType(const char* name) {
  for (int i = 0; i < _numItems; i++) {
    if (!strcmp(_items[i].name,name)) {
      return _items[i].type;
    }
  }
  return CONFIG_UNKNOWN;
}

void Config::setSetupDoneCB(void (*setupDoneCB)()) {
  ESP_LOGD(TAG, "setup done = %p", setupDoneCB);
  _setupDoneCB = setupDoneCB;
}
