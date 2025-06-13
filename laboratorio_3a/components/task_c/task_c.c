/**
 * @file task_c.c
 * @brief Implementación de la tarea encargada de procesar comandos de color con temporización
 *
 * Esta tarea recibe comandos desde una cola compartida con `task_b`, convierte el nombre del color
 * a un valor `enum`, y crea un temporizador de FreeRTOS que, al vencer, actualiza el color mediante `color_set`.
 */

#include <string.h>
#include <stdlib.h>
#include "task_c.h"
#include "task_b.h"
#include "color.h"

typedef struct {
    color_t color;  ///< Color a establecer al vencer el temporizador
} timer_data_t;

/**
 * @brief Callback ejecutado cuando el temporizador vence.
 * 
 * Cambia el color del sistema al especificado y libera la memoria asociada al temporizador.
 *
 * @param xTimer Temporizador que expiró.
 */
static void color_timeout_callback(TimerHandle_t xTimer) {
    timer_data_t *data = (timer_data_t *)pvTimerGetTimerID(xTimer);
    if (data != NULL) {
        printf("Timer vencido, seteando color %d\n", data->color);
        color_set(data->color);
        free(data);  // liberar memoria
    }
}

/**
 * @brief Convierte el nombre del color (string) al tipo `color_t`.
 *
 * @param color_name Cadena con el nombre del color ("rojo", "verde", "azul").
 * @return color_t Valor correspondiente del enum o COLOR_NONE si no coincide.
 */
static color_t parse_color_name(const char *color_name) {
    if (strcmp(color_name, "rojo") == 0) return COLOR_RED;
    if (strcmp(color_name, "verde") == 0) return COLOR_GREEN;
    if (strcmp(color_name, "azul") == 0) return COLOR_BLUE;
    return COLOR_NONE;
}

/**
 * @brief Función de la tarea `task_c`.
 *
 * Recibe comandos desde la cola, interpreta el color y duración, 
 * y configura un temporizador de una sola ejecución que cambiará el color al finalizar.
 *
 * @param param Puntero a la cola de comandos (tipo `QueueHandle_t`).
 */
static void task_c(void *param) {
    QueueHandle_t queue = (QueueHandle_t)param;
    color_command_t command;

    while (1) {
        if (xQueueReceive(queue, &command, portMAX_DELAY)) {
            color_t c = parse_color_name(command.color);
            printf("Task C: Recibido color=%s (%d), duración=%dms\n", command.color, c, command.tiempo_ms);

            // Crear estructura para pasar como ID del timer
            timer_data_t *data = malloc(sizeof(timer_data_t));
            if (data == NULL) {
                printf("Error asignando memoria para timer\n");
                continue;
            }
            data->color = c;

            // Crear el timer que cambiará el color tras el delay
            TimerHandle_t timer = xTimerCreate(
                "color_timer",
                pdMS_TO_TICKS(command.tiempo_ms),
                pdFALSE,
                data,
                color_timeout_callback
            );

            if (timer != NULL) {
                xTimerStart(timer, 0);
            } else {
                printf("Error creando timer\n");
                free(data);  // liberar si falla
            }
        }
    }
    
    vTaskDelete(NULL);
}

/**
 * @brief Inicializa la tarea `task_c`.
 *
 * Crea y lanza la tarea que procesa comandos y maneja timers de cambio de color.
 *
 * @param queue Cola desde donde se reciben los comandos (compartida con `task_b`).
 */
void start_task_c(QueueHandle_t queue) {
    xTaskCreate(task_c, "task_c", 4096, (void *)queue, 1, NULL);
}
