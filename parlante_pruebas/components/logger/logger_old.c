#include "logger.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_timer.h"
#include "esp_task_wdt.h"
#include "freertos/FreeRTOS.h"
#include "freertos/ringbuf.h"
#include "freertos/semphr.h"
#include <stdio.h>
#include <inttypes.h>
#include <string.h>

static const char *TAG = "LOGGER";

// Variables globales
static bool g_logger_initialized = false;
static logger_ring_buffer_t g_ring_buffer;
static SemaphoreHandle_t g_ring_buffer_mutex;

// Funciones auxiliares privadas - declaraciones
static void logger_ring_buffer_init(void);
static esp_err_t logger_ring_buffer_add_event(logger_event_type_t event_type);
static esp_err_t logger_save_ring_buffer_blob_to_nvs(void);
static esp_err_t logger_load_ring_buffer_blob_from_nvs(void);

// Implementación de función auxiliar para guardar (necesaria antes del shutdown handler)
static esp_err_t logger_save_ring_buffer_blob_to_nvs(void)
{
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(LOGGER_NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error opening NVS handle for ring buffer save: %s", esp_err_to_name(err));
        return err;
    }

    if (g_ring_buffer_mutex && xSemaphoreTake(g_ring_buffer_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        ESP_LOGE(TAG, "Failed to acquire mutex for ring buffer save");
        nvs_close(nvs_handle);
        return ESP_ERR_TIMEOUT;
    }

    ESP_LOGI(TAG, "Saving ring buffer to NVS...");
    ESP_LOGI(TAG, "  - Ring buffer size: %zu bytes", sizeof(logger_ring_buffer_t));
    ESP_LOGI(TAG, "  - Events count: %d", g_ring_buffer.count);
    ESP_LOGI(TAG, "  - Head index: %d", g_ring_buffer.head);
    ESP_LOGI(TAG, "  - Total events: %" PRIu32, g_ring_buffer.total_events);

    // Guardar ring buffer como blob
    err = nvs_set_blob(nvs_handle, LOGGER_NVS_KEY_RING_BUFFER, &g_ring_buffer, sizeof(logger_ring_buffer_t));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error saving ring buffer blob: %s", esp_err_to_name(err));
        if (g_ring_buffer_mutex) xSemaphoreGive(g_ring_buffer_mutex);
        nvs_close(nvs_handle);
        return err;
    }

    ESP_LOGI(TAG, "Ring buffer blob set successfully, committing...");
    err = nvs_commit(nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error committing ring buffer blob: %s", esp_err_to_name(err));
    } else {
        ESP_LOGI(TAG, "Ring buffer saved and committed to NVS successfully!");
    }

    if (g_ring_buffer_mutex) xSemaphoreGive(g_ring_buffer_mutex);
    nvs_close(nvs_handle);
    return err;
}

// Shutdown handler para guardar automáticamente antes de reset
static void logger_shutdown_handler(void)
{
    if (g_logger_initialized) {
        ESP_LOGI(TAG, "SHUTDOWN: Auto-saving ring buffer to NVS...");
        esp_err_t err = logger_save_ring_buffer_blob_to_nvs();
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "SHUTDOWN: Failed to save ring buffer: %s", esp_err_to_name(err));
        }
    }
}

esp_err_t logger_init(void)
{
    if (g_logger_initialized) {
        ESP_LOGW(TAG, "Logger already initialized");
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Initializing logger with ring buffer...");

    // Create mutex for ring buffer protection
    g_ring_buffer_mutex = xSemaphoreCreateMutex();
    if (g_ring_buffer_mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create ring buffer mutex");
        return ESP_ERR_NO_MEM;
    }

    // Initialize NVS
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_LOGW(TAG, "NVS partition needs to be erased");
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize NVS: %s", esp_err_to_name(err));
        vSemaphoreDelete(g_ring_buffer_mutex);
        return err;
    }

    // Open NVS
    ESP_LOGI(TAG, "Opening Non-Volatile Storage (NVS) handle...");
    nvs_handle_t nvs_handle;
    err = nvs_open(LOGGER_NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
        vSemaphoreDelete(g_ring_buffer_mutex);
        return err;
    }

    ESP_LOGI(TAG, "NVS handle opened successfully");

    // Close NVS handle for now
    nvs_close(nvs_handle);

    // Load ring buffer from NVS
    ESP_LOGI(TAG, "Loading ring buffer from NVS...");
    err = logger_load_ring_buffer_blob_from_nvs();
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Failed to load ring buffer, starting fresh");
        logger_ring_buffer_init();
    }

    // Register shutdown handler to auto-save on reset
    esp_register_shutdown_handler(&logger_shutdown_handler);
    ESP_LOGI(TAG, "Shutdown handler registered for auto-save");

    g_logger_initialized = true;
    ESP_LOGI(TAG, "Logger initialized successfully with ring buffer");
    
    return ESP_OK;
}

