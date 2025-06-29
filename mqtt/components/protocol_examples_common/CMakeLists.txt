idf_build_get_property(target IDF_TARGET)

set(srcs "wifi_connect.c")

idf_component_register(SRCS "${srcs}"
                       INCLUDE_DIRS "include"
                       PRIV_REQUIRES esp_netif driver esp_wifi vfs console esp_eth openthread)
