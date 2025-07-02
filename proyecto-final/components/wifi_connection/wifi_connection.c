/*
 * SPDX-FileCopyrightText: 2022-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
/* WiFi connection functions */

#include <string.h>
#include "wifi_connection.h"
#include "wifi_connection_private.h"
#include "esp_log.h"

static const char *TAG = "wifi_connection";

/* types of ipv6 addresses to be displayed on ipv6 events */
const char *ipv6_addr_types_to_str[6] = {
    "ESP_IP6_ADDR_IS_UNKNOWN",
    "ESP_IP6_ADDR_IS_GLOBAL",
    "ESP_IP6_ADDR_IS_LINK_LOCAL",
    "ESP_IP6_ADDR_IS_SITE_LOCAL",
    "ESP_IP6_ADDR_IS_UNIQUE_LOCAL",
    "ESP_IP6_ADDR_IS_IPV4_MAPPED_IPV6"
};

/**
 * @brief Checks the netif description if it contains specified prefix.
 * All netifs created within wifi connection component are prefixed with the module TAG,
 * so it returns true if the specified netif is owned by this module
 */
bool is_our_netif(const char *prefix, esp_netif_t *netif)
{
    return strncmp(prefix, esp_netif_get_desc(netif), strlen(prefix) - 1) == 0;
}

static bool netif_desc_matches_with(esp_netif_t *netif, void *ctx)
{
    return strcmp(ctx, esp_netif_get_desc(netif)) == 0;
}

esp_netif_t *get_wifi_netif_from_desc(const char *desc)
{
    return esp_netif_find_if(netif_desc_matches_with, (void*)desc);
}

static esp_netif_t *s_wifi_sta_netif = NULL;
static esp_netif_t *s_wifi_ap_netif = NULL;
static SemaphoreHandle_t s_semph_get_ip_addrs = NULL;
static SemaphoreHandle_t s_semph_get_ip6_addrs = NULL;

static int s_retry_num = 0;
static bool wifi_initialized = false;

static void handler_on_wifi_disconnect(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    // Solo manejar desconexiones, no intentos de auto-conexión
    if (event_id == WIFI_EVENT_STA_START) {
        ESP_LOGI(TAG, "🔗 Iniciando conexión WiFi STA...");
        esp_wifi_connect();
        return;
    }
    
    if (event_id != WIFI_EVENT_STA_DISCONNECTED) {
        return;
    }
    
    s_retry_num++;
    if (s_retry_num > 6) {
        ESP_LOGI(TAG, "❌ WiFi Connect failed %d times, stop reconnect.", s_retry_num);
        /* let wifi_sta_do_connect() return */
        if (s_semph_get_ip_addrs) {
            xSemaphoreGive(s_semph_get_ip_addrs);
        }
        if (s_semph_get_ip6_addrs) {
            xSemaphoreGive(s_semph_get_ip6_addrs);
        }
        return;
    }
    wifi_event_sta_disconnected_t *disconn = event_data;
    if (disconn->reason == WIFI_REASON_ROAMING) {
        ESP_LOGD(TAG, "station roaming, do nothing");
        return;
    }
    ESP_LOGI(TAG, "⚠️ Wi-Fi disconnected (reason: %d), trying to reconnect (%d/6)...", disconn->reason, s_retry_num);
    esp_err_t err = esp_wifi_connect();
    if (err == ESP_ERR_WIFI_NOT_STARTED) {
        return;
    }
    ESP_ERROR_CHECK(err);
}

static void handler_on_wifi_connect(void *esp_netif, esp_event_base_t event_base,
                            int32_t event_id, void *event_data)
{
    esp_netif_create_ip6_linklocal(esp_netif);
}

static void handler_on_sta_got_ip(void *arg, esp_event_base_t event_base,
                      int32_t event_id, void *event_data)
{
    s_retry_num = 0;
    ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
    if (!is_our_netif(WIFI_NETIF_DESC_STA, event->esp_netif)) {
        return;
    }
    ESP_LOGI(TAG, "✅ STA conectado - IPv4: " IPSTR, IP2STR(&event->ip_info.ip));
    if (s_semph_get_ip_addrs) {
        xSemaphoreGive(s_semph_get_ip_addrs);
    }
}

