# Logger de Eventos de Reproducción

Este componente implementa un sistema de logging para eventos de reproducción usando un buffer circular almacenado en memoria no volátil (NVS) de ESP32-S2.

## Características

- ✅ **Buffer Circular**: Almacena hasta 20 eventos en un buffer circular
- 💾 **Persistencia NVS**: Los eventos se mantienen después de reinicios del sistema
- 🔒 **Thread-Safe**: Protegido con mutex para uso en aplicaciones multitarea
- ⚡ **Eficiente**: Operaciones optimizadas para sistemas embebidos
- 📊 **Timestamps**: Cada evento incluye timestamp preciso en microsegundos
- 🔢 **Numeración Secuencial**: Cada evento tiene un número de secuencia único

## Tipos de Eventos Soportados

```c
typedef enum {
    LOGGER_EVENT_PLAY = 0,      // Reproducir
    LOGGER_EVENT_PAUSE,         // Pausar
    LOGGER_EVENT_NEXT,          // Siguiente pista
    LOGGER_EVENT_PREVIOUS,      // Pista anterior
    LOGGER_EVENT_STOP           // Detener
} logger_event_type_t;
```

## Uso Básico

### 1. Inicialización

```c
#include "logger.h"

void app_main(void)
{
    // Inicializar el logger
    esp_err_t ret = logger_init();
    if (ret != ESP_OK) {
        ESP_LOGE("APP", "Failed to initialize logger: %s", esp_err_to_name(ret));
        return;
    }
    
    ESP_LOGI("APP", "Logger initialized successfully");
}
```

### 2. Registrar Eventos

```c
// Registrar evento de reproducción
logger_log_event(LOGGER_EVENT_PLAY);

// Registrar evento de pausa
logger_log_event(LOGGER_EVENT_PAUSE);

// Registrar evento de siguiente pista
logger_log_event(LOGGER_EVENT_NEXT);

// Registrar evento de pista anterior
logger_log_event(LOGGER_EVENT_PREVIOUS);

// Registrar evento de detener
logger_log_event(LOGGER_EVENT_STOP);
```

### 3. Consultar Eventos

```c
// Obtener todos los eventos
logger_event_t events[LOGGER_BUFFER_SIZE];
size_t event_count;
esp_err_t ret = logger_get_events(events, LOGGER_BUFFER_SIZE, &event_count);

if (ret == ESP_OK) {
    printf("Total events: %zu\n", event_count);
    for (size_t i = 0; i < event_count; i++) {
        printf("Event %zu: %s (seq: %lu, time: %llu us)\n",
               i + 1,
               logger_event_type_to_string(events[i].type),
               events[i].sequence_number,
               events[i].timestamp);
    }
}

// Obtener el último evento
logger_event_t last_event;
ret = logger_get_last_event(&last_event);
if (ret == ESP_OK) {
    printf("Last event: %s\n", logger_event_type_to_string(last_event.type));
}
```

### 4. Utilidades

```c
// Imprimir todos los eventos por consola
logger_print_events();

// Limpiar todos los eventos
logger_clear_events();

// Convertir tipo de evento a string
const char* event_str = logger_event_type_to_string(LOGGER_EVENT_PLAY);
printf("Event type: %s\n", event_str); // Output: "Event type: PLAY"
```

### 5. Deinicialización

```c
// Al final de la aplicación (opcional - se hace automáticamente)
logger_deinit();
```

## Estructura del Evento

```c
typedef struct {
    logger_event_type_t type;           // Tipo de evento
    uint64_t timestamp;                 // Timestamp en microsegundos desde boot
    uint32_t sequence_number;           // Número de secuencia único
} logger_event_t;
```

## Comportamiento del Buffer Circular

- **Capacidad**: 20 eventos máximo
- **Funcionamiento**: Cuando el buffer se llena, los eventos más antiguos son sobrescritos
- **Persistencia**: El estado completo del buffer se guarda automáticamente en NVS
- **Recuperación**: Al reiniciar, el buffer se restaura desde NVS con todos los eventos

## Almacenamiento NVS

El logger utiliza las siguientes claves en NVS:

- **Namespace**: `"logger"`
- **Buffer de eventos**: `"events_buf"`
- **Índice head**: `"head_idx"`
- **Índice tail**: `"tail_idx"`
- **Contador de eventos**: `"event_count"`
- **Contador de secuencia**: `"seq_counter"`

## API Reference

### Funciones Principales

| Función | Descripción | Retorno |
|---------|-------------|---------|
| `logger_init()` | Inicializa el sistema de logger | `ESP_OK` en éxito |
| `logger_deinit()` | Deinicializa el logger y guarda en NVS | `ESP_OK` en éxito |
| `logger_log_event(type)` | Registra un evento | `ESP_OK` en éxito |
| `logger_get_events(out, max, count)` | Obtiene todos los eventos | `ESP_OK` en éxito |
| `logger_get_last_event(out)` | Obtiene el último evento | `ESP_OK` en éxito |
| `logger_clear_events()` | Limpia todos los eventos | `ESP_OK` en éxito |
| `logger_print_events()` | Imprime eventos por consola | void |

### Funciones de Utilidad

| Función | Descripción | Retorno |
|---------|-------------|---------|
| `logger_event_type_to_string(type)` | Convierte tipo a string | `const char*` |
| `logger_save_to_nvs()` | Guarda manualmente en NVS | `ESP_OK` en éxito |
| `logger_load_from_nvs()` | Carga manualmente desde NVS | `ESP_OK` en éxito |

## Ejemplo de Salida

```
=== LOGGER EVENTS ===
Total events: 8
Buffer size: 20
Current count: 8
Head: 8, Tail: 0
Sequence counter: 8

Events (oldest to newest):
[1] Seq:1, Type:PLAY, Time:1234567890 us
[2] Seq:2, Type:NEXT, Time:1234568890 us
[3] Seq:3, Type:NEXT, Time:1234569890 us
[4] Seq:4, Type:PAUSE, Time:1234570890 us
[5] Seq:5, Type:PLAY, Time:1234571890 us
[6] Seq:6, Type:PREVIOUS, Time:1234572890 us
[7] Seq:7, Type:STOP, Time:1234573890 us
[8] Seq:8, Type:PLAY, Time:1234574890 us
====================
```

## Configuración

Las siguientes constantes pueden modificarse en `logger.h`:

```c
#define LOGGER_BUFFER_SIZE 20           // Tamaño del buffer circular
#define LOGGER_NVS_NAMESPACE "logger"   // Namespace de NVS
```

## Consideraciones de Memoria

- **RAM**: ~240 bytes para el buffer + overhead del mutex
- **NVS**: ~300 bytes para persistencia
- **Stack**: ~1KB recomendado para tareas que usen el logger

## Ejemplo Completo

Ver el archivo `examples/logger_example.c` para un ejemplo completo de uso.

## Thread Safety

Todas las funciones del logger son thread-safe y pueden ser llamadas desde múltiples tareas simultáneamente. El componente utiliza un mutex interno para proteger el acceso al buffer.

## Manejo de Errores

Todas las funciones retornan códigos de error ESP-IDF estándar:

- `ESP_OK`: Operación exitosa
- `ESP_ERR_INVALID_STATE`: Logger no inicializado
- `ESP_ERR_INVALID_ARG`: Parámetros inválidos
- `ESP_ERR_TIMEOUT`: Timeout al obtener mutex
- `ESP_ERR_NOT_FOUND`: No hay eventos disponibles
