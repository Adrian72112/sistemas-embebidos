# Componente Logger

Un logger de eventos de audio thread-safe para ESP32 con almacenamiento persistente usando sistema de archivos SPIFFS.

## Características

- **Buffer Circular**: Almacena los últimos 20 eventos de audio en un buffer circular
- **Thread Safety**: Usa mutex de FreeRTOS para acceso concurrente seguro
- **Almacenamiento Asíncrono**: Guardado no bloqueante usando tarea de FreeRTOS de baja prioridad
- **Almacenamiento Persistente**: Guarda automáticamente eventos al sistema de archivos SPIFFS
- **Wear Leveling**: Construido sobre SPIFFS para distribución de desgaste de flash
- **Auto-Guardado**: Guarda en background sin afectar el rendimiento de audio
- **Tipos de Eventos**: Soporta eventos PLAY, PAUSE, NEXT, PREVIOUS, STOP
- **Timestamps**: Cada evento incluye timestamp con precisión de microsegundos
- **Números de Secuencia**: Numeración secuencial global para ordenamiento de eventos

## Arquitectura

```
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│   Aplicación    │───▶│  API Logger     │───▶│  Buffer Circular│
└─────────────────┘    └─────────────────┘    └─────────────────┘
                                │                       │
                                │                       │
                                ▼                       ▼
                       ┌─────────────────┐    ┌─────────────────┐
                       │ Tarea Guardado  │    │  Mutex FreeRTOS │
                       │ (Baja Prioridad)│    └─────────────────┘
                       └─────────────────┘
                                │
                                ▼
                       ┌─────────────────┐    ┌─────────────────┐
                       │  SPIFFS VFS     │    │  Cola FreeRTOS  │
                       └─────────────────┘    └─────────────────┘
                                │
                                ▼
                       ┌─────────────────┐
                       │ Almac. Flash    │
                       └─────────────────┘
```

## Uso

### Uso Básico

```c
#include "logger.h"

// Inicializar el logger
esp_err_t err = logger_init();
if (err != ESP_OK) {
    ESP_LOGE("APP", "Falló al inicializar logger");
    return;
}

// Registrar eventos
logger_log_event(LOGGER_EVENT_PLAY);
logger_log_event(LOGGER_EVENT_PAUSE);
logger_log_event(LOGGER_EVENT_STOP);

// Apagado limpio
logger_deinit();
```

### Uso Avanzado

```c
// Obtener contador total de eventos
uint32_t total_events = logger_get_event_count();

// Obtener una copia del buffer circular
logger_ring_buffer_t buffer;
err = logger_get_ring_buffer(&buffer);

// Obtener evento específico por índice
logger_event_t event;
err = logger_get_event_by_index(0, &event); // Obtener evento más antiguo

// Guardado/carga manual (normalmente automático)
logger_save_to_file();
logger_load_from_file();
```

## Configuración

El logger se configura a través de defines en `logger.h`:

- `LOGGER_RING_BUFFER_SIZE`: Máximo de eventos en buffer (por defecto: 20)
- `LOGGER_FILE_PATH`: Ruta del archivo SPIFFS (por defecto: "/spiffs/logger_events.bin")

## Tipos de Eventos

```c
typedef enum {
    LOGGER_EVENT_PLAY = 0,      // Reproducción iniciada
    LOGGER_EVENT_PAUSE,         // Reproducción pausada
    LOGGER_EVENT_NEXT,          // Siguiente pista seleccionada
    LOGGER_EVENT_PREVIOUS,      // Pista anterior seleccionada
    LOGGER_EVENT_STOP           // Reproducción detenida
} logger_event_type_t;
```

## Estructura de Eventos

```c
typedef struct {
    logger_event_type_t type;   // Tipo de evento
    uint64_t timestamp;         // Timestamp en microsegundos desde el arranque
    uint32_t sequence_number;   // Número de secuencia global
} logger_event_t;
```

## Comportamiento del Buffer Circular

El buffer circular opera como un buffer circular:

1. **No Lleno**: Los eventos se añaden secuencialmente desde el índice 0
2. **Lleno**: Los nuevos eventos sobrescriben los eventos más antiguos (comportamiento FIFO)
3. **Indexación**: El índice 0 siempre representa el evento más antiguo en el buffer
4. **Capacidad**: Máximo 20 eventos (configurable)

## Dependencias

Añade a tu `CMakeLists.txt` del componente:

```cmake
idf_component_register(
    SRCS "tus_fuentes.c"
    INCLUDE_DIRS "include"
    REQUIRES logger
)
```

