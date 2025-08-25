idf_component_register(SRCS "at_wifi.c"
                      INCLUDE_DIRS "."
                      REQUIRES at_server esp_driver_uart
                      PRIV_REQUIRES esp_wifi)
