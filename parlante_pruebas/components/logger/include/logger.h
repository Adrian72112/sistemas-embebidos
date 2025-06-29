/**
 * @file logger.h
 * @brief ESP32 Audio Event Logger with SPIFFS Persistence
 * 
 * This logger component provides a thread-safe circular buffer for storing
 * audio playback events (play, pause, next, previous, stop) with persistent
 * storage using SPIFFS filesystem and wear leveling.
 * 
 * Features:
 * - Circular ring buffer with 20 event slots
 * - Thread-safe operations using FreeRTOS mutex
 * - Persistent storage in SPIFFS filesystem
 * - Automatic save on system shutdown
 * - Event sequence numbering and timestamps
 * 
 * @author ESP32 Project
 * @version 1.0
 * @date 2025
 */

#ifndef LOGGER_H
#define LOGGER_H

#include <stdint.h>
#include <stddef.h>
#include "esp_err.h"

/* Configuration Constants */
#define LOGGER_RING_BUFFER_SIZE 20                          ///< Maximum number of events in ring buffer
#define LOGGER_FILE_PATH "/spiffs/logger_events.bin"        ///< SPIFFS file path for persistence

/**
 * @brief Audio playback event types
 */
typedef enum {
    LOGGER_EVENT_PLAY = 0,      ///< Playback started
    LOGGER_EVENT_PAUSE,         ///< Playback paused
    LOGGER_EVENT_NEXT,          ///< Next track selected
    LOGGER_EVENT_PREVIOUS,      ///< Previous track selected
    LOGGER_EVENT_STOP           ///< Playback stopped
} logger_event_type_t;

/**
 * @brief Individual event structure
 */
typedef struct {
    logger_event_type_t type;   ///< Event type
    uint64_t timestamp;         ///< Timestamp in microseconds since boot
    uint32_t sequence_number;   ///< Global sequence number
} logger_event_t;

/**
 * @brief Ring buffer structure for storing events
 */
typedef struct {
    logger_event_t events[LOGGER_RING_BUFFER_SIZE];    ///< Array of events
    uint8_t head;                                       ///< Index for next insertion
    uint8_t count;                                      ///< Current number of events
    uint32_t total_events;                             ///< Total events since initialization
} logger_ring_buffer_t;

/* Public API Functions */

/**
 * @brief Initialize the logger system
 * 
 * Initializes SPIFFS filesystem, creates mutex for thread safety,
 * loads existing events from persistent storage, and registers
 * shutdown handler for automatic saving.
 * 
 * @return ESP_OK on success, error code on failure
 */
esp_err_t logger_init(void);

/**
 * @brief Deinitialize the logger system
 * 
 * Saves current ring buffer to persistent storage, cleans up
 * mutex and SPIFFS resources, unregisters shutdown handler.
 * 
 * @return ESP_OK on success, error code on failure
 */
esp_err_t logger_deinit(void);

/**
 * @brief Log an audio playback event
 * 
 * Adds a new event to the ring buffer with timestamp and sequence number.
 * Automatically saves to persistent storage after each event.
 * 
 * @param event_type Type of audio event to log
 * @return ESP_OK on success, error code on failure
 */
esp_err_t logger_log_event(logger_event_type_t event_type);

/**
 * @brief Get total number of events logged since initialization
 * 
 * @return Total event count, 0 if logger not initialized
 */
uint32_t logger_get_event_count(void);

/**
 * @brief Convert event type to string representation
 * 
 * @param event_type Event type to convert
 * @return String representation of event type
 */
const char* logger_event_type_to_string(logger_event_type_t event_type);

/**
 * @brief Print logger status and configuration information
 */
void logger_print_info(void);

/* Ring Buffer Access Functions */

/**
 * @brief Get a copy of the current ring buffer
 * 
 * Thread-safe operation that copies the entire ring buffer structure.
 * 
 * @param buffer Pointer to buffer structure to fill
 * @return ESP_OK on success, ESP_ERR_INVALID_ARG if buffer is NULL
 */
esp_err_t logger_get_ring_buffer(logger_ring_buffer_t* buffer);

/**
 * @brief Get a specific event by index
 * 
 * Retrieves an event from the ring buffer by its relative index
 * (0 = oldest event, count-1 = newest event).
 * 
 * @param index Event index (0 to count-1)
 * @param event Pointer to event structure to fill
 * @return ESP_OK on success, ESP_ERR_NOT_FOUND if index out of range
 */
esp_err_t logger_get_event_by_index(uint8_t index, logger_event_t* event);

/**
 * @brief Print complete event history
 * 
 * Displays all events in the ring buffer in chronological order
 * with detailed information including timestamps and sequence numbers.
 */
void logger_print_event_history(void);

/* Persistence Functions */

/**
 * @brief Manually save ring buffer to SPIFFS
 * 
 * Forces immediate save of current ring buffer state to persistent storage.
 * Normally called automatically after each event and on shutdown.
 * 
 * @return ESP_OK on success, error code on failure
 */
esp_err_t logger_save_to_file(void);

/**
 * @brief Manually load ring buffer from SPIFFS
 * 
 * Forces reload of ring buffer from persistent storage.
 * Normally called automatically during initialization.
 * 
 * @return ESP_OK on success, error code on failure
 */
esp_err_t logger_load_from_file(void);

#endif // LOGGER_H
