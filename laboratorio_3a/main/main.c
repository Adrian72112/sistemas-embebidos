/**
 * @file main.c
 * @brief Punto de entrada principal del sistema embebido con FreeRTOS
 *
 * Este archivo contiene la función `app_main`, responsable de inicializar 
 * los componentes del sistema y lanzar las tareas concurrentes. 
 * El sistema está basado en FreeRTOS y se compone de un LED RGB, 
 * un módulo de lectura de comandos y una lógica de control.
 *
 * Tareas creadas:
 * - task_a: Controla el parpadeo del LED según el color actual configurado.
 * - task_b: Lee comandos enviados por UART y los envía a través de una cola.
 * - task_c: Procesa los comandos recibidos en la cola y actualiza la configuración del sistema.
 *
 * Todas las tareas comparten recursos sincronizados y utilizan mecanismos
 * de comunicación provistos por FreeRTOS como colas y semáforos.
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "freertos/queue.h"
#include "color.h"
#include "task_a.h"
#include "task_b.h"
#include "task_c.h"

#define QUEUE_LENGTH 10

/**
 * @brief Función principal del programa (`app_main`)
 *
 * Crea la cola de comandos utilizada para la comunicación entre tareas. 
 * Luego inicializa y lanza las tres tareas principales del sistema:
 * - `task_a`: LED parpadeante según el color actual.
 * - `task_b`: Recepción de comandos por UART.
 * - `task_c`: Procesamiento de comandos y configuración del sistema.
 *
 * Si la creación de la cola falla, el sistema imprime un mensaje de error
 * y no continúa con la ejecución.
 *
 * @return int Esta función no retorna; si falla la inicialización de la cola, retorna inmediatamente.
 */
void app_main(void) {
    QueueHandle_t command_queue = xQueueCreate(QUEUE_LENGTH, sizeof(color_command_t));
    if (command_queue == NULL) {
        printf("Error al crear la cola\n");
        return;
    }

    start_task_a();
    start_task_b(command_queue);
    start_task_c(command_queue);
}
