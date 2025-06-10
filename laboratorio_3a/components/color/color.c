#include "color.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

// Variable que almacena el color actual
static color_t current_color = COLOR_NONE;
// Semáforo para proteger acceso concurrente
static SemaphoreHandle_t color_mutex;

void color_init(void) {
    color_mutex = xSemaphoreCreateMutex();
}

void color_set(color_t color) {
    if (xSemaphoreTake(color_mutex, portMAX_DELAY)) {
        current_color = color;
        xSemaphoreGive(color_mutex);
    }
}

color_t color_get(void) {
    color_t c = COLOR_NONE;
    if (xSemaphoreTake(color_mutex, portMAX_DELAY)) {
        c = current_color;
        xSemaphoreGive(color_mutex);
    }
    return c;
}
