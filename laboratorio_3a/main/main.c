
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
#include "led.h"
#include "color.h"
#include "task_a.h"
#include "task_b.h"

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
    // Inicialización del LED
    if (led_init(&led_strip) != ESP_OK) {
        printf("Error al inicializar el LED\n");
        return;
    }
    // Inicialización del módulo COLOR
    color_init();
    // Inicio de las tareas del sistema
    //start_task_a();
    
    start_task_b();
    //start_task_c();
}