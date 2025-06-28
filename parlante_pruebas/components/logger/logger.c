/*
 * Audio Logger Component Implementation
 * Logs playback events to non-volatile storage (NVS) using a circular buffer
 */

#include "logger.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "nvs_flash.h"
#include "nvs.h"
#include <string.h>
#include <stdio.h>

static const char *TAG = "AUDIO_LOGGER";

// NVS namespace for storing logger data
#define NVS_NAMESPACE "audio_log"
#define NVS_KEY_EVENTS "events"
#define NVS_KEY_HEAD "head"
#define NVS_KEY_TAIL "tail"
#define NVS_KEY_COUNT "count"
#define NVS_KEY_TOTAL "total"
#define NVS_KEY_OVERRUNS "overruns"

// Circular buffer structure
typedef struct {
    logger_event_t events[LOGGER_MAX_EVENTS];
    uint32_t head;           // Index of the next write position
    uint32_t tail;           // Index of the oldest event
    uint32_t count;          // Current number of events in buffer
    uint32_t total_events;   // Total events logged since initialization
    uint32_t buffer_overruns; // Number of buffer overruns
    nvs_handle_t nvs_handle; // NVS handle
    bool initialized;        // Initialization flag
} logger_context_t;

// Global logger context
static logger_context_t g_logger_ctx = {0};

// Event type strings for debugging
static const char* event_type_strings[] = {
    "PLAY",
    "PAUSE", 
    "STOP",
    "NEXT",
    "PREVIOUS",
    "VOLUME_UP",
    "VOLUME_DOWN"
};

// Forward declarations
static esp_err_t logger_save_to_nvs(void);
static esp_err_t logger_load_from_nvs(void);

esp_err_t logger_init(void)
{
    esp_err_t ret = ESP_OK;
    
    if (g_logger_ctx.initialized) {
        ESP_LOGW(TAG, "Logger already initialized");
        return ESP_OK;
    }
    
    // Initialize NVS
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS partition was truncated and needs to be erased");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize NVS: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Open NVS handle
    ret = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &g_logger_ctx.nvs_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS handle: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Initialize context
    memset(&g_logger_ctx.events, 0, sizeof(g_logger_ctx.events));
    g_logger_ctx.head = 0;
    g_logger_ctx.tail = 0;
    g_logger_ctx.count = 0;
    g_logger_ctx.total_events = 0;
    g_logger_ctx.buffer_overruns = 0;
    
    // Try to load existing data from NVS
    ret = logger_load_from_nvs();
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "No existing data found in NVS or failed to load: %s", esp_err_to_name(ret));
        // This is not a critical error, continue with empty buffer
    }
    
    g_logger_ctx.initialized = true;
    
    ESP_LOGI(TAG, "Logger initialized successfully");
    ESP_LOGI(TAG, "Events in buffer: %ld, Total events: %ld, Overruns: %ld", 
             g_logger_ctx.count, g_logger_ctx.total_events, g_logger_ctx.buffer_overruns);
    
    return ESP_OK;
}

esp_err_t logger_deinit(void)
{
    if (!g_logger_ctx.initialized) {
        return ESP_OK;
    }
    
    // Save current state to NVS
    esp_err_t ret = logger_save_to_nvs();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save data to NVS during deinit: %s", esp_err_to_name(ret));
    }
    
    // Close NVS handle
    nvs_close(g_logger_ctx.nvs_handle);
    
    // Clear context
    memset(&g_logger_ctx, 0, sizeof(g_logger_ctx));
    g_logger_ctx.initialized = false;
    
    ESP_LOGI(TAG, "Logger deinitialized");
    
    return ret;
}

