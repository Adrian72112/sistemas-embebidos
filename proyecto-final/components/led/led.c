#include "led.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"

static const char *TAG = "LED_CONTROLLER";

// Variables globales para la máquina de estados
static led_strip_t *g_strip = NULL;
static TaskHandle_t g_led_task_handle = NULL;
static QueueHandle_t g_led_state_queue = NULL;
static led_state_t g_current_state = LED_STATE_OFF;

// Prototipo de la tarea
static void led_controller_task(void *pvParameters);
esp_err_t led_init(led_strip_t **strip)
{
    return led_rgb_init(strip); // usa el init que nos dio el profe
}

void led_set_color(led_strip_t *strip, uint8_t r, uint8_t g, uint8_t b)
{
    strip->set_pixel(strip, 0, r, g, b);
    strip->refresh(strip, 100);
}

void led_off(led_strip_t *strip)
{
    strip->clear(strip, 100);
}

void led_blink_colors_loop(led_strip_t *strip)
{
    while (1)
    {
        led_set_color(strip, 255, 0, 0); // rojo
        vTaskDelay(500);
        led_off(strip);
        vTaskDelay(500);

        led_set_color(strip, 0, 255, 0); // verde
        vTaskDelay(500);
        led_off(strip);
        vTaskDelay(500);

        led_set_color(strip, 0, 0, 255); // azul
        vTaskDelay(500);
        led_off(strip);
        vTaskDelay(500);
    }
}

// Implementación de la máquina de estados del LED
esp_err_t led_controller_init(led_strip_t *strip)
{
    if (strip == NULL) {
        ESP_LOGE(TAG, "LED strip pointer is null");
        return ESP_ERR_INVALID_ARG;
    }
    
    if (g_led_task_handle != NULL) {
        ESP_LOGW(TAG, "LED controller already initialized");
        return ESP_OK;
    }
    
    g_strip = strip;
    
    // Crear cola para comandos de cambio de estado
    g_led_state_queue = xQueueCreate(10, sizeof(led_state_t));
    if (g_led_state_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create LED state queue");
        return ESP_ERR_NO_MEM;
    }
    
    // Crear tarea del controlador LED
    BaseType_t result = xTaskCreate(
        led_controller_task,
        "led_controller",
        2048,  // Stack size
        NULL,
        5,     // Priority
        &g_led_task_handle
    );
    
    if (result != pdPASS) {
        ESP_LOGE(TAG, "Failed to create LED controller task");
        vQueueDelete(g_led_state_queue);
        g_led_state_queue = NULL;
        return ESP_ERR_NO_MEM;
    }
    
    ESP_LOGI(TAG, "🚨 LED controller initialized successfully");
    return ESP_OK;
}

void led_set_state(led_state_t new_state)
{
    if (g_led_state_queue == NULL) {
        ESP_LOGW(TAG, "LED controller not initialized");
        return;
    }
    
    if (xQueueSend(g_led_state_queue, &new_state, pdMS_TO_TICKS(100)) != pdTRUE) {
        ESP_LOGW(TAG, "Failed to send new LED state");
    } else {
        ESP_LOGD(TAG, "LED state changed to: %d", new_state);
    }
}

led_state_t led_get_state(void)
{
    return g_current_state;
}

void led_controller_deinit(void)
{
    if (g_led_task_handle != NULL) {
        vTaskDelete(g_led_task_handle);
        g_led_task_handle = NULL;
    }
    
    if (g_led_state_queue != NULL) {
        vQueueDelete(g_led_state_queue);
        g_led_state_queue = NULL;
    }
    
    // Apagar LED al finalizar
    if (g_strip != NULL) {
        led_off(g_strip);
        g_strip = NULL;
    }
    
    g_current_state = LED_STATE_OFF;
    ESP_LOGI(TAG, "LED controller deinitialized");
}

// Tarea principal del controlador LED
static void led_controller_task(void *pvParameters)
{
    led_state_t new_state;
    bool led_on = false;
    TickType_t last_blink = 0;
    const TickType_t blink_interval = pdMS_TO_TICKS(500); // 500ms
    led_state_t previous_state = LED_STATE_OFF;
    bool state_changed = true;  // Para controlar acciones una sola vez
    
    ESP_LOGI(TAG, "🚨 LED controller task started");
    
    while (1) {
        // Verificar si hay cambio de estado en la cola
        if (xQueueReceive(g_led_state_queue, &new_state, pdMS_TO_TICKS(50)) == pdTRUE) {
            if (g_current_state != new_state) {
                previous_state = g_current_state;
                g_current_state = new_state;
                state_changed = true;
                ESP_LOGI(TAG, "🎯 LED state changed from %d to %d", previous_state, g_current_state);
                
                // Reset del parpadeo al cambiar estado
                led_on = false;
                last_blink = xTaskGetTickCount();
            }
        }
        
        // Ejecutar acción según el estado actual
        switch (g_current_state) {
            case LED_STATE_OFF:
            case LED_STATE_PAUSE:
                // Solo apagar una vez cuando cambia al estado OFF/PAUSE
                if (state_changed) {
                    led_off(g_strip);
                    state_changed = false;
                    ESP_LOGD(TAG, "LED turned OFF (state: %d)", g_current_state);
                }
                break;
                
            case LED_STATE_PLAY:
                // Parpadeo azul para PLAY
                if ((xTaskGetTickCount() - last_blink) >= blink_interval) {
                    if (led_on) {
                        led_off(g_strip);
                        led_on = false;
                    } else {
                        led_set_color(g_strip, 0, 0, 255); // azul
                        led_on = true;
                    }
                    last_blink = xTaskGetTickCount();
                }
                state_changed = false;  // Reset flag después del primer ciclo
                break;
                
            case LED_STATE_NEXT:
            case LED_STATE_PREVIOUS:
                // Azul sólido para cambios de pista - solo una vez
                if (state_changed) {
                    led_set_color(g_strip, 0, 0, 255); // azul
                    state_changed = false;
                    ESP_LOGD(TAG, "LED set to solid blue (state: %d)", g_current_state);
                }
                break;
                
            case LED_STATE_ERROR:
                // Parpadeo rojo para errores
                if ((xTaskGetTickCount() - last_blink) >= blink_interval) {
                    if (led_on) {
                        led_off(g_strip);
                        led_on = false;
                    } else {
                        led_set_color(g_strip, 255, 0, 0); // rojo
                        led_on = true;
                    }
                    last_blink = xTaskGetTickCount();
                }
                state_changed = false;  // Reset flag después del primer ciclo
                break;
                
            default:
                if (state_changed) {
                    ESP_LOGW(TAG, "Unknown LED state: %d", g_current_state);
                    led_off(g_strip);
                    state_changed = false;
                }
                break;
        }
        
        // Delay más largo para estados estáticos, más corto para parpadeos
        if (g_current_state == LED_STATE_PLAY || g_current_state == LED_STATE_ERROR) {
            vTaskDelay(pdMS_TO_TICKS(10));  // Delay corto para parpadeos
        } else {
            vTaskDelay(pdMS_TO_TICKS(100)); // Delay más largo para estados estáticos
        }
    }
}
