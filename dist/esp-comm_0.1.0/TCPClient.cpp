/**
 * @file TCPClient.cpp
 * @author Phil Hilger (phil@peergum.com)
 * @brief
 * @version 0.1
 * @date 2022-07-15
 *
 * Copyright (c) 2022 Peerjuice
 *
 */

#include "TCPClient.h"
#include "IPAddress.h"
#include "esp_netif.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"
#include "esp_log.h"

static const char *TAG = "TCPClient";

TCPClient::TCPClient() {}

TCPClient::~TCPClient() {}

bool TCPClient::connect(const char *url) {
  parseUrl(url, _protocol, _hostname, _port, _path);
  if (resolve(_hostname, _ip)) {
    return connect(_ip, atoi(_port));
  }
  return false;
}

bool TCPClient::connect(IPAddress addr, int port) {
  // struct sockaddr_in6 dest_addr = {0};
  if (!_dest_addr) {
    _dest_addr = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));
  }
  if (addr.type() == ESP_IPADDR_TYPE_V4) {
    _dest_addr->sin_addr.s_addr = htonl(addr.toUInt());
    _dest_addr->sin_family = AF_INET;
    _dest_addr->sin_port = htons(port);
    _addr_family = AF_INET;
    _ip_protocol = IPPROTO_IP;
  } /*  else {
     inet6_aton(addr.ipAddress.u_addr.ip6.addr, &dest_addr.sin6_addr);
     dest_addr.sin6_family = AF_INET6;
     dest_addr.sin6_port = htons(port);
     dest_addr.sin6_scope_id =
   esp_netif_get_netif_impl_index(EXAMPLE_INTERFACE); addr_family = AF_INET6;
     ip_protocol = IPPROTO_IPV6;
   } */

  _ip = addr.toAddr();
  _sock = lwip_socket(_addr_family, SOCK_STREAM, _ip_protocol);
  if (_sock < 0) {
    ESP_LOGE(TAG, "Unable to create socket: errno %d", _sock);
    return false;
  }
  ESP_LOGD(TAG, "Socket created, connecting to %s:%d", _ip.toChar(), port);

  int err = lwip_connect(_sock, (struct sockaddr *)_dest_addr,
                         sizeof(struct sockaddr_in));
  if (err != 0) {
    ESP_LOGE(TAG, "Socket unable to connect: errno %d", err);
    return false;
  }
  ESP_LOGD(TAG, "Successfully connected");
  return true;
}

bool TCPClient::connected(void) { return (_sock >= 0); }

bool TCPClient::available() {
  if (_rxPtr < _rxLen) {
    return true;
  }
  _rxLen = recv(_sock, _rxBuffer, sizeof(_rxBuffer), 0);
  _rxPtr = 0;
  if (_rxLen > 0) {
    ESP_LOGD(TAG, "rxLen %d bytes from %s:", _rxLen, _ip.toChar());
    ESP_LOG_BUFFER_HEXDUMP(TAG, _rxBuffer, _rxLen, ESP_LOG_DEBUG);
  }
  return (_rxLen > 0);
}

int TCPClient::read(void) {
  if (available()) {
    return _rxBuffer[_rxPtr++];
  }
  return -1;
}

int TCPClient::peek(void) {
  if (available()) {
    return _rxBuffer[_rxPtr];
  }
  return -1;
}

int TCPClient::read(uint8_t *rxBuffer, size_t rxLen) {
  size_t len = 0;
  for (; len < rxLen && available(); len++) {
    rxBuffer[len] = read();
  }
  return len;
}

int TCPClient::write(char c) { return write(&c, 1); }

int TCPClient::write(uint8_t *txBuffer, size_t txLen) {
  int err = lwip_send(_sock, txBuffer, txLen, 0);
  // ESP_LOG_BUFFER_HEXDUMP(TAG, txBuffer, txLen, ESP_LOG_DEBUG);
  if (err < 0) {
    ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
  }
  return err;
}

int TCPClient::write(char *buffer, size_t len) {
  return write((uint8_t *)buffer, len);
}

int TCPClient::sendAndReceive(uint8_t *txBuffer, size_t txLen,
                              uint8_t *rxBuffer, size_t &rxLen,
                              uint32_t timeout) {
  write(txBuffer, txLen);
  return read(rxBuffer, rxLen);
}

void TCPClient::close(void) {
  if (_sock != -1) {
    lwip_shutdown(_sock, 0);
    lwip_close(_sock);
    ESP_LOGD(TAG, "Socket shutdown");
    _sock = -1;
  }
}

void TCPClient::printf(const char *f, ...) {
  va_list args;
  va_start(args, f);
  char buffer[1024];
  va_end(args);
  sprintf(buffer, f, args);
  ESP_LOG_BUFFER_HEXDUMP(TAG, buffer, strlen(buffer), ESP_LOG_DEBUG);
  write(buffer, strlen(buffer));
}

std::string TCPClient::readUntil(char until) {
  std::string str;
  int c;
  while ((c = read())>=0) {
    str += (char)c;
    if (c == until) {
      break;
    }
  }
  return str;
}