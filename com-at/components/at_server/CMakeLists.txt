idf_component_register(SRCS "at_server.c"
                    INCLUDE_DIRS "."
                    PRIV_REQUIRES esp_wifi esp_netif esp_event driver at_server)
                    