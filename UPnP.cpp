/**
 * @file UPnP.cpp
 * @author Phil Hilger (phil@peergum.com)
 * @brief
 * @version 0.1
 * @date 2022-07-15
 *
 * Copyright (c) 2022 Peerjuice
 *
 * This class is based on the TinyUPnP Library by Ofek Pearl, September 2017
 *
 */

#include "UPnP.h"
#include "Timer.h"
#include <string>

static const char *TAG = "UPnP";

IPAddress ipMulti(239, 255, 255, 250);           // multicast address for SSDP
IPAddress connectivityTestIp(64, 233, 187, 99);  // Google

char packetBuffer[UDP_TX_PACKET_MAX_SIZE];  // buffer to hold incoming packet
                                            // UDP_TX_PACKET_MAX_SIZE=8192
char responseBuffer[UDP_TX_RESPONSE_MAX_SIZE];

char tmpBody[1200];
char integerString[32];

SOAPAction SOAPActionGetSpecificPortMappingEntry = {
    .name = "GetSpecificPortMappingEntry"};
SOAPAction SOAPActionDeletePortMapping = {.name = "DeletePortMapping"};

static const char *const deviceListUpnp[] = {
    "urn:schemas-upnp-org:device:InternetGatewayDevice:1",
    "urn:schemas-upnp-org:device:InternetGatewayDevice:2",
    "urn:schemas-upnp-org:service:WANIPConnection:1",
    "urn:schemas-upnp-org:service:WANIPConnection:2",
    "urn:schemas-upnp-org:service:WANPPPConnection:1",
    // "upnp:rootdevice",
    0};

static const char *const deviceListSsdpAll[] = {"ssdp:all", 0};
const char *UPNP_SERVICE_TYPE_TAG_NAME = "serviceType";
const char *UPNP_SERVICE_TYPE_TAG_START = "<serviceType>";
const char *UPNP_SERVICE_TYPE_TAG_END = "</serviceType>";

void trim(char *text) {
  size_t len = strlen(text);
  for (int i = 1; i < len; i++) {
    if (text[len - i] == ' ') {
      text[len - i] = 0;
    }
  }
  len = strlen(text);
  int i;
  for (i = 0; i < len; i++) {
    if (text[i] != ' ') {
      break;
    }
  }
  if (i>0) {
    memcpy((void *)text, (void *)(text + i), len - i + 1);
  }
}

void replace(std::string str, const char *str1, const char *str2) {
  std::string newstr;
  int pos = str.find(str1);
  int len = strlen(str1);
  newstr = str.substr(0, pos) + str2 + str.substr(pos + len);
  str = newstr;
}

// timeoutMs - timeout in milli seconds for the operations of this class, 0 for
// blocking operation
UPnP::UPnP(unsigned long timeoutMs = 20000) {
  _timeoutMs = timeoutMs;
  _lastUpdateTime = 0;
  _consecutiveFails = 0;
  _headRuleNode = NULL;
  clearGatewayInfo(&_gwInfo);
}

UPnP::~UPnP() {}

void UPnP::addPortMappingConfig(IPAddress ruleIP, int rulePort,
                                const char *ruleProtocol, int ruleLeaseDuration,
                                const char *ruleFriendlyName) {
  static int index = 0;
  upnpRule *newUpnpRule = new upnpRule();
  newUpnpRule->index = index++;
  newUpnpRule->internalAddr = (ruleIP == wifi.localIP())
                                  ? ipNull
                                  : ruleIP;  // for automatic IP change handling
  newUpnpRule->internalPort = rulePort;
  newUpnpRule->externalPort = rulePort;
  newUpnpRule->leaseDuration = ruleLeaseDuration;
  newUpnpRule->protocol = strdup(ruleProtocol);
  newUpnpRule->devFriendlyName = strdup(ruleFriendlyName);

  // linked list insert
  upnpRuleNode *newUpnpRuleNode = new upnpRuleNode();
  newUpnpRuleNode->upnpRule = newUpnpRule;
  newUpnpRuleNode->next = NULL;

  if (_headRuleNode == NULL) {
    _headRuleNode = newUpnpRuleNode;
  } else {
    upnpRuleNode *currNode = _headRuleNode;
    while (currNode->next != NULL) {
      currNode = currNode->next;
    }
    currNode->next = newUpnpRuleNode;
  }
}

portMappingResult UPnP::commitPortMappings() {
  if (!_headRuleNode) {
    ESP_LOGD(TAG, "ERROR: No UPnP port mapping was set.");
    return EMPTY_PORT_MAPPING_CONFIG;
  }

  // verify wifi is connected
  if (!testConnectivity()) {
    ESP_LOGD(TAG, "ERROR: not connected to wifi, cannot continue");
    return NETWORK_ERROR;
  }

  Timer t;
  // get all the needed IGD information using SSDP if we don't have it already
  if (!isGatewayInfoValid(&_gwInfo)) {
    getGatewayInfo(&_gwInfo);
    if (_timeoutMs > 0 && t.check(_timeoutMs)) {
      ESP_LOGD(TAG, "ERROR: Invalid router info, cannot continue");
      _tcpClient.close();
      return NETWORK_ERROR;
    }
    delay(1000);  // longer delay to allow more time for the router to update
                  // its rules
  }

  ESP_LOGD(TAG, "port [%d] actionPort [%d]", _gwInfo.port, _gwInfo.actionPort);

  // double verify gateway information is valid
  if (!isGatewayInfoValid(&_gwInfo)) {
    ESP_LOGD(TAG, "ERROR: Invalid router info, cannot continue");
    return NETWORK_ERROR;
  }

  if (_gwInfo.port != _gwInfo.actionPort) {
    // in this case we need to connect to a different port
    ESP_LOGD(TAG, "Connection port changed, disconnecting from IGD");
    _tcpClient.close();
  }

  bool allPortMappingsAlreadyExist = true;  // for debug
  int addedPortMappings = 0;                // for debug
  upnpRuleNode *currNode = _headRuleNode;

  t.reset();
  while (currNode != NULL) {
    ESP_LOGI(TAG, "Verify port mapping for rule [%s]",
             currNode->upnpRule->devFriendlyName);
    bool currPortMappingAlreadyExists = true;  // for debug
    // TODO: since verifyPortMapping connects to the IGD then
    // addPortMappingEntry can skip it
    if (!verifyPortMapping(&_gwInfo, currNode->upnpRule)) {
      // need to add the port mapping
      currPortMappingAlreadyExists = false;
      allPortMappingsAlreadyExist = false;
      if (_timeoutMs > 0 && t.check(_timeoutMs)) {
        ESP_LOGD(TAG, "Timeout expired while trying to add a port mapping");
        _tcpClient.close();
        return TIMEOUT;
      }

      addPortMappingEntry(&_gwInfo, currNode->upnpRule);

      int tries = 0;
      while (tries <= 3) {
        delay(2000);  // longer delay to allow more time for the router to
                      // update its rules
        if (verifyPortMapping(&_gwInfo, currNode->upnpRule)) {
          break;
        }
        tries++;
      }

      if (tries > 3) {
        _tcpClient.close();
        return VERIFICATION_FAILED;
      }
    }

    if (!currPortMappingAlreadyExists) {
      addedPortMappings++;
      ESP_LOGI(TAG, "Port mapping [%s] was added",
               currNode->upnpRule->devFriendlyName);
    }

    currNode = currNode->next;
  }

  _tcpClient.close();

  if (allPortMappingsAlreadyExist) {
    ESP_LOGI(
        TAG,
        "All port mappings were already found in the IGD, not doing anything");
    return ALREADY_MAPPED;
  } else {
    // addedPortMappings is at least 1 here
    if (addedPortMappings > 1) {
      ESP_LOGI(TAG, "%d UPnP port mappings were added", addedPortMappings);
    } else {
      ESP_LOGI(TAG, "One UPnP port mapping was added");
    }
  }

  return SUCCESS;
}

