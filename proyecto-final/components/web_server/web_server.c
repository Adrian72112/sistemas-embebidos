#include "web_server.h"
#include "wifi_connection.h"
#include "mqtt_lib.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include <stdlib.h>

static const char *TAG = "web_server";
static httpd_handle_t server = NULL;
static char current_mqtt_topic[64] = "/topic/qos1"; // Topic MQTT por defecto

// Referencias a archivos web embebidos
extern const uint8_t index_html_start[] asm("_binary_index_html_start");
extern const uint8_t index_html_end[]   asm("_binary_index_html_end");
extern const uint8_t config_html_start[] asm("_binary_config_html_start");
extern const uint8_t config_html_end[]   asm("_binary_config_html_end");
extern const uint8_t config_saved_html_start[] asm("_binary_config_saved_html_start");
extern const uint8_t config_saved_html_end[]   asm("_binary_config_saved_html_end");
extern const uint8_t status_html_start[] asm("_binary_status_html_start");
extern const uint8_t status_html_end[]   asm("_binary_status_html_end");
extern const uint8_t style_css_start[] asm("_binary_style_css_start");
extern const uint8_t style_css_end[]   asm("_binary_style_css_end");
extern const uint8_t response_html_start[] asm("_binary_response_html_start");
extern const uint8_t response_html_end[]   asm("_binary_response_html_end");

// Forward declarations
static void restart_task(void *pvParameters);
static void schedule_restart(int delay_seconds);

// Función para determinar el modo actual
static const char* get_current_mode(void) {
    wifi_config_nvs_t wifi_config;
    if (wifi_load_config(&wifi_config) == ESP_OK && wifi_config.configured) {
        return "Operación Normal";
    }
    return "Modo Configuración";
}

// Maneja GET a "/"
static esp_err_t root_get_handler(httpd_req_t *req)
{
    const size_t index_html_len = index_html_end - index_html_start;
    
    ESP_LOGI(TAG, "🌐 Sirviendo página principal");
    
    // Crear buffer para la respuesta con el modo insertado
    char* formatted_response = malloc(index_html_len + 100);
    if (formatted_response != NULL) {
        char* temp_response = malloc(index_html_len + 1);
        if (temp_response != NULL) {
            memcpy(temp_response, index_html_start, index_html_len);
            temp_response[index_html_len] = '\0';
            
            // Reemplazar %s con el modo actual
            const char* current_mode = get_current_mode();
            char* placeholder = strstr(temp_response, "%s");
            if (placeholder != NULL) {
                size_t before_len = placeholder - temp_response;
                strncpy(formatted_response, temp_response, before_len);
                formatted_response[before_len] = '\0';
                strcat(formatted_response, current_mode);
                strcat(formatted_response, placeholder + 2);
            } else {
                strcpy(formatted_response, temp_response);
            }
            
            httpd_resp_set_type(req, "text/html");
            httpd_resp_send(req, formatted_response, HTTPD_RESP_USE_STRLEN);
            
            free(temp_response);
        } else {
            httpd_resp_send(req, (const char*)index_html_start, index_html_len);
        }
        free(formatted_response);
    } else {
        httpd_resp_send(req, (const char*)index_html_start, index_html_len);
    }
    
    return ESP_OK;
}

// Maneja GET a "/config"
static esp_err_t config_get_handler(httpd_req_t *req)
{
    const size_t config_html_len = config_html_end - config_html_start;
    
    ESP_LOGI(TAG, "⚙️ Sirviendo página de configuración");
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, (const char*)config_html_start, config_html_len);
    return ESP_OK;
}

