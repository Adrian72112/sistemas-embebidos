# Controlador de Audio ESP32-S2 LyraT 8311 v1.2

Un componente ESP-IDF profesional para el control completo de audio en la placa **ESP32-S2 LyraT 8311 v1.2**, que implementa un **sistema de cola de eventos asíncronos** para el manejo robusto y thread-safe de la reproducción de audio utilizando el codec ES8311 con interfaz I2S.

## 🎵 Características Principales

- **🔄 Reproducción en Loop Infinito** - Las pistas se reproducen continuamente hasta recibir otro comando
- **📱 Control Remoto Completo** - Compatible con MQTT, Web Server y Touch Pad
- **�️ Control de Volumen** - Incrementos/decrementos de 10 en 10 (0-100)
- **📊 Logging de Eventos** - Integración con sistema de logger para auditoría
- **🔧 Configuración Flexible** - Sample rate, volumen inicial y micrófono configurables
- **⚡ Thread-Safe** - Arquitectura robusta con mutex y colas de eventos
- **🎛️ Gestión Inteligente de Estados** - Cambio automático de pistas conservando estado de reproducción

## 🏗️ Arquitectura del Sistema

Este componente utiliza una arquitectura moderna basada en colas de eventos de FreeRTOS:

- **🚀 Procesamiento Asíncrono** - Los comandos retornan inmediatamente
- **🛡️ Thread Safety Completo** - Queue thread-safe + mutex para operaciones críticas  
- **⚡ Alta Performance** - Callbacks instantáneos, procesamiento en background
- **📈 Escalabilidad** - Fácil agregar nuevos tipos de eventos
- **🔄 Loop Continuo Inteligente** - Reproducción ininterrumpida con gestión de memoria eficiente

```
┌─────────────────────────────────────────────────────────────────┐
│           CONTROLADOR DE AUDIO ESP32-S2 LYRAT 8311 v1.2        │
│              (Arquitectura de Cola de Eventos)                 │
├─────────────────────────────────────────────────────────────────┤
│  FLUJO DE EVENTOS:                                              │
│  [MQTT/Touch/Web] → [Queue] → [Event Task] → [Audio Hardware]  │
│      ↓               ↓          ↓              ↓               │
│  Instantáneo     Thread-Safe  Async Process  Hardware Control  │
└─────┬────────────────────────┬────────────────────────┬─────────┘
      │                        │                        │
┌─────▼─────┐        ┌─────────▼─────────┐    ┌─────────▼─────────┐
│EVENT QUEUE│        │ AUDIO EVENT TASK  │    │  AUDIO PLAY TASK  │
│(10 Events)│        │ (Priority 5)      │    │ (Priority 1)      │
│• PLAY     │        │ • Event Processor │    │ • Loop Infinito   │
│• PAUSE    │        │ • Async Handler   │    │ • I2S Chunks 1KB  │
│• NEXT     │        │ • State Machine   │    │ • Thread Control  │
│• PREVIOUS │        │ • Mutex Control   │    │ • Memory Mgmt     │
│• VOL_UP   │        │ • Logger Events   │    │                   │
│• VOL_DOWN │        └─────────┬─────────┘    └─────────┬─────────┘
└───────────┘                  │                        │
                     ┌─────────▼─────────┐    ┌─────────▼─────────┐
                     │   I2S DRIVER      │    │   ES8311 CODEC    │
                     │ • 16-bit Stereo   │    │ • I2C Control     │
                     │ • 8kHz Default    │    │ • Volume 0-100    │
                     │ • DMA Buffers     │    │ • PA Control      │
                     │ • Auto Clear      │    │ • Mic Optional    │
                     └───────────────────┘    └───────────────────┘
```

## 🎯 Tipos de Eventos Soportados

