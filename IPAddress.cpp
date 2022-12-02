/**
 * @file IPAddress.cpp
 * @author Phil Hilger (phil@peergum.com)
 * @brief
 * @version 0.1
 * @date 2022-11-04
 *
 * Copyright (c) 2022, PeerGum
 *
 */

#include "IPAddress.h"
#include <cstring>
#include "esp_log.h"

const char* TAG = "IPAddress";

IPAddress ipNull;

IPAddress::IPAddress(uint8_t a, uint8_t b, uint8_t c, uint8_t d) {
  ipAddress.u_addr.ip4.addr = ESP_IP4TOADDR(a, b, c, d);
  ipAddress.type = ESP_IPADDR_TYPE_V4;
  if (!ipAddress.u_addr.ip4.addr) {
    valid = false;
  }
  ESP_LOGD(TAG, "%s (%lu)", toChar(), toUInt());
}

// IPAddress::IPAddress(IPAddress& src) { ipAddress = src.ipAddress; }

IPAddress::IPAddress(const uint8_t* pAddr) { ipAddress.u_addr.ip4.addr = *((uint32_t*)pAddr); }

IPAddress::IPAddress(const uint32_t addr) { ipAddress.u_addr.ip4.addr = addr; }

const char* IPAddress::toChar() {
  sprintf(string, IPSTR, IP2STR(&ipAddress.u_addr.ip4));
  return string;
}

IPAddress& IPAddress::fromChar(const char* str) {
  if (ESP_OK == esp_netif_str_to_ip4(str, &ipAddress.u_addr.ip4)) {
    ipAddress.type = ESP_IPADDR_TYPE_V4;
    return *this;
  } else if (ESP_OK == esp_netif_str_to_ip6(str, &ipAddress.u_addr.ip6)) {
    ipAddress.type = ESP_IPADDR_TYPE_V6;
    return *this;
  }
  ipAddress.u_addr.ip4.addr = ESP_IP4TOADDR(0, 0, 0, 0);
  ipAddress.type = ESP_IPADDR_TYPE_V4;
  valid = false;
  return *this;
}

bool IPAddress::operator==(IPAddress address) {
  return (address.ipAddress.u_addr.ip4.addr == ipAddress.u_addr.ip4.addr);
}

IPAddress& IPAddress::operator=(esp_ip4_addr_t ip4) {
  if (!ip4.addr) {
    valid = false;
  }
  ipAddress.u_addr.ip4.addr = ip4.addr;
  ipAddress.type = ESP_IPADDR_TYPE_V4;
  return *this;
}

IPAddress& IPAddress::operator=(esp_ip6_addr_t ip6) {
  memcpy(ipAddress.u_addr.ip6.addr, ip6.addr, sizeof(ip6.addr));
  ipAddress.type = ESP_IPADDR_TYPE_V6;
  return *this;
}

IPAddress& IPAddress::operator=(uint8_t * pAddr) {
  if (!pAddr) {
    valid = false;
  } else {
    ipAddress.u_addr.ip4.addr = *(uint32_t *)pAddr;
    ipAddress.type = ESP_IPADDR_TYPE_V4;
  }
  return *this;
}

IPAddress& IPAddress::operator=(uint32_t addr) {
  if (!addr) {
    valid = false;
  }
  ipAddress.u_addr.ip4.addr = addr;
  ipAddress.type = ESP_IPADDR_TYPE_V4;
  return *this;
}

int IPAddress::type(void) { return ipAddress.type; }

void IPAddress::type(int t) { ipAddress.type = t; }

uint32_t IPAddress::toUInt(void) {
  return esp_netif_htonl(ipAddress.u_addr.ip4.addr);
}

uint32_t IPAddress::toAddr(void) {
  return ipAddress.u_addr.ip4.addr;
}