// Maneja GET a "/save_config"
static esp_err_t save_config_handler(httpd_req_t *req)
{
    char query[512];
    char wifi_ssid[32] = "";
    char wifi_password[64] = "";
    char mqtt_broker[128] = "";
    char mqtt_topic[64] = "";
    
    ESP_LOGI(TAG, "💾 Procesando guardado de configuración");
    
    // Obtener parámetros de la URL
    if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK) {
        httpd_query_key_value(query, "wifi_ssid", wifi_ssid, sizeof(wifi_ssid));
        httpd_query_key_value(query, "wifi_password", wifi_password, sizeof(wifi_password));
        httpd_query_key_value(query, "mqtt_broker", mqtt_broker, sizeof(mqtt_broker));
        httpd_query_key_value(query, "mqtt_topic", mqtt_topic, sizeof(mqtt_topic));
    }
    
    // Validar datos mínimos
    if (strlen(wifi_ssid) == 0 || strlen(mqtt_broker) == 0 || strlen(mqtt_topic) == 0) {
        ESP_LOGE(TAG, "❌ Datos de configuración incompletos");
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }
    
    // Guardar configuración WiFi
    esp_err_t ret = wifi_save_config(wifi_ssid, wifi_password);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "❌ Error guardando configuración WiFi: %s", esp_err_to_name(ret));
        httpd_resp_send_500(req);
        return ret;
    }
    
    // Guardar configuración MQTT
    ret = mqtt_save_config(mqtt_broker, mqtt_topic);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "❌ Error guardando configuración MQTT: %s", esp_err_to_name(ret));
        httpd_resp_send_500(req);
        return ret;
    }
    
    ESP_LOGI(TAG, "✅ Configuración guardada - WiFi: %s, MQTT: %s", wifi_ssid, mqtt_broker);
    
    // Preparar respuesta con datos de configuración
    const size_t config_saved_html_len = config_saved_html_end - config_saved_html_start;
    char* formatted_response = malloc(config_saved_html_len + 512);
    
    if (formatted_response != NULL) {
        char* temp_response = malloc(config_saved_html_len + 1);
        if (temp_response != NULL) {
            memcpy(temp_response, config_saved_html_start, config_saved_html_len);
            temp_response[config_saved_html_len] = '\0';
            
            // Reemplazar placeholders
            snprintf(formatted_response, config_saved_html_len + 512, temp_response, 
                    wifi_ssid, mqtt_broker, mqtt_topic);
            
            httpd_resp_set_type(req, "text/html");
            httpd_resp_send(req, formatted_response, HTTPD_RESP_USE_STRLEN);
            
            free(temp_response);
        } else {
            httpd_resp_send(req, (const char*)config_saved_html_start, config_saved_html_len);
        }
        free(formatted_response);
    } else {
        httpd_resp_send(req, (const char*)config_saved_html_start, config_saved_html_len);
    }
    
    // Programar reinicio después de 10 segundos
    ESP_LOGI(TAG, "🔄 Reiniciando sistema en 10 segundos para aplicar configuración...");
    schedule_restart(10);
    
    return ESP_OK;
}

// Maneja GET a "/status"
static esp_err_t status_get_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "📊 Sirviendo página de estado");
    
    // Obtener información del sistema
    wifi_config_nvs_t wifi_config;
    mqtt_config_nvs_t mqtt_config;
    bool wifi_configured = (wifi_load_config(&wifi_config) == ESP_OK);
    bool mqtt_configured = (mqtt_load_config(&mqtt_config) == ESP_OK);
    
    // Preparar datos de estado
    const char* wifi_mode = wifi_configured ? "Cliente (STA)" : "Punto de Acceso (AP)";
    const char* wifi_ssid = wifi_configured ? wifi_config.ssid : "ESP32-Config";
    const char* wifi_ip = "192.168.4.1"; // IP del AP por defecto
    const char* wifi_status_class = wifi_configured ? "connected" : "config-mode";
    const char* wifi_status_text = wifi_configured ? "Conectado" : "Modo Configuración";
    
    const char* mqtt_broker = mqtt_configured ? mqtt_config.broker_uri : "No configurado";
    const char* mqtt_topic = mqtt_configured ? mqtt_config.topic : "No configurado";
    const char* mqtt_status_class = mqtt_lib_is_connected() ? "connected" : "disconnected";
    const char* mqtt_status_text = mqtt_lib_is_connected() ? "Conectado" : "Desconectado";
    
    const char* wifi_config_text = wifi_configured ? "✅ Sí" : "❌ No";
    const char* mqtt_config_text = mqtt_configured ? "✅ Sí" : "❌ No";
    
    // Calcular tiempo activo usando FreeRTOS ticks
    uint32_t uptime_ticks = xTaskGetTickCount();
    uint32_t uptime_s = uptime_ticks / configTICK_RATE_HZ; // Usar la frecuencia de tick configurada
    uint32_t hours = uptime_s / 3600;
    uint32_t minutes = (uptime_s % 3600) / 60;
    uint32_t seconds = uptime_s % 60;
    char uptime_str[32];
    snprintf(uptime_str, sizeof(uptime_str), "%02lu:%02lu:%02lu", hours, minutes, seconds);
    
    // Preparar respuesta HTML
    const size_t status_html_len = status_html_end - status_html_start;
    char* formatted_response = malloc(status_html_len + 1024);
    
    if (formatted_response != NULL) {
        char* temp_response = malloc(status_html_len + 1);
        if (temp_response != NULL) {
            memcpy(temp_response, status_html_start, status_html_len);
            temp_response[status_html_len] = '\0';
            
            // Reemplazar todos los placeholders
            snprintf(formatted_response, status_html_len + 1024, temp_response,
                    wifi_mode, wifi_ssid, wifi_ip, wifi_status_class, wifi_status_text,
                    mqtt_broker, mqtt_topic, mqtt_status_class, mqtt_status_text,
                    wifi_config_text, mqtt_config_text, uptime_str,
                    "Listo", "Track 1", "55");
            
            httpd_resp_set_type(req, "text/html");
            httpd_resp_send(req, formatted_response, HTTPD_RESP_USE_STRLEN);
            
            free(temp_response);
        } else {
            httpd_resp_send(req, (const char*)status_html_start, status_html_len);
        }
        free(formatted_response);
    } else {
        httpd_resp_send(req, (const char*)status_html_start, status_html_len);
    }
    
    return ESP_OK;
}

