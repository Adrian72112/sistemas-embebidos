#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

typedef struct {
    char color[16];
    int tiempo_ms;
} color_command_t;

void start_task_b(QueueHandle_t queue);