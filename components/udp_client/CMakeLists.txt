idf_component_register(SRCS "src/udp_client.c"
                    INCLUDE_DIRS "include"
                    REQUIRES audio_utils esp_wifi nvs_flash lwip)