bool UPnP::getGatewayInfo(gatewayInfo *deviceInfo) {
  Timer t;
  while (!connectUDP()) {
    if (_timeoutMs > 0 && t.check(_timeoutMs)) {
      ESP_LOGD(TAG, "Timeout expired while connecting UDP");
      _udpClient.stop();
      return false;
    }
    delay(500);
  }

  broadcastMSearch();
  IPAddress gatewayIP = wifi.gatewayIP();

  ESP_LOGD(TAG, "Gateway IP [%s]", gatewayIP.toChar());

  t.reset();
  ssdpDevice *ssdpDevice_ptr = NULL;
  while ((ssdpDevice_ptr = waitForUnicastResponseToMSearch(gatewayIP)) ==
         NULL) {
    if (_timeoutMs > 0 && t.check(_timeoutMs)) {
      ESP_LOGD(
          TAG,
          "Timeout expired while waiting for the gateway router to respond to "
          "M-SEARCH message");
      _udpClient.stop();
      return false;
    }
    delay(1);
  }

  deviceInfo->host = ssdpDevice_ptr->host;
  deviceInfo->port = ssdpDevice_ptr->port;
  deviceInfo->path = ssdpDevice_ptr->path;
  // the following is the default and may be overridden if URLBase tag is
  // specified
  deviceInfo->actionPort = ssdpDevice_ptr->port;
  // close the UDP connection
  _udpClient.stop();

  t.reset();
  // connect to IGD (TCP connection)
  while (!connectToIGD(deviceInfo->host, deviceInfo->port)) {
    if (_timeoutMs > 0 && t.check(_timeoutMs)) {
      ESP_LOGD(TAG, "Timeout expired while trying to connect to the IGD");
      _tcpClient.close();
      return false;
    }
    delay(500);
  }

  t.reset();
  // get event urls from the gateway IGD
  while (!getIGDEventURLs(deviceInfo)) {
    if (_timeoutMs > 0 && t.check(_timeoutMs)) {
      ESP_LOGD(TAG, "Timeout expired while adding a new port mapping");
      _tcpClient.close();
      return false;
    }
    delay(500);
  }

  return true;
}

void UPnP::clearGatewayInfo(gatewayInfo *deviceInfo) {
  deviceInfo->host = IPAddress(0, 0, 0, 0);
  deviceInfo->port = 0;
  deviceInfo->path = strdup("");
  deviceInfo->actionPort = 0;
  deviceInfo->actionPath = strdup("");
  deviceInfo->serviceTypeName = strdup("");
}

bool UPnP::isGatewayInfoValid(gatewayInfo *deviceInfo) {
  ESP_LOGD(TAG,
           "isGatewayInfoValid [%s] port [%d] path [%s] actionPort [%d] "
           "actionPath [%s] serviceTypeName [%s]",
           deviceInfo->host.toChar(), deviceInfo->port, deviceInfo->path,
           deviceInfo->actionPort, deviceInfo->actionPath,
           deviceInfo->serviceTypeName);

  if (deviceInfo->host == ipNull || deviceInfo->port == 0 ||
      strlen(deviceInfo->path) == 0 || deviceInfo->actionPort == 0) {
    ESP_LOGD(TAG, "Gateway info is not valid");
    return false;
  }

  ESP_LOGD(TAG, "Gateway info is valid");
  return true;
}

portMappingResult UPnP::updatePortMappings(unsigned long intervalMs,
                                           callback_function fallback) {
  if (millis() - _lastUpdateTime >= intervalMs) {
    ESP_LOGD(TAG, "Updating port mapping");

    // fallback
    if (_consecutiveFails >= MAX_NUM_OF_UPDATES_WITH_NO_EFFECT) {
      ESP_LOGD(
          TAG,
          "ERROR: Too many times with no effect on updatePortMappings. Current "
          "number of fallbacks times [%lu]",
          _consecutiveFails);

      _consecutiveFails = 0;
      clearGatewayInfo(&_gwInfo);
      if (fallback != NULL) {
        ESP_LOGD(TAG, "Executing fallback method");
        fallback();
      }

      return TIMEOUT;
    }

    // } else if (_consecutiveFails > 300) {
    // 	ESP.restart();  // should test as last resort
    // 	return;
    // }

    portMappingResult result = commitPortMappings();

    if (result == SUCCESS || result == ALREADY_MAPPED) {
      _lastUpdateTime = millis();
      _tcpClient.close();
      _consecutiveFails = 0;
      return result;
    } else {
      _lastUpdateTime += intervalMs / 2;  // delay next try
      ESP_LOGD(TAG,
               "ERROR: While updating UPnP port mapping. Failed with error "
               "code [%d]",
               result);
      _tcpClient.close();
      _consecutiveFails++;
      return result;
    }
  }

  _tcpClient.close();
  return NOP;  // no need to check yet
}

