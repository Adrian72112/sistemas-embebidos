# Logger Component

A thread-safe audio event logger for ESP32 with persistent storage using SPIFFS filesystem.

## Features

- **Circular Ring Buffer**: Stores the last 20 audio events in a circular buffer
- **Thread Safety**: Uses FreeRTOS mutex for safe concurrent access
- **Persistent Storage**: Automatically saves events to SPIFFS filesystem
- **Wear Leveling**: Built on SPIFFS for flash wear leveling
- **Auto-Save**: Saves on every event and system shutdown
- **Event Types**: Supports PLAY, PAUSE, NEXT, PREVIOUS, STOP events
- **Timestamps**: Each event includes microsecond-precision timestamp
- **Sequence Numbers**: Global sequence numbering for event ordering

## Architecture

```
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│   Application   │───▶│  Logger API     │───▶│  Ring Buffer    │
└─────────────────┘    └─────────────────┘    └─────────────────┘
                                │                       │
                                │                       │
                                ▼                       ▼
                       ┌─────────────────┐    ┌─────────────────┐
                       │  SPIFFS VFS     │    │  FreeRTOS Mutex │
                       └─────────────────┘    └─────────────────┘
                                │
                                ▼
                       ┌─────────────────┐
                       │  Flash Storage  │
                       └─────────────────┘
```

## Usage

### Basic Usage

```c
#include "logger.h"

// Initialize the logger
esp_err_t err = logger_init();
if (err != ESP_OK) {
    ESP_LOGE("APP", "Failed to initialize logger");
    return;
}

// Log events
logger_log_event(LOGGER_EVENT_PLAY);
logger_log_event(LOGGER_EVENT_PAUSE);
logger_log_event(LOGGER_EVENT_STOP);

// Print current status
logger_print_info();

// Print event history
logger_print_event_history();

// Clean shutdown
logger_deinit();
```

### Advanced Usage

```c
// Get total event count
uint32_t total_events = logger_get_event_count();

// Get a copy of the ring buffer
logger_ring_buffer_t buffer;
err = logger_get_ring_buffer(&buffer);

// Get specific event by index
logger_event_t event;
err = logger_get_event_by_index(0, &event); // Get oldest event

// Manual save/load (normally automatic)
logger_save_to_file();
logger_load_from_file();
```

## Configuration

The logger is configured through defines in `logger.h`:

- `LOGGER_RING_BUFFER_SIZE`: Maximum events in buffer (default: 20)
- `LOGGER_FILE_PATH`: SPIFFS file path (default: "/spiffs/logger_events.bin")

## Event Types

```c
typedef enum {
    LOGGER_EVENT_PLAY = 0,      // Playback started
    LOGGER_EVENT_PAUSE,         // Playback paused
    LOGGER_EVENT_NEXT,          // Next track selected
    LOGGER_EVENT_PREVIOUS,      // Previous track selected
    LOGGER_EVENT_STOP           // Playback stopped
} logger_event_type_t;
```

## Event Structure

```c
typedef struct {
    logger_event_type_t type;   // Event type
    uint64_t timestamp;         // Timestamp in microseconds since boot
    uint32_t sequence_number;   // Global sequence number
} logger_event_t;
```

## Ring Buffer Behavior

The ring buffer operates as a circular buffer:

1. **Not Full**: Events are added sequentially from index 0
2. **Full**: New events overwrite the oldest events (FIFO behavior)
3. **Indexing**: Index 0 always represents the oldest event in the buffer
4. **Capacity**: Maximum 20 events (configurable)

## Dependencies

Add to your component's `CMakeLists.txt`:

```cmake
idf_component_register(
    SRCS "your_sources.c"
    INCLUDE_DIRS "include"
    REQUIRES logger
)
```

The logger component requires:
- `spiffs`: SPIFFS filesystem support
- `wear_levelling`: Flash wear leveling
- `esp_timer`: High-resolution timer
- `freertos`: FreeRTOS for mutex support

## Partition Table

Ensure your partition table includes a SPIFFS partition:

```csv
# Name, Type, SubType, Offset, Size, Flags
nvs,      data, nvs,     0x9000,  0x6000,
phy_init, data, phy,     0xf000,  0x1000,
factory,  app,  factory, 0x10000, 0x180000,
spiffs,   data, spiffs,  ,        0x70000,
```

## Error Handling

All functions return `esp_err_t` status codes:

- `ESP_OK`: Success
- `ESP_ERR_INVALID_STATE`: Logger not initialized
- `ESP_ERR_INVALID_ARG`: Invalid argument
- `ESP_ERR_NO_MEM`: Memory allocation failed
- `ESP_ERR_TIMEOUT`: Mutex timeout
- `ESP_ERR_NOT_FOUND`: File operation failed
- `ESP_FAIL`: General failure

## Thread Safety

The logger is fully thread-safe and can be called from:
- Main task
- FreeRTOS tasks
- Timer callbacks
- Interrupt service routines (with caution)

## Performance Considerations

- Each `logger_log_event()` call writes to flash (SPIFFS)
- For high-frequency logging, consider batching events
- SPIFFS provides wear leveling but has finite write cycles
- Mutex timeout is set to 100ms for all operations

## Memory Usage

- **RAM**: ~500 bytes for ring buffer + mutex overhead
- **Flash**: ~500 bytes per save operation in SPIFFS
- **Code**: ~8KB compiled code size

## Example Output

```
=== LOGGER INFO ===
Initialized: YES
Ring buffer size: 20
Storage: SPIFFS
File path: /spiffs/logger_events.bin
Ring buffer count: 5
Ring buffer head: 5
Total events logged: 15
===================

=== EVENT HISTORY ===
Ring buffer capacity: 20
Current count: 5
Total events since init: 15

Events (oldest to newest):
Index | Seq# | Event      | Timestamp (μs)
------|------|------------|----------------
    0 |   11 | PLAY       |      12345678901
    1 |   12 | PAUSE      |      12345789012
    2 |   13 | PLAY       |      12345890123
    3 |   14 | NEXT       |      12345901234
    4 |   15 | STOP       |      12345912345
======================
```
