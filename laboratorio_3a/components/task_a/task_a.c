/**
 * @file task_a.c
 * @brief Tarea A: Parpadeo del LED RGB según el color actual
 * 
 * Esta tarea lee periódicamente el valor actual del color mediante el módulo `color`
 * y enciende el LED con ese color. Luego lo apaga, generando un efecto de parpadeo.
 * 
 * El delay se realiza con `vTaskDelay()` para no bloquear el scheduler.
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led.h"
#include "color.h"

extern led_strip_t *led_strip; /**< Instancia global del LED RGB, inicializada en main.c */

/**
 * @brief Tarea que controla el parpadeo del LED RGB
 * 
 * Esta tarea se ejecuta en un bucle infinito, leyendo el color actual desde
 * el módulo `color` y haciendo parpadear el LED con ese color.
 * 
 * El parpadeo se realiza con un intervalo fijo usando `vTaskDelay()`.
 * 
 * @param[in] pvParameters No se utiliza
 */
static void task_a(void *pvParameters) {
    const TickType_t delay = pdMS_TO_TICKS(500);
    while (1) {
        color_t color = color_get();

        switch (color) {
            case COLOR_RED:
                led_set_color(led_strip, 255, 0, 0);
                break;
            case COLOR_GREEN:
                led_set_color(led_strip, 0, 255, 0);
                break;
            case COLOR_BLUE:
                led_set_color(led_strip, 0, 0, 255);
                break;
            default:
                led_off(led_strip);
                break;
        }

        vTaskDelay(delay);
        led_off(led_strip);
        vTaskDelay(delay);
    }

    vTaskDelete(NULL);
}

/**
 * @brief Crea la tarea A que parpadea el LED
 * 
 * Esta función debe ser llamada desde `main.c` después de inicializar el LED y el módulo `color`.
 */
void start_task_a() {
    xTaskCreate(task_a, "TaskA_LED", 2048, NULL, 1, NULL);
}
