#include "esp_err.h"
#include <stdbool.h>


/**
 * @brief Tipos de eventos de audio
 */
typedef enum {
    AUDIO_EVENT_PLAY,       /*!< Evento de reproducción */
    AUDIO_EVENT_PAUSE,      /*!< Evento de pausa */
    AUDIO_EVENT_NEXT,       /*!< Evento de siguiente pista */
    AUDIO_EVENT_PREVIOUS,   /*!< Evento de pista anterior */
    AUDIO_EVENT_VOLUME_UP,  /*!< Evento de subir volumen */
    AUDIO_EVENT_VOLUME_DOWN,/*!< Evento de bajar volumen */
    AUDIO_EVENT_STOP,       /*!< Evento de detener */
    AUDIO_EVENT_MAX         /*!< Marcador de eventos máximos */
} audio_event_type_t;

/**
 * @brief Estructura de evento de audio
 */
typedef struct {
    audio_event_type_t type;    /*!< Tipo de evento */
    uint32_t timestamp;         /*!< Timestamp del evento (opcional) */
} audio_event_t;

/**
 * @brief Estructura de pista de audio
 */
typedef struct {
    const uint8_t *data;        /*!< Puntero a los datos de audio */
    size_t size;                /*!< Tamaño de los datos de audio en bytes */
    const char *name;           /*!< Nombre de la pista */
} audio_track_t;

/**
 * @brief Configuración del controlador de audio
 */
typedef struct {
    uint32_t sample_rate;           /*!< Frecuencia de muestreo en Hz */
    uint8_t volume;                 /*!< Nivel de volumen 0-100 */
    bool microphone_enabled;        /*!< Habilitar micrófono */
} audio_controller_config_t;

/**
 * @brief Configuración predeterminada del controlador de audio
 */
#define AUDIO_CONTROLLER_DEFAULT_CONFIG() { \
    .sample_rate = 8000, \
    .volume = 50, \
    .microphone_enabled = false \
}

/**
 * @brief Inicializar el controlador de audio
 * 
 * @param config Configuración del controlador de audio
 * @return esp_err_t ESP_OK en caso de éxito
 */
esp_err_t audio_controller_init(const audio_controller_config_t *config);

/**
 * @brief Cargar lista de reproducción con pistas de audio
 * 
 * @param tracks Array de pistas de audio
 * @param num_tracks Número de pistas en la lista de reproducción
 * @return esp_err_t ESP_OK en caso de éxito
 */
esp_err_t audio_controller_load_playlist(const audio_track_t *tracks, size_t num_tracks);

/**
 * @brief Enviar evento al controlador de audio
 * 
 * @param event_type Tipo de evento a enviar
 * @return esp_err_t ESP_OK en caso de éxito
 */
esp_err_t audio_controller_send_event(audio_event_type_t event_type);