// Maneja GET a "/clear_config"
static esp_err_t clear_config_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "🗑️ Borrando configuración del sistema");
    
    esp_err_t ret = wifi_clear_config();
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "✅ Configuración borrada correctamente");
        
        // Respuesta simple
        const char* response = "<!DOCTYPE html><html><head><meta charset='UTF-8'>"
                              "<title>Configuración Borrada</title><meta http-equiv='refresh' content='3;url=/'>"
                              "</head><body style='font-family:Arial;text-align:center;padding:50px;'>"
                              "<h1>🗑️ Configuración Borrada</h1>"
                              "<p>Toda la configuración ha sido eliminada.</p>"
                              "<p>El sistema se reiniciará en 5 segundos...</p>"
                              "</body></html>";
        
        httpd_resp_set_type(req, "text/html");
        httpd_resp_send(req, response, HTTPD_RESP_USE_STRLEN);
        
        // Programar reinicio
        ESP_LOGI(TAG, "🔄 Reiniciando sistema en 5 segundos...");
        schedule_restart(5);
        
    } else {
        ESP_LOGE(TAG, "❌ Error borrando configuración: %s", esp_err_to_name(ret));
        httpd_resp_send_500(req);
    }
    
    return ESP_OK;
}

// Maneja GET a "/style.css"
static esp_err_t style_get_handler(httpd_req_t *req)
{
    const size_t style_css_len = style_css_end - style_css_start;
    
    ESP_LOGI(TAG, "🎨 Sirviendo hoja de estilos");
    httpd_resp_set_type(req, "text/css");
    httpd_resp_send(req, (const char*)style_css_start, style_css_len);
    return ESP_OK;
}

