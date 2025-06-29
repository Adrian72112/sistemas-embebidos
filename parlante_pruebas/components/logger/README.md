# Logger Component - Ring Buffer Implementation

Este componente implementa un sistema de logging para eventos de reproducción usando un **ring buffer circular de 20 espacios** con persistencia en memoria no volátil (NVS).

## Características

- ✅ **Ring Buffer Circular**: Almacena los últimos 20 eventos de reproducción
- ✅ **Persistencia NVS**: Los eventos persisten entre reinicios del sistema
- ✅ **Thread Safe**: Protegido con mutex para acceso concurrente
- ✅ **Timestamps Precisos**: Cada evento incluye timestamp en microsegundos
- ✅ **Wear Leveling**: Aprovecha el wear leveling automático de NVS
- ✅ **API Completa**: Funciones para leer, escribir y consultar eventos
- ✅ **Optimizado**: Escrituras solo al cerrar o manualmente (reduce wear)

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

## API Principal

### Inicialización
```c
esp_err_t logger_init(void);           // Inicializar logger
esp_err_t logger_deinit(void);         // Deinicializar (auto-save)
```

### Logging de Eventos
```c
esp_err_t logger_log_event(logger_event_type_t event_type);
uint32_t logger_get_event_count(void);  // Total de eventos desde inicio
```

### Consulta de Eventos
```c
// Obtener todo el ring buffer
esp_err_t logger_get_ring_buffer(logger_ring_buffer_t* buffer);

// Obtener evento específico por índice (0 = más antiguo)
esp_err_t logger_get_event_by_index(uint8_t index, logger_event_t* event);

// Imprimir historial completo
void logger_print_event_history(void);
void logger_print_info(void);
```

### Persistencia Manual
```c
esp_err_t logger_save_ring_buffer_to_nvs(void);   // Guardar manualmente
esp_err_t logger_load_ring_buffer_from_nvs(void); // Cargar manualmente
```

## Ejemplo de Uso Completo

```c
#include "logger.h"

void app_main(void) {
    // Inicializar logger
    ESP_ERROR_CHECK(logger_init());
    ESP_LOGI("APP", "Logger initialized successfully");
    
    // Mostrar eventos previos (persistentes)
    logger_print_event_history();
    
    // Registrar algunos eventos
    logger_log_event(LOGGER_EVENT_PLAY);
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    logger_log_event(LOGGER_EVENT_NEXT);
    logger_log_event(LOGGER_EVENT_PAUSE);
    
    // Ver información actual
    logger_print_info();
    
    // Obtener evento específico
    logger_event_t oldest_event;
    if (logger_get_event_by_index(0, &oldest_event) == ESP_OK) {
        printf("Oldest event: %s at %" PRIu64 " μs\n",
               logger_event_type_to_string(oldest_event.type),
               oldest_event.timestamp);
    }
    
    // Obtener total de eventos
    printf("Total events logged: %" PRIu32 "\n", logger_get_event_count());
    
    // Deinicializar (auto-save a NVS)
    logger_deinit();
}

## Estructura de Datos

### Evento Individual
```c
typedef struct {
    logger_event_type_t type;    // Tipo de evento
    uint64_t timestamp;          // Timestamp (μs desde boot)
    uint32_t sequence_number;    // Número de secuencia global
} logger_event_t;
```

### Ring Buffer
```c
typedef struct {
    logger_event_t events[20];   // Array circular de eventos
    uint8_t head;                // Índice del próximo slot
    uint8_t count;               // Número de eventos actuales (0-20)
    uint32_t total_events;       // Total de eventos desde inicio
} logger_ring_buffer_t;
```

## Comportamiento del Ring Buffer

### Buffer No Lleno (< 20 eventos)
```
Eventos: [A] [B] [C] [ ] [ ] ... [ ]
Índices:  0   1   2   3   4  ...  19
                     ↑
                   head
```

### Buffer Lleno (≥ 20 eventos)
```
Eventos: [T] [U] [V] [W] [X] ... [S]
Índices:  0   1   2   3   4  ...  19
              ↑                   ↑
            head               newest
         (oldest)