```c
typedef enum {
    AUDIO_EVENT_PLAY,       /*!< ▶️ Reproducir/Reanudar pista actual */
    AUDIO_EVENT_PAUSE,      /*!< ⏸️ Pausar reproducción manteniendo posición */
    AUDIO_EVENT_NEXT,       /*!< ⏭️ Siguiente pista (auto-play si estaba sonando) */
    AUDIO_EVENT_PREVIOUS,   /*!< ⏮️ Pista anterior (auto-play si estaba sonando) */
    AUDIO_EVENT_VOLUME_UP,  /*!< 🔊 Subir volumen (+10, máx 100) */
    AUDIO_EVENT_VOLUME_DOWN,/*!< 🔉 Bajar volumen (-10, mín 0) */
    AUDIO_EVENT_STOP,       /*!< ⏹️ Detener (reservado para extensiones) */
    AUDIO_EVENT_MAX         /*!< Marcador máximo */
} audio_event_type_t;
```

## � Configuración de Hardware ESP32-S2 LyraT 8311 v1.2

### Pines I2S (Específicos para LyraT 8311 v1.2)
```c
#define I2S_NUM                     (1)        // I2S1 según esquemático
#define I2S_MCK_IO                  (GPIO_NUM_35)  // Master Clock
#define I2S_BCK_IO                  (GPIO_NUM_18)  // Bit Clock  
#define I2S_WS_IO                   (GPIO_NUM_17)  // Word Select
#define I2S_DO_IO                   (GPIO_NUM_12)  // Speaker Out
#define I2S_DI_IO                   (GPIO_NUM_46)  // Mic In
```

### Pines I2C para ES8311
```c
#define I2C_NUM                     (0)        // I2C0
#define I2C_SCL_IO                  (GPIO_NUM_7)   // Clock
#define I2C_SDA_IO                  (GPIO_NUM_8)   // Data
```

### Control de Amplificador de Potencia
```c
#define EXAMPLE_PA_CTRL_IO          (GPIO_NUM_10)  // Power Amplifier Control
```

### Especificaciones de Audio
- **Frecuencia de Muestreo**: 8 kHz (configurable)
- **Resolución**: 16-bit stereo
- **MCLK Multiple**: 384x (para mejor calidad)
- **Volumen**: 0-100 (pasos de 10)
- **Codec**: ES8311 vía I2C

## �🚀 API Principal

### Función de Control de Eventos (Núcleo del Sistema)

```c
/**
 * @brief Enviar evento al controlador de audio de forma asíncrona
 * @param event_type Tipo de evento a procesar
 * @return ESP_OK en caso de éxito, ESP_ERR_TIMEOUT si la queue está llena
 * 
 * Esta función es thread-safe y no bloquea. Los eventos se procesan
 * asíncronamente en una tarea dedicada de alta prioridad.
 */
esp_err_t audio_controller_send_event(audio_event_type_t event_type);
```

### Funciones de Configuración e Inicialización

```c
/**
 * @brief Inicializar el controlador de audio completo
 * Configura: I2S driver, ES8311 codec, Power Amplifier, Event System
 * @param config Configuración del controlador
 * @return ESP_OK en caso de éxito
 */
esp_err_t audio_controller_init(const audio_controller_config_t *config);

/**
 * @brief Cargar playlist con pistas embebidas en flash
 * @param tracks Array de pistas de audio (datos en PROGMEM)
 * @param num_tracks Número de pistas (máximo limitado por memoria)
 * @return ESP_OK en caso de éxito
 */
esp_err_t audio_controller_load_playlist(const audio_track_t *tracks, size_t num_tracks);
```

## 🎵 Estructura de Pista de Audio

```c
typedef struct {
    const uint8_t *data;        /*!< Puntero a datos PCM en flash */
    size_t size;                /*!< Tamaño en bytes */
    const char *name;           /*!< Nombre descriptivo para logs */
} audio_track_t;
```

**Formato Soportado**: PCM 16-bit, 8kHz, Stereo (archivos .pcm embebidos)
```