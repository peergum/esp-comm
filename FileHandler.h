/**
 * @file FileHandler.h
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

#ifndef __FILEHANDLER_H
#define __FILEHANDLER_H

#include "esp_http_server.h"
#include "common.h"

typedef struct {
    const char* uri;
    const char* page;
    httpd_method_t method;
} Alias;

typedef struct {
  const char* name;
  const uint8_t* start;
  const uint8_t* end;
} MemFile;

extern MemFile memFiles[];

class FileHandler {
public:
    FileHandler(int numAliases, Alias *aliases, bool useFAT = false);
    FileHandler& name(const char* filename);
    FileHandler& type(const char* mimeType);
    esp_err_t handler(httpd_req* req);
    void send(void);
    // static void setAliases(Alias *aliases, int numAliases);

    // static Alias *_aliases;
    // static int _numAliases;

private:
    const char *_name;
    const char *_type;
    int numAliases;
    Alias* aliases;
    bool _useFAT;
};

#endif