#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "mqtt_client.h"
#include "esp_netif.h"

#include "leido_uart.h" // Nuestro componente UART, que declara extern las variables

static const char *TAG = "MQTT_MAIN";

// Cliente MQTT global
esp_mqtt_client_handle_t client = NULL;

// Función de callback para mensajes entrantes
static void mqtt_event_handler_cb(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
    esp_mqtt_client_handle_t client = event->client;

    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT conectado");
            // Suscribe al tópico. QOS 0
            // Usa la variable global mqtt_topic de leido_uart.c
            esp_mqtt_client_subscribe(client, mqtt_topic, 0);
            break;

        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT desconectado");
            break;

        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            break;

        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;

        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;

        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "Mensaje recibido:");
            printf("  TOPIC=%.*s\r\n", event->topic_len, event->topic);
            printf("  DATA=%.*s\r\n", event->data_len, event->data);
            break;

        case MQTT_EVENT_ERROR:
            ESP_LOGE(TAG, "MQTT_EVENT_ERROR");
            if (event->error_handle) { // Asegurarse de que error_handle no sea NULL
                if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
                    ESP_LOGE(TAG, "Error de transporte TCP: 0x%x", event->error_handle->connect_return_code);
                }
                // CORRECCIÓN: Ajuste de nombres de tipos de error y acceso a miembros
                else if (event->error_handle->error_type == MQTT_ERROR_TYPE_CONNECTION_REFUSED) {
                    ESP_LOGE(TAG, "Error de conexión MQTT: %d", event->error_handle->connect_return_code);
                }
                // ESP-IDF 5.x suele usar el connect_return_code para la mayoría de los errores de protocolo
                // No hay un miembro 'esp_mqtt_error_type' en la estructura 'esp_mqtt_error_codes_t'
                // Revisa la documentación de ESP-IDF para los códigos de error específicos de protocolo MQTT.
                // Aquí, usamos connect_return_code como un identificador general si no es un error TLS o TCP.
                else if (event->error_handle->error_type == MQTT_ERROR_TYPE_NONE && event->error_handle->connect_return_code != 0) {
                     ESP_LOGE(TAG, "Error de protocolo MQTT (código de retorno): %d", event->error_handle->connect_return_code);
                }
                else if (event->error_handle->error_type == MQTT_ERROR_TYPE_ESP_TLS) { // CORRECCIÓN: El nombre correcto es MQTT_ERROR_TYPE_ESP_TLS
                    ESP_LOGE(TAG, "Error TLS: 0x%x", event->error_handle->esp_tls_stack_err);
                }
                else {
                    ESP_LOGE(TAG, "Otro tipo de error MQTT. Código de retorno: %d, Tipo de error: %d",
                             event->error_handle->connect_return_code, event->error_handle->error_type);
                }
            } else {
                ESP_LOGE(TAG, "MQTT_EVENT_ERROR sin detalles de error (error_handle es NULL).");
            }
            break;

        default:
            ESP_LOGI(TAG, "Otro evento MQTT ID: %" PRIi32, event_id);
            break;
    }
}

// Inicializar MQTT con el tópico
static void mqtt_app_start(void)
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = "mqtt://broker.hivemq.com:1883",
    };

    client = esp_mqtt_client_init(&mqtt_cfg);
    if (client == NULL) {
        ESP_LOGE(TAG, "Fallo al inicializar el cliente MQTT.");
        return;
    }
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler_cb, NULL);
    esp_mqtt_client_start(client);
}

// Inicializar WiFi en modo estación
static void wifi_init_sta(void)
{
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = {0}, // Inicializa a cero para strncpy
            .password = {0}, // Inicializa a cero para strncpy
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = {
                .capable = true,
                .required = false,
            },
        },
    };

    // Copiar valores desde las variables globales (definidas en leido_uart.c)
    // Asegúrate de que leido_uart_init() haya llenado estas variables antes de llamar a esta función,
    // o que haya valores por defecto razonables.
    strncpy((char *)wifi_config.sta.ssid, wifi_ssid, sizeof(wifi_config.sta.ssid) - 1);
    wifi_config.sta.ssid[sizeof(wifi_config.sta.ssid) - 1] = '\0';
    strncpy((char *)wifi_config.sta.password, wifi_pass, sizeof(wifi_config.sta.password) - 1);
    wifi_config.sta.password[sizeof(wifi_config.sta.password) - 1] = '\0';

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "WiFi iniciado con SSID: %s", wifi_ssid);
}

void app_main(void)
{
    // Inicializar NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // 1. Iniciar UART para recibir comandos (SSID, Contraseña, Tópico MQTT)
    // Llama a la función de inicialización de tu componente leido_uart
    leido_uart_init(); // <--- Esta es la función correcta de tu leido_uart.c

    // 2. Inicializar WiFi
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    // 3. Esperar un poco o usar un manejador de eventos para la conexión WiFi
    // NOTA IMPORTANTE: Para una aplicación robusta, NO uses vTaskDelay para esperar la conexión.
    // En su lugar, usa un manejador de eventos para 'WIFI_EVENT_STA_CONNECTED' e 'IP_EVENT_STA_GOT_IP'.
    // Hasta que se establezca la conexión WiFi, los valores de wifi_ssid y wifi_pass podrían no haberse recibido aún por UART.
    ESP_LOGI(TAG, "Esperando 5 segundos para que se establezca la conexión WiFi y se reciban comandos por UART...");
    vTaskDelay(pdMS_TO_TICKS(5000)); // Considera reemplazar esto con un semáforo o evento de FreeRTOS

    wifi_init_sta();

    // 4. Iniciar conexión al broker MQTT
    // De nuevo, podrías querer esperar aquí hasta que la IP esté disponible y los comandos UART estén procesados.
    mqtt_app_start();

    // La tarea principal de app_main continuará ejecutándose.
    // No necesitas un bucle infinito aquí a menos que tengas lógica adicional.
}