/*
 * Audio Logger Component Implementation
 * Implements a circular buffer of 20 events stored in non-volatile memory (NVS)
 * As specified: buffer circular de 20 espacios, en memoria no volátil
 */

#include "logger.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "nvs_flash.h"
#include "nvs.h"
#include <string.h>
#include <stdio.h>
#include <inttypes.h>

static const char *TAG = "AUDIO_LOGGER";

// NVS namespace for storing logger data
#define NVS_NAMESPACE "audio_log"
#define NVS_KEY_BUFFER "events_buf"
#define NVS_KEY_INDEX "buf_index"

// Circular buffer structure stored in NVS
typedef struct {
    logger_event_t events[LOGGER_BUFFER_SIZE];  // 20 events as specified
    uint32_t head_index;                        // Current write position
    uint32_t count;                             // Number of events in buffer
    uint32_t total_events;                      // Total events logged (for statistics)
} circular_buffer_t;

// Logger context structure
typedef struct {
    nvs_handle_t nvs_handle;            // NVS handle
    circular_buffer_t buffer;           // In-memory copy of the circular buffer
    bool initialized;                   // Initialization flag
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
static esp_err_t logger_save_buffer_to_nvs(void);
static esp_err_t logger_load_buffer_from_nvs(void);

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
    
    // Initialize circular buffer
    memset(&g_logger_ctx.buffer, 0, sizeof(circular_buffer_t));
    
    // Try to load existing buffer from NVS
    ret = logger_load_buffer_from_nvs();
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "No existing buffer found in NVS or failed to load: %s", esp_err_to_name(ret));
        // Initialize with empty buffer
        g_logger_ctx.buffer.head_index = 0;
        g_logger_ctx.buffer.count = 0;
        g_logger_ctx.buffer.total_events = 0;
    }
    
    g_logger_ctx.initialized = true;
    
    ESP_LOGI(TAG, "Logger initialized successfully with circular buffer");
    ESP_LOGI(TAG, "Buffer size: %d events, Current count: %" PRIu32 ", Total events: %" PRIu32, 
             LOGGER_BUFFER_SIZE, g_logger_ctx.buffer.count, g_logger_ctx.buffer.total_events);
    
    return ESP_OK;
}

esp_err_t logger_deinit(void)
{
    if (!g_logger_ctx.initialized) {
        return ESP_OK;
    }
    
    // Save current state to NVS
    esp_err_t ret = logger_save_buffer_to_nvs();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save buffer to NVS during deinit: %s", esp_err_to_name(ret));
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
    logger_event_t new_event = {0};
    
    // Fill event data
    new_event.timestamp = esp_timer_get_time();
    new_event.event_type = event_type;
    new_event.track_duration_ms = track_duration_ms;
    new_event.volume_level = volume_level;
    
    // Copy track name (with safety checks)
    if (track_name != NULL) {
        strncpy(new_event.track_name, track_name, LOGGER_MAX_TRACK_NAME_LEN - 1);
        new_event.track_name[LOGGER_MAX_TRACK_NAME_LEN - 1] = '\0';
    } else {
        strcpy(new_event.track_name, "Unknown");
    }
    
    // Copy description (with safety checks)
    if (description != NULL) {
        strncpy(new_event.description, description, LOGGER_MAX_EVENT_DESC_LEN - 1);
        new_event.description[LOGGER_MAX_EVENT_DESC_LEN - 1] = '\0';
    } else {
        new_event.description[0] = '\0';
    }
    
    // Add event to circular buffer
    g_logger_ctx.buffer.events[g_logger_ctx.buffer.head_index] = new_event;
    
    // Update circular buffer indices
    g_logger_ctx.buffer.head_index = (g_logger_ctx.buffer.head_index + 1) % LOGGER_BUFFER_SIZE;
    
    // Update count (max 20 events)
    if (g_logger_ctx.buffer.count < LOGGER_BUFFER_SIZE) {
        g_logger_ctx.buffer.count++;
    }
    
    // Update total events counter
    g_logger_ctx.buffer.total_events++;
    
    // Save to NVS immediately to ensure persistence
    esp_err_t ret = logger_save_buffer_to_nvs();
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to save buffer to NVS: %s", esp_err_to_name(ret));
    }
    
    // Log the event for debugging
    ESP_LOGI(TAG, "Event logged: %s - %s [%s] (Vol: %d%%, Duration: %" PRIu32 "ms) [%" PRIu32 "/%" PRIu32 "]", 
             logger_event_type_to_string(event_type),
             new_event.track_name,
             new_event.description,
             new_event.volume_level,
             new_event.track_duration_ms,
             g_logger_ctx.buffer.count,
             g_logger_ctx.buffer.total_events);
    
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
    
    *num_events_returned = 0;
    
    if (g_logger_ctx.buffer.count == 0) {
        ESP_LOGI(TAG, "No events in circular buffer");
        return ESP_OK;
    }
    
    // Determine how many events to return
    uint32_t events_to_return = (g_logger_ctx.buffer.count < max_events) ? 
                                g_logger_ctx.buffer.count : max_events;
    
    // Calculate the starting index for the oldest event
    uint32_t start_index;
    if (g_logger_ctx.buffer.count < LOGGER_BUFFER_SIZE) {
        // Buffer not full yet, start from beginning
        start_index = 0;
    } else {
        // Buffer is full, oldest event is at head_index
        start_index = g_logger_ctx.buffer.head_index;
    }
    
    // Copy events to output buffer
    for (uint32_t i = 0; i < events_to_return; i++) {
        uint32_t index = (start_index + i) % LOGGER_BUFFER_SIZE;
        events[i] = g_logger_ctx.buffer.events[index];
    }
    
    *num_events_returned = events_to_return;
    
    ESP_LOGI(TAG, "Retrieved %" PRIu32 " events from circular buffer", events_to_return);
    
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
    
    // Fill statistics from circular buffer
    stats->total_events = g_logger_ctx.buffer.total_events;
    stats->events_in_buffer = g_logger_ctx.buffer.count;
    stats->buffer_overruns = 0;  // Not applicable for circular buffer
    stats->nvs_initialized = g_logger_ctx.initialized;
    
    return ESP_OK;
}

