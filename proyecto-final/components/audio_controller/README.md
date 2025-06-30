# Controlador de Audio con Sistema de Eventos

Un componente ESP-IDF avanzado para el control de audio en la placa ESP32-S2-Kaluga-1, que implementa un **sistema de cola de eventos asíncronos** para el manejo robusto y thread-safe de la reproducción de audio utilizando el codec ES8311.

## 🏗️ Arquitectura Basada en Eventos

Este componente utiliza una arquitectura moderna basada en colas de eventos de FreeRTOS que proporciona:

- **🚀 Procesamiento Asíncrono** - Los comandos no bloquean al llamador
- **🛡️ Thread Safety Completo** - Queue automáticamente thread-safe + mutex para operaciones críticas  
- **⚡ Alta Performance** - Callbacks instantáneos, procesamiento en background
- **📈 Escalabilidad** - Fácil agregar nuevos tipos de eventos
- **🔄 Loop de Audio Continuo** - Las pistas se reproducen en bucle automáticamente

```
┌─────────────────────────────────────────────────────────────────┐
│                 CONTROLADOR DE AUDIO AVANZADO                  │
│              (Arquitectura de Cola de Eventos)                 │
├─────────────────────────────────────────────────────────────────┤
│  FLUJO DE EVENTOS:                                              │
│  [MQTT/Main] → [Queue] → [Event Task] → [Audio Hardware]       │
│      ↓           ↓          ↓              ↓                   │
│  Instantáneo  Thread-Safe  Async Process  Hardware Control     │
└─────┬────────────────────────┬────────────────────────┬─────────┘
      │                        │                        │
┌─────▼─────┐        ┌─────────▼─────────┐    ┌─────────▼─────────┐
│EVENT QUEUE│        │ AUDIO EVENT TASK  │    │  AUDIO PLAY TASK  │
│           │        │                   │    │                   │
│• PLAY     │        │ • Event Processor │    │ • Loop Continuo   │
│• PAUSE    │        │ • Async Handler   │    │ • I2S Writing     │
│• NEXT     │        │ • State Machine   │    │ • Hardware Control│
│• PREVIOUS │        │ • Error Handling  │    │                   │
│• STOP     │        └─────────┬─────────┘    └─────────┬─────────┘
└───────────┘                  │                        │
                     ┌─────────▼─────────┐    ┌─────────▼─────────┐
                     │   I2S DRIVER      │    │   ES8311 CODEC    │
                     │ • DMA Buffers     │    │ • Volume Control  │
                     │ • Hardware Config │    │ • Audio Config    │
                     └───────────────────┘    └───────────────────┘
```

## 🎯 Tipos de Eventos

```c
typedef enum {
    AUDIO_EVENT_PLAY,       /*!< ▶️ Reproducir/Reanudar */
    AUDIO_EVENT_PAUSE,      /*!< ⏸️ Pausar reproducción */
    AUDIO_EVENT_NEXT,       /*!< ⏭️ Siguiente pista */
    AUDIO_EVENT_PREVIOUS,   /*!< ⏮️ Pista anterior */
    AUDIO_EVENT_STOP,       /*!< ⏹️ Detener (extensible) */
    AUDIO_EVENT_MAX         /*!< Marcador máximo */
} audio_event_type_t;
```

## 🚀 API Principal

### Función de Control de Eventos

```c
/**
 * @brief Enviar evento al controlador de audio de forma asíncrona
 * @param event_type Tipo de evento a procesar
 * @return ESP_OK en caso de éxito, ESP_ERR_TIMEOUT si la queue está llena
 */
esp_err_t audio_controller_send_event(audio_event_type_t event_type);
```

### Funciones de Configuración

```c
/**
 * @brief Inicializar el controlador de audio con sistema de eventos
 * @param config Configuración del controlador
 * @return ESP_OK en caso de éxito
 */
esp_err_t audio_controller_init(const audio_controller_config_t *config);

/**
 * @brief Cargar lista de reproducción
 * @param tracks Array de pistas de audio
 * @param num_tracks Número de pistas
 * @return ESP_OK en caso de éxito
 */
esp_err_t audio_controller_load_playlist(const audio_track_t *tracks, size_t num_tracks);
```

## 💡 Ejemplo de Uso con MQTT

