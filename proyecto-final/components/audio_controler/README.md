# Audio Controller Component

Un componente ESP-IDF integrado para el control de audio en la placa ESP32-S2-Kaluga-1, que proporciona una interfaz de alto nivel para la reproducción de audio, manejo de playlists y logging automático de eventos utilizando el codec ES8311.

## Descripción General

Este componente abstrae completamente la complejidad del manejo de audio y logging en el ESP32-S2-Kaluga-1, proporcionando una API simple y robusta para:

- **Manejo completo de playlists** con reproducción automática
- **Control de reproducción** (play, pause, stop, next, previous)
- **Logging automático integrado** de todos los eventos de audio
- **Reproducción de audio** a través del codec ES8311
- **Control de volumen** dinámico
- **Soporte para micrófono** (opcional)
- **Gestión automática** de I2S, I2C y logger
- **Thread-safe** con mutex de FreeRTOS
- **Auto-advance** configurable entre tracks

## Arquitectura del Componente

```
┌─────────────────────────────────────────────────────────────────┐
│                    AUDIO CONTROLLER                             │
│                 (Capa de Abstracción Total)                    │
├─────────────────────────────────────────────────────────────────┤
│  PLAYLIST MANAGEMENT:                                           │
│  • audio_controller_load_playlist()                            │
│  • audio_controller_start_playlist()                           │
│  • audio_controller_stop_playlist()                            │
│                                                                 │
│  PLAYER CONTROL:                                                │
│  • audio_controller_play/pause/stop()                          │
│  • audio_controller_next/previous()                            │
│  • audio_controller_get_state/current_track()                  │
└─────┬────────────────────────┬────────────────────────┬─────────┘
      │                        │                        │
┌─────▼─────┐        ┌─────────▼─────────┐    ┌─────────▼─────────┐
│  LOGGER   │        │   I2S DRIVER      │    │   ES8311 CODEC    │
│           │        │                   │    │                   │
│• Event    │        │ • i2s_driver_init │    │ • es8311_codec_   │
│  Logging  │        │ • i2s_driver_write│    │   init            │
│• SPIFFS   │        │ • i2s_driver_     │    │ • es8311_codec_   │
│  Storage  │        │   preload         │    │   set_volume      │
│• Thread   │        │ • i2s_driver_     │    │ • Power Amplifier │
│  Safe     │        │   deinit          │    │   Control         │
└─────┬─────┘        └─────────┬─────────┘    └─────────┬─────────┘
      │                        │                        │
      │              ┌─────────▼─────────┐    ┌─────────▼─────────┐
      │              │   ESP-IDF I2S     │    │    ESP-IDF I2C    │
      │              │                   │    │                   │
      ▼              │ • TX Channel      │    │ • Comunicación    │
┌─────────────────┐  │ • RX Channel      │    │   con ES8311      │
│  SPIFFS + VFS   │  │ • DMA Buffer      │    │ • Configuración   │
│                 │  │ • GPIO Config     │    │   de registros    │
│ • Persistent    │  └─────────┬─────────┘    └─────────┬─────────┘
│   Storage       │            │                        │
│ • Wear Level    │  ┌─────────▼─────────────────────────▼─────────┐
│ • Auto Save     │  │           HARDWARE ESP32-S2-KALUGA-1        │
└─────────────────┘  │                                             │
                     │  I2S Pins:              I2C Pins:          │
   ┌─────────────────┤  • MCLK: GPIO35         • SDA: GPIO8       │
   │                 │  • BCLK: GPIO18         • SCL: GPIO7       │
   │                 │  • WS:   GPIO17                             │
   │                 │  • DOUT: GPIO12         ES8311 Codec       │
   │                 │  • DIN:  GPIO46         Power Amp: GPIO10  │
   │                 └─────────────────────────────────────────────┘
   │
   ▼
┌─────────────────────────────────────────────────────────────────┐
│                      FREERTOS TASKS                             │
│                                                                 │
│  ┌─────────────────┐              ┌─────────────────┐          │
│  │ Playlist Task   │              │ Audio Play Task │          │
│  │ (Priority: 2)   │              │ (Priority: 1)   │          │
│  │                 │              │                 │          │
│  │ • Auto-advance  │              │ • PCM Playback  │          │
│  │ • Track control │              │ • I2S Writing   │          │
│  │ • Timing        │              │ • Buffer Mgmt   │          │
│  └─────────────────┘              └─────────────────┘          │
└─────────────────────────────────────────────────────────────────┘
```

