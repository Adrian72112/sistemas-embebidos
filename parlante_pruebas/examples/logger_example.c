/*
 * Ejemplo de uso del Logger de Eventos de Reproducción
 * 
 * Este ejemplo demuestra cómo usar el logger para almacenar eventos
 * de reproducción en un buffer circular con persistencia en NVS.
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "logger.h"

static const char *TAG = "LOGGER_EXAMPLE";

void logger_demo_task(void *args)
{
    ESP_LOGI(TAG, "🚀 Logger Demo Started");
    
    // Inicializar el logger
    esp_err_t ret = logger_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize logger: %s", esp_err_to_name(ret));
        vTaskDelete(NULL);
        return;
    }
    
    ESP_LOGI(TAG, "✅ Logger initialized successfully");
    
    // Mostrar eventos existentes (de sesiones anteriores)
    ESP_LOGI(TAG, "📋 Showing existing events from previous sessions:");
    logger_print_events();
    
    // Simular eventos de reproducción
    ESP_LOGI(TAG, "🎵 Simulating playback events...");
    
    // Evento PLAY
    logger_log_event(LOGGER_EVENT_PLAY);
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    // Varios eventos NEXT
    for (int i = 0; i < 3; i++) {
        logger_log_event(LOGGER_EVENT_NEXT);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
    
    // Evento PAUSE
    logger_log_event(LOGGER_EVENT_PAUSE);
    vTaskDelay(pdMS_TO_TICKS(800));
    
    // Evento PLAY (resume)
    logger_log_event(LOGGER_EVENT_PLAY);
    vTaskDelay(pdMS_TO_TICKS(600));
    
    // Evento PREVIOUS
    logger_log_event(LOGGER_EVENT_PREVIOUS);
    vTaskDelay(pdMS_TO_TICKS(400));
    
    // Evento STOP
    logger_log_event(LOGGER_EVENT_STOP);
    vTaskDelay(pdMS_TO_TICKS(200));
    
    // Mostrar todos los eventos
    ESP_LOGI(TAG, "📋 Current events in buffer:");
    logger_print_events();
    
    // Obtener el último evento
    logger_event_t last_event;
    ret = logger_get_last_event(&last_event);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "🔍 Last event: %s (seq: %lu, time: %llu us)", 
                 logger_event_type_to_string(last_event.type),
                 last_event.sequence_number,
                 last_event.timestamp);
    } else {
        ESP_LOGW(TAG, "No events found");
    }
    
    // Obtener todos los eventos usando la API
    logger_event_t events[LOGGER_BUFFER_SIZE];
    size_t event_count;
    ret = logger_get_events(events, LOGGER_BUFFER_SIZE, &event_count);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "📊 Retrieved %zu events via API:", event_count);
        for (size_t i = 0; i < event_count; i++) {
            printf("  Event %zu: %s (seq: %lu)\n", 
                   i + 1,
                   logger_event_type_to_string(events[i].type),
                   events[i].sequence_number);
        }
    }
    
    // Llenar el buffer para probar el comportamiento circular
    ESP_LOGI(TAG, "🔄 Testing circular buffer behavior...");
    ESP_LOGI(TAG, "Adding 25 events to test circular buffer (capacity: %d)", LOGGER_BUFFER_SIZE);
    
    for (int i = 0; i < 25; i++) {
        logger_event_type_t event_type = (logger_event_type_t)(i % 5); // Rotar entre tipos
        logger_log_event(event_type);
        vTaskDelay(pdMS_TO_TICKS(50));
    }
    
    // Mostrar eventos después de llenar el buffer
    ESP_LOGI(TAG, "📋 Events after filling circular buffer:");
    logger_print_events();
    
    // Test de persistencia - los datos deberían persistir después de reinicio
    ESP_LOGI(TAG, "💾 Testing NVS persistence...");
    ESP_LOGI(TAG, "Data is automatically saved to NVS. After system restart, events will persist.");
    
    // Ejemplo de limpieza del buffer (descomentrar si quieres probar)
    // ESP_LOGI(TAG, "🧹 Clearing all events...");
    // logger_clear_events();
    // logger_print_events();
    
    // Deinicializar el logger (guarda automáticamente en NVS)
    ESP_LOGI(TAG, "🔚 Deinitializing logger...");
    logger_deinit();
    
    ESP_LOGI(TAG, "✅ Logger demo completed successfully!");
    ESP_LOGI(TAG, "💡 Restart the system to see persistence in action!");
    
    vTaskDelete(NULL);
}

void app_main(void)
{
    printf("\n");
    printf("=====================================\n");
    printf("   ESP32-S2 Logger Demo\n");
    printf("   Buffer Circular + NVS Storage\n");
    printf("=====================================\n");
    printf("\n");
    
    // Crear tarea de demostración
    xTaskCreate(logger_demo_task, "logger_demo", 4096, NULL, 5, NULL);
    
    // Mantener el main corriendo para las tareas
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10000)); // Sleep por 10 segundos
    }
}
