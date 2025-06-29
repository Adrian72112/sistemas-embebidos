idf_component_register(
    SRCS "logger_new.c" "logger.c"
    INCLUDE_DIRS "include"
    REQUIRES spiffs wear_levelling esp_timer freertos
)

