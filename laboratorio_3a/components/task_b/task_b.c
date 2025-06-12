#include "task_b.h"
#include "esp_log.h"
#include "string.h"
#include "stdio.h"
#include "stdlib.h"
#include "ctype.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "sdkconfig.h"

#define UART_PORT UART_NUM_0
#define BUF_SIZE 1024

static QueueHandle_t command_queue;
static QueueHandle_t uart_queue;
static const char *TAG = "TaskB";

static void parse_and_enqueue_commands(const char *input) {
    char buffer[BUF_SIZE];
    strncpy(buffer, input, BUF_SIZE - 1);
    buffer[BUF_SIZE - 1] = '\0';

    char *token = strtok(buffer, ",");
    while (token != NULL) {
        char *dash = strchr(token, '-');
        if (dash != NULL) {
            *dash = '\0';
            char *color = token;
            char *time_str = dash + 1;

            color_command_t cmd;
            strncpy(cmd.color, color, sizeof(cmd.color) - 1);
            cmd.color[sizeof(cmd.color) - 1] = '\0';
            cmd.tiempo_ms = atoi(time_str);

            if (xQueueSend(command_queue, &cmd, pdMS_TO_TICKS(100)) != pdPASS) {
                ESP_LOGW(TAG, "Cola llena, comando descartado: %s-%s", color, time_str);
            } else {
                ESP_LOGI(TAG, "Encolado: %s - %dms", cmd.color, cmd.tiempo_ms);
            }
        } else {
            ESP_LOGW(TAG, "Formato inválido: %s", token);
        }

        token = strtok(NULL, ",");
    }
}

static void task_b(void *pvParameters) {
    uart_event_t event;
    uint8_t *data = (uint8_t *) malloc(BUF_SIZE);
    if (!data) {
        ESP_LOGE(TAG, "No se pudo asignar memoria para el buffer UART");
        vTaskDelete(NULL);
        return;
    }

    while (1) {
        if (xQueueReceive(uart_queue, (void *)&event, portMAX_DELAY)) {
            switch (event.type) {
                case UART_DATA:
                    uart_read_bytes(UART_PORT, data, event.size, portMAX_DELAY);
                    data[event.size] = '\0';
                    ESP_LOGI(TAG, "Recibido por UART: %s", data);
                    parse_and_enqueue_commands((char *)data);
                    break;
                case UART_FIFO_OVF:
                case UART_BUFFER_FULL:
                    uart_flush_input(UART_PORT);
                    xQueueReset(uart_queue);
                    break;
                default:
                    break;
            }
        }
    }

    free(data);
    vTaskDelete(NULL);
}

void start_task_b(QueueHandle_t queue) {
    command_queue = queue;

    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    uart_driver_install(UART_PORT, BUF_SIZE * 2, BUF_SIZE * 2, 10, &uart_queue, 0);
    uart_param_config(UART_PORT, &uart_config);
    uart_set_pin(UART_PORT, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    xTaskCreate(task_b, "task_b", 4096, NULL, 12, NULL); // prioridad mayor
}
