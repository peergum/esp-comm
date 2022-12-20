/**
 * @file UDPClient.cpp
 * @author Phil Hilger (phil@peergum.com)
 * @brief
 * @version 0.1
 * @date 2022-07-15
 *
 * Copyright (c) 2022 Peerjuice
 *
 */

#include "UDPClient.h"
#include "IPAddress.h"
#include "esp_netif.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"
#include "esp_log.h"

static const char *TAG = "UDPClient";

UDPClient::UDPClient() {}

UDPClient::~UDPClient() { stop(); }

// bool UDPClient::begin(const char *url) {
//   if (!strstr(url, "://")) {
//     return false;
//   }
//   int state = 0;
//   bool hostmode = true;
//   memset((void *)_hostname, 0, sizeof(_hostname));
//   memset((void *)_cPort, 0, sizeof(_cPort));
//   memset((void *)_protocol, 0, sizeof(_protocol));
//   for (int i = 0, j = 0, k = 0, l = 0; i < strlen(url); i++) {
//     switch (url[i]) {
//       case ':':
//         if (state == 0) {
//           // first occurrence of :
//           state++;
//           protocolToPort();
//         } else {
//           // second occurrence of :
//           hostmode = false;
//         }
//         break;
//       case '/':
//         if (state > 0) {
//           // after initial protocol
//           state++;
//         }
//         break;
//       default:
//         if (state == 0 && l < sizeof(_protocol) - 1) {
//           // protocol part
//           _protocol[l] = url[i];
//         } else if (state == 3) {
//           // host and port part
//           if (hostmode && j < sizeof(_hostname) - 1) {
//             _hostname[j++] = url[i];
//           } else if (!hostmode && k < sizeof(_cPort) - 1) {
//             _cPort[k++] = url[i];
//           }
//         } else {
//           // uri part
//         }
//         break;
//     }
//   }
//   if (resolve(_hostname, _cPort, _ip)) {
//     return open(_ip, atoi(_cPort));
//   }
//   return false;
// }