## Estructura de Archivos

```
audio_controler/
├── CMakeLists.txt              # Configuración de compilación
├── README.md                   # Esta documentación
├── audio_controler.c           # Implementación principal
├── i2s_driver.c               # Driver I2S personalizado
├── es8311_codec.c             # Driver ES8311 personalizado
└── include/
    ├── audio_controler.h      # API pública
    ├── audio_config.h         # Configuraciones de hardware
    ├── i2s_driver.h          # API del driver I2S
    └── es8311_codec.h        # API del driver ES8311
```

## Configuración de Hardware

### Pines I2S (ESP32-S2-Kaluga-1)
| Señal | GPIO | Descripción |
|-------|------|-------------|
| MCLK  | 35   | Master Clock |
| BCLK  | 18   | Bit Clock |
| WS    | 17   | Word Select |
| DOUT  | 12   | Data Out (Altavoz) |
| DIN   | 46   | Data In (Micrófono) |

### Pines I2C (ES8311)
| Señal | GPIO | Descripción |
|-------|------|-------------|
| SDA   | 8    | Datos I2C |
| SCL   | 7    | Clock I2C |

### Control de Amplificador
| Señal | GPIO | Descripción |
|-------|------|-------------|
| PA_CTRL | 10 | Control del Power Amplifier |

## API Reference

### Configuración

```c
typedef struct {
    uint32_t sample_rate;           // Frecuencia de muestreo (Hz)
    uint8_t volume;                 // Volumen 0-100
    bool microphone_enabled;        // Habilitar micrófono
    bool auto_next;                 // Auto avanzar al siguiente track
    uint32_t track_switch_delay_ms; // Delay entre tracks en ms
} audio_controller_config_t;

// Configuración por defecto
#define AUDIO_CONTROLLER_DEFAULT_CONFIG() { \
    .sample_rate = 8000, \
    .volume = 50, \
    .microphone_enabled = false, \
    .auto_next = true, \
    .track_switch_delay_ms = 2000 \
}
```

### Inicialización y Control

```c
/**
 * @brief Inicializar el controlador de audio con logging integrado
 * @param config Configuración del controlador
 * @return ESP_OK en caso de éxito
 */
esp_err_t audio_controller_init(const audio_controller_config_t *config);

/**
 * @brief Desinicializar el controlador y limpiar recursos
 * @return ESP_OK en caso de éxito
 */
esp_err_t audio_controller_deinit(void);
```

### Manejo de Playlists

```c
/**
 * @brief Cargar playlist con tracks de audio
 * @param tracks Array de tracks de audio
 * @param num_tracks Número de tracks en la playlist
 * @return ESP_OK en caso de éxito
 */
esp_err_t audio_controller_load_playlist(const audio_track_t *tracks, size_t num_tracks);

/**
 * @brief Iniciar reproducción automática de playlist
 * @return ESP_OK en caso de éxito
 */
esp_err_t audio_controller_start_playlist(void);

/**
 * @brief Detener reproducción de playlist
 * @return ESP_OK en caso de éxito
 */
esp_err_t audio_controller_stop_playlist(void);
```

### Control de Reproducción

```c
/**
 * @brief Reproducir track actual
 * @return ESP_OK en caso de éxito
 */
esp_err_t audio_controller_play(void);

/**
 * @brief Pausar track actual
 * @return ESP_OK en caso de éxito
 */
esp_err_t audio_controller_pause(void);

/**
 * @brief Detener track actual
 * @return ESP_OK en caso de éxito
 */
esp_err_t audio_controller_stop(void);

/**
 * @brief Saltar al siguiente track
 * @return ESP_OK en caso de éxito
 */
esp_err_t audio_controller_next(void);

/**
 * @brief Ir al track anterior
 * @return ESP_OK en caso de éxito
 */
esp_err_t audio_controller_previous(void);
```

