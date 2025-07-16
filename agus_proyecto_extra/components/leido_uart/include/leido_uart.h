#pragma once

void leido_uart_init(void);

// Variables globales accesibles desde otros archivos
extern char wifi_ssid[64];
extern char wifi_pass[64];
extern char mqtt_topic[64];