bool UDPClient::begin(IPAddress addr, uint16_t port) {
  // struct sockaddr_in6 dest_addr = {0};
  _sock = lwip_socket(_addr_family, SOCK_DGRAM, _ip_protocol);
  if (_sock < 0) {
    ESP_LOGE(TAG, "Unable to create socket: errno %d", _sock);
    return false;
  }
  ESP_LOGD(TAG, "Socket created");

  int yes = 1;
  if (lwip_setsockopt(_sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0) {
    ESP_LOGD(TAG, "could not set socket option: %d", errno);
    stop();
    return false;
  }

  _remoteIp = addr.toAddr();
  _serverPort = port;

  struct sockaddr_in *_src_addr =
      (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));
  if (addr.type() == ESP_IPADDR_TYPE_V4) {
    _src_addr->sin_addr.s_addr = htonl(addr.toUInt());
    _src_addr->sin_family = AF_INET;
    _src_addr->sin_port = htons(_serverPort);
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

  if (lwip_bind(_sock, (struct sockaddr *)_src_addr, sizeof(struct sockaddr)) <
      0) {
    ESP_LOGD(TAG, "could not bind socket: %d", errno);
    stop();
    return false;
  }
  fcntl(_sock, F_SETFL, O_NONBLOCK);
  return true;
}

bool UDPClient::begin(uint16_t port) { return begin(ipNull, port); }

bool UDPClient::beginMulticast(IPAddress addr, uint16_t port) {
  ESP_LOGD(TAG, "BeginMulticast: %s:%d", addr.toChar(), port);
  if (begin(ipNull, port)) {
    ESP_LOGD(TAG, "IP=%s", addr.toChar());
    if (addr != ipNull) {
      struct ip_mreq mreq;
      mreq.imr_multiaddr.s_addr = (in_addr_t)addr.toAddr();
      mreq.imr_interface.s_addr = ipNull.toAddr();
      if (setsockopt(_sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq,
                     sizeof(mreq)) < 0) {
        ESP_LOGD(TAG, "could not join igmp: %d", errno);
        stop();
        return false;
      }
      _multicastIp = addr;
    }
    return true;
  }
  return false;
}

void UDPClient::stop(void) {
  if (_sock != -1) {
    if (_multicastIp != ipNull) {
      struct ip_mreq mreq;
      mreq.imr_multiaddr.s_addr = _multicastIp.toAddr();
      mreq.imr_interface.s_addr = ipNull.toAddr();
      setsockopt(_sock, IPPROTO_IP, IP_DROP_MEMBERSHIP, &mreq, sizeof(mreq));
      _multicastIp = ipNull;
    }
    lwip_shutdown(_sock, 0);
    lwip_close(_sock);
    ESP_LOGD(TAG, "Socket shutdown");
  }
}

bool UDPClient::beginMulticastPacket() {
  if (!_serverPort || _multicastIp == ipNull) {
    return 0;
  }
  _remoteIp = _multicastIp;
  _remotePort = _serverPort;
  return beginPacket();
}

bool UDPClient::beginPacket() {
  if (!_remotePort) {
    return false;
  }
  _txLen = 0;
  if (_sock < 0) {
    return true;
  }

  if ((_sock = lwip_socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    ESP_LOGD(TAG, "could not create socket: %d", _sock);
    return false;
  }

  fcntl(_sock, F_SETFL, O_NONBLOCK);

  return true;
}

bool UDPClient::beginPacket(IPAddress ip, uint16_t port) {
  _remoteIp = ip;
  _remotePort = port;
  return beginPacket();
}

bool UDPClient::beginPacket(const char *host, uint16_t port) {
  struct hostent *server;
  server = gethostbyname(host);
  if (server == NULL) {
    ESP_LOGD(TAG, "could not get host from dns");
    return false;
  }
  return beginPacket(IPAddress((const uint8_t *)(server->h_addr_list[0])),
                     port);
}

bool UDPClient::endPacket() {
  struct sockaddr_in recipient;
  recipient.sin_addr.s_addr = _remoteIp.toAddr();
  recipient.sin_family = AF_INET;
  recipient.sin_port = htons(_remotePort);
  int sent = lwip_sendto(_sock, _txBuffer, _txLen, 0,
                         (struct sockaddr *)&recipient, sizeof(recipient));
  if (sent < 0) {
    ESP_LOGD(TAG, "could not send data: %d", sent);
    return false;
  }
  return true;
}

size_t UDPClient::write(uint8_t data) {
  if (_txLen == 1460) {
    endPacket();
    _txLen = 0;
  }
  _txBuffer[_txLen++] = data;
  return 1;
}

size_t UDPClient::write(const uint8_t *buffer, size_t size) {
  size_t i;
  for (i = 0; i < size; i++) write(buffer[i]);
  return i;
}

size_t UDPClient::write(const char *buffer, size_t size) {
  return write((uint8_t *)buffer, size);
}

int UDPClient::parsePacket() {
  struct sockaddr_in si_other;
  int slen = sizeof(si_other);
  if ((_rxLen = lwip_recvfrom(_sock, _rxBuffer, 1460, MSG_DONTWAIT,
                              (struct sockaddr *)&si_other,
                              (socklen_t *)&slen)) < 0) {
    if (_rxLen == EWOULDBLOCK) {
      return 0;
    }
    ESP_LOGD(TAG, "could not receive data: %d", _rxLen);
    return 0;
  }
  _remoteIp = IPAddress(si_other.sin_addr.s_addr);
  _remotePort = ntohs(si_other.sin_port);
  _rxPtr = 0;
  return _rxLen;
}

bool UDPClient::available() { return (_rxLen > 0); }

int UDPClient::read() {
  if (_rxLen <= 0) {
    return -1;
  }
  int out = _rxBuffer[_rxPtr++];
  _rxLen--;
  return out;
}

int UDPClient::read(uint8_t *rxBuffer, size_t rxLen) {
  size_t len = min(rxLen, _rxLen);
  if (len <= 0) return 0;
  memcpy((void *)rxBuffer, (void *)_rxBuffer, len);
  rxLen = len;
  if (len < _rxLen) {
    memcpy((void *)_rxBuffer, (void *)(_rxBuffer + (_rxLen - len)),
           _rxLen - len);
    _rxLen -= len;
  }
  return rxLen;
}

int UDPClient::read(char *rxBuffer, size_t rxLen) {
  return read((uint8_t *)rxBuffer, rxLen);
}

int UDPClient::peek() {
  if (_rxLen <= 0) return -1;
  return _rxBuffer[_rxPtr];
}

void UDPClient::setTimeout(uint32_t timeout) {
  // Set timeout
  struct timeval _timeout;
  _timeout.tv_sec = timeout / 1000U;
  _timeout.tv_usec = (timeout % 1000U) * 1000U;
  setsockopt(_sock, SOL_SOCKET, SO_RCVTIMEO, &_timeout, sizeof _timeout);
}

// int UDPClient::send(uint8_t *txBuffer, size_t txLen) {
//   int err =
//       lwip_sendto(_sock, txBuffer, txLen, 0, (struct sockaddr *)_dest_addr,
//                   sizeof(struct sockaddr_in));
//   if (err < 0) {
//     ESP_LOGE(TAG, "Error occurred during sending: errno %d", err);
//   }
//   return err;
// }

// int UDPClient::sendAndReceive(uint8_t *txBuffer, size_t txLen,
//                               uint8_t *rxBuffer, size_t &rxLen,
//                               uint32_t timeout) {
//   int ret = send(txBuffer, txLen, timeout);
//   return ret < 0 ? ret : receive(rxBuffer, rxLen, timeout);
// }

// int UDPClient::receive(uint8_t *rxBuffer, size_t &rxLen, uint32_t timeout)
// {
//   setTimeout(timeout);

//   struct sockaddr_storage source_addr;  // Large enough for both IPv4 or
//   IPv6 socklen_t socklen = sizeof(source_addr); _rxLen =
//   lwip_recvfrom(_sock, _rxBuffer, rxLen, 0,
//                          (struct sockaddr *)&source_addr, &socklen);

//   // Error occurred during receiving
//   if (_rxLen < 0) {
//     ESP_LOGE(TAG, "recv failed: errno %d", _rxLen);
//     return _rxLen;
//   } else if (!_rxLen) {
//     return 0;
//   }
//   // Data rxLen
//   ESP_LOGI(TAG, "rxLen %d bytes from %s:", _rxLen, _remoteIp.toChar());
//   ESP_LOG_BUFFER_HEXDUMP(TAG, _rxBuffer, _rxLen, ESP_LOG_DEBUG);
//   return read(rxBuffer, rxLen);
// }

void UDPClient::printf(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  char buffer[1024];
  va_end(args);
  sprintf(buffer, fmt, args);
  ESP_LOG_BUFFER_HEXDUMP(TAG, buffer, strlen(buffer), ESP_LOG_DEBUG);
  write(buffer, strlen(buffer));
}

void UDPClient::flush() {
  _rxPtr = 0;
  if (_rxLen <= 0) return;
}

IPAddress UDPClient::remoteIP() { return _remoteIp; }

uint16_t UDPClient::remotePort() { return _remotePort; }