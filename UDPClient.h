/**
 * @file UDPClient.h
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

#ifndef __UDPCLIENT_H_
#define __UDPCLIENT_H_

#include "IPAddress.h"
#include "utils.h"

class UDPClient {
 public:
  UDPClient();
  ~UDPClient();
  bool begin(const char *url);
  bool begin(uint16_t port);
  bool begin(IPAddress addr, uint16_t port);
  bool beginMulticast(IPAddress addr, uint16_t port);
  void setTimeout(uint32_t timeout);
  bool available(void);
  int parsePacket(void);
  int read(void);
  int peek(void);
  int read(uint8_t *buffer, size_t len);
  int read(char *buffer, size_t len);
  size_t write(uint8_t data);
  size_t write(const uint8_t *buffer, size_t size);
  size_t write(const char *buffer, size_t size);
  // int send(uint8_t *txBuffer, size_t txLen, uint32_t timeout);
  // int sendAndReceive(uint8_t *txBuffer, size_t txLen, uint8_t *rxBuffer,
  //                    size_t &rxLen, uint32_t timeout = 30000);
  // int receive(uint8_t *rxBuffer, size_t &rxLen, uint32_t timeout);
  void stop(void);
  void printf(const char *fmt, ...);
  bool beginPacket();
  bool beginPacket(IPAddress ip, uint16_t port);
  bool beginPacket(const char *host, uint16_t port);
  bool endPacket();
  bool beginMulticastPacket();

  void flush();

  IPAddress remoteIP();
  uint16_t remotePort();

 private:
  int _sock;
  // struct sockaddr_in _dest_addr, _src_addr;
  uint8_t _rxBuffer[2048];
  int _rxLen = 0;
  int _rxPtr = 0;
  IPAddress _multicastIp;
  IPAddress _remoteIp;
  uint16_t _remotePort = 0;
  uint16_t _serverPort = 0;
  int _addr_family = 0;
  int _ip_protocol = 0;
  char _hostname[100];
  char _cPort[6];
  char _protocol[16];
  uint8_t _txBuffer[2048];
  size_t _txLen = 0;
};

#endif  // __UDPCLIENT_H_