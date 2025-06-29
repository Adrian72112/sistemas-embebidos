/**
 * @file logger.c
 * @brief ESP32 Audio Event Logger Implementation
 * 
 * Implementation of a thread-safe audio event logger with SPIFFS persistence.
 * Uses a circular ring buffer to store the last 20 events with automatic
 * saving on each event and system shutdown.
 */

#include "logger.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_spiffs.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <stdio.h>
#include <inttypes.h>
#include <string.h>

static const char *TAG = "LOGGER";

// Global variables
static bool g_logger_initialized = false;
static logger_ring_buffer_t g_ring_buffer;
static SemaphoreHandle_t g_ring_buffer_mutex;

// Private function declarations
static void logger_ring_buffer_init(void);
static esp_err_t logger_ring_buffer_add_event(logger_event_type_t event_type);
static esp_err_t logger_init_spiffs(void);

// Shutdown handler para guardar automáticamente antes de reset
static void logger_shutdown_handler(void)
{
    if (g_logger_initialized) {
        ESP_LOGI(TAG, "Auto-saving ring buffer on shutdown...");
        esp_err_t err = logger_save_to_file();
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to save ring buffer on shutdown: %s", esp_err_to_name(err));
        }
    }
}

esp_err_t logger_init(void)
{
    if (g_logger_initialized) {
        ESP_LOGW(TAG, "Logger already initialized");
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Initializing logger with SPIFFS storage...");

    // Create mutex for ring buffer protection
    g_ring_buffer_mutex = xSemaphoreCreateMutex();
    if (g_ring_buffer_mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create ring buffer mutex");
        return ESP_ERR_NO_MEM;
    }

    // Initialize SPIFFS
    esp_err_t err = logger_init_spiffs();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize SPIFFS: %s", esp_err_to_name(err));
        vSemaphoreDelete(g_ring_buffer_mutex);
        return err;
    }

    // Set initialized flag before loading from file
    g_logger_initialized = true;

    // Load ring buffer from SPIFFS
    err = logger_load_from_file();
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Failed to load ring buffer, starting fresh");
        logger_ring_buffer_init();
    }

    // Register shutdown handler to auto-save on reset
    esp_register_shutdown_handler(&logger_shutdown_handler);

    ESP_LOGI(TAG, "Logger initialized successfully (events: %" PRIu32 ")", g_ring_buffer.total_events);
    
    return ESP_OK;
}

esp_err_t logger_deinit(void)
{
    if (!g_logger_initialized) {
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Deinitializing logger...");

    // Save ring buffer to SPIFFS before deinitializing
    esp_err_t err = logger_save_to_file();
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Failed to save ring buffer during deinit: %s", esp_err_to_name(err));
    }

    // Clean up mutex
    if (g_ring_buffer_mutex != NULL) {
        vSemaphoreDelete(g_ring_buffer_mutex);
        g_ring_buffer_mutex = NULL;
    }

    // Unregister shutdown handler
    esp_unregister_shutdown_handler(&logger_shutdown_handler);

    // Deinitialize SPIFFS
    esp_vfs_spiffs_unregister(NULL);

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

    ESP_LOGI(TAG, "Event logged: %s (total: %" PRIu32 ")", 
             logger_event_type_to_string(event_type), g_ring_buffer.total_events);

    // Save to SPIFFS after each event for persistence
    err = logger_save_to_file();
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Failed to save to SPIFFS: %s", esp_err_to_name(err));
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
    printf("Storage: SPIFFS\n");
    printf("File path: %s\n", LOGGER_FILE_PATH);
    
    if (xSemaphoreTake(g_ring_buffer_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        printf("Ring buffer count: %d\n", g_ring_buffer.count);
        printf("Ring buffer head: %d\n", g_ring_buffer.head);
        printf("Total events logged: %" PRIu32 "\n", g_ring_buffer.total_events);
        xSemaphoreGive(g_ring_buffer_mutex);
    } else {
        printf("Ring buffer: [LOCKED]\n");
    }
    
    printf("===================\n\n");
}

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

esp_err_t logger_save_to_file(void)
{
    if (!g_logger_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    if (xSemaphoreTake(g_ring_buffer_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        ESP_LOGE(TAG, "Failed to acquire mutex for file save");
        return ESP_ERR_TIMEOUT;
    }

    FILE *file = fopen(LOGGER_FILE_PATH, "wb");
    if (file == NULL) {
        ESP_LOGE(TAG, "Failed to open file for writing: %s", LOGGER_FILE_PATH);
        xSemaphoreGive(g_ring_buffer_mutex);
        return ESP_ERR_NOT_FOUND;
    }

    size_t written = fwrite(&g_ring_buffer, sizeof(logger_ring_buffer_t), 1, file);
    fclose(file);

    if (written != 1) {
        ESP_LOGE(TAG, "Failed to write ring buffer to file");
        xSemaphoreGive(g_ring_buffer_mutex);
        return ESP_FAIL;
    }

    xSemaphoreGive(g_ring_buffer_mutex);
    return ESP_OK;
}

esp_err_t logger_load_from_file(void)
{
    if (!g_logger_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    FILE *file = fopen(LOGGER_FILE_PATH, "rb");
    if (file == NULL) {
        // File not found - first run, initialize empty
        logger_ring_buffer_init();
        return ESP_OK;
    }

    size_t read = fread(&g_ring_buffer, sizeof(logger_ring_buffer_t), 1, file);
    fclose(file);

    if (read != 1) {
        ESP_LOGE(TAG, "Failed to read ring buffer from file");
        logger_ring_buffer_init();
        return ESP_FAIL;
    }

    // Validate loaded data
    if (g_ring_buffer.count > LOGGER_RING_BUFFER_SIZE) {
        ESP_LOGW(TAG, "Invalid count %d, resetting ring buffer", g_ring_buffer.count);
        logger_ring_buffer_init();
    } else if (g_ring_buffer.head >= LOGGER_RING_BUFFER_SIZE) {
        ESP_LOGW(TAG, "Invalid head %d, resetting ring buffer", g_ring_buffer.head);
        logger_ring_buffer_init();
    }

    return ESP_OK;
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

    // Create new event
    logger_event_t new_event;
    new_event.type = event_type;
    new_event.timestamp = esp_timer_get_time();
    new_event.sequence_number = ++g_ring_buffer.total_events;

    // Add to ring buffer (circular)
    g_ring_buffer.events[g_ring_buffer.head] = new_event;
    g_ring_buffer.head = (g_ring_buffer.head + 1) % LOGGER_RING_BUFFER_SIZE;
    
    if (g_ring_buffer.count < LOGGER_RING_BUFFER_SIZE) {
        g_ring_buffer.count++;
    }

    xSemaphoreGive(g_ring_buffer_mutex);
    
    return ESP_OK;
}

static esp_err_t logger_init_spiffs(void)
{
    ESP_LOGI(TAG, "Initializing SPIFFS...");

    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 5,
        .format_if_mount_failed = true
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return ret;
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(NULL, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "SPIFFS: total %d bytes, used %d bytes", (int)total, (int)used);
    }

    return ESP_OK;
}