### Estado e Información

```c
/**
 * @brief Obtener estado actual del player
 * @return audio_player_state_t Estado actual
 */
audio_player_state_t audio_controller_get_state(void);

/**
 * @brief Obtener índice del track actual
 * @return int Índice del track actual, -1 si no hay playlist cargada
 */
int audio_controller_get_current_track(void);

/**
 * @brief Obtener nombre del track actual
 * @return const char* Nombre del track actual, NULL si no hay track
 */
const char* audio_controller_get_current_track_name(void);

/**
 * @brief Obtener número total de tracks en la playlist
 * @return size_t Número de tracks
 */
size_t audio_controller_get_playlist_size(void);
```

## Ejemplo de Uso

### Uso Básico Integrado (Nuevo)

```c
#include "audio_controler.h"

// External references to embedded audio data
extern const uint8_t music1_pcm_start[] asm("_binary_song1_pcm_start");
extern const uint8_t music1_pcm_end[]   asm("_binary_song1_pcm_end");
extern const uint8_t music2_pcm_start[] asm("_binary_song2_pcm_start");
extern const uint8_t music2_pcm_end[]   asm("_binary_song2_pcm_end");

void app_main() {
    // Configurar audio controller con logging integrado
    audio_controller_config_t config = AUDIO_CONTROLLER_DEFAULT_CONFIG();
    config.volume = 75;                     // 75% volume
    config.auto_next = true;                // Enable auto-advance
    config.track_switch_delay_ms = 4000;    // 4 seconds per track
    
    // Inicializar (incluye logger automáticamente)
    ESP_ERROR_CHECK(audio_controller_init(&config));
    
    // Preparar playlist
    audio_track_t tracks[] = {
        {
            .data = music1_pcm_start,
            .size = music1_pcm_end - music1_pcm_start,
            .name = "Song 1"
        },
        {
            .data = music2_pcm_start,
            .size = music2_pcm_end - music2_pcm_start,
            .name = "Song 2"
        }
    };
    
    // Cargar playlist
    ESP_ERROR_CHECK(audio_controller_load_playlist(tracks, 2));
    
    // Iniciar reproducción automática
    ESP_ERROR_CHECK(audio_controller_start_playlist());
    
    // ¡Eso es todo! El audio se reproduce automáticamente con logging
    
    // Opcionalmente, controlar manualmente:
    // audio_controller_next();      // Siguiente canción
    // audio_controller_previous();  // Canción anterior
    // audio_controller_pause();     // Pausar
    // audio_controller_play();      // Reanudar
    
    while (1) {
        // Monitorear estado
        ESP_LOGI("MAIN", "Playing: %s", audio_controller_get_current_track_name());
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
```

### Control Manual de Playlist

```c
void manual_playlist_control() {
    // Controlar reproducción manualmente
    ESP_LOGI("MAIN", "Current track: %d/%zu", 
             audio_controller_get_current_track() + 1,
             audio_controller_get_playlist_size());
    
    // Saltar canciones
    audio_controller_next();
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    // Volver atrás
    audio_controller_previous();
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    // Pausar y reanudar
    audio_controller_pause();
    vTaskDelay(pdMS_TO_TICKS(1000));
    audio_controller_play();
    
    // Todos los eventos se loguean automáticamente
}
```

### Monitoreo de Estado

```c
void monitor_player_status() {
    audio_player_state_t state = audio_controller_get_state();
    int current_track = audio_controller_get_current_track();
    const char* track_name = audio_controller_get_current_track_name();
    size_t playlist_size = audio_controller_get_playlist_size();
    
    const char* state_str = "UNKNOWN";
    switch (state) {
        case AUDIO_STATE_STOPPED: state_str = "STOPPED"; break;
        case AUDIO_STATE_PLAYING: state_str = "PLAYING"; break;
        case AUDIO_STATE_PAUSED:  state_str = "PAUSED"; break;
    }
    
    ESP_LOGI("STATUS", "%s | Track %d/%zu: %s", 
             state_str, current_track + 1, playlist_size, track_name);
}
```

