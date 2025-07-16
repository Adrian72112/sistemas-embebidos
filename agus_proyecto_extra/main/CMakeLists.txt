idf_component_register(SRCS "app_main.c"
                       INCLUDE_DIRS "."
                       REQUIRES mqtt nvs_flash esp_wifi esp_netif esp_event leido_uart)
set(COMPONENT_REQUIRES mqtt nvs_flash esp_wifi) 
set(COMPONENT_PRIV_REQUIRES mqtt nvs_flash esp_wifi)
