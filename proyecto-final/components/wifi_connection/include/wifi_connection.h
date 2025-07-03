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
#define WIFI_NETIF_DESC_AP "wifi_netif_ap"
#define WIFI_NETIF_DESC_ETH "wifi_netif_eth"

#define WIFI_SCAN_METHOD WIFI_ALL_CHANNEL_SCAN
#define WIFI_CONNECT_AP_SORT_METHOD WIFI_CONNECT_AP_BY_SECURITY
#define WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_OPEN

#define WIFI_INTERFACE get_wifi_netif_from_desc(WIFI_NETIF_DESC_STA)
#define get_wifi_netif() get_wifi_netif_from_desc(WIFI_NETIF_DESC_STA)

// Estructura para configuración WiFi
typedef struct {
    char ssid[32];
    char password[64];
    bool configured;
} wifi_config_nvs_t;

// Estructura para configuración MQTT
typedef struct {
    char broker_uri[128];
    char topic[64];
    bool configured;
} mqtt_config_nvs_t;

/**
 * @brief Inicializa WiFi según configuración guardada en NVS
 * 
 * Si existe configuración WiFi guardada, se conecta a esa red.
 * Si no existe configuración, levanta solo en modo AP para configuración.
 * 
 * @return ESP_OK si la inicialización es exitosa
 */
esp_err_t wifi_connect(void);

/**
 * @brief Inicia WiFi solo en modo AP para configuración
 * 
 * @return ESP_OK si el AP se inicia correctamente
 */
esp_err_t wifi_start_config_ap(void);

/**
 * @brief Guarda configuración WiFi en NVS
 * 
 * @param ssid SSID de la red WiFi
 * @param password Contraseña de la red WiFi
 * @return ESP_OK si se guarda correctamente
 */
esp_err_t wifi_save_config(const char* ssid, const char* password);

/**
 * @brief Carga configuración WiFi desde NVS
 * 
 * @param config Puntero a estructura donde cargar la configuración
 * @return ESP_OK si se carga correctamente
 */
esp_err_t wifi_load_config(wifi_config_nvs_t* config);

/**
 * @brief Guarda configuración MQTT en NVS
 * 
 * @param broker_uri URI del broker MQTT
 * @param topic Tópico MQTT por defecto
 * @return ESP_OK si se guarda correctamente
 */
esp_err_t mqtt_save_config(const char* broker_uri, const char* topic);

/**
 * @brief Carga configuración MQTT desde NVS
 * 
 * @param config Puntero a estructura donde cargar la configuración
 * @return ESP_OK si se carga correctamente
 */
esp_err_t mqtt_load_config(mqtt_config_nvs_t* config);

/**
 * @brief Borra toda la configuración guardada en NVS
 * 
 * @return ESP_OK si se borra correctamente
 */
esp_err_t wifi_clear_config(void);

/**
 * @brief Get the WiFi netif handle by description
 *
 * @param desc Textual interface description
 * @return esp_netif_t* Network interface handle
 */
esp_netif_t *get_wifi_netif_from_desc(const char *desc);
