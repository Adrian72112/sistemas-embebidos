#include "mqtt_lib.h"
#include "mqtt_client.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "mqtt_lib";
static mqtt_message_callback_t user_callback = NULL;
static mqtt_connected_callback_t connected_callback = NULL;
static mqtt_published_callback_t published_callback = NULL;
static esp_mqtt_client_handle_t mqtt_client = NULL;
static bool is_connected = false;

static void mqtt_event_handler(void *args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;

    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "Connected to MQTT broker");
            is_connected = true;
            esp_mqtt_client_subscribe(client, "/topic/qos1", 1);
            
            // Llamar callback de conexión establecida si está configurado
            if (connected_callback) {
                connected_callback();
            }
            break;
            
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "Disconnected from MQTT broker");
            is_connected = false;
            break;
            
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "Message published successfully, msg_id=%d", event->msg_id);
            
            // Llamar callback de publicación confirmada si está configurado
            if (published_callback) {
                published_callback(event->msg_id);
            }
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
            
        case MQTT_EVENT_ERROR:
            ESP_LOGE(TAG, "MQTT error occurred");
            break;
            
        default:
            ESP_LOGD(TAG, "MQTT event: %ld", event_id);
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

    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    if (!mqtt_client) return ESP_FAIL;

    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    return esp_mqtt_client_start(mqtt_client);
}

esp_err_t mqtt_lib_set_connected_callback(mqtt_connected_callback_t callback)
{
    connected_callback = callback;
    return ESP_OK;
}

esp_err_t mqtt_lib_set_published_callback(mqtt_published_callback_t callback)
{
    published_callback = callback;
    return ESP_OK;
}

esp_err_t mqtt_lib_publish(const char *topic, const char *data, int len, int qos)
{
    if (!mqtt_client || !is_connected) {
        ESP_LOGE(TAG, "MQTT client not connected");
        return ESP_ERR_INVALID_STATE;
    }
    
    if (!topic || !data) {
        ESP_LOGE(TAG, "Invalid parameters for publish");
        return ESP_ERR_INVALID_ARG;
    }
    
    int msg_id = esp_mqtt_client_publish(mqtt_client, topic, data, len, qos, 0);
    if (msg_id == -1) {
        ESP_LOGE(TAG, "Failed to publish message");
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "Message published with msg_id=%d, QoS=%d", msg_id, qos);
    return ESP_OK;
}

bool mqtt_lib_is_connected(void)
{
    return is_connected;
}
