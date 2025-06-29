#ifndef LOGGER_H
#define LOGGER_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define LOGGER_BUFFER_SIZE 20
#define LOGGER_NVS_NAMESPACE "logger_storage"
#define LOGGER_NVS_KEY_COUNTER "event_counter"

/**
 * @brief Tipos de eventos de reproducción
 */
typedef enum {
    LOGGER_EVENT_PLAY = 0,
    LOGGER_EVENT_PAUSE,
    LOGGER_EVENT_NEXT,
    LOGGER_EVENT_PREVIOUS,
    LOGGER_EVENT_STOP
} logger_event_type_t;

/**
 * @brief Estructura para almacenar un evento de reproducción
 */
typedef struct {
    logger_event_type_t type;           // Tipo de evento
    uint64_t timestamp;                 // Timestamp del evento (microsegundos desde boot)
    uint32_t sequence_number;           // Número de secuencia del evento
} logger_event_t;

/**
 * @brief Inicializa el sistema de logger
 * 
 * @return ESP_OK si la inicialización fue exitosa
 */
esp_err_t logger_init(void);

/**
 * @brief Deinicializa el sistema de logger
 * 
 * @return ESP_OK si la deinicialización fue exitosa
 */
esp_err_t logger_deinit(void);

/**
 * @brief Registra un evento de reproducción
 * 
 * @param event_type Tipo de evento a registrar
 * @return ESP_OK si el evento fue registrado exitosamente
 */
esp_err_t logger_log_event(logger_event_type_t event_type);

/**
 * @brief Obtiene el contador de eventos actual
 * 
 * @return Número total de eventos registrados
 */
uint32_t logger_get_event_count(void);

/**
 * @brief Convierte un tipo de evento a string
 * 
 * @param event_type Tipo de evento
 * @return String representando el tipo de evento
 */
const char* logger_event_type_to_string(logger_event_type_t event_type);

/**
 * @brief Imprime información del logger
 */
void logger_print_info(void);

#ifdef __cplusplus
}
#endif

#endif // LOGGER_H