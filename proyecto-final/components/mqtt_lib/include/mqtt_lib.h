#pragma once

#include "esp_err.h"
#include "mqtt_client.h"

typedef void (*mqtt_message_callback_t)(const char *topic, const char *data, int len);
typedef void (*mqtt_connected_callback_t)(void);
typedef void (*mqtt_published_callback_t)(int msg_id);

// Inicializa la librería MQTT y establece el callback de recepción
esp_err_t mqtt_lib_init(const char *broker_uri, mqtt_message_callback_t callback);

// Establece el callback para cuando se establece la conexión MQTT
esp_err_t mqtt_lib_set_connected_callback(mqtt_connected_callback_t callback);

// Establece el callback para cuando se confirma una publicación (PUBACK)
esp_err_t mqtt_lib_set_published_callback(mqtt_published_callback_t callback);

// Publica un mensaje con QoS especificado
esp_err_t mqtt_lib_publish(const char *topic, const char *data, int len, int qos);

// Obtiene el estado de conexión MQTT
bool mqtt_lib_is_connected(void);

// Configura el topic de suscripción MQTT (debe llamarse antes de mqtt_lib_init)
esp_err_t mqtt_lib_set_subscription_topic(const char *topic);
