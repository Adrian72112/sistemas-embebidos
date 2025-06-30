#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#include "esp_http_server.h"

// Archivos web embebidos
extern const uint8_t index_html_start[] asm("_binary_index_html_start");
extern const uint8_t index_html_end[]   asm("_binary_index_html_end");
extern const uint8_t style_css_start[] asm("_binary_style_css_start");
extern const uint8_t style_css_end[]   asm("_binary_style_css_end");
extern const uint8_t response_html_start[] asm("_binary_response_html_start");
extern const uint8_t response_html_end[]   asm("_binary_response_html_end");  

#define EXAMPLE_ESP_WIFI_SSID    "SeTeLINK"
#define EXAMPLE_ESP_WIFI_PASS    "mvn5ts4k."
#define EXAMPLE_ESP_MAXIMUM_RETRY  CONFIG_ESP_MAXIMUM_RETRY

// Provide a default value for EXAMPLE_H2E_IDENTIFIER
#define EXAMPLE_H2E_IDENTIFIER 0

#if CONFIG_ESP_WPA3_SAE_PWE_HUNT_AND_PECK
#define ESP_WIFI_SAE_MODE WPA3_SAE_PWE_HUNT_AND_PECK
#undef EXAMPLE_H2E_IDENTIFIER
#define EXAMPLE_H2E_IDENTIFIER ""
#elif CONFIG_ESP_WPA3_SAE_PWE_HASH_TO_ELEMENT
#define ESP_WIFI_SAE_MODE WPA3_SAE_PWE_HASH_TO_ELEMENT
#elif CONFIG_ESP_WPA3_SAE_PWE_BOTH
#define ESP_WIFI_SAE_MODE WPA3_SAE_PWE_BOTH
#undef EXAMPLE_H2E_IDENTIFIER
#define EXAMPLE_H2E_IDENTIFIER 0
#if CONFIG_ESP_WIFI_AUTH_OPEN
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_OPEN
#elif CONFIG_ESP_WIFI_AUTH_WEP
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WEP
#elif CONFIG_ESP_WIFI_AUTH_WPA_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA2_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA_WPA2_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_WPA2_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA3_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA3_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA2_WPA3_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_WPA3_PSK
#elif CONFIG_ESP_WIFI_AUTH_WAPI_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WAPI_PSK
#endif

#endif // <-- Add this line to close the #if/#elif block for WPA3_SAE_PWE
static EventGroupHandle_t s_wifi_event_group;
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static const char *TAG = "wifi station";
static int s_retry_num = 0;

static void event_handler(void* arg, esp_event_base_t event_base,
                          int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG,"connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

void wifi_init_apsta(void)
{
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Crea la interfaz station
    esp_netif_create_default_wifi_sta();

    // Crea la interfaz access point
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // Registra handlers
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

    // Config STA con credenciales por defecto
    wifi_config_t wifi_sta_config = { 0 };  // inicializa todo en cero
    strncpy((char*)wifi_sta_config.sta.ssid, EXAMPLE_ESP_WIFI_SSID, sizeof(wifi_sta_config.sta.ssid));
    strncpy((char*)wifi_sta_config.sta.password, EXAMPLE_ESP_WIFI_PASS, sizeof(wifi_sta_config.sta.password));
    wifi_sta_config.sta.threshold.authmode = ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD;
    wifi_sta_config.sta.sae_pwe_h2e = ESP_WIFI_SAE_MODE;
    //memcpy(wifi_sta_config.sta.sae_h2e_identifier, EXAMPLE_H2E_IDENTIFIER, strlen(EXAMPLE_H2E_IDENTIFIER));


    // Config AP (donde se conectarán los usuarios para configurar)
    wifi_config_t wifi_ap_config = {
        .ap = {
            .ssid = "ConfiguradorESP",
            .ssid_len = strlen("ConfiguradorESP"),
            .password = "admin1234",
            .max_connection = 4,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK
        }
    };

    if (strlen("admin1234") == 0) {
        wifi_ap_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_sta_config));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_ap_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_apsta finished. AP SSID: %s, STA SSID: %s",
             "ConfiguradorESP", EXAMPLE_ESP_WIFI_SSID);

    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                           WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                           pdFALSE,
                                           pdFALSE,
                                           portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "connected to STA SSID:%s", EXAMPLE_ESP_WIFI_SSID);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "Failed to connect to STA SSID:%s", EXAMPLE_ESP_WIFI_SSID);
    } else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }
}