bool UPnP::testConnectivity() {
  Timer t;
  ESP_LOGI(TAG, "Testing wifi connection for [%s]", wifi.localIP().toChar());
  while (wifi.status() != CONNECTED) {
    if (_timeoutMs > 0 && t.check(_timeoutMs)) {
      ESP_LOGW(TAG, " ==> Timeout expired while verifying wifi connection");
      _tcpClient.close();
      return false;
    }
    delay(200);
    // ESP_LOGD(TAG,".";
  }
  ESP_LOGD(TAG, "Wifi Connected");  // \n

  ESP_LOGD(TAG, "Testing internet connection");
  _tcpClient.connect(connectivityTestIp, 80);
  t.reset();
  while (!_tcpClient.connected()) {
    if (t.check(TCP_CONNECTION_TIMEOUT_MS)) {
      ESP_LOGW(TAG, "-> Fail");
      _tcpClient.close();
      return false;
    }
  }

  ESP_LOGI(TAG, "-> Success");
  _tcpClient.close();
  return true;
}

bool UPnP::verifyPortMapping(gatewayInfo *deviceInfo, upnpRule *rule_ptr) {
  if (!applyActionOnSpecificPortMapping(&SOAPActionGetSpecificPortMappingEntry,
                                        deviceInfo, rule_ptr)) {
    return false;
  }

  ESP_LOGD(TAG, "verifyPortMapping called");

  // TODO: extract the current lease duration and return it instead of a bool
  bool success = false;
  bool newIP = false;
  while (_tcpClient.available()) {
    std::string line = _tcpClient.readUntil('\r');
    // ESP_LOGD(TAG, "%s", line.c_str());
    if (line.find("errorCode") != -1) {
      success = false;
      // flush response and exit loop
      while (_tcpClient.available()) {
        line = _tcpClient.readUntil('\r');
        // ESP_LOGD(TAG, "%s", line.c_str());
      }
      continue;
    }

    if (line.find("NewInternalClient") != -1) {
      const char *content = getTagContent(line.c_str(), "NewInternalClient");
      if (content) {
        IPAddress ipAddressToVerify = (rule_ptr->internalAddr == ipNull)
                                          ? wifi.localIP()
                                          : rule_ptr->internalAddr;
        if (!strcmp(content, ipAddressToVerify.toChar())) {
          success = true;
        } else {
          newIP = true;
        }
      }
    }
  }

  _tcpClient.close();

  if (success) {
    ESP_LOGI(TAG, "Port mapping found in IGD");
  } else if (newIP) {
    ESP_LOGI(TAG, "Detected a change in IP");
    removeAllPortMappingsFromIGD();
  } else {
    ESP_LOGI(TAG, "Could not find port mapping in IGD");
  }

  return success;
}

bool UPnP::deletePortMapping(gatewayInfo *deviceInfo, upnpRule *rule_ptr) {
  if (!applyActionOnSpecificPortMapping(&SOAPActionDeletePortMapping,
                                        deviceInfo, rule_ptr)) {
    return false;
  }

  bool isSuccess = false;
  while (_tcpClient.available()) {
    std::string line = _tcpClient.readUntil('\r');
    // ESP_LOGD(TAG, "%s", line.c_str());
    if (line.find("errorCode") != -1) {
      isSuccess = false;
      // flush response and exit loop
      while (_tcpClient.available()) {
        line = _tcpClient.readUntil('\r');
        // ESP_LOGD(TAG, "%s", line.c_str());
      }
      continue;
    }
    if (line.find("DeletePortMappingResponse") != -1) {
      isSuccess = true;
    }
  }

  return isSuccess;
}

bool UPnP::applyActionOnSpecificPortMapping(SOAPAction *soapAction,
                                            gatewayInfo *deviceInfo,
                                            upnpRule *rule_ptr) {
  ESP_LOGD(TAG, "Apply action [%s] on port mapping [%s]", soapAction->name,
           rule_ptr->devFriendlyName);

  // connect to IGD (TCP connection) again, if needed, in case we got
  // disconnected after the previous query
  Timer t;
  if (!_tcpClient.connected()) {
    while (!connectToIGD(deviceInfo->host, deviceInfo->actionPort)) {
      if (t.check(TCP_CONNECTION_TIMEOUT_MS)) {
        ESP_LOGD(TAG, "Timeout expired while trying to connect to the IGD");
        _tcpClient.close();
        return false;
      }
      delay(500);
    }
  }

  strcpy(tmpBody,
         "<?xml version=\"1.0\"?>\r\n<s:Envelope "
         "xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" "
         "s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/"
         "\">\r\n<s:Body>\r\n<u:");
  strcat(tmpBody, soapAction->name);
  strcat(tmpBody, " xmlns:u=\"");
  strcat(tmpBody, deviceInfo->serviceTypeName);
  strcat(tmpBody,
         "\">\r\n<NewRemoteHost></NewRemoteHost>\r\n<NewExternalPort>");
  sprintf(integerString, "%d", rule_ptr->internalPort);
  strcat(tmpBody, integerString);
  strcat(tmpBody, "</NewExternalPort>\r\n<NewProtocol>");
  strcat(tmpBody, rule_ptr->protocol);
  strcat(tmpBody, "</NewProtocol>\r\n</u:");
  strcat(tmpBody, soapAction->name);
  strcat(tmpBody, ">\r\n</s:Body>\r\n</s:Envelope>\r\n");

  // printf(tmpBody);

  sprintf(buffer,
          "POST %s HTTP/1.1\r\n"
          "Connection: close\r\n"
          "Content-Type: text/xml; charset=\"utf-8\"\r\n"
          "Host: %s:%d\r\n"
          "SOAPAction: \"%s#%s\"\r\n"
          "Content-Length: %d\r\n\r\n",
          deviceInfo->actionPath, deviceInfo->host.toChar(),
          deviceInfo->actionPort, deviceInfo->serviceTypeName, soapAction->name,
          strlen(tmpBody));
  _tcpClient.write(buffer, strlen(buffer));
  _tcpClient.write(tmpBody,strlen(tmpBody));

  t.reset();
  while (!_tcpClient.available()) {
    if (t.check(TCP_CONNECTION_TIMEOUT_MS)) {
      ESP_LOGD(TAG, "TCP connection timeout while retrieving port mappings");
      _tcpClient.close();
      // TODO: in this case we might not want to add the ports right away
      // might want to try again or only start adding the ports after we
      // definitely did not see them in the router list
      return false;
    }
  }
  return true;
}

