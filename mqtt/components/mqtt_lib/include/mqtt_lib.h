#pragma once

#include "esp_err.h"

typedef void (*mqtt_message_callback_t)(const char *topic, const char *data, int len);

// Inicializa la librería MQTT y establece el callback de recepción
esp_err_t mqtt_lib_init(const char *broker_uri, mqtt_message_callback_t callback);//se declara la funcion
