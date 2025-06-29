idf_component_register(
    SRCS "app_main.c"
    INCLUDE_DIRS "."
    REQUIRES mqtt_lib nvs_flash esp_netif wifi_connection
)