void UPnP::removeAllPortMappingsFromIGD() {
  upnpRuleNode *currNode = _headRuleNode;
  while (currNode != NULL) {
    deletePortMapping(&_gwInfo, currNode->upnpRule);
    currNode = currNode->next;
  }
}

// a single try to connect UDP multicast address and port of UPnP
// (239.255.255.250 and 1900 respectively) this will enable receiving SSDP
// packets after the M-SEARCH multicast message will be broadcasted
bool UPnP::connectUDP() {
  if (_udpClient.beginMulticast(ipMulti, UPNP_SSDP_PORT)) {
    return true;
  }

  ESP_LOGD(TAG, "UDP connection failed");
  return false;
}

// broadcast an M-SEARCH message to initiate messages from SSDP devices
// the router should respond to this message by a packet sent to this device's
// unicast addresss on the same UPnP port (1900)
void UPnP::broadcastMSearch(bool isSsdpAll /*=false*/) {
  ESP_LOGD(TAG, "Sending M-SEARCH to [%s] port [%d]", ipMulti.toChar(),
           UPNP_SSDP_PORT);

  uint8_t beginMulticastPacketRes = _udpClient.beginMulticastPacket();
  ESP_LOGD(TAG, "beginMulticastPacketRes [%d]", beginMulticastPacketRes);

  const char *const *deviceList = deviceListUpnp;
  if (isSsdpAll) {
    deviceList = deviceListSsdpAll;
  }

  for (int i = 0; deviceList[i]; i++) {
    sprintf(tmpBody,
            "M-SEARCH * HTTP/1.1\r\n"
            "HOST: 239.255.255.250:%d\r\n"
            "MAN: \"ssdp:discover\"\r\n"
            "MX: 2\r\n"  // allowed number of seconds to wait before
            "ST: %s\r\n"
            "USER-AGENT: unix/5.1 UPnP/2.0 UPnP/1.0\r\n"
            "\r\n\r\n",
            UPNP_SSDP_PORT, deviceList[i]);

    // ESP_LOGD(TAG, "%s", tmpBody);
    size_t len = strlen(tmpBody);
    ESP_LOGD(TAG, "M-SEARCH packet length is [%d]", len);

    _udpClient.printf(tmpBody);

    int endPacketRes = _udpClient.endPacket();
    ESP_LOGD(TAG, "endPacketRes [%d]", endPacketRes);
  }

  ESP_LOGD(TAG, "M-SEARCH packets sent");
}

ssdpDeviceNode *UPnP::listSsdpDevices() {
  if (_timeoutMs <= 0) {
    ESP_LOGD(TAG,
             "Timeout must be set when initializing UPnP to use this method, "
             "exiting.");
    return NULL;
  }

  Timer t;
  while (!connectUDP()) {
    if (_timeoutMs > 0 && t.check(_timeoutMs)) {
      ESP_LOGD(TAG, "Timeout expired while connecting UDP");
      _udpClient.stop();
      return NULL;
    }
    delay(500);
  }

  broadcastMSearch(true);
  IPAddress gatewayIP = wifi.gatewayIP();

  ESP_LOGI(TAG, "Gateway IP [%s]", gatewayIP.toChar());

  ssdpDeviceNode *ssdpDeviceNode_head = NULL;
  ssdpDeviceNode *ssdpDeviceNode_tail = NULL;
  ssdpDeviceNode *ssdpDeviceNode_ptr = NULL;
  ssdpDevice *ssdpDevice_ptr;
  t.reset();
  while (true) {
    ssdpDevice_ptr = waitForUnicastResponseToMSearch(
        ipNull);  // NULL will cause finding all SSDP device (not just the IGD)
    if (_timeoutMs > 0 && t.check(_timeoutMs)) {
      ESP_LOGD(
          TAG,
          "Timeout expired while waiting for the gateway router to respond to "
          "M-SEARCH message");
      _udpClient.stop();
      break;
    }

    ssdpDevicePrint(ssdpDevice_ptr);

    if (ssdpDevice_ptr != NULL) {
      if (ssdpDeviceNode_head == NULL) {
        ssdpDeviceNode_head = new ssdpDeviceNode();
        ssdpDeviceNode_head->ssdpDevice = ssdpDevice_ptr;
        ssdpDeviceNode_head->next = NULL;
        ssdpDeviceNode_tail = ssdpDeviceNode_head;
      } else {
        ssdpDeviceNode_ptr = new ssdpDeviceNode();
        ssdpDeviceNode_ptr->ssdpDevice = ssdpDevice_ptr;
        ssdpDeviceNode_tail->next = ssdpDeviceNode_ptr;
        ssdpDeviceNode_tail = ssdpDeviceNode_ptr;
      }
    }

    delay(5);
  }

  // close the UDP connection
  _udpClient.stop();

  // dedup SSDP devices fromt the list - O(n^2)
  ssdpDeviceNode_ptr = ssdpDeviceNode_head;
  while (ssdpDeviceNode_ptr != NULL) {
    ssdpDeviceNode *ssdpDeviceNodePrev_ptr = ssdpDeviceNode_ptr;
    ssdpDeviceNode *ssdpDeviceNodeCurr_ptr = ssdpDeviceNode_ptr->next;

    while (ssdpDeviceNodeCurr_ptr != NULL) {
      if (ssdpDeviceNodeCurr_ptr->ssdpDevice->host ==
              ssdpDeviceNode_ptr->ssdpDevice->host &&
          ssdpDeviceNodeCurr_ptr->ssdpDevice->port ==
              ssdpDeviceNode_ptr->ssdpDevice->port &&
          ssdpDeviceNodeCurr_ptr->ssdpDevice->path ==
              ssdpDeviceNode_ptr->ssdpDevice->path) {
        // delete ssdpDeviceNode from the list
        ssdpDeviceNodePrev_ptr->next = ssdpDeviceNodeCurr_ptr->next;
        free(ssdpDeviceNodeCurr_ptr->ssdpDevice);
        free(ssdpDeviceNodeCurr_ptr);
        ssdpDeviceNodeCurr_ptr = ssdpDeviceNodePrev_ptr->next;
      } else {
        ssdpDeviceNodePrev_ptr = ssdpDeviceNodeCurr_ptr;
        ssdpDeviceNodeCurr_ptr = ssdpDeviceNodeCurr_ptr->next;
      }
    }
    ssdpDeviceNode_ptr = ssdpDeviceNode_ptr->next;
  }

  return ssdpDeviceNode_head;
}

