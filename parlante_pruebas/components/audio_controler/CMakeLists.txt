idf_component_register(SRCS "audio_controler.c"
                           "i2s_driver.c"
                           "es8311_codec.c"
                    INCLUDE_DIRS "include"
                    REQUIRES driver esp_driver_i2s esp_driver_i2c esp_driver_gpio 
                             freertos esp_common espressif__es8311)
