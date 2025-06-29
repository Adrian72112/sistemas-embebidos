/* Private Functions for wifi connection */

#pragma once

#include "esp_err.h"
#include "esp_wifi.h"
#include "sdkconfig.h"

#define MAX_IP6_ADDRS_PER_NETIF (5)
#define CONNECT_PREFERRED_IPV6_TYPE ESP_IP6_ADDR_IS_LINK_LOCAL

extern const char *ipv6_addr_types_to_str[6];

void wifi_start(void);
void wifi_stop(void);
esp_err_t wifi_sta_do_connect(wifi_config_t wifi_config, bool wait);
esp_err_t wifi_sta_do_disconnect(void);
bool is_our_netif(const char *prefix, esp_netif_t *netif);
void print_all_netif_ips(const char *prefix);
void wifi_shutdown(void);
esp_err_t wifi_connect_to_ap(void);