// Assuming an M-SEARCH message was broadcaseted, wait for the response from the
// IGD (Internet Gateway Device) Note: the response from the IGD is sent back as
// unicast to this device Note: only gateway defined IGD response will be
// considered, the rest will be ignored
ssdpDevice *UPnP::waitForUnicastResponseToMSearch(IPAddress gatewayIP) {
  int packetSize = _udpClient.parsePacket();

  // only continue if a packet is available
  if (packetSize <= 0) {
    return NULL;
  }

  IPAddress remoteIP = _udpClient.remoteIP();
  // only continue if the packet was received from the gateway router
  // for SSDP discovery we continue anyway
  if (gatewayIP != ipNull && remoteIP != gatewayIP) {
    ESP_LOGD(TAG,
             "Discarded packet not originating from IGD - gatewayIP [%s] "
             "remoteIP [%s]",
             gatewayIP.toChar(), ipMulti.toChar());
    return NULL;
  }

  ESP_LOGD(TAG, "Received packet of size [%d] ip [%s] port [%d]", packetSize,
           remoteIP.toChar(), _udpClient.remotePort());

  // sanity check
  if (packetSize > UDP_TX_RESPONSE_MAX_SIZE) {
    ESP_LOGD(
        TAG,
        "Received packet with size larged than the response buffer, cannot "
        "proceed.");
    return NULL;
  }

  int idx = 0;
  while (idx < packetSize) {
    memset(packetBuffer, 0, UDP_TX_PACKET_MAX_SIZE);
    int len = _udpClient.read(packetBuffer, UDP_TX_PACKET_MAX_SIZE);
    if (len <= 0) {
      break;
    }
    ESP_LOGD(TAG, "UDP packet read bytes [%d] out of [%d]", len, packetSize);
    memcpy(responseBuffer + idx, packetBuffer, len);
    idx += len;
  }
  responseBuffer[idx] = '\0';

  // ESP_LOGD(TAG, "Gateway packet content: %s", responseBuffer);

  const char *const *deviceList = deviceListUpnp;
  if (gatewayIP == ipNull) {
    deviceList = deviceListSsdpAll;
  }

  // only continue if the packet is a response to M-SEARCH and it originated
  // from a gateway device for SSDP discovery we continue anyway
  if (gatewayIP != ipNull) {  // for the use of listSsdpDevices
    bool foundIGD = false;
    for (int i = 0; deviceList[i]; i++) {
      if (strstr(responseBuffer, deviceList[i]) != NULL) {
        foundIGD = true;
        ESP_LOGI(TAG, "IGD of type [%s] found", deviceList[i]);
        break;
      }
    }

    if (!foundIGD) {
      ESP_LOGW(TAG, "IGD was not found");
      return NULL;
    }
  }

  char *location;
  char *location_indexStart = strstr(responseBuffer, "location:");
  if (location_indexStart == NULL) {
    location_indexStart = strstr(responseBuffer, "Location:");
  }
  if (location_indexStart == NULL) {
    location_indexStart = strstr(responseBuffer, "LOCATION:");
  }
  if (location_indexStart != NULL) {
    location_indexStart += 10;  // "location:".length()
    char *location_indexEnd = strstr(location_indexStart, "\r\n");
    if (location_indexEnd != NULL) {
      int urlLength = location_indexEnd - location_indexStart;
      int arrLength = urlLength + 1;  // + 1 for '\0'
      // converting the start index to be inside the packetBuffer rather than
      // responseBuffer
      location = (char *)malloc(arrLength);
      memcpy(location, location_indexStart, urlLength);
      location[arrLength - 1] = '\0';

    } else {
      ESP_LOGD(TAG, "ERROR: could not extract value from LOCATION param");
      return NULL;
    }
  } else {
    ESP_LOGD(TAG, "ERROR: LOCATION param was not found");
    return NULL;
  }

  ESP_LOGD(TAG, "Device location found [%s]", location);

  IPAddress host;
  char port[6];
  char path[1024];
  char hostname[256];
  char protocol[30];

  parseUrl(location, protocol, hostname, port, path);
  free(location);
  ssdpDevice *newSsdpDevice_ptr = new ssdpDevice();

  if (!newSsdpDevice_ptr) {
    return NULL;
  }
  newSsdpDevice_ptr->host.fromChar(hostname);
  newSsdpDevice_ptr->port = atoi(port);
  newSsdpDevice_ptr->path = strdup(path);

  // ESP_LOGD(TAG,host.toChar());
  // ESP_LOGD(TAG,char *(port));
  // ESP_LOGD(TAG,path);

  return newSsdpDevice_ptr;
}

// a single trial to connect to the IGD (with TCP)
bool UPnP::connectToIGD(IPAddress host, int port) {
  ESP_LOGD(TAG, "Connecting to IGD with host [%s] port [%d]", host.toChar(),
           port);
  if (_tcpClient.connect(host, port)) {
    ESP_LOGD(TAG, "Connected to IGD");
    return true;
  }
  return false;
}

