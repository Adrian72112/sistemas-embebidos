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

static QueueHandle_t uart_queue;
static const char *TAG = "TaskB";

static void task_b(void *pvParameters) {
    uart_event_t event;
    uint8_t *data = (uint8_t *) malloc(BUF_SIZE);

    while (1) {
        // Esperamos evento UART
        if (xQueueReceive(uart_queue, (void *)&event, portMAX_DELAY)) {
            switch (event.type) {
                case UART_DATA:
                    uart_read_bytes(UART_PORT, data, event.size, portMAX_DELAY);
                    uart_write_bytes(UART_PORT, (const char *)data, event.size);
                    data[event.size] = '\0';
                    ESP_LOGI(TAG, "Recibido: %s", data);
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

void start_task_b(void) {
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

    xTaskCreate(task_b, "task_b", 2048, NULL, 10, NULL);
}
