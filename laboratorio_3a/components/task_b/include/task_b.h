#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

/**
 * @brief Inicializa y lanza la tarea B
 * 
 * @param queue Cola de strings que representa comandos "color-tiempo"
 */
void start_task_b(void);