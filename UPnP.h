/**
 * @file UPnP.h
 * @author Phil Hilger (phil@peergum.com)
 * @brief
 * @version 0.1
 * @date 2022-07-15
 *
 * Copyright (c) 2022 Peerjuice
 *
 * This class is based on the UPnP Library by Ofek Pearl, September 2017
 * see: https://github.com/ofekp/TinyUPnP
 *
 */

#ifndef __UPNP_H_
#define __UPNP_H_

// #include <Arduino.h>
// #include <WiFiUdp.h>
// #include <WiFiClient.h>
// #include <limits.h>

// #include <char *>
#include "IPAddress.h"
#include "Wifi.h"
// #include <cchar *>
#include "esp_log.h"
#include "utils.h"
#include "TCPClient.h"
#include "UDPClient.h"

// #define F(char *) char *
// #define PSTR(char *) char *
// #define strcpy_P strcpy
// #define strcat_P strcat

// typedef std::char * char *;

#define UPNP_DEBUG
#define UPNP_SSDP_PORT 1900
#define TCP_CONNECTION_TIMEOUT_MS 6000
#define PORT_MAPPING_INVALID_INDEX \
  "<errorDescription>SpecifiedArrayIndexInvalid</errorDescription>"
#define PORT_MAPPING_INVALID_ACTION \
  "<errorDescription>Invalid Action</errorDescription>"

#define RULE_PROTOCOL_TCP "TCP"
#define RULE_PROTOCOL_UDP "UDP"

#define MAX_NUM_OF_UPDATES_WITH_NO_EFFECT \
  6  // after 6 tries of updatePortMappings we will execute the more extensive
     // addPortMapping

#define UDP_TX_PACKET_MAX_SIZE \
  1000  // reduce max UDP packet size to conserve memory (by default
        // UDP_TX_PACKET_MAX_SIZE=8192)
#define UDP_TX_RESPONSE_MAX_SIZE 8192

// TODO: idealy the SOAP actions should be verified as supported by the IGD
// before they are used 		 a struct can be created for each action and filled when
// the XML descriptor file is read
/*const char * SOAPActions [] = {
    "AddPortMapping",
    "GetSpecificPortMappingEntry",
    "DeletePortMapping",
    "GetGenericPortMappingEntry",
    "GetExternalIPAddress"
};*/

/*
#define SOAP_ERROR_TAG "errorDescription";
const char * SOAPErrors [] = {
    "SpecifiedArrayIndexInvalid",
    "Invalid Action"
};*/

/*
enum soapActionResult {
// TODO
}*/

typedef struct _SOAPAction {
  const char *name;
} SOAPAction;

typedef void (*callback_function)(void);

typedef struct _gatewayInfo {
  // router info
  IPAddress host;
  int port;  // this port is used when getting router capabilities and xml files
  char * path;  // this is the path that is used to retrieve router information
                // from xml files

  // info for actions
  int actionPort;          // this port is used when performing SOAP API actions
  char *actionPath;       // this is the path used to perform SOAP API actions
  char *serviceTypeName;  // i.e "WANPPPConnection:1" or "WANIPConnection:1"
} gatewayInfo;

typedef struct _upnpRule {
  int index;
  char *devFriendlyName;
  IPAddress internalAddr;
  int internalPort;
  int externalPort;
  char *protocol;
  int leaseDuration;
} upnpRule;

typedef struct _upnpRuleNode {
  _upnpRule *upnpRule;
  _upnpRuleNode *next;
} upnpRuleNode;

typedef struct _ssdpDevice {
  IPAddress host;
  int port;  // this port is used when getting router capabilities and xml files
  char * path;  // this is the path that is used to retrieve router information
                // from xml files
} ssdpDevice;

typedef struct _ssdpDeviceNode {
  _ssdpDevice *ssdpDevice;
  _ssdpDeviceNode *next;
} ssdpDeviceNode;

typedef enum {
  SUCCESS,         // port mapping was added
  ALREADY_MAPPED,  // the port mapping is already found in the IGD
  EMPTY_PORT_MAPPING_CONFIG,
  NETWORK_ERROR,
  TIMEOUT,
  VERIFICATION_FAILED,
  NOP  // the check is delayed
} portMappingResult;

class UPnP {
 public:
  UPnP(unsigned long timeoutMs);
  ~UPnP();
  // when the ruleIP is set to the current device IP, the IP of the rule will
  // change if the device changes its IP this makes sure the traffic will be
  // directed to the device even if the IP chnages
  void addPortMappingConfig(IPAddress ruleIP /* can be NULL */, int rulePort,
                            const char * ruleProtocol, int ruleLeaseDuration,
                            const char * ruleFriendlyName);
  portMappingResult commitPortMappings();
  portMappingResult updatePortMappings(
      unsigned long intervalMs,
      callback_function fallback = NULL /* optional */);
  bool printAllPortMappings();
  void printPortMappingConfig();  // prints all the port mappings that were
                                  // added using `addPortMappingConfig`
  bool testConnectivity();
  /* API extensions - additional methods to the UPnP API */
  ssdpDeviceNode *listSsdpDevices();  // will create an object with all SSDP
                                      // devices on the network
  void printSsdpDevices(ssdpDeviceNode *ssdpDeviceNode);  // will print all SSDP
                                                          // devices in teh list

  gatewayInfo _gwInfo;

 private:
  bool connectUDP();
  void broadcastMSearch(bool isSsdpAll = false);
  ssdpDevice *waitForUnicastResponseToMSearch(IPAddress gatewayIP);
  bool getGatewayInfo(gatewayInfo *deviceInfo);
  bool isGatewayInfoValid(gatewayInfo *deviceInfo);
  void clearGatewayInfo(gatewayInfo *deviceInfo);
  bool connectToIGD(IPAddress host, int port);
  bool getIGDEventURLs(gatewayInfo *deviceInfo);
  bool addPortMappingEntry(gatewayInfo *deviceInfo, upnpRule *rule_ptr);
  bool verifyPortMapping(gatewayInfo *deviceInfo, upnpRule *rule_ptr);
  bool deletePortMapping(gatewayInfo *deviceInfo, upnpRule *rule_ptr);
  bool applyActionOnSpecificPortMapping(SOAPAction *soapAction,
                                           gatewayInfo *deviceInfo,
                                           upnpRule *rule_ptr);
  void removeAllPortMappingsFromIGD();

  void upnpRulePrint(upnpRule *rule_ptr);
  // char * getSpaceschar *(int num);
  IPAddress getHost(char * url);
  int getPort(char * url);
  const char * getPath(char * url);
  const char * getTagContent(const char * line, const char * tagName);
  void ssdpDevicePrint(ssdpDevice *ssdpDevice);

  /* members */
  upnpRuleNode *_headRuleNode;
  unsigned long _lastUpdateTime;
  long _timeoutMs;  // 0 for blocking operation
  UDPClient _udpClient;
  TCPClient _tcpClient;
  unsigned long _consecutiveFails;
  char buffer[2048];
  char temp[255];
};

#endif  // __UPNP_H_