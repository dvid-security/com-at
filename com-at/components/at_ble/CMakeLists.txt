idf_component_register(
    SRCS "at_ble.c"
    INCLUDE_DIRS "."
    REQUIRES bt nvs_flash at_server esp_driver_uart
    PRIV_REQUIRES esp_wifi)
