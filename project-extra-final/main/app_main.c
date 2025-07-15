#define MQTT_SUPPORTED_FEATURE_EVENT 1
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include "driver/gpio.h"

#define WIFI_SSID CONFIG_WIFI_SSID
#define WIFI_PASS CONFIG_WIFI_PASSWORD
#define MQTT_BROKER CONFIG_MQTT_BROKER_URI
#define MQTT_TOPIC CONFIG_MQTT_TOPIC

static const char *TAG = "mqtt5_example";

// Tarea que publica un mensaje cada 50 segundos
void publish_task(void *pvParameters)
{
    esp_mqtt_client_handle_t client = (esp_mqtt_client_handle_t)pvParameters;

    while (1) {
        esp_mqtt_client_publish(client, MQTT_TOPIC, "Kaluga activo", 0, 1, 0);
        ESP_LOGI(TAG, "\U0001f4e4 Publicado: Kaluga activo");
        vTaskDelay(50000 / portTICK_PERIOD_MS);
    }
}

// Handler para eventos MQTT
static esp_err_t mqtt_event_handler_cb(esp_mqtt_event_handle_t event)
{
    esp_mqtt_client_handle_t client = event->client;

    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "\u2705 Conectado a MQTT.");
            esp_mqtt_client_subscribe(client, MQTT_TOPIC, 0);
            ESP_LOGI(TAG, "\U0001f4e5 Suscripto al tópico: %s", MQTT_TOPIC);
            break;

        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGW(TAG, "\u26a0\ufe0f MQTT desconectado");
            break;

        case MQTT_EVENT_DATA:
            printf("\U0001f4e9 Mensaje recibido en %.*s: %.*s\n",
                   event->topic_len, event->topic,
                   event->data_len, event->data);

            // Prender LED al recibir mensaje (opcional)
            gpio_set_level(GPIO_NUM_2, 1);
            vTaskDelay(500 / portTICK_PERIOD_MS);
            gpio_set_level(GPIO_NUM_2, 0);
            break;

        default:
            break;
    }
    return ESP_OK;
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    mqtt_event_handler_cb((esp_mqtt_event_handle_t)event_data);
}

// Inicialización y arranque del cliente MQTT
static void mqtt5_app_start(void)
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = MQTT_BROKER,
        .session.protocol_ver = MQTT_PROTOCOL_V_5,
    };

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);

    xTaskCreate(&publish_task, "publish_task", 4096, client, 5, NULL);
}


void app_main(void)
{
    ESP_LOGI(TAG, "Inicializando sistema...");

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Conectar a WiFi
    ESP_ERROR_CHECK(example_connect());

    // Inicializar LED (opcional)
    gpio_reset_pin(GPIO_NUM_2);
    gpio_set_direction(GPIO_NUM_2, GPIO_MODE_OUTPUT);

    // Arrancar cliente MQTT
    mqtt5_app_start();
}
