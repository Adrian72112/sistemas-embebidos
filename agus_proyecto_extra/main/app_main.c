#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "mqtt_client.h" // Revertido a esta inclusión
#include "esp_netif.h"

#include "leido_uart.h" // Nuestro componente UART, que declara extern las variables

static const char *TAG = "MQTT_MAIN";

// Cliente MQTT global
esp_mqtt_client_handle_t client = NULL;

// Semáforo para esperar que los datos UART estén listos
SemaphoreHandle_t uart_data_ready_semaphore;
// Semáforo para esperar que el WiFi esté conectado y haya obtenido IP
SemaphoreHandle_t wifi_connected_semaphore;

// Tarea para publicar mensajes MQTT periódicamente
static void mqtt_publish_task(void *pvParameters)
{
    // Esperar hasta que el Wi-Fi esté conectado y haya obtenido una IP
    // y, por ende, el cliente MQTT también esté intentando conectarse o ya conectado.
    // Esto asegura que no intentemos publicar sin conexión de red.
    if (xSemaphoreTake(wifi_connected_semaphore, portMAX_DELAY) == pdTRUE) {
        ESP_LOGI(TAG, "WiFi conectado, iniciando tarea de publicación MQTT.");
    } else {
        ESP_LOGE(TAG, "Fallo al obtener semáforo de conexión WiFi. Tarea de publicación abortada.");
        vTaskDelete(NULL);
        return;
    }

    while (true) {
        // La biblioteca MQTT del ESP-IDF maneja automáticamente la reconexión.
        // Simplemente intentamos publicar. Si no está conectado, la función
        // esp_mqtt_client_publish devolverá un error y el cliente intentará reconectar.
        const char *payload = "Kaluga activo!"; // Mensaje de ejemplo
        int msg_id = esp_mqtt_client_publish(client, mqtt_topic, payload, 0, 0, 0);
        if (msg_id != -1) { // -1 indica un error en la publicación (ej. cliente no conectado)
            ESP_LOGI(TAG, "Mensaje publicado al tópico '%s', ID: %d", mqtt_topic, msg_id);
        } else {
            ESP_LOGW(TAG, "Fallo al publicar mensaje. Cliente MQTT probablemente no conectado. Reintentando...");
        }
        vTaskDelay(pdMS_TO_TICKS(50000)); // Publica cada 50 segundos
    }
}

// Función de callback para eventos Wi-Fi y IP
static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect(); // Intentar conectar al Wi-Fi
        ESP_LOGI(TAG, "WiFi STA iniciado. Intentando conectar...");
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGI(TAG, "WiFi desconectado. Reintentando conexión..."); //
        esp_wifi_connect(); // Reintentar la conexión
        // Asegurarse de que el semáforo de conexión Wi-Fi no esté dado si se desconecta
        // Esto previene que la tarea de publicación intente publicar si el WiFi se ha caído.
        if (xSemaphoreTake(wifi_connected_semaphore, 0) == pdTRUE) { // Intentar tomarlo sin esperar si lo poseemos
             // Si lo tomamos es porque estaba dado, lo que significa que el WiFi se desconectó.
             // No hacemos nada más, simplemente aseguramos que no esté dado.
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "WiFi conectado. IP: " IPSTR, IP2STR(&event->ip_info.ip)); //

        // Obtener y mostrar RSSI
        wifi_ap_record_t ap_info;
        if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
            ESP_LOGI(TAG, "Conectado a SSID: %s, RSSI: %d", ap_info.ssid, ap_info.rssi); //
        }

        // Liberar el semáforo para la tarea de publicación
        if (wifi_connected_semaphore != NULL) {
            xSemaphoreGive(wifi_connected_semaphore);
        }
    }
}

// Función de callback para mensajes entrantes de MQTT
static void mqtt_event_handler_cb(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
    esp_mqtt_client_handle_t client = event->client;

    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT conectado");
            // Suscribe al tópico. QOS 0
            esp_mqtt_client_subscribe(client, mqtt_topic, 0); //
            break;

        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT desconectado. Intentando reconectar..."); //
            // El cliente MQTT de ESP-IDF tiene lógica de reconexión automática por defecto.
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
            ESP_LOGI(TAG, "Mensaje recibido:"); //
            printf("  TOPIC=%.*s\r\n", event->topic_len, event->topic); //
            printf("  DATA=%.*s\r\n", event->data_len, event->data); //
            break;

        case MQTT_EVENT_ERROR:
            ESP_LOGE(TAG, "MQTT_EVENT_ERROR");
            if (event->error_handle) {
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
        .broker.address.uri = "mqtt://broker.hivemq.com:1883", // Broker MQTT URI
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
    ESP_ERROR_CHECK(esp_wifi_start()); // Esto enviará WIFI_EVENT_STA_START
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

    // Crear los semáforos binarios
    uart_data_ready_semaphore = xSemaphoreCreateBinary();
    if (uart_data_ready_semaphore == NULL) {
        ESP_LOGE(TAG, "Fallo al crear semáforo UART. Reiniciando...");
        esp_restart();
    }
    wifi_connected_semaphore = xSemaphoreCreateBinary();
    if (wifi_connected_semaphore == NULL) {
        ESP_LOGE(TAG, "Fallo al crear semáforo WiFi. Reiniciando...");
        esp_restart();
    }

    // Registrar el manejador de eventos Wi-Fi y IP
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, NULL));

    // 1. Iniciar UART para recibir comandos (SSID, Contraseña, Tópico MQTT)
    leido_uart_init(uart_data_ready_semaphore); // Pasa el handle del semáforo al componente UART

    // 2. Inicializar WiFi y Event Loop
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    // Esperar aquí hasta que los datos de UART sean recibidos y el semáforo sea liberado
    ESP_LOGI(TAG, "Esperando comandos de WiFi y MQTT por UART. Envía !wifi <ssid> <pass> y !topic <topico>, luego !done.");
    
    if (xSemaphoreTake(uart_data_ready_semaphore, portMAX_DELAY) == pdTRUE) {
        ESP_LOGI(TAG, "Comandos UART recibidos. Continuando con la inicialización de WiFi y MQTT.");
    } else {
        ESP_LOGE(TAG, "Error: El semáforo UART no se liberó inesperadamente.");
    }

    // 3. Inicializar WiFi usando los datos recibidos del UART
    wifi_init_sta();

    // 4. Iniciar conexión al broker MQTT
    mqtt_app_start();

    // 5. Crear la tarea de publicación MQTT
    // Esta tarea esperará internamente por el semáforo 'wifi_connected_semaphore'
    // antes de empezar a publicar.
    xTaskCreate(mqtt_publish_task, "mqtt_pub_task", 4096, NULL, 5, NULL);

    // La tarea principal de app_main continuará ejecutándose.
}