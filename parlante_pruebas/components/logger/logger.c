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
#include "freertos/task.h"
#include "freertos/queue.h"
#include <stdio.h>
#include <inttypes.h>
#include <string.h>

static const char *TAG = "LOGGER";

// Configuración de la tarea de guardado
#define LOGGER_SAVE_TASK_STACK_SIZE     4096
#define LOGGER_SAVE_TASK_PRIORITY       10        // Baja prioridad
#define LOGGER_SAVE_QUEUE_SIZE          10       // Cola para eventos de guardado
#define LOGGER_SAVE_TASK_NAME           "logger_save"

// Tipos de comandos para la tarea de guardado
typedef enum {
    LOGGER_SAVE_CMD_SAVE_BUFFER = 0,
    LOGGER_SAVE_CMD_SHUTDOWN
} logger_save_cmd_t;

// Global variables
static bool g_logger_initialized = false;
static logger_ring_buffer_t g_ring_buffer;
static SemaphoreHandle_t g_ring_buffer_mutex;
static TaskHandle_t g_save_task_handle = NULL;
static QueueHandle_t g_save_queue = NULL;

// Private function declarations
static void logger_ring_buffer_init(void);
static esp_err_t logger_ring_buffer_add_event(logger_event_type_t event_type);
static esp_err_t logger_init_spiffs(void);
static void logger_save_task(void *pvParameters);
static esp_err_t logger_create_save_task(void);
static esp_err_t logger_destroy_save_task(void);
static esp_err_t logger_request_save_async(void);

