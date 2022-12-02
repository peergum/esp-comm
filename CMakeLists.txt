idf_component_register(SRCS "UPnP.cpp"
  "Wifi.cpp"
  "TCPClient.cpp"
  "UDPClient.cpp"
  "IPAddress.cpp"
  "utils.c"
  "Timer.cpp"
  REQUIRES esp_rom esp_http_client esp-tls mbedtls json esp_netif esp_wifi esp_common
  INCLUDE_DIRS ".")
