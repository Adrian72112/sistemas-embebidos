/**
 * @file task_c.h
 * @brief Declaración de la función para inicializar la tarea `task_c`
 *
 * La tarea `task_c` se encarga de recibir comandos desde una cola,
 * configurar temporizadores y cambiar el color luego de setear el timmer.
 */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "freertos/queue.h"

/**
 * @brief Inicia la tarea encargada de manejar temporizadores para cambio de color.
 * 
 * @param queue Cola de comandos que será utilizada por la tarea.
 */
void start_task_c(QueueHandle_t queue);
