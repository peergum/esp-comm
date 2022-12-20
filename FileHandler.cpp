/**
 * @file FileHandler.cpp
 * @author Phil Hilger (phil@peergum.com)
 * @brief
 * @version 0.1
 * @date 2022-02-11
 *
 * @copyright (c) 2022, PeerGum
 *
 */

#include "FileHandler.h"
#include "esp_log.h"
#include <stdio.h>
#include <string.h>

static const char* TAG = "FileHdlr";

static char buffer[2048];
static char uri[255];

FileHandler::FileHandler(int numAliases, Alias* aliases, bool useFAT)
    : numAliases(numAliases), aliases(aliases), _useFAT(useFAT) {}

FileHandler& FileHandler::name(const char* filename) {
    _name = filename;
    return *this;
}

FileHandler& FileHandler::type(const char* mimeType) {
    _type = mimeType;
    return *this;
}

esp_err_t FileHandler::handler(httpd_req_t* req) {
    // size_t len = 0;
    ESP_LOGD(TAG, "uri = %s", req->uri);
    strcpy(uri, req->uri);
    char *parts[5];
    int part = 0;
    for (char* pos = strtok(uri, "/"); pos; pos = strtok(NULL, "/")) {
        ESP_LOGD(TAG, "[%s]", pos);
        parts[part] = pos;
        part++;
    }
    if (req->method == HTTP_POST) {
        ESP_LOGD(TAG, "Analyzing POST");
    // while (req->content_len>sizeof(buffer)) {
        int ret = httpd_req_recv(req, buffer, req->content_len);
        if (ret > 0) {
            ESP_LOGD(TAG, "buffer = %.*s", ret, buffer);
        }
        else if (ret < 0) {
            return ret;
        } else {
            ESP_LOGD(TAG, "empty body");
        }
        // }
    }
    // if (part == 0) {
    //     for (int i=0; i<numAliases; i++) {
            
    //     }
    // } else {
    //     if (!strcmp())
    // }
    char filename[255];
    strcpy(filename,FAT_MOUNT_POINT);
    strcat(filename, "/web");
    int i;
    for (i=0; i<numAliases; i++) {
      if (!strcmp(req->uri, aliases[i].uri)) {
        strcat(filename, aliases[i].page);
        break;
      }
    }
    if (i == numAliases) {
        strcat(filename,req->uri);
    }
    char contentType[30];
    if (part > 0) {
      if (strstr(parts[part - 1], ".jpg")) {
        strcpy(contentType, "image/jpg");
      } else if (strstr(parts[part - 1], ".png")) {
        strcpy(contentType, "image/png");
      } else if (strstr(parts[part - 1], ".svg")) {
        strcpy(contentType, "image/svg+xml");
      }
      // else if (strstr(parts[part - 1], ".json")) {
      //     strcpy(contentType,"application/json");
      //     for (int j = 0; j < numApis; j++) {
      //         if (!strcmp(parts[part - 1], apis[j].name)) {
      //             return apis[j].handler(req);
      //         }
      //     }
      //     // no API found.
      //     return httpd_resp_send_404(req);
      // }
      else if (strstr(parts[part - 1], ".js")) {
        strcpy(contentType, "text/javascript");
      } else if (strstr(parts[part - 1], ".css")) {
        strcpy(contentType, "text/css");
      } else {
        strcpy(contentType, "text/html");
      }
    } else {
      strcpy(contentType, "text/html");
    }
    ESP_LOGD(TAG, "file: %s", filename);
    int packet = 0;
    int read = 0;
    size_t readSize = sizeof(buffer);
    esp_err_t err = ESP_OK;
    if (_useFAT) {
      FILE* file = fopen(filename, "r");
      while (file && (read = fread(buffer, 1, readSize, file)) >= 0) {
        httpd_resp_set_status(req, "200 OK");
        httpd_resp_set_type(req, contentType);
        httpd_resp_set_hdr(req, "Cache-Control", "max-age=120");
        // ESP_LOGD(TAG,"read=%d",read);
        if (read < readSize && packet == 0) {
          httpd_resp_send(req, buffer, read);
          break;
        }
        err = httpd_resp_send_chunk(req, buffer, read);
        if (err != ESP_OK || read == 0) {
          break;
        }
        packet++;
      }
      if (file) {
        if (packet>0) {
          httpd_resp_set_status(req, "200 OK");
          httpd_resp_set_type(req, contentType);
          httpd_resp_set_hdr(req, "Cache-Control", "max-age=120");
          err = httpd_resp_send_chunk(req, buffer, 0);
        }
        fclose(file);
      } else {
        return httpd_resp_send_404(req);
      }
    } else {
      char* pageName = strrchr(filename, '/')+1;
      uint8_t *start = NULL, *end;
      int i = 0;
      do {
        if (!strcmp(pageName,memFiles[i].name)) {
          start = (uint8_t *)memFiles
          [i].start;
          end = (uint8_t*)memFiles[i].end;
          ESP_LOGD(TAG, "Mem Page found: %s", pageName);
        }
        i++;
      } while (!start && memFiles[i].name);
      if (!start) {
        return httpd_resp_send_404(req);
      }
      size_t length = end - start - 1;
      char contentLength[10];
      sprintf(contentLength, "%d", length);
      httpd_resp_set_hdr(req, "Content-Length", contentLength);
      for (int i = 0; i < length; i += readSize) {
        httpd_resp_set_status(req, "200 OK");
        httpd_resp_set_type(req, contentType);
        httpd_resp_set_hdr(req, "Cache-Control", "max-age=120");
        read = (readSize < length - i) ? readSize : (length - i);
        memcpy((void*)buffer, (void*)(start + i), read);
        if ((read < readSize && packet == 0) || length == read) {
          err = httpd_resp_send(req, buffer, (ssize_t)read);
          break;
        }
        err = httpd_resp_send_chunk(req, buffer, (ssize_t)read);
        if (err != ESP_OK) {
          break;
        }
        packet++;
      }
      if (packet > 0) {
        httpd_resp_set_status(req, "200 OK");
        httpd_resp_set_type(req, contentType);
        httpd_resp_set_hdr(req, "Cache-Control", "max-age=120");
        err = httpd_resp_send_chunk(req, buffer, 0);
      }
    }
    return err;
}

// void FileHandler::setAliases(Alias *aliases, int numAliases) {
//     _aliases = aliases;
//     _numAliases = numAliases;
// }