// updates deviceInfo with the commands' information of the IGD
bool UPnP::getIGDEventURLs(gatewayInfo *deviceInfo) {
  ESP_LOGD(TAG, "called getIGDEventURLs");
  ESP_LOGD(TAG, "deviceInfo->actionPath [%s] deviceInfo->path [%s]",
           deviceInfo->actionPath, deviceInfo->path);

  // make an HTTP request
  sprintf(buffer,
      "GET %s HTTP/1.1\r\n"
      "Content-Type: text/xml; charset=\"utf-8\"\r\n"
      "Host: %s:%d\r\n"
      "Content-Length: 0\r\n\r\n",
      deviceInfo->path, deviceInfo->host.toChar(), deviceInfo->actionPort);

  _tcpClient.write(buffer, strlen(buffer));

  Timer t;
  // wait for the response
  while (_tcpClient.available() == 0) {
    if (t.check(TCP_CONNECTION_TIMEOUT_MS)) {
      ESP_LOGD(TAG, "TCP connection timeout while executing getIGDEventURLs");
      _tcpClient.close();
      return false;
    }
  }

  // read all the lines of the reply from server
  bool upnpServiceFound = false;
  bool urlBaseFound = false;
  std::string line;
  while (_tcpClient.available()) {
    line = _tcpClient.readUntil('\r');
    int index_in_line = 0;
    if (!urlBaseFound && line.find("<URLBase>") != -1) {
      // e.g. <URLBase>http://192.168.1.1:5432/</URLBase>
      // Note: assuming URL path will only be found in a specific action under
      // the 'controlURL' xml tag
      char *baseUrl = strdup(getTagContent(line.c_str(), "URLBase"));
      if (strlen(baseUrl) > 0) {
        trim(baseUrl);
        IPAddress host = getHost(baseUrl);  // this is ignored, assuming router
                                            // host IP will not change
        int port = getPort(baseUrl);
        deviceInfo->actionPort = port;

        ESP_LOGD(TAG,"URLBase tag found [%s]", baseUrl);
        ESP_LOGD(TAG, "Translated to base host [%s] and base port [%d]",
                 host.toChar(), port);
        urlBaseFound = true;
      }
    }

    // to support multiple <serviceType> tags
    int service_type_index_start = 0;

    for (int i = 0; deviceListUpnp[i]; i++) {
      char serviceTag[100];
      sprintf(serviceTag, "%s%s", UPNP_SERVICE_TYPE_TAG_START,
              deviceListUpnp[i]);
      int service_type_index =
          line.find(serviceTag);
      if (service_type_index >= 0) {
        ESP_LOGD(TAG, "[%s] service_type_index [%d]",
                 deviceInfo->serviceTypeName, service_type_index);
        service_type_index_start = service_type_index;
        service_type_index =
            line.find(UPNP_SERVICE_TYPE_TAG_END, service_type_index_start);
      }
      if (!upnpServiceFound && service_type_index >= 0) {
        index_in_line += service_type_index;
        upnpServiceFound = true;
        deviceInfo->serviceTypeName = strdup(getTagContent(
            line.substr(service_type_index_start).c_str(), UPNP_SERVICE_TYPE_TAG_NAME));
        ESP_LOGD(TAG, "[%s] service found! deviceType [%s]",
                 deviceInfo->serviceTypeName, deviceListUpnp[i]);
        break;  // will start looking for 'controlURL' now
      }
    }

    if (upnpServiceFound &&
        (index_in_line = line.find("<controlURL>", index_in_line)) >= 0) {
      const char *controlURLContent =
          getTagContent(line.substr(index_in_line).c_str(), "controlURL");
      if (strlen(controlURLContent) > 0) {
        deviceInfo->actionPath = strdup(controlURLContent);

        ESP_LOGD(TAG, "controlURL tag found! setting actionPath to [%s]",
                 controlURLContent);

        // clear buffer
        ESP_LOGD(TAG, "Flushing the rest of the response");
        while (_tcpClient.available()) {
          _tcpClient.read();
        }

        // now we have (upnpServiceFound && controlURLFound)
        return true;
      }
    }
  }

  return false;
}

// assuming a connection to the IGD has been formed
// will add the port mapping to the IGD
bool UPnP::addPortMappingEntry(gatewayInfo *deviceInfo, upnpRule *rule_ptr) {
  ESP_LOGD(TAG, "called addPortMappingEntry");


  // connect to IGD (TCP connection) again, if needed, in case we got
  // disconnected after the previous query
  Timer t;
  if (!_tcpClient.connected()) {
    while (!connectToIGD(_gwInfo.host, _gwInfo.actionPort)) {
      if (t.check(TCP_CONNECTION_TIMEOUT_MS)) {
        ESP_LOGD(TAG, "Timeout expired while trying to connect to the IGD");
        _tcpClient.close();
        return false;
      }
      delay(500);
    }
  }

  ESP_LOGD(TAG, "deviceInfo->actionPath [%s]", deviceInfo->actionPath);

  ESP_LOGD(TAG, "deviceInfo->serviceTypeName [%s]",
           deviceInfo->serviceTypeName);

  sprintf(integerString, "%d", rule_ptr->internalPort);
  IPAddress ipAddress = (rule_ptr->internalAddr == ipNull)
                            ? wifi.localIP()
                            : rule_ptr->internalAddr;
  strcat(tmpBody, ipAddress.toChar());
  strcpy(tmpBody,
         "<?xml version=\"1.0\"?><s:Envelope "
         "xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" "
         "s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/"
         "\"><s:Body><u:AddPortMapping xmlns:u=\"");
  strcat(tmpBody, deviceInfo->serviceTypeName);
  strcat(tmpBody, "\"><NewRemoteHost></NewRemoteHost><NewExternalPort>");
  strcat(tmpBody, integerString);
  strcat(tmpBody, "</NewExternalPort><NewProtocol>");
  strcat(tmpBody, rule_ptr->protocol);
  strcat(tmpBody, "</NewProtocol><NewInternalPort>");
  strcat(tmpBody, integerString);
  strcat(tmpBody, "</NewInternalPort><NewInternalClient>");
  strcat(tmpBody, ipAddress.toChar());
  strcat(tmpBody,
         "</NewInternalClient><NewEnabled>1</"
         "NewEnabled><NewPortMappingDescription>");
  strcat(tmpBody, rule_ptr->devFriendlyName);
  strcat(tmpBody, "</NewPortMappingDescription><NewLeaseDuration>");
  sprintf(integerString, "%d", rule_ptr->leaseDuration);
  strcat(tmpBody, integerString);
  strcat(tmpBody,
         "</NewLeaseDuration></u:AddPortMapping></s:Body></s:Envelope>\r\n");

  // printf(tmpBody);
  sprintf(buffer,
          "POST %s HTTP/1.1\r\n"
          "Content-Type: text/xml; charset=\"utf-8\"\r\n"
          "Host: %s:%d\r\n"
          "SOAPAction: \"%s#AddPortMapping\"\r\n"
          "Content-Length: %d\r\n\r\n",
          deviceInfo->actionPath, deviceInfo->host.toChar(),
          deviceInfo->actionPort, deviceInfo->serviceTypeName, strlen(tmpBody));

  _tcpClient.write(buffer, strlen(buffer));
  _tcpClient.write(tmpBody,strlen(tmpBody));

  ESP_LOGD(TAG, "Content-Length was: %d", strlen(tmpBody));

  t.reset();
  while (_tcpClient.available() == 0) {
    if (t.check(TCP_CONNECTION_TIMEOUT_MS)) {
      ESP_LOGD(TAG, "TCP connection timeout while adding a port mapping");
      _tcpClient.close();
      return false;
    }
  }

  // TODO: verify success
  bool isSuccess = true;
  while (_tcpClient.available()) {
    std::string line = _tcpClient.readUntil('\r');
    if (line.find("errorCode") != -1) {
      isSuccess = false;
    }
    // ESP_LOGD(TAG, "%s", line.c_str());
  }

  if (!isSuccess) {
    _tcpClient.close();
  }

  return isSuccess;
}

