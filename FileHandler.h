/**
 * @file FileHandler.h
 * @author Phil Hilger (phil@peergum.com)
 * @brief
 * @version 0.1
 * @date 2022-02-11
 *
 * @copyright (c) 2022, PeerGum
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