## Configuraciones Soportadas

### Frecuencias de Muestreo
- **8 kHz** - Calidad de voz básica (por defecto)
- **16 kHz** - Calidad de voz mejorada
- **22.05 kHz** - Calidad multimedia básica
- **44.1 kHz** - Calidad CD
- **48 kHz** - Calidad profesional

### Formato de Audio
- **Bits por muestra**: 16-bit
- **Canales**: Estéreo
- **Formato**: PCM sin comprimir
- **Endianness**: Little-endian

### Dependencias del Sistema

```cmake
# En tu CMakeLists.txt principal o del componente
idf_component_register(
    SRCS "tu_codigo.c"
    INCLUDE_DIRS "include"
    REQUIRES audio_controler  # Incluye automáticamente logger
)
```

### Componentes ESP-IDF Requeridos
- `driver` - Drivers de hardware
- `esp_driver_i2s` - Driver I2S
- `esp_driver_i2c` - Driver I2C
- `esp_driver_gpio` - Control de GPIO
- `freertos` - Sistema operativo en tiempo real
- `esp_common` - Utilidades comunes
- `espressif__es8311` - Driver oficial ES8311
- `logger` - **Integrado automáticamente** para logging de eventos

### Hardware Requerido
- **ESP32-S2-Kaluga-1** Development Kit
- **ES8311** Audio Codec (integrado en la placa)
- **Altavoz** conectado a la salida de audio
- **Micrófono** (opcional)
- **Partición SPIFFS** para almacenamiento de logs

## Nuevas Características Integradas

### 🎵 **Manejo Completo de Playlists**
- Carga múltiples tracks de audio
- Reproducción automática secuencial
- Avance automático configurable
- Control manual (next/previous)

### 📊 **Logging Automático Integrado**
- Todos los eventos de audio se loguean automáticamente
- Almacenamiento persistente en SPIFFS
- Buffer circular de 20 eventos
- Thread-safe y no bloqueante

### 🎛️ **Control de Estado Avanzado**
- Estados: STOPPED, PLAYING, PAUSED
- Información de track actual
- Monitoreo de progreso de playlist
- Thread-safe con mutex

### ⚡ **Gestión de Tareas FreeRTOS**
- Tarea de gestión de playlist (Prioridad 2)
- Tarea de reproducción de audio (Prioridad 1)
- Tarea de logging asíncrono (Prioridad 1)
- Sincronización automática entre tareas

## Ventajas del Diseño Integrado

### ✅ **Simplicidad de Uso**
```c
// Antes (código complejo en main)
logger_init();
audio_controller_init();
// Manejo manual de tracks, tareas, logging...

// Ahora (una sola llamada)
audio_controller_init(&config);  // Incluye todo automáticamente
audio_controller_load_playlist(tracks, count);
audio_controller_start_playlist();  // ¡Listo!
```

### ✅ **Gestión Automática de Recursos**
- **Sin memory leaks**: Limpieza automática de tareas y memoria
- **Thread safety**: Mutex interno protege todas las operaciones
- **Error handling**: Gestión robusta de errores en todas las capas

### ✅ **Logging Transparente**
- **Eventos automáticos**: PLAY, PAUSE, STOP, NEXT, PREVIOUS se loguean automáticamente
- **Persistencia**: Todos los logs se guardan en SPIFFS sin intervención del usuario
- **Thread-safe**: Logging asíncrono que no afecta la reproducción de audio

### ✅ **Configuración Flexible**
```c
audio_controller_config_t config = AUDIO_CONTROLLER_DEFAULT_CONFIG();
config.auto_next = true;                // Auto-advance entre tracks
config.track_switch_delay_ms = 4000;    // 4 segundos por track
config.volume = 75;                     // Volumen al 75%
```

## Limitaciones y Consideraciones

