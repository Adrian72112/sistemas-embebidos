#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "leido_uart.h" // Asegurate de que este archivo incluya las declaraciones extern

#define EX_UART_NUM UART_NUM_0
#define BUF_SIZE (1024)
#define RD_BUF_SIZE (BUF_SIZE)

static QueueHandle_t uart0_queue;
static const char *TAG = "leido_uart";

// Variables globales. Se inicializan con valores por defecto.
// Estas variables son las que app_main.c usará para la conexión.
char wifi_ssid[64] = "ssid_por_defecto";
char wifi_pass[64] = "pass_por_defecto";
char mqtt_topic[64] = "/topico/por/defecto";

static void uart_event_task(void *pvParameters)
{
    uart_event_t event;
    uint8_t* dtmp = (uint8_t*) malloc(RD_BUF_SIZE);
    if (dtmp == NULL) {
        ESP_LOGE(TAG, "No se pudo asignar memoria para el búfer UART. La tarea de UART finalizará.");
        vTaskDelete(NULL);
        return;
    }

    for (;;) {
        // Espera indefinidamente por eventos en la cola UART
        if (xQueueReceive(uart0_queue, (void *)&event, portMAX_DELAY)) {
            // Limpia el buffer temporal para cada nuevo evento
            bzero(dtmp, RD_BUF_SIZE);

            switch (event.type) {
                case UART_DATA:
                    // Lee los bytes recibidos y asegura la terminación nula
                    uart_read_bytes(EX_UART_NUM, dtmp, event.size, portMAX_DELAY);
                    dtmp[event.size] = '\0';
                    ESP_LOGI(TAG, "Recibido: %s", dtmp);

                    // Procesa los comandos
                    if (strncmp((char *)dtmp, "!wifi", 5) == 0) {
                        char ssid_temp[64], pass_temp[64];
                        // sscanf con %63s para evitar desbordamientos de búfer en los temporales
                        if (sscanf((char *)dtmp, "!wifi %63s %63s", ssid_temp, pass_temp) == 2) {
                            // Copia de forma segura a las variables globales
                            strncpy(wifi_ssid, ssid_temp, sizeof(wifi_ssid) - 1);
                            wifi_ssid[sizeof(wifi_ssid) - 1] = '\0'; // Asegura terminación nula
                            strncpy(wifi_pass, pass_temp, sizeof(wifi_pass) - 1);
                            wifi_pass[sizeof(wifi_pass) - 1] = '\0'; // Asegura terminación nula
                            ESP_LOGI(TAG, "Nuevo WiFi -> SSID: %s, PASS: [OCULTO]", wifi_ssid);
                        } else {
                            ESP_LOGW(TAG, "Formato incorrecto para !wifi. Uso: !wifi <ssid> <pass>");
                        }
                    } else if (strncmp((char *)dtmp, "!topic", 6) == 0) {
                        char topic_temp[64];
                        if (sscanf((char *)dtmp, "!topic %63s", topic_temp) == 1) {
                            strncpy(mqtt_topic, topic_temp, sizeof(mqtt_topic) - 1);
                            mqtt_topic[sizeof(mqtt_topic) - 1] = '\0'; // Asegura terminación nula
                            ESP_LOGI(TAG, "Nuevo tópico -> %s", mqtt_topic);
                        } else {
                            ESP_LOGW(TAG, "Formato incorrecto para !topic. Uso: !topic <topico>");
                        }
                    } else {
                        ESP_LOGW(TAG, "Comando desconocido: %s", dtmp);
                    }
                    break;

                case UART_FIFO_OVF:
                    ESP_LOGW(TAG, "UART FIFO Overflow. Vaciando buffer de entrada.");
                    uart_flush_input(EX_UART_NUM);
                    xQueueReset(uart0_queue);
                    break;
                case UART_BUFFER_FULL:
                    ESP_LOGW(TAG, "UART Buffer Lleno. Vaciando buffer de entrada.");
                    uart_flush_input(EX_UART_NUM);
                    xQueueReset(uart0_queue);
                    break;
                case UART_PATTERN_DET:
                    ESP_LOGI(TAG, "Patrón UART detectado.");
                    // Si estás usando detección de patrón, podrías añadir lógica aquí
                    break;
                case UART_BREAK:
                    ESP_LOGI(TAG, "Break UART detectado.");
                    break;
                case UART_PARITY_ERR:
                    ESP_LOGE(TAG, "Error de paridad UART.");
                    break;
                case UART_FRAME_ERR:
                    ESP_LOGE(TAG, "Error de trama UART.");
                    break;
                default:
                    ESP_LOGW(TAG, "Evento UART no manejado: %d", event.type);
                    break;
            }
        }
    }

    // Esta línea es inalcanzable porque la tarea se ejecuta en un bucle infinito
    // free(dtmp);
    // vTaskDelete(NULL);
}

// Función de inicialización del UART de comandos, expuesta a app_main.c
void leido_uart_init(void)
{
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    // Instala el driver UART, configura los pines y crea la cola de eventos
    // buffer de lectura y escritura (doble tamaño para búfer doble), y tamaño de la cola de eventos
    ESP_ERROR_CHECK(uart_driver_install(EX_UART_NUM, BUF_SIZE * 2, BUF_SIZE * 2, 20, &uart0_queue, 0));
    ESP_ERROR_CHECK(uart_param_config(EX_UART_NUM, &uart_config));
    // No cambiamos los pines TX/RX; usa los predeterminados si no se especifican.
    ESP_ERROR_CHECK(uart_set_pin(EX_UART_NUM, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    // Crea la tarea para manejar los eventos UART
    // La pila de 4096 bytes suele ser suficiente para esta tarea.
    xTaskCreate(uart_event_task, "uart_event_task", 4096, NULL, 10, NULL);
    ESP_LOGI(TAG, "UART inicializado. Envía comandos: !wifi <ssid> <pass> o !topic <topico>");
}