esp_err_t logger_clear_events(void)
{
    if (!g_logger_ctx.initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    // Clear the circular buffer
    memset(&g_logger_ctx.buffer, 0, sizeof(circular_buffer_t));
    g_logger_ctx.buffer.head_index = 0;
    g_logger_ctx.buffer.count = 0;
    // Keep total_events for historical tracking
    
    // Clear NVS data
    esp_err_t ret = nvs_erase_key(g_logger_ctx.nvs_handle, NVS_KEY_BUFFER);
    if (ret != ESP_OK && ret != ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGE(TAG, "Failed to clear buffer from NVS: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ret = nvs_commit(g_logger_ctx.nvs_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to commit NVS changes: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ESP_LOGI(TAG, "All events cleared from circular buffer and NVS");
    
    return ESP_OK;
}

void logger_print_events(void)
{
    if (!g_logger_ctx.initialized) {
        printf("Logger not initialized\n");
        return;
    }
    
    printf("\n=== Audio Logger Events (Circular Buffer in NVS) ===\n");
    printf("Buffer size: %d events, Current count: %" PRIu32 ", Total events: %" PRIu32 "\n", 
           LOGGER_BUFFER_SIZE, g_logger_ctx.buffer.count, g_logger_ctx.buffer.total_events);
    printf("Head index: %" PRIu32 "\n\n", g_logger_ctx.buffer.head_index);
    
    if (g_logger_ctx.buffer.count == 0) {
        printf("No events in circular buffer.\n");
        printf("Events are stored in a 20-slot circular buffer in NVS.\n");
        return;
    }
    
    // Print all events in chronological order (oldest first)
    uint32_t start_index;
    if (g_logger_ctx.buffer.count < LOGGER_BUFFER_SIZE) {
        // Buffer not full yet, start from beginning
        start_index = 0;
    } else {
        // Buffer is full, oldest event is at head_index
        start_index = g_logger_ctx.buffer.head_index;
    }
    
    printf("Events (oldest to newest):\n");
    for (uint32_t i = 0; i < g_logger_ctx.buffer.count; i++) {
        uint32_t index = (start_index + i) % LOGGER_BUFFER_SIZE;
        logger_event_t *event = &g_logger_ctx.buffer.events[index];
        
        printf("[%02" PRIu32 "] %s - %s [%s] (Vol: %d%%, Duration: %" PRIu32 "ms) @ %" PRId64 "us\n",
               i + 1,
               logger_event_type_to_string(event->event_type),
               event->track_name,
               event->description,
               event->volume_level,
               event->track_duration_ms,
               event->timestamp);
    }
    printf("\n");
}

const char* logger_event_type_to_string(logger_event_type_t event_type)
{
    if (event_type >= LOGGER_EVENT_MAX) {
        return "UNKNOWN";
    }
    
    return event_type_strings[event_type];
}

// Private functions

static esp_err_t logger_save_buffer_to_nvs(void)
{
    esp_err_t ret;
    
    // Save the entire circular buffer to NVS
    ret = nvs_set_blob(g_logger_ctx.nvs_handle, NVS_KEY_BUFFER, 
                      &g_logger_ctx.buffer, sizeof(circular_buffer_t));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save buffer to NVS: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Commit changes
    ret = nvs_commit(g_logger_ctx.nvs_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to commit NVS changes: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ESP_LOGD(TAG, "Circular buffer saved to NVS (count: %" PRIu32 ", total: %" PRIu32 ")", 
             g_logger_ctx.buffer.count, g_logger_ctx.buffer.total_events);
    return ESP_OK;
}

static esp_err_t logger_load_buffer_from_nvs(void)
{
    esp_err_t ret;
    size_t required_size = sizeof(circular_buffer_t);
    
    ret = nvs_get_blob(g_logger_ctx.nvs_handle, NVS_KEY_BUFFER, 
                      &g_logger_ctx.buffer, &required_size);
    if (ret != ESP_OK) {
        if (ret == ESP_ERR_NVS_NOT_FOUND) {
            ESP_LOGI(TAG, "No existing buffer found in NVS - starting fresh");
        } else {
            ESP_LOGE(TAG, "Failed to load buffer from NVS: %s", esp_err_to_name(ret));
        }
        return ret;
    }
    
    // Validate loaded data
    if (g_logger_ctx.buffer.head_index >= LOGGER_BUFFER_SIZE) {
        ESP_LOGW(TAG, "Invalid head_index in loaded buffer, resetting");
        g_logger_ctx.buffer.head_index = 0;
    }
    
    if (g_logger_ctx.buffer.count > LOGGER_BUFFER_SIZE) {
        ESP_LOGW(TAG, "Invalid count in loaded buffer, resetting");
        g_logger_ctx.buffer.count = 0;
    }
    
    ESP_LOGI(TAG, "Loaded circular buffer from NVS (count: %" PRIu32 ", total: %" PRIu32 ")", 
             g_logger_ctx.buffer.count, g_logger_ctx.buffer.total_events);
    
    return ESP_OK;
}
