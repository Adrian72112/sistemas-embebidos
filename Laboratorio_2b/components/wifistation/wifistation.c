#include <stdio.h>
#include "wifistation.h"
#include <string.h>
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "lwip/err.h"
#include "lwip/sys.h"

static EventGroupHandle_t s_wifi_event_group;

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1
#define CONFIG_ESP_MAXIMUM_RETRY 5

static const char* TAG = "wifi_component";
static int s_retry_num = 0;

// Manejador de eventos WiFi/IP
static void event_handler(void* arg, esp_event_base_t event_base,
                          int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        // La interfaz WiFi arrancó, intentar conectar
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        // Se perdió la conexión, reintentar hasta el máximo
        if (s_retry_num < CONFIG_ESP_MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "Reintentando conexión al punto de acceso");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG, "Fallo al conectar al punto de acceso");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        // Se obtuvo una IP
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Dirección IP obtenida:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

esp_err_t wifi_init_station(const char* ssid, const char* password) {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // Formatear NVS si está corrupta o hay nueva versión
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Crear grupo de eventos para señalizar conexión / falla
    s_wifi_event_group = xEventGroupCreate();

    // Inicializar TCP/IP y loop de eventos
    esp_netif_create_default_wifi_sta();

    // Configuración inicial WiFi
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // Registrar manejadores de evento para WiFi y IP
    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    // Configurar SSID y contraseña
    wifi_config_t wifi_config = {
        .sta = {
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };
    strncpy((char*)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));
    strncpy((char*)wifi_config.sta.password, password, sizeof(wifi_config.sta.password));

    // Poner en modo estación y arrancar WiFi
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_LOGI(TAG, "wifi_init_station finalizado");

    // Esperar evento de conexión o falla
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                           WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                           pdFALSE, pdFALSE, portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "Conectado a SSID:%s", ssid);
        return ESP_OK;
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "Fallo al conectar a SSID:%s", ssid);
        return ESP_FAIL;
    }
    return ESP_FAIL;
}