```

**Cuando el buffer se llena, los eventos más antiguos se sobrescriben automáticamente.**

## Configuración NVS

- **Namespace**: `"logger_storage"`
- **Key**: `"ring_buffer"`
- **Tamaño**: ~1KB por ring buffer completo
- **Persistencia**: Automática en `logger_deinit()` o manual con `logger_save_ring_buffer_to_nvs()`

## Dependencias CMake

```cmake
idf_component_register(
    SRCS "logger.c"
    INCLUDE_DIRS "include"
    REQUIRES nvs_flash esp_ringbuf esp_timer freertos
)
```

## Consideraciones de Rendimiento

- **RAM**: ~1KB para ring buffer + overhead de mutex
- **Flash**: Escrituras solo en persistencia (deinit o manual)
- **CPU**: Operaciones O(1) para agregar/leer eventos
- **Thread Safety**: Protegido con mutex (timeout 100ms)

## Ventajas vs Implementación Anterior

| Característica | Anterior (Counter) | Actual (Ring Buffer) |
|----------------|-------------------|----------------------|
| Almacenamiento | Solo contador | Eventos completos con timestamps |
| Persistencia | Cada evento | Solo al cerrar/manual |
| Historial | ❌ No | ✅ Últimos 20 eventos |
| Wear Leveling | Medio | Excelente |
| Información | Básica | Rica (tipo, tiempo, secuencia) |

## Testing

Ejecuta el ejemplo incluido para probar todas las funcionalidades:

```bash
# El ejemplo demuestra:
# - Carga de eventos persistentes
# - Logging de nuevos eventos  
# - Comportamiento circular
# - Persistencia automática
```

## Ejemplo de Salida

```
=== LOGGER INFO ===
Initialized: YES
Ring buffer size: 20
Ring buffer count: 8
Ring buffer head: 8
Total events logged: 25
NVS Namespace: logger_storage
NVS Key Ring Buffer: ring_buffer
===================

=== EVENT HISTORY ===
Ring buffer capacity: 20
Current count: 8
Total events since init: 25

Events (oldest to newest):
Index | Seq# | Event      | Timestamp (μs)
------|------|------------|----------------
    0 |   18 | PLAY       |   12345678901234
    1 |   19 | NEXT       |   12345678902234
    2 |   20 | NEXT       |   12345678903234
    3 |   21 | PAUSE      |   12345678904234
    4 |   22 | PLAY       |   12345678905234
    5 |   23 | PREVIOUS   |   12345678906234
    6 |   24 | STOP       |   12345678907234
    7 |   25 | PLAY       |   12345678908234
======================
```

## Troubleshooting

### Error: Ring buffer mutex timeout
- **Causa**: Acceso concurrente intenso
- **Solución**: Verificar que no hay locks largos

### Error: NVS write failed
- **Causa**: Partición NVS llena
- **Solución**: `nvs_flash_erase()` en desarrollo

### Warning: Ring buffer not found in NVS
- **Normal**: Primera ejecución, se inicializa vacío
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

Ver el archivo `examples/logger_example.c` para un ejemplo completo que demuestra:
- Inicialización y carga de eventos persistentes
- Logging de eventos de diferentes tipos
- Consulta de eventos individuales y historial completo
- Comportamiento del ring buffer circular
- Persistencia automática y manual

---

**¡Implementación completada!** 🎉

El logger ahora utiliza un ring buffer circular optimizado con persistencia inteligente en NVS, eliminando la redundancia del contador anterior y aprovechando al máximo las capacidades del ESP32-S2.

## Thread Safety

Todas las funciones del logger son thread-safe y pueden ser llamadas desde múltiples tareas simultáneamente. El componente utiliza un mutex interno para proteger el acceso al buffer.

## Manejo de Errores

Todas las funciones retornan códigos de error ESP-IDF estándar:

- `ESP_OK`: Operación exitosa
- `ESP_ERR_INVALID_STATE`: Logger no inicializado
- `ESP_ERR_INVALID_ARG`: Parámetros inválidos
- `ESP_ERR_TIMEOUT`: Timeout al obtener mutex
- `ESP_ERR_NOT_FOUND`: No hay eventos disponibles
