
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "leido_uart.h"

#define EX_UART_NUM UART_NUM_0
#define BUF_SIZE (1024)
#define RD_BUF_SIZE (BUF_SIZE)

static QueueHandle_t uart0_queue;
static const char *TAG = "leido_uart";

// Variables globales con valores por defecto más claros
char wifi_ssid[64] = "";  // Iniciamos vacío para detectar si no se configuró
char wifi_pass[64] = "";  // Iniciamos vacío para detectar si no se configuró
char mqtt_topic[64] = "/test/topic";  // Tópico por defecto más específico
char mqtt_uri[128] = "mqtt://broker.hivemq.com:1883"; // Declaración global de mqtt_uri

// Puntero global al semáforo para que la tarea de eventos UART pueda acceder a él.
// Será inicializado en uart_cmd_init
static SemaphoreHandle_t uart_sync_semaphore = NULL;

static void uart_event_task(void *pvParameters)
{
    uart_event_t event;
    uint8_t* dtmp = (uint8_t*) malloc(RD_BUF_SIZE);
    if (dtmp == NULL) {
        ESP_LOGE(TAG, "No se pudo asignar memoria para el búfer UART. La tarea de UART finalizará.");
        // Se ha ELIMINADO la declaración redundante de mqtt_uri aquí.
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
                    
                    // Limpiar caracteres de control (CR, LF, etc.)
                    for (int i = 0; i < event.size; i++) {
                        if (dtmp[i] == '\r' || dtmp[i] == '\n') {
                            dtmp[i] = '\0';
                            break;
                        }
                    }
                    
                    ESP_LOGI(TAG, "Comando recibido: '%s' (longitud: %d)", dtmp, strlen((char*)dtmp));

                    if (strncmp((char *)dtmp, "!wifi", 5) == 0) {
                        char ssid_temp[64], pass_temp[64];
                        int parsed = sscanf((char *)dtmp, "!wifi %63s %63s", ssid_temp, pass_temp);
                        
                        if (parsed == 2) {
                            // Validar que SSID y password no estén vacíos
                            if (strlen(ssid_temp) > 0 && strlen(pass_temp) > 0) {
                                strncpy(wifi_ssid, ssid_temp, sizeof(wifi_ssid) - 1);
                                wifi_ssid[sizeof(wifi_ssid) - 1] = '\0';
                                strncpy(wifi_pass, pass_temp, sizeof(wifi_pass) - 1);
                                wifi_pass[sizeof(wifi_pass) - 1] = '\0';
                                ESP_LOGI(TAG, "✓ WiFi configurado -> SSID: '%s', Password: [%d caracteres]", 
                                        wifi_ssid, strlen(wifi_pass));
                            } else {
                                ESP_LOGW(TAG, "✗ SSID o password vacíos");
                            }
                        } else {
                            ESP_LOGW(TAG, "✗ Formato incorrecto para !wifi. Uso: !wifi <ssid> <pass>");
                        }
                    } else if (strncmp((char *)dtmp, "!topic", 6) == 0) {
                        char topic_temp[64];
                        int parsed = sscanf((char *)dtmp, "!topic %63s", topic_temp);
                        
                        if (parsed == 1) {
                            if (strlen(topic_temp) > 0) {
                                strncpy(mqtt_topic, topic_temp, sizeof(mqtt_topic) - 1);
                                mqtt_topic[sizeof(mqtt_topic) - 1] = '\0';
                                ESP_LOGI(TAG, "✓ Tópico MQTT configurado -> '%s'", mqtt_topic);
                            } else {
                                ESP_LOGW(TAG, "✗ Tópico vacío");
                            }
                        } else {
                            ESP_LOGW(TAG, "✗ Formato incorrecto para !topic. Uso: !topic <topico>");
                        }
                    } else if (strncmp((char *)dtmp, "!uri", 4) == 0) {
                        char uri_temp[128];
                        int parsed = sscanf((char *)dtmp, "!uri %127s", uri_temp);
                        
                        if (parsed == 1) {
                            if (strlen(uri_temp) > 0) {
                                strncpy(mqtt_uri, uri_temp, sizeof(mqtt_uri) - 1);
                                mqtt_uri[sizeof(mqtt_uri) - 1] = '\0';
                                ESP_LOGI(TAG, "✓ URI MQTT configurado -> '%s'", mqtt_uri);
                            } else {
                                ESP_LOGW(TAG, "✗ URI vacío");
                            }
                        } else {
                            ESP_LOGW(TAG, "✗ Formato incorrecto para !uri. Uso: !uri <uri>");
                        }
                    } else if (strncmp((char *)dtmp, "!done", 5) == 0) {
                        ESP_LOGI(TAG, "=== COMANDO !done RECIBIDO ===");
                        
                        // Validar que tenemos la configuración mínima necesaria
                        bool config_valid = true;
                        
                        if (strlen(wifi_ssid) == 0) {
                            ESP_LOGE(TAG, "✗ ERROR: SSID WiFi no configurado");
                            config_valid = false;
                        }
                        
                        if (strlen(wifi_pass) == 0) {
                            ESP_LOGE(TAG, "✗ ERROR: Password WiFi no configurado");
                            config_valid = false;
                        }
                        
                        if (strlen(mqtt_topic) == 0) {
                            ESP_LOGE(TAG, "✗ ERROR: Tópico MQTT no configurado");
                            config_valid = false;
                        }
                        
                        if (config_valid) {
                            ESP_LOGI(TAG, "✓ Configuración completa y válida:");
                            ESP_LOGI(TAG, "  - WiFi SSID: '%s'", wifi_ssid);
                            ESP_LOGI(TAG, "  - WiFi Pass: [%d caracteres]", strlen(wifi_pass));
                            ESP_LOGI(TAG, "  - MQTT Topic: '%s'", mqtt_topic);
                            ESP_LOGI(TAG, "  - MQTT URI: '%s'", mqtt_uri);
                            ESP_LOGI(TAG, "Liberando semáforo para continuar...");
                            
                            if (uart_sync_semaphore != NULL) {
                                xSemaphoreGive(uart_sync_semaphore); // Liberar el semáforo para desbloquear app_main
                            }
                        } else {
                            ESP_LOGE(TAG, "✗ Configuración incompleta. Configure todos los parámetros antes de usar !done");
                        }
                    } else if (strncmp((char *)dtmp, "!help", 5) == 0) {
                        ESP_LOGI(TAG, "=== COMANDOS DISPONIBLES ===");
                        ESP_LOGI(TAG, "!wifi <ssid> <password> - Configurar red WiFi");
                        ESP_LOGI(TAG, "!topic <topico>        - Configurar tópico MQTT");
                        ESP_LOGI(TAG, "!uri <uri>             - Configurar URI broker MQTT (opcional)");
                        ESP_LOGI(TAG, "!done                  - Finalizar configuración e iniciar");
                        ESP_LOGI(TAG, "!help                  - Mostrar esta ayuda");
                    } else { // Este 'else' ahora captura todos los comandos desconocidos
                        ESP_LOGW(TAG, "✗ Comando desconocido: '%s'. Envía !help para ver comandos disponibles", dtmp);
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
void leido_uart_init(SemaphoreHandle_t sync_semaphore)
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
    ESP_LOGI(TAG, "=== UART INICIALIZADO ===");
    ESP_LOGI(TAG, "Envía comandos:");
    ESP_LOGI(TAG, "  !wifi <ssid> <password>");
    ESP_LOGI(TAG, "  !topic <topico>");
    ESP_LOGI(TAG, "  !uri <uri_broker> (opcional)");
    ESP_LOGI(TAG, "  !done");
    ESP_LOGI(TAG, "  !help (para ayuda)");
}