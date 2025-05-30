idf_component_register(SRCS "touch_pad.c"
                    INCLUDE_DIRS "include"
                    REQUIRES touch_element delay led esp_timer)