El componente logger requiere:
- `spiffs`: Soporte para sistema de archivos SPIFFS
- `wear_levelling`: Distribución de desgaste de flash
- `esp_timer`: Timer de alta resolución
- `freertos`: FreeRTOS para soporte de mutex

## Tabla de Particiones

Asegúrate de que tu tabla de particiones incluye una partición SPIFFS:

```csv
# Name, Type, SubType, Offset, Size, Flags
nvs,      data, nvs,     0x9000,  0x6000,
phy_init, data, phy,     0xf000,  0x1000,
factory,  app,  factory, 0x10000, 0x180000,
spiffs,   data, spiffs,  ,        0x70000,
```

## Manejo de Errores

Todas las funciones retornan códigos de estado `esp_err_t`:

- `ESP_OK`: Éxito
- `ESP_ERR_INVALID_STATE`: Logger no inicializado
- `ESP_ERR_INVALID_ARG`: Argumento inválido
- `ESP_ERR_NO_MEM`: Falló la asignación de memoria
- `ESP_ERR_TIMEOUT`: Timeout del mutex
- `ESP_ERR_NOT_FOUND`: Falló la operación de archivo
- `ESP_FAIL`: Fallo general

## Thread Safety

El logger es completamente thread-safe y puede ser llamado desde:
- Tarea principal
- Tareas de FreeRTOS
- Callbacks de timers
- Rutinas de servicio de interrupción (con precaución)

## Consideraciones de Rendimiento

- **Logging No Bloqueante**: `logger_log_event()` retorna inmediatamente sin afectar audio
- **Guardado Asíncrono**: Una tarea de baja prioridad maneja el guardado a flash
- **Cola de Comandos**: Sistema de cola previene pérdida de datos durante alta carga
- **SPIFFS**: Provee distribución de desgaste pero tiene ciclos de escritura finitos
- **Prioridad Baja**: La tarea de guardado no interfiere con tareas críticas de audio
- **Timeout del Mutex**: Configurado a 100ms para todas las operaciones de lectura

## Uso de Memoria

- **RAM**: ~500 bytes para buffer circular + mutex + tarea + cola (~2KB total)
- **Flash**: ~500 bytes por operación de guardado en SPIFFS
- **Código**: ~10KB de tamaño de código compilado
- **Stack de Tarea**: 4KB para la tarea de guardado asíncrono

## Ejemplo de Salida

```
=== LOGGER INFO ===
Initialized: YES
Ring buffer size: 20
Storage: SPIFFS (Async)
File path: /spiffs/logger_events.bin
Save task: RUNNING
Save queue: CREATED
Ring buffer count: 5
Ring buffer head: 5
Total events logged: 15
===================

=== EVENT HISTORY ===
Ring buffer capacity: 20
Current count: 5 (only last 20 events stored)
Total events since init: 15 (counter only)
Storage: Only last 20 events are persisted to flash

Events stored in memory (oldest to newest):
Index | Seq# | Event      | Timestamp (μs)
------|------|------------|----------------
    0 |   11 | PLAY       |      12345678901
    1 |   12 | PAUSE      |      12345789012
    2 |   13 | PLAY       |      12345890123
    3 |   14 | NEXT       |      12345901234
    4 |   15 | STOP       |      12345912345
======================
```

## Limitaciones de Almacenamiento

- **Buffer en RAM**: Solo mantiene los últimos 20 eventos en memoria
- **Persistencia en Flash**: Solo se guardan los últimos 20 eventos al archivo SPIFFS
- **Contador Global**: `total_events` es solo un contador, no indica cuántos eventos están almacenados
- **Auto-Reset**: El contador se resetea automáticamente cuando supera 10,000 para prevenir overflow
- **Gestión de Espacio**: El sistema garantiza que nunca se almacenen más de 20 eventos en flash

## Notas Importantes

1. **Comportamiento Circular**: Cuando el buffer está lleno, los nuevos eventos sobrescriben los más antiguos
2. **Persistencia Limitada**: Solo los últimos 20 eventos se mantienen tanto en RAM como en flash
3. **Thread Safety**: Todas las operaciones están protegidas por mutex
4. **Guardado Asíncrono**: Los eventos se guardan en background sin bloquear el audio
5. **Gestión de Contador**: El contador total se puede resetear manualmente o automáticamente
6. **Prioridad de Tarea**: La tarea de guardado tiene prioridad 1 (muy baja) para no interferir
7. **Cola de Guardado**: Si la cola se llena, los comandos se descartan pero no afecta la funcionalidad
