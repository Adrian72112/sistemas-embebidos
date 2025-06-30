idf_component_register(SRCS "mqtt_lib.c"
                       INCLUDE_DIRS "include"
                       REQUIRES cJSON mqtt nvs_flash)
