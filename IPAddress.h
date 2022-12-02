/**
 * @file IPAddress.h
 * @author Phil Hilger (phil@peergum.com)
 * @brief
 * @version 0.1
 * @date 2022-11-04
 *
 * Copyright (c) 2022, PeerGum
 *
 */

#ifndef __IP_ADDRESS_H__
#define __IP_ADDRESS_H__

#include "esp_netif_ip_addr.h"
#include "esp_wifi.h"

class IPAddress;

extern IPAddress ipNull;

class IPAddress {
 public:
  IPAddress(uint8_t a, uint8_t b, uint8_t c, uint8_t d);
  // IPAddress(IPAddress& address);
  IPAddress(const uint8_t* pAddr);
  IPAddress(const uint32_t addr);
  IPAddress() {
    // IPAddress(0, 0, 0, 0);
    }
  ~IPAddress() {}

  bool operator==(IPAddress address);

  int type(void);
  void type(int);
  const char* toChar();
  IPAddress& fromChar(const char *str);

  uint32_t toUInt(void);
  uint32_t toAddr(void);
  
  IPAddress& operator=(esp_ip4_addr_t ip4);
  IPAddress& operator=(esp_ip6_addr_t ip6);
  IPAddress& operator=(uint32_t addr);
  IPAddress& operator=(uint8_t *pAddr);

  bool isValid() { return valid; }

  char string[32];

 private:
  esp_ip_addr_t ipAddress = {.u_addr = {.ip4 = {.addr = 0UL}},
                             .type = ESP_IPADDR_TYPE_V4};
  bool valid = false;
};

#endif