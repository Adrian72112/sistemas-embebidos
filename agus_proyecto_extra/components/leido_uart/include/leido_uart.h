#ifndef _LEIDO_UART_H_
#define _LEIDO_UART_H_

#include "freertos/FreeRTOS.h" // Necesario para SemaphoreHandle_t
#include "freertos/semphr.h"   // ¡NUEVO! Incluir FreeRTOS semaphores
#include "freertos/queue.h"    // Para QueueHandle_t

// Estructura para mensajes personalizados de MQTT
typedef struct {
    char message[256];  // Mensaje a publicar
} mqtt_custom_message_t;

// Declaraciones de variables globales que serán definidas en leido_uart.c
extern char wifi_ssid[64];
extern char wifi_pass[64];
extern char mqtt_topic[64];
extern char mqtt_uri[128];

// Cola global para enviar mensajes personalizados desde UART a MQTT
extern QueueHandle_t mqtt_message_queue;

// Declaración de la función de inicialización del UART de comandos
// Ahora recibe un handle a un semáforo y opcionalmente una cola para mensajes MQTT
void leido_uart_init(SemaphoreHandle_t sync_semaphore, QueueHandle_t message_queue);

#endif // _LEIDO_UART_H_