esp_err_t logger_log_event(logger_event_type_t event_type, 
                          const char *track_name,
                          const char *description,
                          uint32_t track_duration_ms,
                          uint8_t volume_level)
{
    if (!g_logger_ctx.initialized) {
        ESP_LOGE(TAG, "Logger not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    
    if (event_type >= LOGGER_EVENT_MAX) {
        ESP_LOGE(TAG, "Invalid event type: %d", event_type);
        return ESP_ERR_INVALID_ARG;
    }
    
    // Create new event
    logger_event_t *new_event = &g_logger_ctx.events[g_logger_ctx.head];
    
    // Fill event data
    new_event->timestamp = esp_timer_get_time();
    new_event->event_type = event_type;
    new_event->track_duration_ms = track_duration_ms;
    new_event->volume_level = volume_level;
    
    // Copy track name (with safety checks)
    if (track_name != NULL) {
        strncpy(new_event->track_name, track_name, LOGGER_MAX_TRACK_NAME_LEN - 1);
        new_event->track_name[LOGGER_MAX_TRACK_NAME_LEN - 1] = '\0';
    } else {
        strcpy(new_event->track_name, "Unknown");
    }
    
    // Copy description (with safety checks)
    if (description != NULL) {
        strncpy(new_event->description, description, LOGGER_MAX_EVENT_DESC_LEN - 1);
        new_event->description[LOGGER_MAX_EVENT_DESC_LEN - 1] = '\0';
    } else {
        new_event->description[0] = '\0';
    }
    
    // Update circular buffer pointers
    g_logger_ctx.head = (g_logger_ctx.head + 1) % LOGGER_MAX_EVENTS;
    
    if (g_logger_ctx.count < LOGGER_MAX_EVENTS) {
        g_logger_ctx.count++;
    } else {
        // Buffer is full, advance tail (overwrite oldest event)
        g_logger_ctx.tail = (g_logger_ctx.tail + 1) % LOGGER_MAX_EVENTS;
        g_logger_ctx.buffer_overruns++;
    }
    
    g_logger_ctx.total_events++;
    
    // Log the event for debugging
    ESP_LOGI(TAG, "Event logged: %s - %s [%s] (Vol: %d%%, Duration: %ldms)", 
             logger_event_type_to_string(event_type),
             new_event->track_name,
             new_event->description,
             new_event->volume_level,
             new_event->track_duration_ms);
    
    // Periodically save to NVS (every 5 events to reduce wear)
    if (g_logger_ctx.total_events % 5 == 0) {
        esp_err_t ret = logger_save_to_nvs();
        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "Failed to save to NVS: %s", esp_err_to_name(ret));
        }
    }
    
    return ESP_OK;
}

