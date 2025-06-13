/**
 * @file task_b.c
 * @brief Implementación de la tarea encargada de recibir comandos por UART y encolarlos
 *
 * Esta tarea escucha eventos del UART, interpreta comandos con el formato `color-tiempo`,
 * y los coloca en una cola compartida para ser procesados por `task_c`.
 */
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
#define BUF_SIZE 2048
#define MAX_RETRY_ENQUEUE 5

static QueueHandle_t command_queue;
static QueueHandle_t uart_queue;
static const char *TAG = "TaskB";

static char input_buffer[BUF_SIZE * 2];
static size_t input_len = 0;

/**
 * @brief Parsea una cadena de entrada y encola comandos válidos en la `command_queue`.
 *
 * El formato esperado para cada comando es `color-tiempo`, separados por comas si hay varios.
 * Por ejemplo: `"rojo-1000,verde-2000"`.
 *
 * @param input Cadena recibida por UART con uno o más comandos.
 */
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

            int retry = 0;
            while (xQueueSend(command_queue, &cmd, pdMS_TO_TICKS(50)) != pdPASS && retry < MAX_RETRY_ENQUEUE) {
                ESP_LOGW(TAG, "Cola llena, reintentando encolar (%d/%d): %s-%s", retry + 1, MAX_RETRY_ENQUEUE, color, time_str);
                retry++;
                vTaskDelay(pdMS_TO_TICKS(100));
            }

            if (retry == MAX_RETRY_ENQUEUE) {
                ESP_LOGE(TAG, "¡No se pudo encolar comando tras %d reintentos! DESCARTADO: %s-%s", MAX_RETRY_ENQUEUE, color, time_str);
            } else {
                ESP_LOGI(TAG, "Encolado: %s - %dms", cmd.color, cmd.tiempo_ms);
            }
        } else {
            ESP_LOGW(TAG, "Formato inválido: %s", token);
        }

        token = strtok(NULL, ",");
    }
}



/**
 * @brief Función principal de la tarea `task_b`.
 *
 * Escucha eventos del UART, lee datos cuando llegan, e invoca el parser
 * que genera y encola los comandos en `command_queue`.
 *
 * @param pvParameters No se usa.
 */
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
                ESP_LOGI(TAG, "Evento UART_DATA: size = %d", event.size);
                int len = uart_read_bytes(UART_PORT, data, event.size, portMAX_DELAY);
                if (len <= 0) {
                    ESP_LOGW(TAG, "No se leyeron datos del UART");
                    break;
                }

                for (int i = 0; i < len; i++) {
                    char ch = data[i];

                    if (ch == '.') {
                        input_buffer[input_len] = '\0';  // Terminar string
                        ESP_LOGI(TAG, "Recibido por UART (completo): %s", input_buffer);
                        parse_and_enqueue_commands(input_buffer);
                        input_len = 0;
                    } else {
                        if (input_len < sizeof(input_buffer) - 1) {
                            input_buffer[input_len++] = ch;
                        } else {
                            ESP_LOGW(TAG, "Input buffer overflow. Mensaje descartado");
                            input_len = 0;
                        }
                    }
                }
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

/**
 * @brief Inicializa la tarea `task_b` y configura el UART.
 *
 * La tarea se encarga de escuchar el puerto serie, parsear los comandos
 * y enviarlos a la cola indicada.
 *
 * @param queue Cola a la que se enviarán los comandos de tipo `color_command_t`.
 */
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

    xTaskCreate(task_b, "task_b", 4096, NULL, 5, NULL); // prioridad mayor
}
