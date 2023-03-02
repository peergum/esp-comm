/**
 * @file TCPClient.h
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

#ifndef __TCPCLIENT_H_
#define __TCPCLIENT_H_

#include "IPAddress.h"
#include "utils.h"
#include <string>

class TCPClient {
 public:
  TCPClient();
  ~TCPClient();
  bool connect(const char *url);
  bool connect(IPAddress addr, int port);
  bool connected(void);
  bool available(void);
  int read(void);
  int peek(void);
  int read(uint8_t *buffer, size_t len);
  int read(char *buffer, size_t len);
  int write(char c);
  int write(uint8_t *buffer, size_t len);
  int write(char *buffer, size_t len);
  int sendAndReceive(uint8_t *txBuffer, size_t txLen, uint8_t *rxBuffer,
                     size_t &rxLen, uint32_t timeout = 30000);
  std::string readUntil(char until);
  void close(void);
  void printf(const char *fmt, ...);

 private:
  int _sock;
  struct sockaddr_in *_dest_addr;
  uint8_t _rxBuffer[8192];
  int _rxLen = 0;
  int _rxPtr = 0;
  IPAddress _ip;
  int _addr_family = 0;
  int _ip_protocol = 0;
  char _hostname[100];
  char _path[255];
  char _port[6];
  char _protocol[16];
};

#endif  // __TCPCLIENT_H_