// Maneja GET a "/comando" - Procesa comandos desde la interfaz web
static esp_err_t comando_handler(httpd_req_t *req)
{
    char query[100];
    char cmd[64] = "Sin comando";
    
    // Obtener el comando de los parámetros URL
    if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK) {
        if (httpd_query_key_value(query, "cmd", cmd, sizeof(cmd)) == ESP_OK) {
            ESP_LOGI(TAG, "🎮 Comando recibido desde web: %s", cmd);
            
            // Publicar comando por MQTT si está conectado
            if (mqtt_lib_is_connected()) {
                esp_err_t ret = mqtt_lib_publish(current_mqtt_topic, cmd, strlen(cmd), 1);
                if (ret == ESP_OK) {
                    ESP_LOGI(TAG, "📤 Comando enviado por MQTT: %s (topic: %s)", cmd, current_mqtt_topic);
                } else {
                    ESP_LOGE(TAG, "❌ Error enviando comando por MQTT: %s", esp_err_to_name(ret));
                }
            } else {
                ESP_LOGW(TAG, "⚠️ MQTT no conectado, comando no enviado");
            }
        }
    }
    
    // Usar el archivo embebido de respuesta
    const size_t response_html_len = response_html_end - response_html_start;
    
    // Crear buffer para la respuesta con el comando insertado
    char* formatted_response = malloc(response_html_len + strlen(cmd) + 100);
    if (formatted_response != NULL) {
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
                strcat(formatted_response, placeholder + 2);
            } else {
                strcpy(formatted_response, temp_response);
            }
            
            httpd_resp_set_type(req, "text/html");
            httpd_resp_send(req, formatted_response, HTTPD_RESP_USE_STRLEN);
            
            free(temp_response);
        } else {
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

// Tarea para reiniciar el sistema después de un delay
static void restart_task(void *pvParameters)
{
    int delay_seconds = (int)pvParameters;
    ESP_LOGI(TAG, "⏱️ Reiniciando en %d segundos...", delay_seconds);
    
    vTaskDelay(pdMS_TO_TICKS(delay_seconds * 1000));
    ESP_LOGI(TAG, "🔄 Ejecutando reinicio del sistema");
    esp_restart();
}

// Función para programar un reinicio
static void schedule_restart(int delay_seconds)
{
    xTaskCreate(restart_task, "restart_task", 2048, (void*)delay_seconds, 1, NULL);
}

esp_err_t web_server_start(void)
{
    if (server != NULL) {
        ESP_LOGW(TAG, "⚠️ El servidor web ya está funcionando");
        return ESP_OK;
    }

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.stack_size = 8192;
    config.server_port = 80;

    esp_err_t ret = httpd_start(&server, &config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "❌ Error al iniciar el servidor HTTP: %s", esp_err_to_name(ret));
        return ret;
    }

    // Registrar todos los endpoints
    const httpd_uri_t endpoints[] = {
        {
            .uri      = "/",
            .method   = HTTP_GET,
            .handler  = root_get_handler,
            .user_ctx = NULL
        },
        {
            .uri      = "/config",
            .method   = HTTP_GET,
            .handler  = config_get_handler,
            .user_ctx = NULL
        },
        {
            .uri      = "/save_config",
            .method   = HTTP_GET,
            .handler  = save_config_handler,
            .user_ctx = NULL
        },
        {
            .uri      = "/status",
            .method   = HTTP_GET,
            .handler  = status_get_handler,
            .user_ctx = NULL
        },
        {
            .uri      = "/clear_config",
            .method   = HTTP_GET,
            .handler  = clear_config_handler,
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
        ret = httpd_register_uri_handler(server, &endpoints[i]);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "❌ Error registrando endpoint %s: %s", 
                     endpoints[i].uri, esp_err_to_name(ret));
            web_server_stop();
            return ret;
        }
    }

    ESP_LOGI(TAG, "✅ Servidor HTTP iniciado en puerto 80");
    ESP_LOGI(TAG, "🌐 Accede a http://192.168.4.1/ para configurar o controlar el dispositivo");
    
    return ESP_OK;
}

esp_err_t web_server_stop(void)
{
    if (server == NULL) {
        ESP_LOGW(TAG, "⚠️ El servidor web no está funcionando");
        return ESP_OK;
    }

    esp_err_t ret = httpd_stop(server);
    if (ret == ESP_OK) {
        server = NULL;
        ESP_LOGI(TAG, "🛑 Servidor web detenido");
    } else {
        ESP_LOGE(TAG, "❌ Error deteniendo servidor web: %s", esp_err_to_name(ret));
    }

    return ret;
}

bool web_server_is_running(void)
{
    return server != NULL;
}

esp_err_t web_server_set_mqtt_topic(const char* topic)
{
    if (!topic) {
        ESP_LOGE(TAG, "Topic cannot be NULL");
        return ESP_ERR_INVALID_ARG;
    }
    
    // Copiar el topic a la variable global
    strncpy(current_mqtt_topic, topic, sizeof(current_mqtt_topic) - 1);
    current_mqtt_topic[sizeof(current_mqtt_topic) - 1] = '\0';
    
    ESP_LOGI(TAG, "🔧 MQTT topic configurado: %s", current_mqtt_topic);
    return ESP_OK;
}
