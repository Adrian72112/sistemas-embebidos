#include "audio_controller.h"
#include "i2s_driver.h"
#include "es8311_codec.h" 
#include "logger.h"
#include "esp_log.h"
#include "esp_check.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include <string.h>

static const char *TAG = "audio_controller";

#define AUDIO_EVENT_QUEUE_SIZE 10
#define AUDIO_EVENT_TASK_STACK_SIZE 4096
#define AUDIO_EVENT_TASK_PRIORITY 5

// Variables privadas
static i2s_chan_handle_t tx_handle = NULL;
static i2s_chan_handle_t rx_handle = NULL;
static bool is_initialized = false;

// Variables de lista de reproducción
static audio_track_t *playlist = NULL;
static size_t playlist_size = 0;
static int current_track_index = 0;

// Control de tareas
static TaskHandle_t audio_task_handle = NULL;
static SemaphoreHandle_t player_mutex = NULL;
static QueueHandle_t event_queue = NULL;
static bool stop_current_track = false;
static bool is_paused = false;
static uint8_t current_volume = 50; // Volumen actual (0-100)

// Declaraciones anticipadas
static void audio_play_task(void *args);
static void audio_event_task(void *args);
static esp_err_t audio_controller_play_internal(void);
static esp_err_t audio_controller_pause_internal(void);
static esp_err_t audio_controller_next_internal(void);
static esp_err_t audio_controller_previous_internal(void);
static esp_err_t audio_controller_volume_up_internal(void);
static esp_err_t audio_controller_volume_down_internal(void);

esp_err_t audio_controller_init(const audio_controller_config_t *config)
{
    ESP_LOGI(TAG, "Initializing audio controller with event queue system");
    
    if (is_initialized) {
        ESP_LOGW(TAG, "Already initialized");
        return ESP_OK;
    }
    
    ESP_RETURN_ON_FALSE(config, ESP_ERR_INVALID_ARG, TAG, "Config cannot be NULL");
    
    // Crear mutex
    player_mutex = xSemaphoreCreateMutex();
    ESP_RETURN_ON_FALSE(player_mutex, ESP_ERR_NO_MEM, TAG, "Failed to create mutex");
    
    // Crear cola de eventos
    event_queue = xQueueCreate(AUDIO_EVENT_QUEUE_SIZE, sizeof(audio_event_t));
    ESP_RETURN_ON_FALSE(event_queue, ESP_ERR_NO_MEM, TAG, "Failed to create event queue");
    
    // Crear tarea de procesamiento de eventos
    BaseType_t result = xTaskCreate(
        audio_event_task,
        "audio_event",
        AUDIO_EVENT_TASK_STACK_SIZE,
        NULL,
        AUDIO_EVENT_TASK_PRIORITY,
        NULL
    );
    ESP_RETURN_ON_FALSE(result == pdPASS, ESP_FAIL, TAG, "Failed to create event task");
    
    // Inicializar logger
    esp_err_t ret = logger_init();
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Logger init failed: %s", esp_err_to_name(ret));
    }
    
    // Inicializar driver I2S
    ESP_RETURN_ON_ERROR(i2s_driver_init(config->sample_rate, &tx_handle, &rx_handle), 
                       TAG, "Failed to initialize I2S driver");
    
    // Inicializar codec ES8311
    ESP_RETURN_ON_ERROR(es8311_codec_init(config->sample_rate, config->volume, config->microphone_enabled),
                       TAG, "Failed to initialize ES8311 codec");
    
    // Guardar volumen inicial
    current_volume = config->volume;
    
    is_initialized = true;
    ESP_LOGI(TAG, "Audio controller initialized successfully with event system");
    return ESP_OK;
}