esp_err_t logger_deinit(void)
{
    if (!g_logger_initialized) {
        ESP_LOGW(TAG, "Logger deinit called but not initialized");
        return ESP_OK;
    }

    ESP_LOGI(TAG, "=== DEINITIALIZING LOGGER ===");
    ESP_LOGI(TAG, "Final ring buffer state before save:");
    ESP_LOGI(TAG, "  - Count: %d", g_ring_buffer.count);
    ESP_LOGI(TAG, "  - Head: %d", g_ring_buffer.head);
    ESP_LOGI(TAG, "  - Total events: %" PRIu32, g_ring_buffer.total_events);

    // Save ring buffer to NVS before deinitializing
    ESP_LOGI(TAG, "Saving ring buffer to NVS during deinit...");
    esp_err_t err = logger_save_ring_buffer_blob_to_nvs();
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Failed to save ring buffer to NVS during deinit: %s", esp_err_to_name(err));
    } else {
        ESP_LOGI(TAG, "Ring buffer saved successfully during deinit");
    }

    // Clean up mutex
    if (g_ring_buffer_mutex != NULL) {
        vSemaphoreDelete(g_ring_buffer_mutex);
        g_ring_buffer_mutex = NULL;
        ESP_LOGI(TAG, "Ring buffer mutex deleted");
    }

    // Unregister shutdown handler
    esp_unregister_shutdown_handler(&logger_shutdown_handler);
    ESP_LOGI(TAG, "Shutdown handler unregistered");

    g_logger_initialized = false;
    ESP_LOGI(TAG, "=== LOGGER DEINITIALIZED ===");
    
    return ESP_OK;
}

esp_err_t logger_log_event(logger_event_type_t event_type)
{
    if (!g_logger_initialized) {
        ESP_LOGE(TAG, "Logger not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    if (event_type < LOGGER_EVENT_PLAY || event_type > LOGGER_EVENT_STOP) {
        ESP_LOGE(TAG, "Invalid event type: %d", event_type);
        return ESP_ERR_INVALID_ARG;
    }

    // Add event to ring buffer
    esp_err_t err = logger_ring_buffer_add_event(event_type);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add event to ring buffer: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "Event logged: %s (total events: %" PRIu32 ")", 
             logger_event_type_to_string(event_type), g_ring_buffer.total_events);

    // TEMPORARY DEBUG: Save immediately after each event to debug persistence
    ESP_LOGI(TAG, "DEBUG: Saving ring buffer to NVS after event for debugging...");
    err = logger_save_ring_buffer_blob_to_nvs();
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "DEBUG: Failed to save ring buffer to NVS: %s", esp_err_to_name(err));
    } else {
        ESP_LOGI(TAG, "DEBUG: Ring buffer saved successfully after event");
    }

    return ESP_OK;
}

uint32_t logger_get_event_count(void)
{
    if (!g_logger_initialized) {
        return 0;
    }
    return g_ring_buffer.total_events;
}

const char* logger_event_type_to_string(logger_event_type_t event_type)
{
    switch (event_type) {
        case LOGGER_EVENT_PLAY:     return "PLAY";
        case LOGGER_EVENT_PAUSE:    return "PAUSE";
        case LOGGER_EVENT_NEXT:     return "NEXT";
        case LOGGER_EVENT_PREVIOUS: return "PREVIOUS";
        case LOGGER_EVENT_STOP:     return "STOP";
        default:                    return "UNKNOWN";
    }
}

