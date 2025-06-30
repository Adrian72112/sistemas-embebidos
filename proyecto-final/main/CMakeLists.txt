idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES audio_controller mqtt_lib nvs_flash esp_netif wifi_connection
                    EMBED_FILES "../audios/victory8bit.pcm"
                                "../audios/8bit.pcm"
                                "../audios/start.pcm"
                                "../audios/lose.pcm"
                                "../audios/whistle.pcm"
                                "../audios/buenass.pcm")