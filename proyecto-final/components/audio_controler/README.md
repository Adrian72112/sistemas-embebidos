# Controlador de Audio Mínimo

Un componente ESP-IDF minimalista para el control básico de audio en la placa ESP32-S2-Kaluga-1, que proporciona una interfaz simple para la reproducción de listas de audio utilizando el codec ES8311.

## Descripción General

Este componente proporciona una API mínima y eficiente para:

- **Inicialización simple** del sistema de audio
- **Manejo básico de listas de reproducción** 
- **Controles esenciales de reproducción** (play, pause, next, previous)
- **Reproducción de audio** a través del codec ES8311
- **Logging integrado** de eventos básicos
- **Thread-safe** con mutex de FreeRTOS

## Arquitectura Minimalista

```
┌─────────────────────────────────────────────────────────────────┐
│                 CONTROLADOR DE AUDIO MÍNIMO                    │
│                  (API Esencial Únicamente)                     │
├─────────────────────────────────────────────────────────────────┤
│  FUNCIONES ESENCIALES:                                          │
│  • audio_controller_init()                                     │
│  • audio_controller_load_playlist()                            │
│  • audio_controller_play()                                     │
│  • audio_controller_pause()                                    │
│  • audio_controller_next()                                     │
│  • audio_controller_previous()                                 │
└─────┬────────────────────────┬────────────────────────┬─────────┘
      │                        │                        │
┌─────▼─────┐        ┌─────────▼─────────┐    ┌─────────▼─────────┐
│  LOGGER   │        │   I2S DRIVER      │    │   ES8311 CODEC    │
│  BÁSICO   │        │                   │    │                   │
│           │        │ • i2s_driver_init │    │ • es8311_codec_   │
│• Event    │        │ • i2s_driver_write│    │   init            │
│  Logging  │        └─────────┬─────────┘    │ • Volume control  │
└───────────┘                  │              └─────────┬─────────┘
                     ┌─────────▼─────────┐    ┌─────────▼─────────┐
                     │   ESP-IDF I2S     │    │    ESP-IDF I2C    │
                     │   • TX Channel    │    │ • Comunicación    │
                     │   • DMA Buffer    │    │   con ES8311      │
                     └───────────────────┘    └───────────────────┘
```

## Estructura de Archivos

```
audio_controler/
├── CMakeLists.txt              # Configuración de compilación
├── README.md                   # Esta documentación
├── audio_controler.c           # Implementación mínima
├── i2s_driver.c               # Driver I2S básico
├── es8311_codec.c             # Driver ES8311 básico
└── include/
    ├── audio_controler.h      # API mínima pública
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

## API Mínima

### Configuración

```c
typedef struct {
    uint32_t sample_rate;           /*!< Frecuencia de muestreo en Hz */
    uint8_t volume;                 /*!< Nivel de volumen 0-100 */
    bool microphone_enabled;        /*!< Habilitar micrófono */
} audio_controller_config_t;

#define AUDIO_CONTROLLER_DEFAULT_CONFIG() { \
    .sample_rate = 8000, \
    .volume = 50, \
    .microphone_enabled = false \
}
```

### Funciones Principales

```c
/**
 * @brief Inicializar el controlador de audio
 * @param config Configuración del controlador
 * @return ESP_OK en caso de éxito
 */
esp_err_t audio_controller_init(const audio_controller_config_t *config);

/**
 * @brief Cargar lista de reproducción con pistas de audio
 * @param tracks Array de pistas de audio
 * @param num_tracks Número de pistas en la lista
 * @return ESP_OK en caso de éxito
 */
esp_err_t audio_controller_load_playlist(const audio_track_t *tracks, size_t num_tracks);

/**
 * @brief Reproducir pista actual o reanudar reproducción
 * @return ESP_OK en caso de éxito
 */
esp_err_t audio_controller_play(void);

/**
 * @brief Pausar pista actual
 * @return ESP_OK en caso de éxito
 */
esp_err_t audio_controller_pause(void);

/**
 * @brief Saltar a la siguiente pista
 * @return ESP_OK en caso de éxito
 */
esp_err_t audio_controller_next(void);

/**
 * @brief Ir a la pista anterior
 * @return ESP_OK en caso de éxito
 */
esp_err_t audio_controller_previous(void);
```

## Ejemplo de Uso Básico

```c
#include "audio_controler.h"

// Referencias externas a datos de audio embebidos
extern const uint8_t music1_pcm_start[] asm("_binary_song1_pcm_start");
extern const uint8_t music1_pcm_end[]   asm("_binary_song1_pcm_end");
extern const uint8_t music2_pcm_start[] asm("_binary_song2_pcm_start");
extern const uint8_t music2_pcm_end[]   asm("_binary_song2_pcm_end");