void logger_print_info(void)
{
    if (!g_logger_initialized) {
        printf("Logger not initialized\n");
        return;
    }

    printf("\n=== LOGGER INFO ===\n");
    printf("Initialized: %s\n", g_logger_initialized ? "YES" : "NO");
    printf("Ring buffer size: %d\n", LOGGER_RING_BUFFER_SIZE);
    
    if (xSemaphoreTake(g_ring_buffer_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        printf("Ring buffer count: %d\n", g_ring_buffer.count);
        printf("Ring buffer head: %d\n", g_ring_buffer.head);
        printf("Total events logged: %" PRIu32 "\n", g_ring_buffer.total_events);
        xSemaphoreGive(g_ring_buffer_mutex);
    } else {
        printf("Ring buffer: [LOCKED]\n");
    }
    
    printf("NVS Namespace: %s\n", LOGGER_NVS_NAMESPACE);
    printf("NVS Key Ring Buffer: %s\n", LOGGER_NVS_KEY_RING_BUFFER);
    printf("===================\n\n");
}

// Nuevas funciones públicas para manejo del ring buffer

esp_err_t logger_get_ring_buffer(logger_ring_buffer_t* buffer)
{
    if (!g_logger_initialized || buffer == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (xSemaphoreTake(g_ring_buffer_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        ESP_LOGE(TAG, "Failed to acquire mutex for ring buffer copy");
        return ESP_ERR_TIMEOUT;
    }

    memcpy(buffer, &g_ring_buffer, sizeof(logger_ring_buffer_t));
    xSemaphoreGive(g_ring_buffer_mutex);
    
    return ESP_OK;
}

void logger_print_event_history(void)
{
    if (!g_logger_initialized) {
        printf("Logger not initialized\n");
        return;
    }

    if (xSemaphoreTake(g_ring_buffer_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        printf("Failed to acquire ring buffer lock\n");
        return;
    }

    printf("\n=== EVENT HISTORY ===\n");
    printf("Ring buffer capacity: %d\n", LOGGER_RING_BUFFER_SIZE);
    printf("Current count: %d\n", g_ring_buffer.count);
    printf("Total events since init: %" PRIu32 "\n", g_ring_buffer.total_events);
    
    if (g_ring_buffer.count == 0) {
        printf("No events in history\n");
    } else {
        printf("\nEvents (oldest to newest):\n");
        printf("Index | Seq# | Event      | Timestamp (μs)\n");
        printf("------|------|------------|----------------\n");
        
        for (int i = 0; i < g_ring_buffer.count; i++) {
            int actual_index;
            if (g_ring_buffer.count < LOGGER_RING_BUFFER_SIZE) {
                // Buffer not full, events start from index 0
                actual_index = i;
            } else {
                // Buffer is full, start from head position (oldest)
                actual_index = (g_ring_buffer.head + i) % LOGGER_RING_BUFFER_SIZE;
            }
            
            logger_event_t* event = &g_ring_buffer.events[actual_index];
            printf("%5d | %4" PRIu32 " | %-10s | %16" PRIu64 "\n", 
                   i, 
                   event->sequence_number, 
                   logger_event_type_to_string(event->type),
                   event->timestamp);
        }
    }
    
    xSemaphoreGive(g_ring_buffer_mutex);
    printf("======================\n\n");
}

esp_err_t logger_get_event_by_index(uint8_t index, logger_event_t* event)
{
    if (!g_logger_initialized || event == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (xSemaphoreTake(g_ring_buffer_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        ESP_LOGE(TAG, "Failed to acquire mutex for event access");
        return ESP_ERR_TIMEOUT;
    }

    if (index >= g_ring_buffer.count) {
        xSemaphoreGive(g_ring_buffer_mutex);
        return ESP_ERR_NOT_FOUND;
    }

    int actual_index;
    if (g_ring_buffer.count < LOGGER_RING_BUFFER_SIZE) {
        // Buffer not full, events start from index 0
        actual_index = index;
    } else {
        // Buffer is full, start from head position (oldest)
        actual_index = (g_ring_buffer.head + index) % LOGGER_RING_BUFFER_SIZE;
    }

    memcpy(event, &g_ring_buffer.events[actual_index], sizeof(logger_event_t));
    xSemaphoreGive(g_ring_buffer_mutex);
    
    return ESP_OK;
}

esp_err_t logger_save_ring_buffer_to_nvs(void)
{
    if (!g_logger_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    return logger_save_ring_buffer_blob_to_nvs();
}

esp_err_t logger_load_ring_buffer_from_nvs(void)
{
    if (!g_logger_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    return logger_load_ring_buffer_blob_from_nvs();
}

// Implementaciones de funciones auxiliares privadas

static void logger_ring_buffer_init(void)
{
    memset(&g_ring_buffer, 0, sizeof(logger_ring_buffer_t));
    g_ring_buffer.head = 0;
    g_ring_buffer.count = 0;
    g_ring_buffer.total_events = 0;
}

static esp_err_t logger_ring_buffer_add_event(logger_event_type_t event_type)
{
    if (g_ring_buffer_mutex == NULL) {
        ESP_LOGE(TAG, "Ring buffer mutex not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    if (xSemaphoreTake(g_ring_buffer_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        ESP_LOGE(TAG, "Failed to acquire ring buffer mutex");
        return ESP_ERR_TIMEOUT;
    }

    // Crear nuevo evento
    logger_event_t new_event;
    new_event.type = event_type;
    new_event.timestamp = esp_timer_get_time();
    new_event.sequence_number = ++g_ring_buffer.total_events;

    // Agregar al ring buffer
    g_ring_buffer.events[g_ring_buffer.head] = new_event;
    g_ring_buffer.head = (g_ring_buffer.head + 1) % LOGGER_RING_BUFFER_SIZE;
    
    if (g_ring_buffer.count < LOGGER_RING_BUFFER_SIZE) {
        g_ring_buffer.count++;
    }

    xSemaphoreGive(g_ring_buffer_mutex);
    
    ESP_LOGI(TAG, "Ring buffer event added: %s (seq: %" PRIu32 ", count: %d)", 
             logger_event_type_to_string(event_type), 
             new_event.sequence_number, 
             g_ring_buffer.count);
    
    return ESP_OK;
}

static esp_err_t logger_load_ring_buffer_blob_from_nvs(void)
{
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(LOGGER_NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error opening NVS handle for ring buffer load: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "Attempting to load ring buffer from NVS...");
    
    size_t required_size = sizeof(logger_ring_buffer_t);
    ESP_LOGI(TAG, "Expected ring buffer size: %zu bytes", required_size);
    
    err = nvs_get_blob(nvs_handle, LOGGER_NVS_KEY_RING_BUFFER, &g_ring_buffer, &required_size);
    
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGI(TAG, "Ring buffer not found in NVS (first run), initializing empty");
        logger_ring_buffer_init();
        err = ESP_OK;
    } else if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error loading ring buffer from NVS: %s", esp_err_to_name(err));
        ESP_LOGI(TAG, "Initializing empty ring buffer due to load error");
        logger_ring_buffer_init();
    } else {
        ESP_LOGI(TAG, "Ring buffer loaded from NVS successfully!");
        ESP_LOGI(TAG, "  - Ring buffer size loaded: %zu bytes", required_size);
        ESP_LOGI(TAG, "  - Events count: %d", g_ring_buffer.count);
        ESP_LOGI(TAG, "  - Head index: %d", g_ring_buffer.head);
        ESP_LOGI(TAG, "  - Total events: %" PRIu32, g_ring_buffer.total_events);
        
        // Validar datos cargados
        if (g_ring_buffer.count > LOGGER_RING_BUFFER_SIZE) {
            ESP_LOGW(TAG, "Invalid count %d, resetting ring buffer", g_ring_buffer.count);
            logger_ring_buffer_init();
        } else if (g_ring_buffer.head >= LOGGER_RING_BUFFER_SIZE) {
            ESP_LOGW(TAG, "Invalid head %d, resetting ring buffer", g_ring_buffer.head);
            logger_ring_buffer_init();
        } else {
            ESP_LOGI(TAG, "Ring buffer data validation passed");
        }
    }

    nvs_close(nvs_handle);
    return err;
}

// Función de debug para ver qué hay en NVS
void logger_debug_nvs_info(void)
{
    if (!g_logger_initialized) {
        printf("Logger not initialized for NVS debug\n");
        return;
    }

    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(LOGGER_NVS_NAMESPACE, NVS_READONLY, &nvs_handle);
    if (err != ESP_OK) {
        printf("Failed to open NVS for debug: %s\n", esp_err_to_name(err));
        return;
    }

    printf("\n=== NVS DEBUG INFO ===\n");
    printf("Namespace: %s\n", LOGGER_NVS_NAMESPACE);
    
    // Check if ring buffer exists
    size_t required_size = 0;
    err = nvs_get_blob(nvs_handle, LOGGER_NVS_KEY_RING_BUFFER, NULL, &required_size);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        printf("Ring buffer key not found in NVS\n");
    } else if (err == ESP_OK) {
        printf("Ring buffer found in NVS:\n");
        printf("  - Key: %s\n", LOGGER_NVS_KEY_RING_BUFFER);
        printf("  - Size: %zu bytes\n", required_size);
        printf("  - Expected size: %zu bytes\n", sizeof(logger_ring_buffer_t));
        
        if (required_size == sizeof(logger_ring_buffer_t)) {
            printf("  - Size matches: ✅\n");
        } else {
            printf("  - Size mismatch: ❌\n");
        }
    } else {
        printf("Error checking ring buffer: %s\n", esp_err_to_name(err));
    }
    
    printf("======================\n\n");
    nvs_close(nvs_handle);
}