/* Common functions for wifi connection management
 *
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#pragma once

#include "sdkconfig.h"
#include "esp_err.h"
#include "esp_netif.h"
#include "esp_wifi.h"

#define WIFI_NETIF_DESC_STA "wifi_netif_sta"
#define WIFI_NETIF_DESC_ETH "wifi_netif_eth"

#define WIFI_SCAN_METHOD WIFI_ALL_CHANNEL_SCAN

#define WIFI_CONNECT_AP_SORT_METHOD WIFI_CONNECT_AP_BY_SECURITY

#define WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_OPEN

#define WIFI_INTERFACE get_wifi_netif_from_desc(WIFI_NETIF_DESC_STA)
#define get_wifi_netif() get_wifi_netif_from_desc(WIFI_NETIF_DESC_STA)

/**
 * @brief Configure Wi-Fi, connect, wait for IP
 *
 * This function configures and connects to WiFi network
 *
 * @return ESP_OK on successful connection
 */
esp_err_t wifi_connect(void);

/**
 * @brief Disconnect from WiFi and deinitialize
 */
esp_err_t wifi_disconnect(void);

/**
 * @brief Get the WiFi netif handle by description
 *
 * @param desc Textual interface description
 * @return esp_netif_t* Network interface handle
 */
esp_netif_t *get_wifi_netif_from_desc(const char *desc);