bool UPnP::printAllPortMappings() {
  // verify gateway information is valid
  // TODO: use this _gwInfo to skip the UDP part completely if it is not empty
  if (!isGatewayInfoValid(&_gwInfo)) {
    ESP_LOGD(TAG, "Invalid router info, cannot continue");
    return false;
  }

  upnpRuleNode *ruleNodeHead_ptr = NULL;
  upnpRuleNode *ruleNodeTail_ptr = NULL;

  bool reachedEnd = false;
  int index = 0;
  Timer t;
  while (!reachedEnd) {
    // connect to IGD (TCP connection) again, if needed, in case we got
    // disconnected after the previous query
    if (!_tcpClient.connected()) {
      while (!connectToIGD(_gwInfo.host, _gwInfo.actionPort)) {
        if (t.check(TCP_CONNECTION_TIMEOUT_MS)) {
          ESP_LOGD(TAG, "Timeout expired while trying to connect to the IGD");
          _tcpClient.close();
          return false;
        }
        delay(1000);
      }
    }

    ESP_LOGD(TAG, "Sending query for index [%d]", index);

    strcpy(
        tmpBody,
            "<?xml version=\"1.0\"?>"
            "<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" "
            "s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">"
            "<s:Body>"
            "<u:GetGenericPortMappingEntry xmlns:u=\"");
    strcat(tmpBody, _gwInfo.serviceTypeName);
    strcat(tmpBody, "\"><NewPortMappingIndex>");

    sprintf(integerString, "%d", index);
    strcat(tmpBody, integerString);
    strcat(tmpBody, "</NewPortMappingIndex>"
                         "</u:GetGenericPortMappingEntry>"
                         "</s:Body>"
                         "</s:Envelope>");

    // printf(tmpBody);

    sprintf(buffer,
            "POST %s HTTP/1.1\r\n"
            "Connection: keep-alive\r\n"
            "Content-Type: text/xml; charset=\"utf-8\"\r\n"
            "Host: %s:%d\r\n"
            "SOAPAction: \"%s#GetGenericPortMappingEntry\"\r\n"
            "Content-Length: %d\r\n\r\n",
            _gwInfo.actionPath, _gwInfo.host.toChar(), _gwInfo.actionPort,
            _gwInfo.serviceTypeName, strlen(tmpBody));
    _tcpClient.write(buffer, strlen(buffer));
    _tcpClient.write(tmpBody,strlen(tmpBody));

    t.reset();
    while (_tcpClient.available() == 0) {
      if (t.check(TCP_CONNECTION_TIMEOUT_MS)) {
        ESP_LOGD(TAG, "TCP connection timeout while retrieving port mappings");
        _tcpClient.close();
        reachedEnd = true;
        break;
      }
    }

    std::string line = "";
    upnpRule *rule_ptr = NULL;
    while (_tcpClient.available()) {
      line = _tcpClient.readUntil('\r');
      // ESP_LOGD(TAG, "%s", line.c_str());
      if (line.find(PORT_MAPPING_INVALID_INDEX) != -1) {
        reachedEnd = true;
      } else if (line.find(PORT_MAPPING_INVALID_ACTION) != -1) {
        ESP_LOGD(TAG, "Invalid action while reading port mappings");
        reachedEnd = true;
      } else if (line.find("HTTP/1.1 500 ") != -1) {
        ESP_LOGD(TAG,
                 "Internal server error, likely because we have shown all the "
                 "mappings");
        reachedEnd = true;
      } else if (line.find("GetGenericPortMappingEntryResponse") != -1) {
        rule_ptr = new upnpRule();
        rule_ptr->index = index;
      } else if (line.find("NewPortMappingDescription") != - 1) {
        rule_ptr->devFriendlyName =
            strdup(getTagContent(line.c_str(), "NewPortMappingDescription"));
      } else if (line.find("NewInternalClient") != -1) {
        const char *newInternalClient = getTagContent(line.c_str(), "NewInternalClient");
        if (!newInternalClient[0]) {
          continue;
        }
        rule_ptr->internalAddr.fromChar(newInternalClient);
      } else if (line.find("NewInternalPort") != -1) {
        rule_ptr->internalPort = atoi(getTagContent(line.c_str(), "NewInternalPort"));
      } else if (line.find("NewExternalPort") != -1) {
        rule_ptr->externalPort =
            atoi(getTagContent(line.c_str(), "NewExternalPort"));
      } else if (line.find("NewProtocol") != -1) {
        rule_ptr->protocol = strdup(getTagContent(line.c_str(), "NewProtocol"));
      } else if (line.find("NewLeaseDuration") != -1) {
        rule_ptr->leaseDuration =
            atoi(getTagContent(line.c_str(), "NewLeaseDuration"));

        upnpRuleNode *currRuleNode_ptr = new upnpRuleNode();
        currRuleNode_ptr->upnpRule = rule_ptr;
        currRuleNode_ptr->next = NULL;
        if (ruleNodeHead_ptr == NULL) {
          ruleNodeHead_ptr = currRuleNode_ptr;
          ruleNodeTail_ptr = currRuleNode_ptr;
        } else {
          ruleNodeTail_ptr->next = currRuleNode_ptr;
          ruleNodeTail_ptr = currRuleNode_ptr;
        }
      }
    }

    index++;
    delay(250);
  }

  // print nicely and free heap memory
  ESP_LOGD(TAG, "IGD current port mappings:");
  upnpRuleNode *curr_ptr = ruleNodeHead_ptr;
  upnpRuleNode *del_prt = ruleNodeHead_ptr;
  while (curr_ptr != NULL) {
    upnpRulePrint(curr_ptr->upnpRule);
    del_prt = curr_ptr;
    curr_ptr = curr_ptr->next;
    delete del_prt->upnpRule;
    delete del_prt;
  }

  _tcpClient.close();

  return true;
}

