// leido_uart.h
#ifndef __LEIDO_UART_H__
#define __LEIDO_UART_H__

// Declaraciones de variables globales que serán definidas en leido_uart.c
extern char wifi_ssid[64];
extern char wifi_pass[64];
extern char mqtt_topic[64];

// Declaración de la función de inicialización
void leido_uart_init(void);

#endif // __LEIDO_UART_H__