// Shutdown handler para guardar automáticamente antes de reset
static void logger_shutdown_handler(void)
{
    if (g_logger_initialized && g_save_queue != NULL) {
        ESP_LOGI(TAG, "Auto-saving ring buffer on shutdown...");
        
        // Enviar comando de shutdown a la tarea de guardado
        logger_save_cmd_t cmd = LOGGER_SAVE_CMD_SHUTDOWN;
        if (xQueueSend(g_save_queue, &cmd, pdMS_TO_TICKS(1000)) != pdTRUE) {
            ESP_LOGE(TAG, "Failed to send shutdown command to save task");
            // Fallback: guardar directamente
            esp_err_t err = logger_save_to_file();
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "Failed to save ring buffer on shutdown: %s", esp_err_to_name(err));
            }
        } else {
            // Dar tiempo a la tarea para procesar el comando
            vTaskDelay(pdMS_TO_TICKS(500));
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

    // Create save task and queue
    err = logger_create_save_task();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create save task: %s", esp_err_to_name(err));
        esp_vfs_spiffs_unregister(NULL);
        vSemaphoreDelete(g_ring_buffer_mutex);
        g_logger_initialized = false;
        return err;
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

    // Destroy save task first (this will also save the buffer one last time)
    esp_err_t err = logger_destroy_save_task();
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Failed to destroy save task: %s", esp_err_to_name(err));
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

    ESP_LOGI(TAG, "Event logged: %s (buffer: %d/%d, total: %" PRIu32 ")", 
             logger_event_type_to_string(event_type), 
             g_ring_buffer.count, 
             LOGGER_RING_BUFFER_SIZE,
             g_ring_buffer.total_events);

    // Request async save to SPIFFS (non-blocking)
    logger_request_save_async();

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
    printf("Storage: SPIFFS (Async)\n");
    printf("File path: %s\n", LOGGER_FILE_PATH);
    printf("Save task: %s\n", (g_save_task_handle != NULL) ? "RUNNING" : "STOPPED");
    printf("Save queue: %s\n", (g_save_queue != NULL) ? "CREATED" : "NOT CREATED");
    
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
    printf("Current count: %d (only last %d events stored)\n", g_ring_buffer.count, LOGGER_RING_BUFFER_SIZE);
    printf("Total events since init: %" PRIu32 " (counter only)\n", g_ring_buffer.total_events);
    printf("Storage: Only last %d events are persisted to flash\n", LOGGER_RING_BUFFER_SIZE);
    
    if (g_ring_buffer.count == 0) {
        printf("No events in history\n");
    } else {
        printf("\nEvents stored in memory (oldest to newest):\n");
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

    // Create a copy of the ring buffer to ensure we only save the last 20 events
    logger_ring_buffer_t save_buffer;
    memcpy(&save_buffer, &g_ring_buffer, sizeof(logger_ring_buffer_t));
    
    // Ensure we never save more than LOGGER_RING_BUFFER_SIZE events
    if (save_buffer.count > LOGGER_RING_BUFFER_SIZE) {
        ESP_LOGW(TAG, "Ring buffer count (%d) exceeds max size (%d), trimming to last %d events", 
                 save_buffer.count, LOGGER_RING_BUFFER_SIZE, LOGGER_RING_BUFFER_SIZE);
        save_buffer.count = LOGGER_RING_BUFFER_SIZE;
    }

    FILE *file = fopen(LOGGER_FILE_PATH, "wb");
    if (file == NULL) {
        ESP_LOGE(TAG, "Failed to open file for writing: %s", LOGGER_FILE_PATH);
        xSemaphoreGive(g_ring_buffer_mutex);
        return ESP_ERR_NOT_FOUND;
    }

    size_t written = fwrite(&save_buffer, sizeof(logger_ring_buffer_t), 1, file);
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

    if (xSemaphoreTake(g_ring_buffer_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        ESP_LOGE(TAG, "Failed to acquire mutex for file load");
        return ESP_ERR_TIMEOUT;
    }

    FILE *file = fopen(LOGGER_FILE_PATH, "rb");
    if (file == NULL) {
        ESP_LOGI(TAG, "No previous log file found, starting fresh");
        logger_ring_buffer_init();
        xSemaphoreGive(g_ring_buffer_mutex);
        return ESP_OK;
    }

    logger_ring_buffer_t loaded_buffer;
    size_t read = fread(&loaded_buffer, sizeof(logger_ring_buffer_t), 1, file);
    fclose(file);

    if (read != 1) {
        ESP_LOGE(TAG, "Failed to read ring buffer from file");
        logger_ring_buffer_init();
        xSemaphoreGive(g_ring_buffer_mutex);
        return ESP_FAIL;
    }

    // Validate loaded data and enforce 20-event limit
    if (loaded_buffer.count > LOGGER_RING_BUFFER_SIZE) {
        ESP_LOGW(TAG, "Loaded buffer has %d events, trimming to last %d events", 
                 loaded_buffer.count, LOGGER_RING_BUFFER_SIZE);
        
        // Trim to keep only the last LOGGER_RING_BUFFER_SIZE events
        // Calculate how many events to skip
        uint32_t events_to_skip = loaded_buffer.count - LOGGER_RING_BUFFER_SIZE;
        
        // Create new buffer with only the last 20 events
        logger_ring_buffer_t trimmed_buffer = {0};
        
        for (uint32_t i = 0; i < LOGGER_RING_BUFFER_SIZE; i++) {
            uint32_t source_index = (loaded_buffer.head + events_to_skip + i) % LOGGER_RING_BUFFER_SIZE;
            trimmed_buffer.events[i] = loaded_buffer.events[source_index];
        }
        
        trimmed_buffer.count = LOGGER_RING_BUFFER_SIZE;
        trimmed_buffer.head = 0;
        
        // Copy the trimmed buffer to the global buffer
        memcpy(&g_ring_buffer, &trimmed_buffer, sizeof(logger_ring_buffer_t));
        
        ESP_LOGI(TAG, "Trimmed log buffer to %d events", LOGGER_RING_BUFFER_SIZE);
        
        // Save the trimmed buffer back to file to ensure persistence matches memory
        xSemaphoreGive(g_ring_buffer_mutex);
        esp_err_t save_result = logger_save_to_file();
        if (save_result != ESP_OK) {
            ESP_LOGW(TAG, "Failed to save trimmed buffer back to file");
        }
        return ESP_OK;
        
    } else if (loaded_buffer.head >= LOGGER_RING_BUFFER_SIZE) {
        ESP_LOGW(TAG, "Invalid head %d, resetting ring buffer", loaded_buffer.head);
        logger_ring_buffer_init();
        xSemaphoreGive(g_ring_buffer_mutex);
        return ESP_OK;
    }

    // Copy the loaded buffer directly (it's within limits)
    memcpy(&g_ring_buffer, &loaded_buffer, sizeof(logger_ring_buffer_t));
    ESP_LOGI(TAG, "Loaded %d events from file", loaded_buffer.count);

    xSemaphoreGive(g_ring_buffer_mutex);
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

// Tarea de guardado asíncrono
static void logger_save_task(void *pvParameters)
{
    logger_save_cmd_t cmd;
    
    ESP_LOGI(TAG, "Logger save task started");
    
    while (1) {
        // Esperar por comandos en la cola
        if (xQueueReceive(g_save_queue, &cmd, portMAX_DELAY) == pdTRUE) {
            switch (cmd) {
                case LOGGER_SAVE_CMD_SAVE_BUFFER:
                    // Guardar buffer a SPIFFS
                    esp_err_t err = logger_save_to_file();
                    if (err != ESP_OK) {
                        ESP_LOGW(TAG, "Async save failed: %s", esp_err_to_name(err));
                    }
                    break;
                    
                case LOGGER_SAVE_CMD_SHUTDOWN:
                    // Comando de apagado - hacer guardado final y salir
                    ESP_LOGI(TAG, "Save task received shutdown command");
                    esp_err_t shutdown_err = logger_save_to_file();
                    if (shutdown_err != ESP_OK) {
                        ESP_LOGE(TAG, "Final save failed: %s", esp_err_to_name(shutdown_err));
                    } else {
                        ESP_LOGI(TAG, "Final save completed successfully");
                    }
                    
                    // Salir del bucle para terminar la tarea
                    goto task_exit;
                    
                default:
                    ESP_LOGW(TAG, "Unknown save command: %d", cmd);
                    break;
            }
        }
    }
    
task_exit:
    ESP_LOGI(TAG, "Logger save task exiting");
    vTaskDelete(NULL);
}

// Crear tarea de guardado y cola
static esp_err_t logger_create_save_task(void)
{
    // Crear cola para comandos de guardado
    g_save_queue = xQueueCreate(LOGGER_SAVE_QUEUE_SIZE, sizeof(logger_save_cmd_t));
    if (g_save_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create save queue");
        return ESP_ERR_NO_MEM;
    }
    
    // Crear tarea de guardado
    BaseType_t task_created = xTaskCreate(logger_save_task, 
                                         LOGGER_SAVE_TASK_NAME,
                                         LOGGER_SAVE_TASK_STACK_SIZE,
                                         NULL,
                                         LOGGER_SAVE_TASK_PRIORITY,
                                         &g_save_task_handle);
    
    if (task_created != pdPASS) {
        ESP_LOGE(TAG, "Failed to create save task");
        vQueueDelete(g_save_queue);
        g_save_queue = NULL;
        return ESP_ERR_NO_MEM;
    }
    
    ESP_LOGI(TAG, "Save task created successfully (priority: %d)", LOGGER_SAVE_TASK_PRIORITY);
    return ESP_OK;
}

// Destruir tarea de guardado y cola
static esp_err_t logger_destroy_save_task(void)
{
    if (g_save_queue != NULL && g_save_task_handle != NULL) {
        // Enviar comando de shutdown
        logger_save_cmd_t cmd = LOGGER_SAVE_CMD_SHUTDOWN;
        if (xQueueSend(g_save_queue, &cmd, pdMS_TO_TICKS(1000)) == pdTRUE) {
            // Esperar a que la tarea termine
            vTaskDelay(pdMS_TO_TICKS(1000));
        } else {
            ESP_LOGW(TAG, "Failed to send shutdown command, forcing task deletion");
            vTaskDelete(g_save_task_handle);
        }
        
        g_save_task_handle = NULL;
    }
    
    if (g_save_queue != NULL) {
        vQueueDelete(g_save_queue);
        g_save_queue = NULL;
    }
    
    ESP_LOGI(TAG, "Save task destroyed");
    return ESP_OK;
}

// Solicitar guardado asíncrono (no bloqueante)
static esp_err_t logger_request_save_async(void)
{
    if (g_save_queue == NULL) {
        ESP_LOGW(TAG, "Save queue not available");
        return ESP_ERR_INVALID_STATE;
    }
    
    logger_save_cmd_t cmd = LOGGER_SAVE_CMD_SAVE_BUFFER;
    
    // Enviar comando sin bloquear (si la cola está llena, no importa)
    if (xQueueSend(g_save_queue, &cmd, 0) != pdTRUE) {
        // La cola está llena, pero no es crítico
        // El próximo evento será guardado cuando haya espacio
        return ESP_OK;
    }
    
    return ESP_OK;
}
