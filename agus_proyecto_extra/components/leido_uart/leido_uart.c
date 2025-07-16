#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"   // Semáforos FreeRTOS
#include "driver/uart.h"
#include "esp_log.h"
#include "leido_uart.h"

#define EX_UART_NUM UART_NUM_0
#define BUF_SIZE (1024)
#define RD_BUF_SIZE (BUF_SIZE)

static QueueHandle_t uart0_queue;
static const char *TAG = "leido_uart";

// Variables globales accesibles desde otros archivos (extern en leido_uart.h)
char wifi_ssid[64] = "ssid_por_defecto";
char wifi_pass[64] = "pass_por_defecto";
char mqtt_topic[64] = "/topico/por/defecto";
char mqtt_uri[128] = "mqtt://broker.hivemq.com:1883";

// Puntero al semáforo recibido en leido_uart_init; permite desbloquear app_main cuando se recibe !done
static SemaphoreHandle_t uart_sync_semaphore = NULL;

// Tarea principal que gestiona los eventos UART
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

                // Evento: se recibió un dato por UART
                case UART_DATA:
                    uart_read_bytes(EX_UART_NUM, dtmp, event.size, portMAX_DELAY);
                    dtmp[event.size] = '\0';
                    ESP_LOGI(TAG, "Recibido: %s", dtmp);

                    // Comando: !wifi ssid password
                    if (strncmp((char *)dtmp, "!wifi", 5) == 0) {
                        char *ssid = strtok((char *)dtmp + 6, " ");
                        char *pass = strtok(NULL, " ");
                        if (ssid && pass) {
                            strncpy(wifi_ssid, ssid, sizeof(wifi_ssid));
                            strncpy(wifi_pass, pass, sizeof(wifi_pass));
                            ESP_LOGI(TAG, "SSID configurado: %s", wifi_ssid);
                            ESP_LOGI(TAG, "Password configurada: %s", wifi_pass);
                        } else {
                            ESP_LOGE(TAG, "Formato incorrecto. Uso: !wifi ssid password");
                        }

                    // Comando: !topic <tópico>
                    } else if (strncmp((char *)dtmp, "!topic", 6) == 0) {
                        char topic_temp[64];
                        if (sscanf((char *)dtmp, "!topic %63s", topic_temp) == 1) {
                            strncpy(mqtt_topic, topic_temp, sizeof(mqtt_topic) - 1);
                            mqtt_topic[sizeof(mqtt_topic) - 1] = '\0';
                            ESP_LOGI(TAG, "Nuevo tópico -> %s", mqtt_topic);
                        } else {
                            ESP_LOGW(TAG, "Formato incorrecto para !topic. Uso: !topic <topico>");
                        }

                    // Comando: !uri <mqtt://broker>
                    } else if (strncmp((char *)dtmp, "!uri", 4) == 0) {
                        char uri_temp[128];
                        if (sscanf((char *)dtmp, "!uri %127s", uri_temp) == 1) {
                            strncpy(mqtt_uri, uri_temp, sizeof(mqtt_uri) - 1);
                            mqtt_uri[sizeof(mqtt_uri) - 1] = '\0';
                            ESP_LOGI(TAG, "Nuevo URI MQTT -> %s", mqtt_uri);
                        } else {
                            ESP_LOGW(TAG, "Formato incorrecto para !uri. Uso: !uri <uri>");
                        }

                    // Comando: !done
                    } else if (strncmp((char *)dtmp, "!done", 5) == 0) {
                        ESP_LOGI(TAG, "Comando !done recibido. Liberando semáforo para continuar.");
                        if (uart_sync_semaphore != NULL) {
                            xSemaphoreGive(uart_sync_semaphore); // Desbloquea app_main
                        }

                    // Comando desconocido
                    } else {
                        ESP_LOGW(TAG, "Comando desconocido: %s", dtmp);
                    }
                    break;

                // Evento: desbordamiento FIFO UART
                case UART_FIFO_OVF:
                    ESP_LOGW(TAG, "UART FIFO Overflow. Vaciando buffer de entrada.");
                    uart_flush_input(EX_UART_NUM);
                    xQueueReset(uart0_queue);
                    break;

                // Evento: buffer UART lleno
                case UART_BUFFER_FULL:
                    ESP_LOGW(TAG, "UART Buffer Lleno. Vaciando buffer de entrada.");
                    uart_flush_input(EX_UART_NUM);
                    xQueueReset(uart0_queue);
                    break;

                // Evento: patrón detectado (no usado)
                case UART_PATTERN_DET:
                    ESP_LOGI(TAG, "Patrón UART detectado.");
                    break;

                // Evento: break
                case UART_BREAK:
                    ESP_LOGI(TAG, "Break UART detectado.");
                    break;

                // Evento: error de paridad
                case UART_PARITY_ERR:
                    ESP_LOGE(TAG, "Error de paridad UART.");
                    break;

                // Evento: error de trama
                case UART_FRAME_ERR:
                    ESP_LOGE(TAG, "Error de trama UART.");
                    break;

                // Evento no manejado
                default:
                    ESP_LOGW(TAG, "Evento UART no manejado: %d", event.type);
                    break;
            }
        }
    }
}

// Inicializa UART para leer comandos desde consola serial
void leido_uart_init(SemaphoreHandle_t sync_semaphore)
{
    // Guardamos el semáforo para que pueda ser liberado desde uart_event_task
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

    // Crear la tarea para manejar los eventos UART
    xTaskCreate(uart_event_task, "uart_event_task", 4096, NULL, 10, NULL);

    // Mensaje de bienvenida
    ESP_LOGI(TAG, "UART inicializado. Envía comandos: !wifi <ssid> <pass>, !topic <topico>, !uri <uri>. Finaliza con !done.");
}