void app_main() {
    // Configuración básica del controlador de audio
    audio_controller_config_t config = AUDIO_CONTROLLER_DEFAULT_CONFIG();
    config.volume = 75;  // Volumen al 75%
    
    // Inicializar el controlador de audio
    ESP_ERROR_CHECK(audio_controller_init(&config));
    
    // Preparar lista de reproducción
    audio_track_t tracks[] = {
        {
            .data = music1_pcm_start,
            .size = music1_pcm_end - music1_pcm_start,
            .name = "Canción 1"
        },
        {
            .data = music2_pcm_start,
            .size = music2_pcm_end - music2_pcm_start,
            .name = "Canción 2"
        }
    };
    
    // Cargar lista de reproducción
    ESP_ERROR_CHECK(audio_controller_load_playlist(tracks, 2));
    
    // Loop de demostración simple
    while (true) {
        ESP_LOGI("MAIN", "🎵 Reproduciendo...");
        audio_controller_play();
        vTaskDelay(pdMS_TO_TICKS(5000));  // Reproducir por 5 segundos
        
        ESP_LOGI("MAIN", "⏸️ Pausando...");
        audio_controller_pause();
        vTaskDelay(pdMS_TO_TICKS(2000));  // Pausa por 2 segundos
        
        ESP_LOGI("MAIN", "⏭️ Siguiente pista...");
        audio_controller_next();
        vTaskDelay(pdMS_TO_TICKS(1000));  // Esperar 1 segundo
        
        ESP_LOGI("MAIN", "⏮️ Pista anterior...");
        audio_controller_previous();
        vTaskDelay(pdMS_TO_TICKS(1000));  // Esperar 1 segundo
    }
}
```

## Uso con GPIO Buttons

```c
#include "audio_controler.h"
#include "driver/gpio.h"

#define BUTTON_PLAY_PAUSE  GPIO_NUM_0
#define BUTTON_NEXT        GPIO_NUM_1
#define BUTTON_PREVIOUS    GPIO_NUM_2

void configure_buttons() {
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_NEGEDGE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << BUTTON_PLAY_PAUSE) | 
                       (1ULL << BUTTON_NEXT) | 
                       (1ULL << BUTTON_PREVIOUS),
        .pull_down_en = 0,
        .pull_up_en = 1,
    };
    gpio_config(&io_conf);
}

void button_task(void *args) {
    static bool is_playing = false;
    
    while (true) {
        if (gpio_get_level(BUTTON_PLAY_PAUSE) == 0) {
            if (is_playing) {
                audio_controller_pause();
                is_playing = false;
                ESP_LOGI("BUTTON", "⏸️ Pausado");
            } else {
                audio_controller_play();
                is_playing = true;
                ESP_LOGI("BUTTON", "▶️ Reproduciendo");
            }
            vTaskDelay(pdMS_TO_TICKS(300)); // Debounce
        }
        
        if (gpio_get_level(BUTTON_NEXT) == 0) {
            audio_controller_next();
            ESP_LOGI("BUTTON", "⏭️ Siguiente");
            vTaskDelay(pdMS_TO_TICKS(300)); // Debounce
        }
        
        if (gpio_get_level(BUTTON_PREVIOUS) == 0) {
            audio_controller_previous();
            ESP_LOGI("BUTTON", "⏮️ Anterior");
            vTaskDelay(pdMS_TO_TICKS(300)); // Debounce
        }
        
        vTaskDelay(pdMS_TO_TICKS(50)); // Check buttons every 50ms
    }
}

void app_main() {
    // Inicializar audio
    audio_controller_config_t config = AUDIO_CONTROLLER_DEFAULT_CONFIG();
    ESP_ERROR_CHECK(audio_controller_init(&config));
    
    // Configurar botones
    configure_buttons();
    
    // Cargar playlist (ejemplo)
    // ... código para cargar pistas ...
    
    // Crear tarea para manejar botones
    xTaskCreate(button_task, "button_task", 2048, NULL, 1, NULL);
}
```

## Configuración del Proyecto

### 1. Agregar al CMakeLists.txt principal

```cmake
# CMakeLists.txt del proyecto principal
cmake_minimum_required(VERSION 3.16)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(audio_project)
```

### 2. Configurar dependencias en el componente

```cmake
# components/audio_controler/CMakeLists.txt
idf_component_register(
    SRCS "audio_controler.c" "i2s_driver.c" "es8311_codec.c"
    INCLUDE_DIRS "include"
    REQUIRES "driver" "esp_timer" "logger"
)
```

## Troubleshooting

### Problemas Comunes

1. **Error de inicialización I2S**
   - Verificar que los pines GPIO estén correctamente configurados
   - Asegurar que no haya conflicto con otros drivers I2S

2. **Sin audio**
   - Verificar conexiones del amplificador (GPIO10)
   - Comprobar configuración del codec ES8311
   - Verificar que el volumen no esté en 0

3. **Errores de memoria**
   - Asegurar que las pistas de audio no sean demasiado grandes
   - Verificar que hay suficiente heap disponible

### Logs de Debug

El componente proporciona logs detallados. Para habilitar debug completo:

```
idf.py menuconfig
→ Component config
→ Log output
→ Default log verbosity → Debug
```

## Limitaciones

- **API Mínima**: Solo funciones esenciales, sin controles avanzados
- **Un solo formato**: Solo PCM sin compresión
- **Sin streaming**: Los datos deben estar en memoria
- **Thread safety básico**: Un solo mutex para toda la operación
- **Sin auto-advance**: Las pistas se reproducen manualmente

## Licencia

Este componente está bajo la licencia CC0-1.0 (dominio público).
