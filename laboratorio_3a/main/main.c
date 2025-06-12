
/**
 * @file main.c
 * @brief Punto de entrada principal del programa
 * 
 * Inicializa el LED RGB, el módulo de color y lanza las tres tareas del sistema:
 * - task_a: Parpadeo del LED según el color actual
 * - task_b: Lectura desde UART y envío de comandos a la cola
 * - task_c: Procesamiento de comandos desde la cola y configuración de timers
 * 
 * Todas las tareas utilizan FreeRTOS.
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "freertos/queue.h"
#include "led.h"
#include "color.h"
#include "task_a.h"
#include "task_b.h"

#define QUEUE_LENGTH 10
// Declaración del puntero global al LED RGB
led_strip_t *led_strip = NULL;

/**
 * @brief Función principal del programa
 * 
 * Inicializa periféricos, el sistema de color con semáforo,
 * y arranca las tareas del sistema.
 * 
 * @return int No retorna nunca. Si falla la inicialización, retorna -1.
 */
void app_main(void) {
    if (led_init(&led_strip) != ESP_OK) {
        printf("Error al inicializar el LED\n");
        return;
    }

    color_init();

    QueueHandle_t command_queue = xQueueCreate(QUEUE_LENGTH, sizeof(color_command_t));
    if (command_queue == NULL) {
        printf("Error al crear la cola\n");
        return;
    }

    start_task_b(command_queue);
    color_command_t command;

    while (1) {
        if (xQueueReceive(command_queue, &command, portMAX_DELAY)) {
            ESP_LOGW("MAIN","Consumido de la cola -> Color: %s, Tiempo: %d ms\n", command.color, command.tiempo_ms);
            vTaskDelay(pdMS_TO_TICKS(500));
        }
    }
    // start_task_a(); // si ya tenés task_a lista
    // start_task_c(command_queue); // cuando esté lista la C
}