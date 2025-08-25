idf_component_register(SRCS "com-at.c"
                      INCLUDE_DIRS "."
                      REQUIRES esp_driver_uart
                      PRIV_REQUIRES esp_wifi at_wifi at_server nvs_flash)
