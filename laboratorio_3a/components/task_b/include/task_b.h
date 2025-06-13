/**
 * @file task_b.h
 * @brief Declaraciones para la tarea `task_b` encargada de leer comandos por UART
 *
 * Esta tarea interpreta cadenas del tipo `color-tiempo` y encola comandos para su procesamiento.
 */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

/**
 * @brief Estructura que representa un comando con nombre de color y duración en milisegundos.
 */
typedef struct {
    char color[16];      ///< Nombre del color, ej: "rojo", "verde", "azul"
    int tiempo_ms;       ///< Tiempo en milisegundos para mantener ese color
} color_command_t;

/**
 * @brief Inicializa la tarea que lee comandos por UART y los encola.
 *
 * @param queue Cola a la que se enviarán los comandos `color_command_t`.
 */
void start_task_b(QueueHandle_t queue);
