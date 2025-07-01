/* Private Functions for wifi connection */

#pragma once

#include "esp_err.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "sdkconfig.h"

#define MAX_IP6_ADDRS_PER_NETIF (5)
#define CONNECT_PREFERRED_IPV6_TYPE ESP_IP6_ADDR_IS_LINK_LOCAL

extern const char *ipv6_addr_types_to_str[6];

// Función principal de inicialización WiFi AP+STA
esp_err_t wifi_connect_apsta(void);

// Funciones de utilidad
bool is_our_netif(const char *prefix, esp_netif_t *netif);
esp_netif_t *get_wifi_netif_from_desc(const char *desc);