// Maneja GET a "/"
esp_err_t root_get_handler(httpd_req_t *req)
{
    const size_t index_html_len = index_html_end - index_html_start;
    
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, (const char*)index_html_start, index_html_len);
    return ESP_OK;
}

// Maneja GET a "/style.css"
esp_err_t style_get_handler(httpd_req_t *req)
{
    const size_t style_css_len = style_css_end - style_css_start;
    
    httpd_resp_set_type(req, "text/css");
    httpd_resp_send(req, (const char*)style_css_start, style_css_len);
    return ESP_OK;
}

// Maneja GET a "/comando"
esp_err_t comando_handler(httpd_req_t *req)
{
    char query[100];
    char cmd[64] = "Sin comando";
    
    if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK) {
        if (httpd_query_key_value(query, "cmd", cmd, sizeof(cmd)) == ESP_OK) {
            ESP_LOGI("WEB", "Comando recibido: %s", cmd);
            // Aquí en el futuro: publicar por MQTT
        }
    }
    
    // Usar el archivo embebido de respuesta
    const size_t response_html_len = response_html_end - response_html_start;
    
    // Crear buffer para la respuesta con el comando insertado
    char* formatted_response = malloc(response_html_len + strlen(cmd) + 100);
    if (formatted_response != NULL) {
        // Copiar el contenido del archivo embebido a un string temporal
        char* temp_response = malloc(response_html_len + 1);
        if (temp_response != NULL) {
            memcpy(temp_response, response_html_start, response_html_len);
            temp_response[response_html_len] = '\0';
            
            // Buscar y reemplazar %s con el comando
            char* placeholder = strstr(temp_response, "%s");
            if (placeholder != NULL) {
                size_t before_len = placeholder - temp_response;
                strncpy(formatted_response, temp_response, before_len);
                formatted_response[before_len] = '\0';
                strcat(formatted_response, cmd);
                strcat(formatted_response, placeholder + 2); // +2 para saltar "%s"
            } else {
                strcpy(formatted_response, temp_response);
            }
            
            httpd_resp_set_type(req, "text/html");
            httpd_resp_send(req, formatted_response, HTTPD_RESP_USE_STRLEN);
            
            free(temp_response);
        } else {
            // Si no se puede asignar memoria, enviar respuesta simple
            httpd_resp_send(req, (const char*)response_html_start, response_html_len);
        }
        free(formatted_response);
    } else {
        // Fallback a respuesta simple si no se puede asignar memoria
        char simple_response[200];
        snprintf(simple_response, sizeof(simple_response), 
                "<!DOCTYPE html><html><body><h2>Comando Recibido</h2>"
                "<p>Comando: %s</p><a href='/'>Volver</a></body></html>", cmd);
        httpd_resp_send(req, simple_response, HTTPD_RESP_USE_STRLEN);
    }
    return ESP_OK;
}

// No necesitamos funciones de inicialización del sistema de archivos ya que usamos archivos embebidos

void start_webserver(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;

    // Aumentamos el stack size para manejar las páginas embebidas
    config.stack_size = 8192;

    if (httpd_start(&server, &config) == ESP_OK) {
        // Registramos todos los endpoints usando archivos embebidos
        const httpd_uri_t endpoints[] = {
            {
                .uri      = "/",
                .method   = HTTP_GET,
                .handler  = root_get_handler,
                .user_ctx = NULL
            },
            {
                .uri      = "/style.css",
                .method   = HTTP_GET,
                .handler  = style_get_handler,
                .user_ctx = NULL
            },
            {
                .uri      = "/comando",
                .method   = HTTP_GET,
                .handler  = comando_handler,
                .user_ctx = NULL
            }
        };

        // Registrar todos los endpoints
        for (size_t i = 0; i < sizeof(endpoints) / sizeof(endpoints[0]); i++) {
            if (httpd_register_uri_handler(server, &endpoints[i]) != ESP_OK) {
                ESP_LOGE("WEB", "Error registrando endpoint %s", endpoints[i].uri);
            }
        }

        ESP_LOGI("WEB", "Servidor HTTP iniciado");
    } else {
        ESP_LOGE("WEB", "Error al iniciar el servidor HTTP");
    }
}

void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // No necesitamos inicializar sistema de archivos, los archivos están embebidos en el binario

    ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");
    wifi_init_apsta();

    start_webserver(); //arranca el servidor web
}