static void handler_on_sta_got_ipv6(void *arg, esp_event_base_t event_base,
                        int32_t event_id, void *event_data)
{
    ip_event_got_ip6_t *event = (ip_event_got_ip6_t *)event_data;
    if (!is_our_netif(WIFI_NETIF_DESC_STA, event->esp_netif)) {
        return;
    }
    esp_ip6_addr_type_t ipv6_type = esp_netif_ip6_get_addr_type(&event->ip6_info.ip);
    ESP_LOGI(TAG, "✅ STA IPv6: " IPV6STR ", type: %s", IPV62STR(event->ip6_info.ip), ipv6_addr_types_to_str[ipv6_type]);

    if (ipv6_type == CONNECT_PREFERRED_IPV6_TYPE) {
        if (s_semph_get_ip6_addrs) {
            xSemaphoreGive(s_semph_get_ip6_addrs);
        }
    }
}

// Función principal para inicializar WiFi en modo AP+STA
esp_err_t wifi_connect_apsta(void)
{
    ESP_LOGI(TAG, "🔧 Configurando WiFi en modo AP+STA");
    
    // Evitar inicialización múltiple
    if (wifi_initialized) {
        ESP_LOGW(TAG, "⚠️ WiFi ya está inicializado");
        return ESP_OK;
    }
    
    // Inicializar WiFi una sola vez
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // Crear interfaz Station
    esp_netif_inherent_config_t esp_netif_config_sta = ESP_NETIF_INHERENT_DEFAULT_WIFI_STA();
    esp_netif_config_sta.if_desc = WIFI_NETIF_DESC_STA;
    esp_netif_config_sta.route_prio = 128;
    s_wifi_sta_netif = esp_netif_create_wifi(WIFI_IF_STA, &esp_netif_config_sta);
    esp_wifi_set_default_wifi_sta_handlers();

    // Crear interfaz Access Point
    s_wifi_ap_netif = esp_netif_create_default_wifi_ap();

    // Crear semáforos para sincronización de IP
    s_semph_get_ip_addrs = xSemaphoreCreateBinary();
    if (s_semph_get_ip_addrs == NULL) {
        return ESP_ERR_NO_MEM;
    }

    s_semph_get_ip6_addrs = xSemaphoreCreateBinary();
    if (s_semph_get_ip6_addrs == NULL) {
        vSemaphoreDelete(s_semph_get_ip_addrs);
        return ESP_ERR_NO_MEM;
    }

    // Registrar event handlers una sola vez
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_START, &handler_on_wifi_disconnect, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &handler_on_wifi_disconnect, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &handler_on_sta_got_ip, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_CONNECTED, &handler_on_wifi_connect, s_wifi_sta_netif));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_GOT_IP6, &handler_on_sta_got_ipv6, NULL));

    // Configuración de Station (conexión a red existente)
    wifi_config_t wifi_sta_config = {
        .sta = {
            .ssid = "SeTeLINK",
            .password = "mvn5ts4k.",
            .scan_method = WIFI_SCAN_METHOD,
            .sort_method = WIFI_CONNECT_AP_SORT_METHOD,
            .threshold.rssi = -127,
            .threshold.authmode = WIFI_SCAN_AUTH_MODE_THRESHOLD,
        },
    };

    // Configuración de Access Point (para que otros se conecten)
    wifi_config_t wifi_ap_config = {
        .ap = {
            .ssid = "ConfiguradorESP",
            .ssid_len = strlen("ConfiguradorESP"),
            .password = "admin1234",
            .max_connection = 4,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK
        }
    };

    // Si no hay contraseña, usar modo abierto
    if (strlen("admin1234") == 0) {
        wifi_ap_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_sta_config));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_ap_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    wifi_initialized = true;

    ESP_LOGI(TAG, "✅ WiFi configurado en modo AP+STA");
    ESP_LOGI(TAG, "📡 AP SSID: ConfiguradorESP, contraseña: admin1234");
    ESP_LOGI(TAG, "🔗 STA conectando a: SeTeLINK");
    ESP_LOGI(TAG, "🌐 Accede a http://192.168.4.1/ desde el AP para control web");

    // Esperar a obtener IP (con timeout)
    ESP_LOGI(TAG, "⏳ Esperando IP del STA...");
    if (xSemaphoreTake(s_semph_get_ip_addrs, pdMS_TO_TICKS(15000)) == pdTRUE) {
        ESP_LOGI(TAG, "🎉 Conexión STA exitosa");
    } else {
        ESP_LOGW(TAG, "⚠️ Timeout obteniendo IP STA - Solo modo AP disponible");
    }

    return ESP_OK;
}

// Función principal y única para conectar WiFi
esp_err_t wifi_connect(void)
{
    ESP_LOGI(TAG, "🚀 Iniciando conexión WiFi en modo AP+STA");
    
    // Llamar directamente a la función unificada
    esp_err_t ret = wifi_connect_apsta();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "❌ Error configurando WiFi AP+STA: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "✅ WiFi inicializado correctamente");
    return ESP_OK;
}
