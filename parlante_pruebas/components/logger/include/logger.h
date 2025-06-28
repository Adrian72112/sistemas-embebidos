/*
 * Audio Logger Component
 * Logs playback events to non-volatile storage (NVS) using a circular buffer
 */

#ifndef LOGGER_H
#define LOGGER_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// Configuration constants
#define LOGGER_BUFFER_SIZE 20  // Number of events in circular buffer (as specified)

// Maximum length for track name and event description
#define LOGGER_MAX_TRACK_NAME_LEN 32
#define LOGGER_MAX_EVENT_DESC_LEN 64

// Event types
typedef enum {
    LOGGER_EVENT_PLAY = 0,
    LOGGER_EVENT_PAUSE,
    LOGGER_EVENT_STOP,
    LOGGER_EVENT_NEXT,
    LOGGER_EVENT_PREVIOUS,
    LOGGER_EVENT_VOLUME_UP,
    LOGGER_EVENT_VOLUME_DOWN,
    LOGGER_EVENT_MAX
} logger_event_type_t;

// Event structure
typedef struct {
    int64_t timestamp;                                    // System timestamp in microseconds
    logger_event_type_t event_type;                       // Type of event
    char track_name[LOGGER_MAX_TRACK_NAME_LEN];           // Name of the track
    char description[LOGGER_MAX_EVENT_DESC_LEN];          // Additional description
    uint32_t track_duration_ms;                           // Track duration in milliseconds
    uint8_t volume_level;                                 // Volume level (0-100)
} __attribute__((packed)) logger_event_t;

// Logger statistics
typedef struct {
    uint32_t total_events;        // Total events logged since initialization
    uint32_t events_in_buffer;    // Current number of events in buffer
    uint32_t buffer_overruns;     // Number of times buffer was overrun
    bool nvs_initialized;         // NVS initialization status
} logger_stats_t;

/**
 * @brief Initialize the logger component
 * 
 * This function initializes the NVS partition and loads existing events
 * from non-volatile storage into the circular buffer.
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t logger_init(void);

/**
 * @brief Deinitialize the logger component
 * 
 * Saves current buffer to NVS and cleans up resources.
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t logger_deinit(void);

/**
 * @brief Log a playback event
 * 
 * @param event_type Type of event to log
 * @param track_name Name of the track (can be NULL)
 * @param description Additional description (can be NULL)
 * @param track_duration_ms Track duration in milliseconds (0 if unknown)
 * @param volume_level Current volume level (0-100)
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t logger_log_event(logger_event_type_t event_type, 
                          const char *track_name,
                          const char *description,
                          uint32_t track_duration_ms,
                          uint8_t volume_level);

/**
 * @brief Get the last N events from the logger
 * 
 * @param events Array to store the events
 * @param max_events Maximum number of events to retrieve
 * @param num_events_returned Pointer to store the actual number of events returned
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t logger_get_events(logger_event_t *events, 
                           uint32_t max_events, 
                           uint32_t *num_events_returned);

/**
 * @brief Get logger statistics
 * 
 * @param stats Pointer to store the statistics
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t logger_get_stats(logger_stats_t *stats);

/**
 * @brief Clear all events from the logger
 * 
 * This function clears both the in-memory buffer and the NVS storage.
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t logger_clear_events(void);

/**
 * @brief Print all events to console (for debugging)
 */
void logger_print_events(void);

/**
 * @brief Convert event type to string
 * 
 * @param event_type Event type to convert
 * @return String representation of the event type
 */
const char* logger_event_type_to_string(logger_event_type_t event_type);

#ifdef __cplusplus
}
#endif

#endif // LOGGER_H
