/*
 * WiFi connection functions with NVS configuration support
 */

#include <string.h>
#include "wifi_connection.h"
#include "wifi_connection_private.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"

static const char *TAG = "wifi_connection";

/* NVS keys for configuration storage */
#define NVS_NAMESPACE "wifi_config"
#define NVS_WIFI_SSID_KEY "wifi_ssid"
#define NVS_WIFI_PASS_KEY "wifi_pass"
#define NVS_WIFI_CONFIGURED_KEY "wifi_configured"
#define NVS_MQTT_BROKER_KEY "mqtt_broker"
#define NVS_MQTT_TOPIC_KEY "mqtt_topic"
#define NVS_MQTT_CONFIGURED_KEY "mqtt_configured"

/* Global variables */
static esp_netif_t *s_wifi_sta_netif = NULL;
static esp_netif_t *s_wifi_ap_netif = NULL;
static SemaphoreHandle_t s_semph_get_ip_addrs = NULL;
static SemaphoreHandle_t s_semph_get_ip6_addrs = NULL;
static int s_retry_num = 0;
static bool wifi_initialized = false;
static bool config_mode = false;

/* IPv6 address types for debugging */
const char *ipv6_addr_types_to_str[6] = {
    "ESP_IP6_ADDR_IS_UNKNOWN",
    "ESP_IP6_ADDR_IS_GLOBAL", 
    "ESP_IP6_ADDR_IS_LINK_LOCAL",
    "ESP_IP6_ADDR_IS_SITE_LOCAL",
    "ESP_IP6_ADDR_IS_UNIQUE_LOCAL",
    "ESP_IP6_ADDR_IS_IPV4_MAPPED_IPV6"
};

/**
 * @brief Checks if netif description contains specified prefix
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

/* Event handlers */
static void handler_on_wifi_disconnect(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (config_mode) {
        // En modo configuración, no intentar reconectar
        return;
    }
    
    if (event_id == WIFI_EVENT_STA_START) {
        ESP_LOGI(TAG, "🔗 Iniciando conexión WiFi STA...");
        esp_wifi_connect();
        return;
    }
    
    if (event_id != WIFI_EVENT_STA_DISCONNECTED) {
        return;
    }
    
    s_retry_num++;
    if (s_retry_num > 5) {
        ESP_LOGI(TAG, "❌ WiFi Connect failed %d times, stopping reconnect", s_retry_num);
        if (s_semph_get_ip_addrs) {
            xSemaphoreGive(s_semph_get_ip_addrs);
        }
        if (s_semph_get_ip6_addrs) {
            xSemaphoreGive(s_semph_get_ip6_addrs);
        }
        return;
    }
    
    wifi_event_sta_disconnected_t *disconn = event_data;
    ESP_LOGI(TAG, "⚠️ Wi-Fi disconnected (reason: %d), trying to reconnect (%d/5)...", 
             disconn->reason, s_retry_num);
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
    ESP_LOGI(TAG, "✅ STA IPv6: " IPV6STR ", type: %s", 
             IPV62STR(event->ip6_info.ip), ipv6_addr_types_to_str[ipv6_type]);

    if (ipv6_type == CONNECT_PREFERRED_IPV6_TYPE) {
        if (s_semph_get_ip6_addrs) {
            xSemaphoreGive(s_semph_get_ip6_addrs);
        }
    }
}

/* NVS Functions */
esp_err_t wifi_save_config(const char* ssid, const char* password)
{
    nvs_handle_t nvs_handle;
    esp_err_t err;

    err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error opening NVS handle: %s", esp_err_to_name(err));
        return err;
    }

    // Guardar SSID
    err = nvs_set_str(nvs_handle, NVS_WIFI_SSID_KEY, ssid);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error saving WiFi SSID: %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return err;
    }

    // Guardar contraseña
    err = nvs_set_str(nvs_handle, NVS_WIFI_PASS_KEY, password);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error saving WiFi password: %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return err;
    }

    // Marcar como configurado
    err = nvs_set_u8(nvs_handle, NVS_WIFI_CONFIGURED_KEY, 1);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error saving WiFi configured flag: %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return err;
    }

    err = nvs_commit(nvs_handle);
    nvs_close(nvs_handle);

    if (err == ESP_OK) {
        ESP_LOGI(TAG, "✅ WiFi configuration saved to NVS");
    }

    return err;
}

