idf_component_register(SRCS "app_main.c"
                    INCLUDE_DIRS ".")
set(COMPONENT_REQUIRES esp-mqtt)
set(COMPONENT_PRIV_REQUIRES esp-mqtt)