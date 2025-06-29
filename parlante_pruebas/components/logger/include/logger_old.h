#ifndef LOGGER_H
#define LOGGER_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define LOGGER_RING_BUFFER_SIZE 20
#define LOGGER_NVS_NAMESPACE "logger_storage"
#define LOGGER_NVS_KEY_RING_BUFFER "ring_buffer"

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
 * @brief Estructura del ring buffer para eventos
 */
typedef struct {
    logger_event_t events[LOGGER_RING_BUFFER_SIZE];
    uint8_t head;                       // Índice del próximo elemento a escribir
    uint8_t count;                      // Número de elementos actuales
    uint32_t total_events;              // Total de eventos desde el inicio
} logger_ring_buffer_t;

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

/**
 * @brief Obtiene una copia del ring buffer de eventos
 * 
 * @param buffer Puntero donde copiar el ring buffer
 * @return ESP_OK si se obtuvo exitosamente
 */
esp_err_t logger_get_ring_buffer(logger_ring_buffer_t* buffer);

/**
 * @brief Imprime el historial de eventos del ring buffer
 */
void logger_print_event_history(void);

/**
 * @brief Obtiene un evento específico del ring buffer por índice
 * 
 * @param index Índice del evento (0 = más antiguo, count-1 = más reciente)
 * @param event Puntero donde copiar el evento
 * @return ESP_OK si se obtuvo exitosamente
 */
esp_err_t logger_get_event_by_index(uint8_t index, logger_event_t* event);

/**
 * @brief Guarda el ring buffer a NVS (útil para persistencia manual)
 * 
 * @return ESP_OK si se guardó exitosamente
 */
esp_err_t logger_save_ring_buffer_to_nvs(void);

/**
 * @brief Carga el ring buffer desde NVS
 * 
 * @return ESP_OK si se cargó exitosamente
 */
esp_err_t logger_load_ring_buffer_from_nvs(void);

/**
 * @brief Función de debug para verificar el estado de NVS
 */
void logger_debug_nvs_info(void);

#ifdef __cplusplus
}
#endif

#endif // LOGGER_H