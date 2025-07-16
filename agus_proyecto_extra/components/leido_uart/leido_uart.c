#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "leido_uart.h"

#define EX_UART_NUM UART_NUM_0
#define BUF_SIZE (1024)
#define RD_BUF_SIZE (BUF_SIZE)

static QueueHandle_t uart0_queue;
static const char *TAG = "leido_uart";

// Variables globales
char wifi_ssid[64] = "ssid";
char wifi_pass[64] = "pass";
char mqtt_topic[64] = "/topic";

static void uart_event_task(void *pvParameters)
{
    uart_event_t event;
    uint8_t* dtmp = (uint8_t*) malloc(RD_BUF_SIZE);

    for (;;) {
        if (xQueueReceive(uart0_queue, (void *)&event, portMAX_DELAY)) {
            bzero(dtmp, RD_BUF_SIZE);
            switch (event.type) {
                case UART_DATA:
                    uart_read_bytes(EX_UART_NUM, dtmp, event.size, portMAX_DELAY);
                    dtmp[event.size] = '\0'; // asegurarse que sea un string
                    ESP_LOGI(TAG, "Recibido: %s", dtmp);

                    if (strncmp((char *)dtmp, "!wifi", 5) == 0) {
                        char ssid[64], pass[64];
                        if (sscanf((char *)dtmp, "!wifi %s %s", ssid, pass) == 2) {
                            strcpy(wifi_ssid, ssid);
                            strcpy(wifi_pass, pass);
                            ESP_LOGI(TAG, "Nuevo WiFi -> SSID: %s, PASS: %s", wifi_ssid, wifi_pass);
                        }
                    } else if (strncmp((char *)dtmp, "!topic", 6) == 0) {
                        char topic[64];
                        if (sscanf((char *)dtmp, "!topic %s", topic) == 1) {
                            strcpy(mqtt_topic, topic);
                            ESP_LOGI(TAG, "Nuevo tópico -> %s", mqtt_topic);
                        }
                    }
                    break;

                case UART_FIFO_OVF:
                case UART_BUFFER_FULL:
                    uart_flush_input(EX_UART_NUM);
                    xQueueReset(uart0_queue);
                    break;
                default:
                    break;
            }
        }
    }

    free(dtmp);
    vTaskDelete(NULL);
}

void uart_cmd_init(void)
{
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    uart_driver_install(EX_UART_NUM, BUF_SIZE * 2, BUF_SIZE * 2, 20, &uart0_queue, 0);
    uart_param_config(EX_UART_NUM, &uart_config);
    uart_set_pin(EX_UART_NUM, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    xTaskCreate(uart_event_task, "uart_event_task", 4096, NULL, 10, NULL);
}