void UPnP::printPortMappingConfig() {
  ESP_LOGD(TAG, "UPnP configured port mappings:");
  upnpRuleNode *currRuleNode = _headRuleNode;
  while (currRuleNode != NULL) {
    upnpRulePrint(currRuleNode->upnpRule);
    currRuleNode = currRuleNode->next;
  }
}

// TODO: remove use of char *
void UPnP::upnpRulePrint(upnpRule *rule_ptr) {
  IPAddress ipAddress = (rule_ptr->internalAddr == ipNull)
                            ? wifi.localIP()
                            : rule_ptr->internalAddr;
  ESP_LOGI(TAG, "%d. %30s %15s %6d %6d %10s %d", rule_ptr->index,
           rule_ptr->devFriendlyName, ipAddress.toChar(),
           rule_ptr->internalPort, rule_ptr->externalPort, rule_ptr->protocol,
           rule_ptr->leaseDuration);
}

void UPnP::printSsdpDevices(ssdpDeviceNode *ssdpDeviceNode_head) {
  ssdpDeviceNode *ssdpDeviceNodeCurr = ssdpDeviceNode_head;
  while (ssdpDeviceNodeCurr != NULL) {
    ssdpDevicePrint(ssdpDeviceNodeCurr->ssdpDevice);
    ssdpDeviceNodeCurr = ssdpDeviceNodeCurr->next;
  }
}

void UPnP::ssdpDevicePrint(ssdpDevice *ssdpDevice) {
  ESP_LOGD(TAG, "SSDP device [%s] port [%d] path [%s]",
           ssdpDevice->host.toChar(), ssdpDevice->port, ssdpDevice->path);
}

IPAddress UPnP::getHost(char *url) {
  IPAddress result(0, 0, 0, 0);
  std::string sUrl = url;

  replace(sUrl, "https://", "");
  replace(sUrl, "http://", "");

  int endIndex = sUrl.find('/');
  if (endIndex != -1) {
    sUrl = sUrl.substr(0, endIndex);
  }
  int colonsIndex = sUrl.find(':');
  if (colonsIndex != -1) {
    sUrl = sUrl.substr(0, colonsIndex);
  }
  result.fromChar(sUrl.c_str());
  return result;
}

int UPnP::getPort(char *url) {
  int port = -1;
  std::string sUrl = url;

  replace(sUrl, "https://", "");
  replace(sUrl, "http://", "");

  // if (sUrl.find("https://") != -1) {
  //   sUrl.replace("https://", "");
  // }
  // if (sUrl.find("http://") != -1) {
  //   sUrl.replace("http://", "");
  // }
  int portEndIndex = sUrl.find("/");
  if (portEndIndex == -1) {
    portEndIndex = sUrl.length();
  }
  sUrl = sUrl.substr(0, portEndIndex);
  int colonsIndex = sUrl.find(":");
  if (colonsIndex != -1) {
    sUrl = sUrl.substr(colonsIndex + 1, portEndIndex);
    port = std::stoi(sUrl);

  } else {
    port = 80;
  }
  return port;
}

const char *UPnP::getPath(char *url) {
  std::string sUrl = url;
  replace(sUrl, "https://", "");
  replace(sUrl, "http://", "");

  // if (sUrl.find(sUrl, "https://")) {
  //   sUrl.replace("https://", "");
  // }
  // if (sUrl.find("http://") != -1) {
  //   sUrl.replace("http://", "");
  // }
  int firstSlashIndex = sUrl.find("/");
  if (firstSlashIndex == -1) {
    ESP_LOGD(TAG, "ERROR: Cannot find path in sUrl [%s]", url);
    return "";
  }
  return sUrl.substr(firstSlashIndex, sUrl.length()).c_str();
}

const char *UPnP::getTagContent(const char *line, const char *tagName) {
  char tag[30];
  sprintf(tag, "<%s>", tagName);
  char *tagStart = strstr(line, tag);
  if (!tagStart) {
    ESP_LOGD(TAG,
             "ERROR: Cannot find tag content in line [%s] for start tag [%s]",
             line, tag);
    return "";
  }
  sprintf(tag, "</%s>", tagName);
  char *tagEnd = strstr(tagStart, tag);
  if (!tagEnd) {
    ESP_LOGD(TAG,
             "ERROR: Cannot find tag content in line [%s] for end tag [%s]",
             line, tagEnd);
    return "";
  }
  sprintf(temp, "%.*s", tagEnd - tagStart - strlen(tagName) - 2,
          tagStart + strlen(tagName) + 2);
  return (const char *)temp;  // return in temp buffer
}