### Limitaciones Técnicas
1. **Formato de Audio**: Solo soporta PCM 16-bit estéreo
2. **Hardware Específico**: Diseñado específicamente para ESP32-S2-Kaluga-1
3. **Frecuencias**: Limitado a las frecuencias soportadas por el ES8311
4. **Memoria**: Los tracks se cargan completamente en memoria (no streaming)
5. **Pause/Resume**: Implementación simplificada (reinicia el track)

### Consideraciones de Rendimiento
- **Memoria RAM**: Cada track debe caber completamente en memoria
- **Tareas FreeRTOS**: Usa 3 tareas concurrentes (playlist, audio, logger)
- **Prioridades**: Configuradas para no interferir con tareas críticas
- **Mutex Timeout**: 100ms para operaciones de control, 10ms para consultas

### Configuración de Particiones
Asegúrate de incluir SPIFFS en tu tabla de particiones:

```csv
# Name, Type, SubType, Offset, Size, Flags
nvs,      data, nvs,     0x9000,  0x6000,
phy_init, data, phy,     0xf000,  0x1000,
factory,  app,  factory, 0x10000, 0x180000,
spiffs,   data, spiffs,  ,        0x70000,  # Para logs de audio
```

## Migración desde Versión Anterior

### Cambios en la API

#### ❌ **Código Anterior (Deprecated)**
```c
// Manejo manual en main.c
logger_init();
audio_controller_init(&config);

// Tareas manuales para cada track
xTaskCreate(audio_play_task, "audio", 4096, track_data, 1, NULL);
logger_log_event(LOGGER_EVENT_PLAY);  // Manual logging
```

#### ✅ **Código Nuevo (Recomendado)**
```c
// Todo integrado en el componente
audio_controller_init(&config);  // Incluye logger automáticamente
audio_controller_load_playlist(tracks, count);
audio_controller_start_playlist();  // Logging automático
```

### Funciones Renombradas
- `audio_controller_play(data, size)` → `audio_controller_play_data(data, size)` (deprecated)
- `audio_controller_play()` → **Nueva función para control de playlist**

### Nuevas Estructuras
```c
// Nueva estructura para tracks
typedef struct {
    const uint8_t *data;
    size_t size;
    const char *name;
} audio_track_t;

// Nuevos estados
typedef enum {
    AUDIO_STATE_STOPPED,
    AUDIO_STATE_PLAYING,
    AUDIO_STATE_PAUSED
} audio_player_state_t;
```

## Troubleshooting

### Problemas Comunes

#### No se escucha audio
- Verificar conexiones de hardware
- Comprobar que el Power Amplifier esté habilitado
- Verificar nivel de volumen (no esté en 0)
- Revisar formato de datos de audio

#### Audio distorsionado
- Verificar frecuencia de muestreo correcta
- Comprobar que los datos estén en formato PCM 16-bit
- Revisar nivel de volumen (podría estar muy alto)

#### Error de inicialización
- Verificar que los pines estén libres y no en uso por otros periféricos
- Comprobar configuración de I2C e I2S
- Verificar que el ES8311 esté correctamente conectado

### Logs de Debug

Para habilitar logs detallados, configurar el nivel de log:
```c
esp_log_level_set("audio_controller", ESP_LOG_DEBUG);
esp_log_level_set("i2s_driver", ESP_LOG_DEBUG);
esp_log_level_set("es8311_codec", ESP_LOG_DEBUG);
```

## Contribuciones

Para contribuir al desarrollo de este componente:

1. Hacer fork del repositorio
2. Crear una branch para la nueva característica
3. Implementar los cambios con pruebas
4. Enviar un pull request

## Licencia

```
SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
SPDX-License-Identifier: CC0-1.0
```

## Changelog

### v1.0.0
- Implementación inicial
- Soporte para reproducción de audio
- Control de volumen
- Funciones de preload
- Soporte para ESP32-S2-Kaluga-1

## Referencias

- [ESP32-S2-Kaluga-1 Getting Started Guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s2/hw-reference/esp32s2/user-guide-esp32-s2-kaluga-1-kit.html)
- [ESP-IDF I2S Driver](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s2/api-reference/peripherals/i2s.html)
- [ES8311 Codec Datasheet](https://www.everest-semi.com/pdf/ES8311%20PB.pdf)
