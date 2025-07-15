#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "mqtt_client.h"
#include "driver/gpio.h"

#ifndef CONFIG_LOG_MAXIMUM_LEVEL
#define CONFIG_LOG_MAXIMUM_LEVEL ESP_LOG_INFO
#endif

#define MQTT_BROKER "mqtt://broker.hivemq.com"
#define MQTT_TOPIC "test/topic"

static const char *TAG = "MQTT5_APP";

static esp_err_t mqtt_event_handler_cb(esp_mqtt_event_handle_t event) {
    esp_mqtt_client_handle_t client = event->client;

    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "✅ Conectado a MQTT");
            esp_mqtt_client_subscribe(client, MQTT_TOPIC, 0);
            break;

        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGW(TAG, "⚠️ Desconectado de MQTT");
            break;

        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "📩 Mensaje recibido: %.*s", event->data_len, event->data);
            gpio_set_level(GPIO_NUM_2, 1);
            vTaskDelay(500 / portTICK_PERIOD_MS);
            gpio_set_level(GPIO_NUM_2, 0);
            break;

        default:
            break;
    }
    return ESP_OK;
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    mqtt_event_handler_cb((esp_mqtt_event_handle_t)event_data);
}

void publish_task(void *pvParameters) {
    esp_mqtt_client_handle_t client = (esp_mqtt_client_handle_t)pvParameters;
    while (1) {
        esp_mqtt_client_publish(client, MQTT_TOPIC, "Kaluga activo", 0, 1, 0);
        ESP_LOGI(TAG, "📤 Publicado: Kaluga activo");
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
}

static void mqtt5_app_start(void) {
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = MQTT_BROKER,
        .session.protocol_ver = MQTT_PROTOCOL_V_5,
    };

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt5_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);

    xTaskCreate(&publish_task, "publish_task", 4096, client, 5, NULL);
}

void app_main(void) {
    ESP_LOGI(TAG, "🔧 Inicializando sistema...");

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(example_connect());

    gpio_reset_pin(GPIO_NUM_2);
    gpio_set_direction(GPIO_NUM_2, GPIO_MODE_OUTPUT);

    mqtt5_app_start();
}
