#include "web_server.h"
#include "mqtt_lib.h"
#include "esp_log.h"
#include <string.h>
#include <stdlib.h>

static const char *TAG = "web_server";
static httpd_handle_t server = NULL;

// Referencias a archivos web embebidos
extern const uint8_t index_html_start[] asm("_binary_index_html_start");
extern const uint8_t index_html_end[]   asm("_binary_index_html_end");
extern const uint8_t style_css_start[] asm("_binary_style_css_start");
extern const uint8_t style_css_end[]   asm("_binary_style_css_end");
extern const uint8_t response_html_start[] asm("_binary_response_html_start");
extern const uint8_t response_html_end[]   asm("_binary_response_html_end");

// Maneja GET a "/"
static esp_err_t root_get_handler(httpd_req_t *req)
{
    const size_t index_html_len = index_html_end - index_html_start;
    
    ESP_LOGI(TAG, "🌐 Sirviendo página principal");
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, (const char*)index_html_start, index_html_len);
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
                esp_err_t ret = mqtt_lib_publish("/topic/qos1", cmd, strlen(cmd), 1);
                if (ret == ESP_OK) {
                    ESP_LOGI(TAG, "📤 Comando enviado por MQTT: %s", cmd);
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

esp_err_t web_server_start(void)
{
    if (server != NULL) {
        ESP_LOGW(TAG, "⚠️ El servidor web ya está funcionando");
        return ESP_OK;
    }

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    
    // Aumentar el stack size para manejar las páginas embebidas
    config.stack_size = 8192;
    config.server_port = 80;

    esp_err_t ret = httpd_start(&server, &config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "❌ Error al iniciar el servidor HTTP: %s", esp_err_to_name(ret));
        return ret;
    }

    // Registrar todos los endpoints usando archivos embebidos
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
        ret = httpd_register_uri_handler(server, &endpoints[i]);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "❌ Error registrando endpoint %s: %s", 
                     endpoints[i].uri, esp_err_to_name(ret));
            web_server_stop();
            return ret;
        }
    }

    ESP_LOGI(TAG, "✅ Servidor HTTP iniciado en puerto 80");
    ESP_LOGI(TAG, "🌐 Accede a http://192.168.4.1/ desde el AP 'ConfiguradorESP'");
    
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
