idf_component_register(SRCS "web_server.c"
                    INCLUDE_DIRS "include"
                    REQUIRES esp_http_server nvs_flash mqtt_lib wifi_connection esp_timer freertos
                    EMBED_FILES "../web/index.html"
                                "../web/config.html"
                                "../web/config_saved.html"
                                "../web/status.html"
                                "../web/style.css"
                                "../web/response.html")