esp_err_t audio_controller_load_playlist(const audio_track_t *tracks, size_t num_tracks)
{
    ESP_RETURN_ON_FALSE(is_initialized, ESP_ERR_INVALID_STATE, TAG, "Not initialized");
    ESP_RETURN_ON_FALSE(tracks && num_tracks > 0, ESP_ERR_INVALID_ARG, TAG, "Invalid tracks");
    
    if (xSemaphoreTake(player_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    
    // Liberar lista de reproducción existente
    if (playlist) {
        free(playlist);
    }
    
    // Asignar y copiar nueva lista de reproducción
    playlist = malloc(sizeof(audio_track_t) * num_tracks);
    if (!playlist) {
        xSemaphoreGive(player_mutex);
        return ESP_ERR_NO_MEM;
    }
    
    memcpy(playlist, tracks, sizeof(audio_track_t) * num_tracks);
    playlist_size = num_tracks;
    current_track_index = 0;
    
    xSemaphoreGive(player_mutex);
    
    ESP_LOGI(TAG, "Playlist loaded: %zu tracks", num_tracks);
    return ESP_OK;
}

esp_err_t audio_controller_send_event(audio_event_type_t event_type)
{
    ESP_RETURN_ON_FALSE(is_initialized, ESP_ERR_INVALID_STATE, TAG, "Not initialized");
    ESP_RETURN_ON_FALSE(event_type < AUDIO_EVENT_MAX, ESP_ERR_INVALID_ARG, TAG, "Invalid event type");
    
    audio_event_t event = {
        .type = event_type,
        .timestamp = xTaskGetTickCount()
    };
    
    BaseType_t result = xQueueSend(event_queue, &event, pdMS_TO_TICKS(100));
    if (result != pdPASS) {
        ESP_LOGW(TAG, "Failed to send event to queue (queue full?)");
        return ESP_ERR_TIMEOUT;
    }
    
    ESP_LOGD(TAG, "Event %d sent to queue", event_type);
    return ESP_OK;
}

// =============================================================================
// IMPLEMENTACIONES DE TAREAS PRIVADAS
// =============================================================================

static void audio_event_task(void *args)
{
    ESP_LOGI(TAG, "🎛️ Audio event task started");
    
    audio_event_t event;
    
    while (1) {
        // Esperar eventos de la cola
        if (xQueueReceive(event_queue, &event, portMAX_DELAY) == pdTRUE) {
            ESP_LOGD(TAG, "Processing event: %d", event.type);
            
            switch (event.type) {
                case AUDIO_EVENT_PLAY:
                    ESP_LOGI(TAG, "🎵 Processing PLAY event");
                    audio_controller_play_internal();
                    break;
                    
                case AUDIO_EVENT_PAUSE:
                    ESP_LOGI(TAG, "⏸️ Processing PAUSE event");
                    audio_controller_pause_internal();
                    break;
                    
                case AUDIO_EVENT_NEXT:
                    ESP_LOGI(TAG, "⏭️ Processing NEXT event");
                    audio_controller_next_internal();
                    break;
                    
                case AUDIO_EVENT_PREVIOUS:
                    ESP_LOGI(TAG, "⏮️ Processing PREVIOUS event");
                    audio_controller_previous_internal();
                    break;
                    
                case AUDIO_EVENT_VOLUME_UP:
                    ESP_LOGI(TAG, "🔊 Processing VOLUME UP event");
                    audio_controller_volume_up_internal();
                    break;
                    
                case AUDIO_EVENT_VOLUME_DOWN:
                    ESP_LOGI(TAG, "🔉 Processing VOLUME DOWN event");
                    audio_controller_volume_down_internal();
                    break;
                    
                case AUDIO_EVENT_STOP:
                    ESP_LOGI(TAG, "⏹️ Processing STOP event");
                    // Implementar stop si es necesario
                    break;
                    
                default:
                    ESP_LOGW(TAG, "Unknown event type: %d", event.type);
                    break;
            }
        }
    }
}

static esp_err_t audio_controller_play_internal(void)
{
    ESP_RETURN_ON_FALSE(is_initialized, ESP_ERR_INVALID_STATE, TAG, "Not initialized");
    ESP_RETURN_ON_FALSE(playlist && playlist_size > 0, ESP_ERR_INVALID_STATE, TAG, "No playlist");
    
    if (xSemaphoreTake(player_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    
    // Detener pista actual si está reproduciéndose
    stop_current_track = true;
    if (audio_task_handle) {
        xSemaphoreGive(player_mutex);
        
        // Wait for task to finish
        int timeout = 50;
        while (audio_task_handle && timeout-- > 0) {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
        
        if (audio_task_handle) {
            vTaskDelete(audio_task_handle);
            audio_task_handle = NULL;
        }
        
        if (xSemaphoreTake(player_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
            return ESP_ERR_TIMEOUT;
        }
    }
    
    // Start new track
    stop_current_track = false;
    is_paused = false;
    
    BaseType_t result = xTaskCreate(
        audio_play_task,
        "audio_play",
        4096,
        &playlist[current_track_index],
        1,
        &audio_task_handle
    );
    
    xSemaphoreGive(player_mutex);
    
    if (result != pdPASS) {
        ESP_LOGE(TAG, "Failed to create audio task");
        return ESP_FAIL;
    }
    
    logger_log_event(LOGGER_EVENT_PLAY);
    ESP_LOGI(TAG, "▶️ Playing: %s", playlist[current_track_index].name);
    return ESP_OK;
}

static esp_err_t audio_controller_pause_internal(void)
{
    ESP_RETURN_ON_FALSE(is_initialized, ESP_ERR_INVALID_STATE, TAG, "Not initialized");
    
    if (xSemaphoreTake(player_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    
    if (!is_paused && audio_task_handle) {
        is_paused = true;
        stop_current_track = true;
        
        xSemaphoreGive(player_mutex);
        
        // Wait for task to finish
        int timeout = 50;
        while (audio_task_handle && timeout-- > 0) {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
        
        if (audio_task_handle) {
            vTaskDelete(audio_task_handle);
            audio_task_handle = NULL;
        }
        
        logger_log_event(LOGGER_EVENT_PAUSE);
        ESP_LOGI(TAG, "⏸️ Paused: %s", playlist[current_track_index].name);
    } else {
        xSemaphoreGive(player_mutex);
    }
    
    return ESP_OK;
}

static esp_err_t audio_controller_next_internal(void)
{
    ESP_RETURN_ON_FALSE(is_initialized, ESP_ERR_INVALID_STATE, TAG, "Not initialized");
    ESP_RETURN_ON_FALSE(playlist && playlist_size > 0, ESP_ERR_INVALID_STATE, TAG, "No playlist");
    
    if (xSemaphoreTake(player_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    
    // Stop current track
    stop_current_track = true;
    bool was_playing = (audio_task_handle != NULL && !is_paused);
    is_paused = false;
    
    // Wait for current task to finish if it exists
    if (audio_task_handle) {
        xSemaphoreGive(player_mutex);
        
        int timeout = 50;
        while (audio_task_handle && timeout-- > 0) {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
        
        if (audio_task_handle) {
            vTaskDelete(audio_task_handle);
            audio_task_handle = NULL;
        }
        
        if (xSemaphoreTake(player_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
            return ESP_ERR_TIMEOUT;
        }
    }
    
    // Move to next track
    current_track_index = (current_track_index + 1) % playlist_size;
    
    // Si estaba reproduciendo, iniciar la nueva canción automáticamente
    if (was_playing) {
        stop_current_track = false;
        
        BaseType_t result = xTaskCreate(
            audio_play_task,
            "audio_play",
            4096,
            &playlist[current_track_index],
            1,
            &audio_task_handle
        );
        
        if (result != pdPASS) {
            xSemaphoreGive(player_mutex);
            ESP_LOGE(TAG, "Failed to create audio task for next track");
            return ESP_FAIL;
        }
    }
    
    xSemaphoreGive(player_mutex);
    
    logger_log_event(LOGGER_EVENT_NEXT);
    ESP_LOGI(TAG, "⏭️ Next: %s%s", playlist[current_track_index].name, 
             was_playing ? " (playing)" : " (ready)");
    
    return ESP_OK;
}

static esp_err_t audio_controller_previous_internal(void)
{
    ESP_RETURN_ON_FALSE(is_initialized, ESP_ERR_INVALID_STATE, TAG, "Not initialized");
    ESP_RETURN_ON_FALSE(playlist && playlist_size > 0, ESP_ERR_INVALID_STATE, TAG, "No playlist");
    
    if (xSemaphoreTake(player_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    
    // Stop current track
    stop_current_track = true;
    bool was_playing = (audio_task_handle != NULL && !is_paused);
    is_paused = false;
    
    // Wait for current task to finish if it exists
    if (audio_task_handle) {
        xSemaphoreGive(player_mutex);
        
        int timeout = 50;
        while (audio_task_handle && timeout-- > 0) {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
        
        if (audio_task_handle) {
            vTaskDelete(audio_task_handle);
            audio_task_handle = NULL;
        }
        
        if (xSemaphoreTake(player_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
            return ESP_ERR_TIMEOUT;
        }
    }
    
    // Move to previous track
    current_track_index = (current_track_index - 1 + playlist_size) % playlist_size;
    
    // Si estaba reproduciendo, iniciar la nueva canción automáticamente
    if (was_playing) {
        stop_current_track = false;
        
        BaseType_t result = xTaskCreate(
            audio_play_task,
            "audio_play",
            4096,
            &playlist[current_track_index],
            1,
            &audio_task_handle
        );
        
        if (result != pdPASS) {
            xSemaphoreGive(player_mutex);
            ESP_LOGE(TAG, "Failed to create audio task for previous track");
            return ESP_FAIL;
        }
    }
    
    xSemaphoreGive(player_mutex);
    
    logger_log_event(LOGGER_EVENT_PREVIOUS);
    ESP_LOGI(TAG, "⏮️ Previous: %s%s", playlist[current_track_index].name,
             was_playing ? " (playing)" : " (ready)");
    
    return ESP_OK;
}

static void audio_play_task(void *args)
{
    audio_track_t *track = (audio_track_t *)args;
    if (!track || !track->data || track->size == 0) {
        ESP_LOGE(TAG, "Invalid track data");
        audio_task_handle = NULL;
        vTaskDelete(NULL);
        return;
    }
    
    ESP_LOGI(TAG, "🎵 Playing: %s (%zu bytes) - LOOP MODE", track->name, track->size);
    
    // Loop infinito hasta que se pare la canción
    while (!stop_current_track && !is_paused) {
        size_t bytes_written = 0;
        const uint8_t *data_ptr = track->data;
        size_t remaining = track->size;
        
        // Write audio data in chunks
        while (!stop_current_track && !is_paused && remaining > 0) {
            size_t chunk_size = (remaining > 1024) ? 1024 : remaining;
            
            esp_err_t ret = i2s_driver_write(tx_handle, data_ptr, chunk_size, &bytes_written);
            if (ret != ESP_OK || bytes_written == 0) {
                ESP_LOGE(TAG, "Failed to write audio data");
                break;
            }
            
            data_ptr += bytes_written;
            remaining -= bytes_written;
            
            // Small delay to prevent overwhelming I2S
            vTaskDelay(pdMS_TO_TICKS(1));
        }
        
        // Si terminó la canción completa y no se pidió parar, reiniciar
        if (remaining == 0 && !stop_current_track && !is_paused) {
            ESP_LOGD(TAG, "🔄 Looping: %s", track->name);
            // Pequeña pausa entre loops para evitar clicks
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }
    
    ESP_LOGI(TAG, "🎵 Stopped: %s", track->name);
    
    audio_task_handle = NULL;
    vTaskDelete(NULL);
}

static esp_err_t audio_controller_volume_up_internal(void)
{
    ESP_RETURN_ON_FALSE(is_initialized, ESP_ERR_INVALID_STATE, TAG, "Not initialized");
    
    if (xSemaphoreTake(player_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    
    // Incrementar volumen en pasos de 10, máximo 100
    if (current_volume < 100) {
        current_volume += 10;
        if (current_volume > 100) {
            current_volume = 100;
        }
        
        esp_err_t ret = es8311_codec_set_volume(current_volume);
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "🔊 Volume UP: %d", current_volume);
            logger_log_event(LOGGER_EVENT_VOLUME_UP);
        } else {
            ESP_LOGE(TAG, "Failed to set volume: %s", esp_err_to_name(ret));
        }
    } else {
        ESP_LOGI(TAG, "🔊 Volume already at maximum: %d", current_volume);
    }
    
    xSemaphoreGive(player_mutex);
    return ESP_OK;
}

static esp_err_t audio_controller_volume_down_internal(void)
{
    ESP_RETURN_ON_FALSE(is_initialized, ESP_ERR_INVALID_STATE, TAG, "Not initialized");
    
    if (xSemaphoreTake(player_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    
    // Decrementar volumen en pasos de 10, mínimo 0
    if (current_volume > 0) {
        if (current_volume >= 10) {
            current_volume -= 10;
        } else {
            current_volume = 0;
        }
        
        esp_err_t ret = es8311_codec_set_volume(current_volume);
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "🔉 Volume DOWN: %d", current_volume);
            logger_log_event(LOGGER_EVENT_VOLUME_DOWN);
        } else {
            ESP_LOGE(TAG, "Failed to set volume: %s", esp_err_to_name(ret));
        }
    } else {
        ESP_LOGI(TAG, "🔉 Volume already at minimum: %d", current_volume);
    }
    
    xSemaphoreGive(player_mutex);
    return ESP_OK;
}
