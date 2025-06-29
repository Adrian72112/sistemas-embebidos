/*
 * Ejemplo de uso del Logger de Eventos de Reproducción con Ring Buffer
 * 
 * Este ejemplo demuestra cómo usar el logger para almacenar eventos
 * de reproducción en un buffer circular de 20 espacios con persistencia en NVS.
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
    ESP_LOGI(TAG, "🚀 Logger Ring Buffer Demo Started");
    
    // Inicializar el logger
    esp_err_t ret = logger_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize logger: %s", esp_err_to_name(ret));
        vTaskDelete(NULL);
        return;
    }
    
    ESP_LOGI(TAG, "✅ Logger initialized successfully");
    
    // Mostrar información inicial del logger
    logger_print_info();
    
    // Mostrar historial de eventos existentes (de sesiones anteriores)
    ESP_LOGI(TAG, "📋 Showing existing event history from previous sessions:");
    logger_print_event_history();
    
    // Simular eventos de reproducción
    ESP_LOGI(TAG, "🎵 Simulating playback events...");
    
    // Evento PLAY
    ESP_LOGI(TAG, "▶️ Playing music...");
    logger_log_event(LOGGER_EVENT_PLAY);
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    // Varios eventos NEXT
    ESP_LOGI(TAG, "⏭️ Skipping tracks...");
    for (int i = 0; i < 3; i++) {
        logger_log_event(LOGGER_EVENT_NEXT);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
    
    // Evento PAUSE
    ESP_LOGI(TAG, "⏸️ Pausing music...");
    logger_log_event(LOGGER_EVENT_PAUSE);
    vTaskDelay(pdMS_TO_TICKS(800));
    
    // Evento PLAY (resume)
    ESP_LOGI(TAG, "▶️ Resuming music...");
    logger_log_event(LOGGER_EVENT_PLAY);
    vTaskDelay(pdMS_TO_TICKS(600));
    
    // Evento PREVIOUS
    ESP_LOGI(TAG, "⏮️ Previous track...");
    logger_log_event(LOGGER_EVENT_PREVIOUS);
    vTaskDelay(pdMS_TO_TICKS(400));
    
    // Evento STOP
    ESP_LOGI(TAG, "⏹️ Stopping music...");
    logger_log_event(LOGGER_EVENT_STOP);
    vTaskDelay(pdMS_TO_TICKS(200));
    
    // Mostrar todos los eventos del ring buffer
    ESP_LOGI(TAG, "📋 Current events in ring buffer:");
    logger_print_event_history();
    
    // Obtener un evento específico por índice
    logger_event_t specific_event;
    ret = logger_get_event_by_index(0, &specific_event); // Obtener el evento más antiguo
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "🔍 Oldest event: %s (seq: %" PRIu32 ", time: %" PRIu64 " μs)", 
                 logger_event_type_to_string(specific_event.type),
                 specific_event.sequence_number,
                 specific_event.timestamp);
    } else {
        ESP_LOGW(TAG, "No events found at index 0");
    }
    
    // Obtener todo el ring buffer usando la API
    logger_ring_buffer_t ring_buffer_copy;
    ret = logger_get_ring_buffer(&ring_buffer_copy);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "📊 Ring buffer info via API:");
        ESP_LOGI(TAG, "  - Count: %d", ring_buffer_copy.count);
        ESP_LOGI(TAG, "  - Head: %d", ring_buffer_copy.head);
        ESP_LOGI(TAG, "  - Total events: %" PRIu32, ring_buffer_copy.total_events);
    }
    
    // Llenar el buffer para probar el comportamiento circular
    ESP_LOGI(TAG, "🔄 Testing circular buffer behavior...");
    ESP_LOGI(TAG, "Adding 25 events to test circular buffer (capacity: %d)", LOGGER_RING_BUFFER_SIZE);
    
    for (int i = 0; i < 25; i++) {
        logger_event_type_t event_type = (logger_event_type_t)(i % 5); // Rotar entre tipos
        ESP_LOGI(TAG, "Adding event %d: %s", i+1, logger_event_type_to_string(event_type));
        logger_log_event(event_type);
        vTaskDelay(pdMS_TO_TICKS(100));
        
        // Mostrar información cada 5 eventos
        if ((i + 1) % 5 == 0) {
            logger_print_info();
        }
    }
    
    // Mostrar eventos después de llenar el buffer circular
    ESP_LOGI(TAG, "📋 Events after filling circular buffer (should show only last %d):", LOGGER_RING_BUFFER_SIZE);
    logger_print_event_history();
    
    // Test de persistencia manual
    ESP_LOGI(TAG, "💾 Testing manual NVS persistence...");
    ret = logger_save_ring_buffer_to_nvs();
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "✅ Ring buffer saved to NVS manually");
    } else {
        ESP_LOGW(TAG, "❌ Failed to save ring buffer to NVS: %s", esp_err_to_name(ret));
    }
    
    ESP_LOGI(TAG, "📊 Final logger statistics:");
    logger_print_info();
    
    // Deinicializar el logger (guarda automáticamente en NVS)
    ESP_LOGI(TAG, "🔚 Deinitializing logger (auto-save to NVS)...");
    logger_deinit();
    
    ESP_LOGI(TAG, "✅ Logger ring buffer demo completed successfully!");
    ESP_LOGI(TAG, "💡 Restart the system to see persistence in action!");
    ESP_LOGI(TAG, "🔄 The ring buffer will retain the last %d events across reboots", LOGGER_RING_BUFFER_SIZE);
    
    vTaskDelete(NULL);
}

void app_main(void)
{
    printf("\n");
    printf("=========================================\n");
    printf("   ESP32-S2 Logger Ring Buffer Demo\n");
    printf("   Circular Buffer (20) + NVS Storage\n");
    printf("=========================================\n");
    printf("\n");
    
    // Crear tarea de demostración
    xTaskCreate(logger_demo_task, "logger_demo", 8192, NULL, 5, NULL);
    
    // Mantener viva la aplicación
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
    // Mantener el main corriendo para las tareas
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10000)); // Sleep por 10 segundos
    }
}
