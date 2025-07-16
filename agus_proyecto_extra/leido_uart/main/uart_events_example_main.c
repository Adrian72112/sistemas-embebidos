#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"   // ¡NUEVO! Incluir FreeRTOS semaphores
#include "driver/uart.h"
#include "esp_log.h"
#include "leido_uart.h"

#define EX_UART_NUM UART_NUM_0
#define BUF_SIZE (1024)
#define RD_BUF_SIZE (BUF_SIZE)

static QueueHandle_t uart0_queue;
static const char *TAG = "leido_uart";

// Variables globales
char wifi_ssid[64] = "ssid_por_defecto";
char wifi_pass[64] = "pass_por_defecto";
char mqtt_topic[64] = "/topico/por/defecto";
char mqtt_uri[128] = "mqtt://broker.hivemq.com";

// Puntero global al semáforo para que la tarea de eventos UART pueda acceder a él.
// Será inicializado en uart_cmd_init
static SemaphoreHandle_t uart_sync_semaphore = NULL;

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
        if (xQueueReceive(uart0_queue, (void *)&event, portMAX_DELAY)) {
            bzero(dtmp, RD_BUF_SIZE);
            switch (event.type) {
                case UART_DATA:
                    uart_read_bytes(EX_UART_NUM, dtmp, event.size, portMAX_DELAY);
                    dtmp[event.size] = '\0';
                    ESP_LOGI(TAG, "Recibido: %s", dtmp);

                    if (strncmp((char *)dtmp, "!wifi", 5) == 0) {
                        char ssid_temp[64], pass_temp[64];
                        if (sscanf((char *)dtmp, "!wifi %63s %63s", ssid_temp, pass_temp) == 2) {
                            strncpy(wifi_ssid, ssid_temp, sizeof(wifi_ssid) - 1);
                            wifi_ssid[sizeof(wifi_ssid) - 1] = '\0';
                            strncpy(wifi_pass, pass_temp, sizeof(wifi_pass) - 1);
                            wifi_pass[sizeof(wifi_pass) - 1] = '\0';
                            ESP_LOGI(TAG, "Nuevo WiFi -> SSID: %s, PASS: [OCULTO]", wifi_ssid);
                        } else {
                            ESP_LOGW(TAG, "Formato incorrecto para !wifi. Uso: !wifi <ssid> <pass>");
                        }
                    } else if (strncmp((char *)dtmp, "!topic", 6) == 0) {
                        char topic_temp[64];
                        if (sscanf((char *)dtmp, "!topic %63s", topic_temp) == 1) {
                            strncpy(mqtt_topic, topic_temp, sizeof(mqtt_topic) - 1);
                            mqtt_uri[sizeof(mqtt_uri) - 1] = '\0';
                            ESP_LOGI(TAG, "Nuevo URI MQTT ->%s",mqtt_uri);
                        } else {
                            ESP_LOGW(TAG, "Formato incorrecto para !topic. Uso: !topic <topico>");
                        }
                    } else if (strncmp((char *)dtmp, "!done", 5) == 0) { // ¡NUEVO COMANDO!
                        ESP_LOGI(TAG, "Comando !done recibido. Liberando semáforo para continuar.");
                        if (uart_sync_semaphore != NULL) {
                            xSemaphoreGive(uart_sync_semaphore); // Liberar el semáforo para desbloquear app_main
                        }
                    } else {
                        ESP_LOGW(TAG, "Comando desconocido: %s", dtmp);
                    } else if (strncmp((char *)dtmp, "!uri", 4) == 0) {
                        char uri_temp[128];
                        if (sscanf((char *)dtmp, "!uri %127s", uri_temp) == 1) {
                            strncpy(mqtt_uri, uri_temp, sizeof(mqtt_uri) - 1);
                            mqtt_uri[sizeof(mqtt_uri) - 1] = '\0';
                            ESP_LOGI(TAG, "Nuevo URI MQTT -> %s", mqtt_uri);
                        } else {
                            ESP_LOGW(TAG, "Formato incorrecto para !uri. Uso: !uri <uri>");
                        }
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
    // free(dtmp); // Inalcanzable en un for(;;)
    // vTaskDelete(NULL); // Inalcanzable en un for(;;)
}

// Función de inicialización del UART de comandos
void leido_uart_init(SemaphoreHandle_t sync_semaphore) // ¡NUEVA FIRMA!
{
    // Almacenar el handle del semáforo para que la tarea uart_event_task pueda usarlo
    uart_sync_semaphore = sync_semaphore;

    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    ESP_ERROR_CHECK(uart_driver_install(EX_UART_NUM, BUF_SIZE * 2, BUF_SIZE * 2, 20, &uart0_queue, 0));
    ESP_ERROR_CHECK(uart_param_config(EX_UART_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(EX_UART_NUM, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    xTaskCreate(uart_event_task, "uart_event_task", 4096, NULL, 10, NULL);
    ESP_LOGI(TAG, "UART inicializado. Envía comandos: !wifi <ssid> <pass>, !topic <topico>. Finaliza con !done.");
}