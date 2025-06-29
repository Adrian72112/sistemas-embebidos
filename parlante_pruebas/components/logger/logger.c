#include "logger.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_timer.h"
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

// Funciones auxiliares privadas
static void logger_ring_buffer_init(void);
static esp_err_t logger_ring_buffer_add_event(logger_event_type_t event_type);
static esp_err_t logger_save_ring_buffer_blob_to_nvs(void);
static esp_err_t logger_load_ring_buffer_blob_from_nvs(void);

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

    g_logger_initialized = true;
    ESP_LOGI(TAG, "Logger initialized successfully with ring buffer");
    
    return ESP_OK;
}

esp_err_t logger_deinit(void)
{
    if (!g_logger_initialized) {
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Deinitializing logger...");

    // Save ring buffer to NVS before deinitializing
    esp_err_t err = logger_save_ring_buffer_blob_to_nvs();
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Failed to save ring buffer to NVS: %s", esp_err_to_name(err));
    }

    // Clean up mutex
    if (g_ring_buffer_mutex != NULL) {
        vSemaphoreDelete(g_ring_buffer_mutex);
        g_ring_buffer_mutex = NULL;
    }

    g_logger_initialized = false;
    ESP_LOGI(TAG, "Logger deinitialized");
    
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

    // Optionally save to NVS immediately for critical events
    // For now, we'll save periodically or on deinit to reduce wear
    // Uncomment the following lines for immediate persistence:
    /*
    err = logger_save_ring_buffer_blob_to_nvs();
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Failed to save ring buffer to NVS: %s", esp_err_to_name(err));
    }
    */

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

static esp_err_t logger_save_ring_buffer_blob_to_nvs(void)
{
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(LOGGER_NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error opening NVS handle for ring buffer save: %s", esp_err_to_name(err));
        return err;
    }

    if (xSemaphoreTake(g_ring_buffer_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        ESP_LOGE(TAG, "Failed to acquire mutex for ring buffer save");
        nvs_close(nvs_handle);
        return ESP_ERR_TIMEOUT;
    }

    // Guardar ring buffer como blob
    err = nvs_set_blob(nvs_handle, LOGGER_NVS_KEY_RING_BUFFER, &g_ring_buffer, sizeof(logger_ring_buffer_t));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error saving ring buffer blob: %s", esp_err_to_name(err));
        xSemaphoreGive(g_ring_buffer_mutex);
        nvs_close(nvs_handle);
        return err;
    }

    err = nvs_commit(nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error committing ring buffer blob: %s", esp_err_to_name(err));
    } else {
        ESP_LOGI(TAG, "Ring buffer saved to NVS successfully");
    }

    xSemaphoreGive(g_ring_buffer_mutex);
    nvs_close(nvs_handle);
    return err;
}

static esp_err_t logger_load_ring_buffer_blob_from_nvs(void)
{
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(LOGGER_NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error opening NVS handle for ring buffer load: %s", esp_err_to_name(err));
        return err;
    }

    size_t required_size = sizeof(logger_ring_buffer_t);
    err = nvs_get_blob(nvs_handle, LOGGER_NVS_KEY_RING_BUFFER, &g_ring_buffer, &required_size);
    
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGI(TAG, "Ring buffer not found in NVS, initializing empty");
        logger_ring_buffer_init();
        err = ESP_OK;
    } else if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error loading ring buffer from NVS: %s", esp_err_to_name(err));
        logger_ring_buffer_init();
    } else {
        ESP_LOGI(TAG, "Ring buffer loaded from NVS successfully (count: %d, total: %" PRIu32 ")", 
                 g_ring_buffer.count, g_ring_buffer.total_events);
    }

    nvs_close(nvs_handle);
    return err;
}