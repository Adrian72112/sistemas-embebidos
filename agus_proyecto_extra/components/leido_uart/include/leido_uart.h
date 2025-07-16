// leido_uart.h
#ifndef __LEIDO_UART_H__
#define __LEIDO_UART_H__

#include "freertos/FreeRTOS.h" // Necesario para SemaphoreHandle_t
#include "freertos/semphr.h"   // ¡NUEVO! Incluir FreeRTOS semaphores

// Declaraciones de variables globales que serán definidas en leido_uart.c
extern char wifi_ssid[64];
extern char wifi_pass[64];
extern char mqtt_topic[64];
extern char mqtt_uri[128];
// Declaración de la función de inicialización del UART de comandos
// Ahora recibe un handle a un semáforo
void leido_uart_init(SemaphoreHandle_t sync_semaphore);
#endif // __LEIDO_UART_H__