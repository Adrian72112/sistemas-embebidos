#include <string.h>
#include <stdlib.h>
#include "task_c.h"
#include "task_b.h"
#include "color.h"

typedef struct {
    color_t color;
} timer_data_t;

// Esta función se llama cuando el timer vence
static void color_timeout_callback(TimerHandle_t xTimer) {
    timer_data_t *data = (timer_data_t *)pvTimerGetTimerID(xTimer);
    if (data != NULL) {
        printf("Timer vencido, seteando color %d\n", data->color);
        color_set(data->color);
        free(data);  // liberar memoria
    }
}

// Devuelve el enum color_t desde un string
static color_t parse_color_name(const char *color_name) {
    if (strcmp(color_name, "rojo") == 0) return COLOR_RED;
    if (strcmp(color_name, "verde") == 0) return COLOR_GREEN;
    if (strcmp(color_name, "azul") == 0) return COLOR_BLUE;
    return COLOR_NONE;
}

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
}

void start_task_c(QueueHandle_t queue) {
    xTaskCreate(task_c, "task_c", 4096, (void *)queue, 5, NULL);
}

