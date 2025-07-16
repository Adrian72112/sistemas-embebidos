#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"    // Para semáforos de sincronización
#include "esp_log.h"
#include "esp_system.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "mqtt_client.h"
#include "esp_netif.h"
#include "esp_mac.h"
#include "sdkconfig.h"
#include "leido_uart.h" // Archivo donde definís el manejo de comandos UART

static const char *TAG = "MQTT_MAIN";



// Cliente MQTT global
esp_mqtt_client_handle_t client = NULL;

// Semáforo para sincronizar cuando se reciban comandos por UART
SemaphoreHandle_t uart_data_ready_semaphore;

// 🟢 Tarea que publica cada 50 segundos un mensaje "<ID> activo"
static void mqtt_publish_task(void *pvParameters)
{
    char payload[100];
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA); // Obtener dirección MAC como ID
    char device_id[13];
    sprintf(device_id, "%02X%02X%02X%02X%02X%02X",
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    while (true) {
        if (client != NULL) {
            sprintf(payload, "%s activo", device_id);
            int msg_id = esp_mqtt_client_publish(client, mqtt_topic, payload, 0, 1, 0);
            if (msg_id != -1) {
                ESP_LOGI(TAG, "Mensaje publicado: '%s' en tópico '%s'", payload, mqtt_topic);
            } else {
                ESP_LOGW(TAG, "Error al publicar. Cliente MQTT desconectado.");
            }
        } else {
            ESP_LOGW(TAG, "Cliente MQTT no inicializado.");
        }
        vTaskDelay(pdMS_TO_TICKS(50000)); // Esperar 50 segundos
    }
}

// 🟢 Callback de eventos MQTT (suscripción, publicación, recepción, errores)
static void mqtt_event_handler_cb(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
    client = event->client;

    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT conectado");
            esp_mqtt_client_subscribe(client, mqtt_topic, 0); // Suscribirse al tópico configurado
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "Mensaje recibido:");
            printf("  TOPIC=%.*s\r\n", event->topic_len, event->topic);
            printf("  DATA=%.*s\r\n", event->data_len, event->data);
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGW(TAG, "MQTT desconectado");
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGE(TAG, "Error MQTT");
            break;
        default:
            ESP_LOGI(TAG, "Otro evento MQTT ID: %" PRIi32, event_id);
            break;
    }
}

// 🟢 Inicializar cliente MQTT con la URI configurada por UART
static void mqtt_app_start(void)
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = mqtt_uri,
    };

    client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler_cb, NULL);
    esp_mqtt_client_start(client);
}

// 🟢 Manejador de eventos WiFi
static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGW(TAG, "WiFi desconectado. Reintentando...");
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Conectado. IP: " IPSTR, IP2STR(&event->ip_info.ip));

        // Mostrar RSSI
        wifi_ap_record_t ap_info;
        if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
            ESP_LOGI(TAG, "RSSI: %d dBm", ap_info.rssi);
        }
    }
}

// 🟢 Inicializar WiFi en modo estación con los datos de UART
static void wifi_init_sta(void)
{
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = "",
            .password = "",
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = {.capable = true, .required = false},
        },
    };

    strncpy((char *)wifi_config.sta.ssid, wifi_ssid, sizeof(wifi_config.sta.ssid) - 1);
    strncpy((char *)wifi_config.sta.password, wifi_pass, sizeof(wifi_config.sta.password) - 1);

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "WiFi iniciado con SSID: %s", wifi_ssid);
}

// 🔵 Función principal
void app_main(void)
{
    // Inicializar almacenamiento NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Crear semáforo para sincronizar datos UART
    uart_data_ready_semaphore = xSemaphoreCreateBinary();
    if (uart_data_ready_semaphore == NULL) {
        ESP_LOGE(TAG, "Error al crear semáforo. Reiniciando...");
        esp_restart();
    }

    // Inicializar red y UART
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    leido_uart_init(uart_data_ready_semaphore);

    // Esperar ingreso por UART (!wifi, !uri, !topic, !done)
    ESP_LOGI(TAG, "Esperando comandos UART. Enviar !done para continuar...");
    xSemaphoreTake(uart_data_ready_semaphore, portMAX_DELAY);

    // Conectar a WiFi
    wifi_init_sta();

    // Conectar al broker MQTT
    mqtt_app_start();

    // Lanzar tarea de publicación periódica
    xTaskCreate(mqtt_publish_task, "mqtt_pub_task", 4096, NULL, 5, NULL);
}
