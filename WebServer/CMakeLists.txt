idf_component_register(SRCS Server.cpp FileHandler.cpp FirmwareUpdater.cpp Config.cpp
                    INCLUDE_DIRS "."
                    REQUIRES esp_rom esp-comm esp_http_server json nvs_flash bootloader_support app_update esp_wifi
)