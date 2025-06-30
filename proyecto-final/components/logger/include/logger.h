/**
 * @file logger.h
 * @brief Logger de Eventos de Audio ESP32 con Persistencia SPIFFS
 * 
 * Este componente logger proporciona un buffer circular thread-safe para almacenar
 * eventos de reproducción de audio (play, pause, next, previous, stop) con 
 * almacenamiento persistente usando sistema de archivos SPIFFS y wear leveling.
 * 
 * Características:
 * - Buffer circular con 20 slots para eventos
 * - Operaciones thread-safe usando mutex de FreeRTOS
 * - Almacenamiento persistente en sistema de archivos SPIFFS
 * - Guardado automático al apagar el sistema
 * - Numeración secuencial de eventos y timestamps
 * 
 * @author Proyecto ESP32
 * @version 1.0
 * @date 2025
 */

#ifndef LOGGER_H
#define LOGGER_H

#include <stdint.h>
#include <stddef.h>
#include "esp_err.h"

/* Constantes de Configuración */
#define LOGGER_RING_BUFFER_SIZE 20                          ///< Número máximo de eventos en el buffer circular
#define LOGGER_FILE_PATH "/spiffs/logger_events.bin"        ///< Ruta del archivo SPIFFS para persistencia

/**
 * @brief Tipos de eventos de reproducción de audio
 */
typedef enum {
    LOGGER_EVENT_PLAY = 0,      ///< Reproducción iniciada
    LOGGER_EVENT_PAUSE,         ///< Reproducción pausada
    LOGGER_EVENT_NEXT,          ///< Siguiente pista seleccionada
    LOGGER_EVENT_PREVIOUS,      ///< Pista anterior seleccionada
    LOGGER_EVENT_STOP           ///< Reproducción detenida
} logger_event_type_t;

/**
 * @brief Estructura de evento individual
 */
typedef struct {
    logger_event_type_t type;   ///< Tipo de evento
    uint64_t timestamp;         ///< Timestamp en microsegundos desde el arranque
    uint32_t sequence_number;   ///< Número de secuencia global
} logger_event_t;

/**
 * @brief Estructura del buffer circular para almacenar eventos
 */
typedef struct {
    logger_event_t events[LOGGER_RING_BUFFER_SIZE];    ///< Array de eventos
    uint8_t head;                                       ///< Índice para la siguiente inserción
    uint8_t count;                                      ///< Número actual de eventos
    uint32_t total_events;                             ///< Total de eventos desde la inicialización
} logger_ring_buffer_t;

/* Funciones de API Pública */

/**
 * @brief Inicializar el sistema logger
 * 
 * Inicializa el sistema de archivos SPIFFS, crea mutex para thread safety,
 * carga eventos existentes desde almacenamiento persistente, y registra
 * handler de apagado para guardado automático.
 * 
 * @return ESP_OK en éxito, código de error en fallo
 */
esp_err_t logger_init(void);

/**
 * @brief Desinicializar el sistema logger
 * 
 * Guarda el buffer circular actual al almacenamiento persistente, limpia
 * los recursos del mutex y SPIFFS, desregistra el handler de apagado.
 * 
 * @return ESP_OK en éxito, código de error en fallo
 */
esp_err_t logger_deinit(void);

/**
 * @brief Registrar un evento de reproducción de audio
 * 
 * Añade un nuevo evento al buffer circular con timestamp y número de secuencia.
 * Guarda automáticamente al almacenamiento persistente después de cada evento.
 * 
 * @param event_type Tipo de evento de audio a registrar
 * @return ESP_OK en éxito, código de error en fallo
 */
esp_err_t logger_log_event(logger_event_type_t event_type);

/**
 * @brief Obtener el número total de eventos registrados desde la inicialización
 * 
 * @return Contador total de eventos, 0 si el logger no está inicializado
 */
uint32_t logger_get_event_count(void);

/**
 * @brief Convertir tipo de evento a representación de string
 * 
 * @param event_type Tipo de evento a convertir
 * @return Representación en string del tipo de evento
 */
const char* logger_event_type_to_string(logger_event_type_t event_type);

/* Funciones de Acceso al Buffer Circular */

/**
 * @brief Obtener una copia del buffer circular actual
 * 
 * Operación thread-safe que copia toda la estructura del buffer circular.
 * 
 * @param buffer Puntero a la estructura de buffer a llenar
 * @return ESP_OK en éxito, ESP_ERR_INVALID_ARG si buffer es NULL
 */
esp_err_t logger_get_ring_buffer(logger_ring_buffer_t* buffer);

/**
 * @brief Obtener un evento específico por índice
 * 
 * Recupera un evento del buffer circular por su índice relativo
 * (0 = evento más antiguo, count-1 = evento más reciente).
 * 
 * @param index Índice del evento (0 a count-1)
 * @param event Puntero a la estructura de evento a llenar
 * @return ESP_OK en éxito, ESP_ERR_NOT_FOUND si el índice está fuera de rango
 */
esp_err_t logger_get_event_by_index(uint8_t index, logger_event_t* event);

/* Funciones de Persistencia */

/**
 * @brief Guardar manualmente el buffer circular a SPIFFS
 * 
 * Fuerza el guardado inmediato del estado actual del buffer circular al almacenamiento persistente.
 * Normalmente se llama automáticamente después de cada evento y al apagar.
 * 
 * @return ESP_OK en éxito, código de error en fallo
 */
esp_err_t logger_save_to_file(void);

/**
 * @brief Cargar manualmente el buffer circular desde SPIFFS
 * 
 * Fuerza la recarga del buffer circular desde el almacenamiento persistente.
 * Normalmente se llama automáticamente durante la inicialización.
 * 
 * @return ESP_OK en éxito, código de error en fallo
 */
esp_err_t logger_load_from_file(void);

/**
 * @brief Obtener todos los eventos en orden cronológico (más antiguos primero)
 * 
 * Recupera todos los eventos del buffer circular ordenados por su número de secuencia.
 * Los eventos se devuelven en un array que debe ser liberado por el llamador.
 * 
 * @param events Puntero donde se almacenará el array de eventos (será asignado por malloc)
 * @param count Puntero donde se almacenará el número de eventos recuperados
 * @return ESP_OK en éxito, ESP_ERR_NO_MEM si no hay memoria, ESP_ERR_INVALID_ARG si los parámetros son NULL
 */
esp_err_t logger_get_all_events_chronological(logger_event_t **events, uint8_t *count);

/**
 * @brief Convertir un evento a formato JSON para transmisión MQTT
 * 
 * Convierte un evento del logger a formato JSON con todos sus campos.
 * 
 * @param event Puntero al evento a convertir
 * @param json_buffer Buffer donde se escribirá el JSON (debe tener al menos 256 bytes)
 * @param buffer_size Tamaño del buffer JSON
 * @return ESP_OK en éxito, ESP_ERR_INVALID_ARG si los parámetros son NULL o buffer muy pequeño
 */
esp_err_t logger_event_to_json(const logger_event_t *event, char *json_buffer, size_t buffer_size);

#endif // LOGGER_H