```c
#include "audio_controller.h"
#include "mqtt_lib.h"

// Callback MQTT - Procesamiento instantáneo
void mqtt_callback(const char *topic, const char *data, int len) {
    char command[len + 1];
    strncpy(command, data, len);
    command[len] = '\0';
    
    // Envío de eventos asíncrono - no bloquea
    if (strcmp(command, "play") == 0) {
        audio_controller_send_event(AUDIO_EVENT_PLAY);
    }
    else if (strcmp(command, "pause") == 0) {
        audio_controller_send_event(AUDIO_EVENT_PAUSE);
    }
    else if (strcmp(command, "next") == 0) {
        audio_controller_send_event(AUDIO_EVENT_NEXT);
    }
    else if (strcmp(command, "previous") == 0) {
        audio_controller_send_event(AUDIO_EVENT_PREVIOUS);
    }
    // Callback termina inmediatamente - procesamiento en background
}

void app_main() {
    // Inicializar sistema completo
    ESP_ERROR_CHECK(wifi_connect());
    ESP_ERROR_CHECK(mqtt_lib_init("mqtt://broker.hivemq.com", mqtt_callback));
    
    // Configurar audio con sistema de eventos
    audio_controller_config_t config = AUDIO_CONTROLLER_DEFAULT_CONFIG();
    config.volume = 75;
    ESP_ERROR_CHECK(audio_controller_init(&config));
    
    // Cargar playlist
    audio_track_t tracks[] = {
        { music1_start, music1_end - music1_start, "Victory Theme" },
        { music2_start, music2_end - music2_start, "8-bit Classic" },
        { music3_start, music3_end - music3_start, "Game Start" }
    };
    ESP_ERROR_CHECK(audio_controller_load_playlist(tracks, 3));
    
    ESP_LOGI("MAIN", "🎵 Sistema listo - Control remoto via MQTT activado");
    
    // Sistema queda escuchando eventos
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
```

## 🔧 Configuración Avanzada

### Parámetros del Sistema de Eventos

```c
#define AUDIO_EVENT_QUEUE_SIZE 10              // Eventos en cola
#define AUDIO_EVENT_TASK_STACK_SIZE 4096       // Stack de tarea de eventos  
#define AUDIO_EVENT_TASK_PRIORITY 5            // Prioridad alta
```

### Configuración de Hardware

```c
typedef struct {
    uint32_t sample_rate;           /*!< Frecuencia de muestreo (8000 Hz por defecto) */
    uint8_t volume;                 /*!< Volumen 0-100 */
    bool microphone_enabled;        /*!< Habilitar micrófono */
} audio_controller_config_t;

#define AUDIO_CONTROLLER_DEFAULT_CONFIG() { \
    .sample_rate = 8000, \
    .volume = 50, \
    .microphone_enabled = false \
}
```

## 🎵 Características del Reproductor

### Loop Continuo Inteligente
- Las pistas se reproducen en **bucle infinito** hasta recibir otro comando
- **Cambio de pista automático**: `next`/`previous` continúan reproduciéndose si estaba activo
- **Pausa inteligente**: Mantiene la posición, `play` reanuda desde la misma pista

### Thread Safety Robusto
- **Queue de eventos**: Thread-safe automáticamente por FreeRTOS
- **Mutex para operaciones críticas**: Protege estado interno y hardware
- **Manejo de tareas**: Terminación controlada de tareas de audio

### Manejo de Errores
- **Timeouts configurables**: Para queue y mutex
- **Logging detallado**: Estados, eventos y errores
- **Recuperación automática**: Limpieza de recursos en caso de error

## 📊 Flujo de Estados

```
Estado: DETENIDO
    ↓ AUDIO_EVENT_PLAY
Estado: REPRODUCIENDO (Loop)
    ↓ AUDIO_EVENT_PAUSE
Estado: PAUSADO
    ↓ AUDIO_EVENT_PLAY  
Estado: REPRODUCIENDO (continúa)
    ↓ AUDIO_EVENT_NEXT
Estado: REPRODUCIENDO (nueva pista)
    ↓ AUDIO_EVENT_PREVIOUS
Estado: REPRODUCIENDO (pista anterior)
```

## 🔍 Debug y Troubleshooting

### Logging Detallado
```bash
# Logs de eventos (nivel INFO)
I (1234) audio_controller: 🎛️ Audio event task started
I (1235) audio_controller: 🎵 Processing PLAY event
I (1236) audio_controller: ▶️ Playing: Victory Theme

# Logs de debug (nivel DEBUG) 
D (1234) audio_controller: Event 0 sent to queue
D (1235) audio_controller: Processing event: 0
D (1236) audio_controller: 🔄 Looping: Victory Theme
```

### Problemas Comunes

1. **Queue llena (ESP_ERR_TIMEOUT)**
   - Reducir frecuencia de comandos
   - Aumentar `AUDIO_EVENT_QUEUE_SIZE`

2. **Audio entrecortado**
   - Verificar prioridad de tareas
   - Aumentar buffer I2S

3. **Comandos ignorados**
   - Verificar inicialización completa
   - Check logs de la tarea de eventos

## 📈 Rendimiento

- **Latencia de comando**: ~1ms (tiempo de queue)
- **Memoria heap**: ~8KB (buffers + estructuras)
- **Stack usage**: 4KB (tarea de eventos) + 4KB (tarea de audio)
- **CPU overhead**: Mínimo, operaciones asíncronas

## 🚀 Extensibilidad

Agregar nuevos eventos es trivial:

```c
// En el enum
AUDIO_EVENT_VOLUME_UP,
AUDIO_EVENT_VOLUME_DOWN,

// En el switch de la tarea de eventos
case AUDIO_EVENT_VOLUME_UP:
    audio_controller_volume_up_internal();
    break;
```

## 📄 Licencia

Este componente está licenciado bajo **CC0-1.0** (Dominio Público).
