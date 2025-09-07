idf_component_register(SRCS "at_ota.c"
                    INCLUDE_DIRS "."
                    PRIV_REQUIRES at_server esp_driver_uart esp_https_ota esp_http_client esp_wifi nvs_flash app_update)
                    