esp_err_t wifi_load_config(wifi_config_nvs_t* config)
{
    nvs_handle_t nvs_handle;
    esp_err_t err;
    size_t required_size;

    if (!config) {
        return ESP_ERR_INVALID_ARG;
    }

    // Inicializar estructura
    memset(config, 0, sizeof(wifi_config_nvs_t));

    err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "No WiFi configuration found in NVS");
        return err;
    }

    // Verificar si está configurado
    uint8_t configured = 0;
    err = nvs_get_u8(nvs_handle, NVS_WIFI_CONFIGURED_KEY, &configured);
    if (err != ESP_OK || configured == 0) {
        ESP_LOGW(TAG, "WiFi not configured");
        nvs_close(nvs_handle);
        return ESP_ERR_NOT_FOUND;
    }

    // Cargar SSID
    required_size = sizeof(config->ssid);
    err = nvs_get_str(nvs_handle, NVS_WIFI_SSID_KEY, config->ssid, &required_size);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error loading WiFi SSID: %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return err;
    }

    // Cargar contraseña
    required_size = sizeof(config->password);
    err = nvs_get_str(nvs_handle, NVS_WIFI_PASS_KEY, config->password, &required_size);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error loading WiFi password: %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return err;
    }

    config->configured = true;
    nvs_close(nvs_handle);

    ESP_LOGI(TAG, "✅ WiFi configuration loaded from NVS - SSID: %s", config->ssid);
    return ESP_OK;
}

esp_err_t mqtt_save_config(const char* broker_uri, const char* topic)
{
    nvs_handle_t nvs_handle;
    esp_err_t err;

    err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error opening NVS handle: %s", esp_err_to_name(err));
        return err;
    }

    // Guardar broker URI
    err = nvs_set_str(nvs_handle, NVS_MQTT_BROKER_KEY, broker_uri);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error saving MQTT broker: %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return err;
    }

    // Guardar topic
    err = nvs_set_str(nvs_handle, NVS_MQTT_TOPIC_KEY, topic);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error saving MQTT topic: %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return err;
    }

    // Marcar como configurado
    err = nvs_set_u8(nvs_handle, NVS_MQTT_CONFIGURED_KEY, 1);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error saving MQTT configured flag: %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return err;
    }

    err = nvs_commit(nvs_handle);
    nvs_close(nvs_handle);

    if (err == ESP_OK) {
        ESP_LOGI(TAG, "✅ MQTT configuration saved to NVS");
    }

    return err;
}

esp_err_t mqtt_load_config(mqtt_config_nvs_t* config)
{
    nvs_handle_t nvs_handle;
    esp_err_t err;
    size_t required_size;

    if (!config) {
        return ESP_ERR_INVALID_ARG;
    }

    // Inicializar estructura
    memset(config, 0, sizeof(mqtt_config_nvs_t));

    err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "No MQTT configuration found in NVS");
        return err;
    }

    // Verificar si está configurado
    uint8_t configured = 0;
    err = nvs_get_u8(nvs_handle, NVS_MQTT_CONFIGURED_KEY, &configured);
    if (err != ESP_OK || configured == 0) {
        ESP_LOGW(TAG, "MQTT not configured");
        nvs_close(nvs_handle);
        return ESP_ERR_NOT_FOUND;
    }

    // Cargar broker URI
    required_size = sizeof(config->broker_uri);
    err = nvs_get_str(nvs_handle, NVS_MQTT_BROKER_KEY, config->broker_uri, &required_size);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error loading MQTT broker: %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return err;
    }

    // Cargar topic
    required_size = sizeof(config->topic);
    err = nvs_get_str(nvs_handle, NVS_MQTT_TOPIC_KEY, config->topic, &required_size);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error loading MQTT topic: %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return err;
    }

    config->configured = true;
    nvs_close(nvs_handle);
    
    // Verificar si el broker URI ya tiene el prefijo mqtt://
    if (strncmp(config->broker_uri, "mqtt://", 7) != 0) {
        // Crear un buffer temporal para concatenar
        char temp_uri[256];
        snprintf(temp_uri, sizeof(temp_uri), "mqtt://%s", config->broker_uri);
        // Copiar de vuelta al buffer original
        strncpy(config->broker_uri, temp_uri, sizeof(config->broker_uri) - 1);
        config->broker_uri[sizeof(config->broker_uri) - 1] = '\0';
    }

    ESP_LOGI(TAG, "✅ MQTT configuration loaded from NVS - Broker: %s", config->broker_uri);
    return ESP_OK;
}

esp_err_t wifi_clear_config(void)
{
    nvs_handle_t nvs_handle;
    esp_err_t err;

    err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error opening NVS handle: %s", esp_err_to_name(err));
        return err;
    }

    err = nvs_erase_all(nvs_handle);
    if (err == ESP_OK) {
        err = nvs_commit(nvs_handle);
        ESP_LOGI(TAG, "✅ All configuration cleared from NVS");
    } else {
        ESP_LOGE(TAG, "Error clearing NVS: %s", esp_err_to_name(err));
    }

    nvs_close(nvs_handle);
    return err;
}

