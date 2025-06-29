#ifndef LOGGER_H
#define LOGGER_H

#include <stdint.h>
#include <stddef.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// Configuración del logger
#define LOGGER_RING_BUFFER_SIZE 20
#define LOGGER_FILE_PATH "/spiffs/logger_events.bin"

// Tipos de eventos que puede loggear
typedef enum {
    LOGGER_EVENT_PLAY = 0,
    LOGGER_EVENT_PAUSE,
    LOGGER_EVENT_NEXT,
    LOGGER_EVENT_PREVIOUS,
    LOGGER_EVENT_STOP
} logger_event_type_t;

// Estructura para un evento individual
typedef struct {
    logger_event_type_t type;
    uint64_t timestamp;     // Timestamp en microsegundos
    uint32_t sequence_number; // Número de secuencia global
} logger_event_t;

// Estructura del ring buffer
typedef struct {
    logger_event_t events[LOGGER_RING_BUFFER_SIZE];
    uint8_t head;           // Índice donde se inserta el próximo evento
    uint8_t count;          // Número actual de eventos en el buffer
    uint32_t total_events;  // Contador total de eventos desde el inicio
} logger_ring_buffer_t;

// Funciones públicas del logger
esp_err_t logger_init(void);
esp_err_t logger_deinit(void);
esp_err_t logger_log_event(logger_event_type_t event_type);
uint32_t logger_get_event_count(void);
const char* logger_event_type_to_string(logger_event_type_t event_type);
void logger_print_info(void);

// Funciones para manejo del ring buffer
esp_err_t logger_get_ring_buffer(logger_ring_buffer_t* buffer);
esp_err_t logger_get_event_by_index(uint8_t index, logger_event_t* event);
void logger_print_event_history(void);

// Funciones para persistencia en SPIFFS
esp_err_t logger_save_to_file(void);
esp_err_t logger_load_from_file(void);

#ifdef __cplusplus
}
#endif

#endif // LOGGER_H