esp_err_t logger_get_events(logger_event_t *events, 
                           uint32_t max_events, 
                           uint32_t *num_events_returned)
{
    if (!g_logger_ctx.initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    if (events == NULL || num_events_returned == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    uint32_t events_to_copy = (max_events < g_logger_ctx.count) ? max_events : g_logger_ctx.count;
    *num_events_returned = events_to_copy;
    
    if (events_to_copy == 0) {
        return ESP_OK;
    }
    
    // Copy events from circular buffer (starting from oldest)
    uint32_t current_index = g_logger_ctx.tail;
    for (uint32_t i = 0; i < events_to_copy; i++) {
        memcpy(&events[i], &g_logger_ctx.events[current_index], sizeof(logger_event_t));
        current_index = (current_index + 1) % LOGGER_MAX_EVENTS;
    }
    
    return ESP_OK;
}

esp_err_t logger_get_stats(logger_stats_t *stats)
{
    if (!g_logger_ctx.initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    if (stats == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    stats->total_events = g_logger_ctx.total_events;
    stats->events_in_buffer = g_logger_ctx.count;
    stats->buffer_overruns = g_logger_ctx.buffer_overruns;
    stats->nvs_initialized = g_logger_ctx.initialized;
    
    return ESP_OK;
}

esp_err_t logger_clear_events(void)
{
    if (!g_logger_ctx.initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    // Clear in-memory buffer
    memset(&g_logger_ctx.events, 0, sizeof(g_logger_ctx.events));
    g_logger_ctx.head = 0;
    g_logger_ctx.tail = 0;
    g_logger_ctx.count = 0;
    // Note: Keep total_events and buffer_overruns for statistics
    
    // Clear NVS data
    esp_err_t ret = nvs_erase_key(g_logger_ctx.nvs_handle, NVS_KEY_EVENTS);
    if (ret != ESP_OK && ret != ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGE(TAG, "Failed to clear events from NVS: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ret = nvs_erase_key(g_logger_ctx.nvs_handle, NVS_KEY_HEAD);
    ret = nvs_erase_key(g_logger_ctx.nvs_handle, NVS_KEY_TAIL);
    ret = nvs_erase_key(g_logger_ctx.nvs_handle, NVS_KEY_COUNT);
    
    ret = nvs_commit(g_logger_ctx.nvs_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to commit NVS changes: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ESP_LOGI(TAG, "All events cleared");
    
    return ESP_OK;
}

esp_err_t logger_print_events(void)
{
    if (!g_logger_ctx.initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    printf("\n=== Audio Logger Events ===\n");
    printf("Total events: %ld, In buffer: %ld, Overruns: %ld\n\n", 
           g_logger_ctx.total_events, g_logger_ctx.count, g_logger_ctx.buffer_overruns);
    
    if (g_logger_ctx.count == 0) {
        printf("No events in buffer.\n");
        return ESP_OK;
    }
    
    uint32_t current_index = g_logger_ctx.tail;
    for (uint32_t i = 0; i < g_logger_ctx.count; i++) {
        logger_event_t *event = &g_logger_ctx.events[current_index];
        
        printf("[%lld] %s - %s [%s] (Vol: %d%%, Duration: %ldms)\n",
               event->timestamp,
               logger_event_type_to_string(event->event_type),
               event->track_name,
               event->description,
               event->volume_level,
               event->track_duration_ms);
        
        current_index = (current_index + 1) % LOGGER_MAX_EVENTS;
    }
    
    printf("\n");
    return ESP_OK;
}

const char* logger_event_type_to_string(logger_event_type_t event_type)
{
    if (event_type >= LOGGER_EVENT_MAX) {
        return "UNKNOWN";
    }
    
    return event_type_strings[event_type];
}

// Private functions

static esp_err_t logger_save_to_nvs(void)
{
    esp_err_t ret;
    
    // Save events array
    ret = nvs_set_blob(g_logger_ctx.nvs_handle, NVS_KEY_EVENTS, 
                       g_logger_ctx.events, sizeof(g_logger_ctx.events));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save events to NVS: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Save circular buffer state
    ret = nvs_set_u32(g_logger_ctx.nvs_handle, NVS_KEY_HEAD, g_logger_ctx.head);
    if (ret != ESP_OK) return ret;
    
    ret = nvs_set_u32(g_logger_ctx.nvs_handle, NVS_KEY_TAIL, g_logger_ctx.tail);
    if (ret != ESP_OK) return ret;
    
    ret = nvs_set_u32(g_logger_ctx.nvs_handle, NVS_KEY_COUNT, g_logger_ctx.count);
    if (ret != ESP_OK) return ret;
    
    ret = nvs_set_u32(g_logger_ctx.nvs_handle, NVS_KEY_TOTAL, g_logger_ctx.total_events);
    if (ret != ESP_OK) return ret;
    
    ret = nvs_set_u32(g_logger_ctx.nvs_handle, NVS_KEY_OVERRUNS, g_logger_ctx.buffer_overruns);
    if (ret != ESP_OK) return ret;
    
    // Commit changes
    ret = nvs_commit(g_logger_ctx.nvs_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to commit NVS changes: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ESP_LOGD(TAG, "Logger state saved to NVS");
    return ESP_OK;
}

static esp_err_t logger_load_from_nvs(void)
{
    esp_err_t ret;
    size_t required_size;
    
    // Load events array
    required_size = sizeof(g_logger_ctx.events);
    ret = nvs_get_blob(g_logger_ctx.nvs_handle, NVS_KEY_EVENTS, 
                       g_logger_ctx.events, &required_size);
    if (ret != ESP_OK) {
        if (ret == ESP_ERR_NVS_NOT_FOUND) {
            ESP_LOGI(TAG, "No existing events found in NVS");
        } else {
            ESP_LOGE(TAG, "Failed to load events from NVS: %s", esp_err_to_name(ret));
        }
        return ret;
    }
    
    // Load circular buffer state
    ret = nvs_get_u32(g_logger_ctx.nvs_handle, NVS_KEY_HEAD, &g_logger_ctx.head);
    if (ret != ESP_OK) return ret;
    
    ret = nvs_get_u32(g_logger_ctx.nvs_handle, NVS_KEY_TAIL, &g_logger_ctx.tail);
    if (ret != ESP_OK) return ret;
    
    ret = nvs_get_u32(g_logger_ctx.nvs_handle, NVS_KEY_COUNT, &g_logger_ctx.count);
    if (ret != ESP_OK) return ret;
    
    ret = nvs_get_u32(g_logger_ctx.nvs_handle, NVS_KEY_TOTAL, &g_logger_ctx.total_events);
    if (ret != ESP_OK) return ret;
    
    ret = nvs_get_u32(g_logger_ctx.nvs_handle, NVS_KEY_OVERRUNS, &g_logger_ctx.buffer_overruns);
    if (ret != ESP_OK) return ret;
    
    // Validate loaded data
    if (g_logger_ctx.head >= LOGGER_MAX_EVENTS || 
        g_logger_ctx.tail >= LOGGER_MAX_EVENTS ||
        g_logger_ctx.count > LOGGER_MAX_EVENTS) {
        ESP_LOGE(TAG, "Invalid data loaded from NVS, resetting buffer");
        memset(&g_logger_ctx.events, 0, sizeof(g_logger_ctx.events));
        g_logger_ctx.head = 0;
        g_logger_ctx.tail = 0;
        g_logger_ctx.count = 0;
        return ESP_ERR_INVALID_STATE;
    }
    
    ESP_LOGI(TAG, "Logger state loaded from NVS");
    return ESP_OK;
}
