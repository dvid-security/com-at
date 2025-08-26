idf_component_register(SRCS "at_mqtt.c"
                    INCLUDE_DIRS "."
                    REQUIRES mqtt at_server esp_driver_uart)
