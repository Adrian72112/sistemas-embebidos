#include "mqtt_lib.h"
#include "mqtt_client.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "mqtt_lib";
static mqtt_message_callback_t user_callback = NULL;

static void mqtt_event_handler(void *args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;

    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "Connected");
            esp_mqtt_client_subscribe(client, "/topic/qos1", 1);
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "Message received on topic: %.*s", event->topic_len, event->topic);
            ESP_LOGI(TAG, "Data: %.*s", event->data_len, event->data);
            if (user_callback) {
                char topic[event->topic_len + 1];
                char data[event->data_len + 1];
                memcpy(topic, event->topic, event->topic_len);
                topic[event->topic_len] = '\0';
                memcpy(data, event->data, event->data_len);
                data[event->data_len] = '\0';
                user_callback(topic, data, event->data_len);
            }
            break;
        default:
            break;
    }
}

esp_err_t mqtt_lib_init(const char *broker_uri, mqtt_message_callback_t callback)
{
    if (!broker_uri || !callback) return ESP_ERR_INVALID_ARG;

    user_callback = callback;

    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = broker_uri,
        .session.protocol_ver = MQTT_PROTOCOL_V_5,
    };

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    if (!client) return ESP_FAIL;

    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    return esp_mqtt_client_start(client);
}
