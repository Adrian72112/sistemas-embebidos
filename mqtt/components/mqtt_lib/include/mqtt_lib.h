#ifndef MQTT_LIB_H
#define MQTT_LIB_H
#include "esp_err.h"

// Tipo de función callback que se llamará cuando llegue un mensaje MQTT válido
typedef void (*mqtt_message_callback_t)(const char *topic, const char *data, int len);

// Inicializa el cliente MQTT con la URI del broker y un callback para mensajes recibidos
esp_err_t mqtt_lib_init(const char *broker_uri, mqtt_message_callback_t callback);

// Publica un mensaje en el topic dado, con QoS especificado (0 o 1)
esp_err_t mqtt_lib_publish(const char *topic, const char *payload, int qos);

#endif // MQTT_LIB_H