/* WiFi connection functions */
esp_err_t wifi_start_config_ap(void)
{
    ESP_LOGI(TAG, "🔧 Iniciando WiFi en modo AP (configuración)");
    
    if (wifi_initialized) {
        ESP_LOGW(TAG, "⚠️ WiFi ya está inicializado");
        return ESP_OK;
    }
    
    config_mode = true;
    
    // Inicializar WiFi
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // Crear interfaz Access Point
    s_wifi_ap_netif = esp_netif_create_default_wifi_ap();

    // Configuración de Access Point
    wifi_config_t wifi_ap_config = {
        .ap = {
            .ssid = "ESP32-Config",
            .ssid_len = strlen("ESP32-Config"),
            .password = "config123",
            .max_connection = 4,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK
        }
    };

    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_ap_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    wifi_initialized = true;

    ESP_LOGI(TAG, "✅ WiFi AP iniciado para configuración");
    ESP_LOGI(TAG, "📡 AP SSID: ESP32-Config, contraseña: config123");
    ESP_LOGI(TAG, "🌐 Accede a http://192.168.4.1/ para configurar");

    return ESP_OK;
}

static esp_err_t wifi_connect_with_config(const wifi_config_nvs_t* wifi_cfg)
{
    ESP_LOGI(TAG, "🔧 Conectando WiFi con configuración guardada");
    
    if (wifi_initialized) {
        ESP_LOGW(TAG, "⚠️ WiFi ya está inicializado");
        return ESP_OK;
    }
    
    config_mode = false;
    
    // Inicializar WiFi
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // Crear interfaz Station
    esp_netif_inherent_config_t esp_netif_config_sta = ESP_NETIF_INHERENT_DEFAULT_WIFI_STA();
    esp_netif_config_sta.if_desc = WIFI_NETIF_DESC_STA;
    esp_netif_config_sta.route_prio = 128;
    s_wifi_sta_netif = esp_netif_create_wifi(WIFI_IF_STA, &esp_netif_config_sta);
    esp_wifi_set_default_wifi_sta_handlers();

    // Crear semáforos
    s_semph_get_ip_addrs = xSemaphoreCreateBinary();
    if (s_semph_get_ip_addrs == NULL) {
        return ESP_ERR_NO_MEM;
    }

    s_semph_get_ip6_addrs = xSemaphoreCreateBinary();
    if (s_semph_get_ip6_addrs == NULL) {
        vSemaphoreDelete(s_semph_get_ip_addrs);
        return ESP_ERR_NO_MEM;
    }

    // Registrar event handlers
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_START, 
                                             &handler_on_wifi_disconnect, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, 
                                             &handler_on_wifi_disconnect, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, 
                                             &handler_on_sta_got_ip, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_CONNECTED, 
                                             &handler_on_wifi_connect, s_wifi_sta_netif));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_GOT_IP6, 
                                             &handler_on_sta_got_ipv6, NULL));

    // Configuración de Station con datos guardados
    wifi_config_t wifi_sta_config = {0};
    strcpy((char*)wifi_sta_config.sta.ssid, wifi_cfg->ssid);
    strcpy((char*)wifi_sta_config.sta.password, wifi_cfg->password);
    wifi_sta_config.sta.scan_method = WIFI_SCAN_METHOD;
    wifi_sta_config.sta.sort_method = WIFI_CONNECT_AP_SORT_METHOD;
    wifi_sta_config.sta.threshold.rssi = -127;
    wifi_sta_config.sta.threshold.authmode = WIFI_SCAN_AUTH_MODE_THRESHOLD;

    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_sta_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    wifi_initialized = true;

    ESP_LOGI(TAG, "🔗 Conectando a WiFi: %s", wifi_cfg->ssid);

    // Esperar a obtener IP (con timeout)
    if (xSemaphoreTake(s_semph_get_ip_addrs, pdMS_TO_TICKS(15000)) == pdTRUE) {
        ESP_LOGI(TAG, "🎉 Conexión WiFi exitosa");
        return ESP_OK;
    } else {
        ESP_LOGE(TAG, "❌ Timeout conectando a WiFi");
        return ESP_ERR_TIMEOUT;
    }
}

esp_err_t wifi_connect(void)
{
    ESP_LOGI(TAG, "🚀 Iniciando sistema WiFi");
    
    // Intentar cargar configuración WiFi desde NVS
    wifi_config_nvs_t wifi_config;
    esp_err_t ret = wifi_load_config(&wifi_config);
    
    if (ret == ESP_OK && wifi_config.configured) {
        ESP_LOGI(TAG, "📋 Configuración WiFi encontrada, conectando...");
        return wifi_connect_with_config(&wifi_config);
    } else {
        ESP_LOGI(TAG, "⚙️ No hay configuración WiFi, iniciando modo configuración");
        return wifi_start_config_ap();
    }
}
