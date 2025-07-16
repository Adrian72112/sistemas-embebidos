#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"    // Incluir FreeRTOS semaforos
#include "esp_log.h"
#include "esp_system.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "mqtt_client.h"
#include "esp_netif.h"
#include "esp_mac.h"
#include "sdkconfig.h"
#include "leido_uart.h" // Nuestro componente UART, que declara externas las variables

static const char *TAG = "MQTT_MAIN";

// Cliente MQTT global
esp_mqtt_client_handle_t client = NULL;

// Declarar el semáforo globalmente en app_main.c
SemaphoreHandle_t uart_data_ready_semaphore;

// Tarea para publicar mensajes MQTT periódicamente
static void mqtt_publish_task(void *pvParameters)
{
    char payload[100]; // Buffer para el mensaje a publicar
    // Puedes obtener un ID único para tu dispositivo, por ejemplo, usando la MAC address
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    // Formato el ID del dispositivo
    char device_id[13]; // 12 caracteres para la MAC + null terminator
    sprintf(device_id, "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);


    while (true) {
        // Asegúrate de que el cliente MQTT esté inicializado y conectado antes de intentar publicar
        // La biblioteca MQTT ya maneja las reconexiones internamente.
        if (client != NULL) { // Solo publica si el cliente MQTT ha sido inicializado
            // Crea el mensaje a enviar. Por ejemplo: "<ID> activo"
            sprintf(payload, "%s activo", device_id);

            int msg_id = esp_mqtt_client_publish(client, mqtt_topic, payload, 0, 1, 0);
            if (msg_id != -1) {
                ESP_LOGI(TAG, "Mensaje publicado al tópico '%s', ID: %d", mqtt_topic, msg_id);
            } else {
                // Si la publicación falla, es probable que el cliente no esté conectado o haya un problema de red.
                // La librería MQTT intentará reconectar automáticamente.
                ESP_LOGW(TAG, "Fallo al publicar mensaje. Cliente MQTT probablemente no conectado. Reintentando...");
            }
        } else {
            ESP_LOGW(TAG, "Cliente MQTT no inicializado, esperando...");
        }

        // Retraso de 50 segundos entre publicaciones
        vTaskDelay(pdMS_TO_TICKS(50000));
    }
}


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
            esp_mqtt_client_subscribe(event->client, mqtt_topic, 0);
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
            printf("Mensaje recibido:\n");
            printf("Tópico: %.*s\n", event->topic_len, event->topic);
            printf("Mensaje: %.*s\n", event->data_len, event->data);
            break;

        case MQTT_EVENT_ERROR:
            ESP_LOGE(TAG, "MQTT_EVENT_ERROR");
            if (event->error_handle) { // Asegurarse de que error_handle no sea NULL
                if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
                    ESP_LOGE(TAG, "Error de transporte TCP: 0x%x", event->error_handle->connect_return_code);
                }
                else if (event->error_handle->error_type == MQTT_ERROR_TYPE_CONNECTION_REFUSED) {
                    ESP_LOGE(TAG, "Error de conexión MQTT: %d", event->error_handle->connect_return_code);
                }
                else if (event->error_handle->error_type == MQTT_ERROR_TYPE_NONE && event->error_handle->connect_return_code != 0) {
                     ESP_LOGE(TAG, "Error de protocolo MQTT (código de retorno): %d", event->error_handle->connect_return_code);
                }
                else if (event->error_handle->error_type == MQTT_ERROR_TYPE_ESP_TLS) {
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
        .broker.address.uri = mqtt_uri,
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

    // Crear el semáforo binario
    uart_data_ready_semaphore = xSemaphoreCreateBinary();
    if (uart_data_ready_semaphore == NULL) {
        ESP_LOGE(TAG, "Fallo al crear el semáforo. Reiniciando...");
        esp_restart();
    }

    // 1. Iniciar UART para recibir comandos (SSID, Contraseña, Tópico MQTT)
    // Pasamos el handle del semáforo a la función de inicialización del UART
    leido_uart_init(uart_data_ready_semaphore);

    // 2. Inicializar WiFi y Event Loop antes de esperar
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    // Esperar aquí hasta que los datos de UART sean recibidos y el semáforo sea liberado
    ESP_LOGI(TAG, "Esperando comandos de WiFi y MQTT por UART. Envía !wifi <ssid> <pass> y !topic <topico>, luego !done.");
    
    if (xSemaphoreTake(uart_data_ready_semaphore, portMAX_DELAY) == pdTRUE) {
        ESP_LOGI(TAG, "Comandos UART recibidos. Continuando con la inicialización de WiFi y MQTT.");
    } else {
        ESP_LOGE(TAG, "Error: El semáforo no se liberó inesperadamente.");
    }

    // 3. Inicializar WiFi usando los datos recibidos del UART
    wifi_init_sta();

    // 4. Iniciar conexión al broker MQTT
    mqtt_app_start();

    // 5. Crear la tarea de publicación MQTT después de iniciar el cliente
    xTaskCreate(mqtt_publish_task, "mqtt_pub_task", 4096, NULL, 5, NULL);

    // La tarea principal de app_main continuará ejecutándose.
}