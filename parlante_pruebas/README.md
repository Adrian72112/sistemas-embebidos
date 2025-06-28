# ESP32-S2 Kaluga Kit - Audio Player

Este proyecto implementa un reproductor de audio modular para ESP32-S2 Kaluga Kit usando I2S + ES8311 codec.

## Características

- 🎵 **Reproductor de audio modular** con componente `audio_controller`
- 🔀 **Alternancia automática** entre múltiples canciones cada 4 segundos
- 🎛️ **Control de volumen** programático
- 📁 **Archivos PCM optimizados** para ESP32-S2
- 🏗️ **Arquitectura modular** fácil de extender

## Estructura del Proyecto

```
parlante_pruebas/
├── main/
│   ├── main.c              # Aplicación principal
│   └── CMakeLists.txt      # Configuración de build
├── components/
│   └── audio_controler/    # Componente modular de audio
│       ├── include/
│       │   ├── audio_controler.h    # API pública
│       │   ├── audio_config.h       # Configuraciones
│       │   ├── i2s_driver.h         # Driver I2S
│       │   └── es8311_codec.h       # Driver ES8311
│       ├── audio_controler.c        # Controlador principal
│       ├── i2s_driver.c             # Implementación I2S
│       ├── es8311_codec.c           # Implementación ES8311
│       └── CMakeLists.txt
└── audios/
    ├── victory8bit.pcm     # Audio 1 (optimizado)
    └── 8bit.pcm           # Audio 2 (optimizado)
```

## Hardware: ESP32-S2 Kaluga Kit

### Pines utilizados:
- **I2C** (ES8311 control): GPIO 7 (SCL), GPIO 8 (SDA)
- **I2S** (Audio data): GPIO 12, 17, 18, 35, 46
- **PA Control**: GPIO 10 (Power Amplifier)

## ES8311 Codec

ES8311 low power mono audio codec features:

- High performance and low power multi-bit delta-sigma audio ADC and DAC
- I2S/PCM master or slave serial data port
- I2C interface for configuration
- ADC: 24-bit, 8 to 96 kHz sampling frequency
- ADC: 100 dB signal to noise ratio, -93 dB THD+N
- DAC: 24-bit, 8 to 96 kHz sampling frequency
- DAC: 110 dB signal to noise ratio, -80 dB THD+N

For more details, see [ES8311 datasheet](http://www.everest-semi.com/pdf/ES8311%20PB.pdf)

## How to Use Example

### Hardware Required

* A development board with any supported Espressif SOC chip (see `Supported Targets` table above)
    * The example can be preconfigured for [ESP-BOX](https://components.espressif.com/components/espressif/esp-box), [ESP32-S2-Kaluga-kit](https://components.espressif.com/components/espressif/esp32_s2_kaluga_kit) and [ESP32-S3-LCD-EV-board](https://components.espressif.com/components/espressif/esp32_s3_lcd_ev_board). More information is in 'Configure the Project' section.
* A USB cable for power supply and programming.
* A board with ES8311 codec, mic and earphone interface(e.g. ESP-LyraT-8311A extension board).

### Connection
```
┌─────────────────┐           ┌──────────────────────────┐
│       ESP       │           │          ES8311          │
│                 │           │                          │
│       I2S_MCK_IO├──────────►│PIN2-MCLK                 │
│                 │           │                          │           ┌─────────┐
│       I2S_BCK_IO├──────────►│PIN6-BCLK       PIN12-OUTP├───────────┤         │
│                 │           │                          │           │ EARPHONE│
│        I2S_WS_IO├──────────►│PIN8-LRCK       PIN13-OUTN├───────────┤         │
│                 │           │                          │           └─────────┘
│        I2S_DO_IO├──────────►│PIN9-SDIN                 │
│                 │           │                          │
│        I2S_DI_IO│◄──────────┤PIN7-SDOUT                │
│                 │           │                          │           ┌─────────┐
│                 │           │               PIN18-MIC1P├───────────┤         │
│       I2C_SCL_IO├──────────►│PIN1 -CCLK                │           │  MIC    │
│                 │           │               PIN17-MIC1N├───────────┤         │
│       I2C_SDA_IO│◄─────────►│PIN19-CDATA               │           └─────────┘
│                 │           │                          │
│          VCC 3.3├───────────┤VCC                       │
│                 │           │                          │
│              GND├───────────┤GND                       │
└─────────────────┘           └──────────────────────────┘
```
Note: Since ESP32-C3 & ESP32-H2 board does not have GPIO 16/17, you can use other available GPIOs instead. In this example, we set GPIO 6/7 as I2C pins for ESP32-C3 and GPIO 8/9 ESP32-H2 and GPIO 16/17 for other chips, same as GPIO 18/19, we use GPIO 2/3 instead.

### Dependency

This example is based on [es8311 component](https://components.espressif.com/component/espressif/es8311)

The component can be installed by [IDF Component Manager](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/tools/idf-component-manager.html). This example already includes it. If you want to install [es8311 component](https://components.espressif.com/components/espressif/es8311) separately in your project, you can input the following command:
```
idf.py add-dependency "espressif/es8311^1.0.0"
```

If the dependency is added, you can check `idf_component.yml` for more detail. When building this example or other projects with managed components, the component manager will search for the required components online and download them into the `managed_components` folder.

### Configure the Project

```
idf.py menuconfig
```
You can find configurations for this example in 'Example Configuration' tag.

* In 'Example mode' subtag, you can set the example mode to 'music' or 'echo'. You can hear a piece of music in 'music' mode and echo the sound sampled by mic in 'echo' mode. You can also customize you own music to play as shown below.

* In 'Set MIC gain' subtag, you can set the mic gain for echo mode.

* In 'Voice volume', you can set the volume between 0 to 100.

* In 'Enable Board Support Package (BSP) support' you can enable support for BSP. You can pick specific BSP in [idf_component.yml](main/idf_component.yml).

### Build and Flash

Build the project and flash it to the board, then run monitor tool to view serial output:

```
idf.py -p PORT flash monitor
```

(To exit the serial monitor, type ``Ctrl-]``.)

See the Getting Started Guide for full steps to configure and use ESP-IDF to build projects.

## Example Output

Running this example in music mode, you can hear a piece of music (canon), the log is shown as follow:

```
I (348) I2S: DMA Malloc info, datalen=blocksize=1200, dma_desc_num=6
I (348) I2S: DMA Malloc info, datalen=blocksize=1200, dma_desc_num=6
I (358) I2S: I2S0, MCLK output by GPIO0
I (368) DRV8311: ES8311 in Slave mode
I (378) gpio: GPIO[10]| InputEn: 0| OutputEn: 1| OpenDrain: 0| Pullup: 0| Pulldown: 0| Intr:0
I (3718) i2s_es8311: I2S music played, 213996 bytes are written.
I (7948) i2s_es8311: I2S music played, 213996 bytes are written.
......
```

Running this example in echo mode, you can hear the sound in earphone that collected by mic.
```
I (312) I2S: DMA Malloc info, datalen=blocksize=1200, dma_desc_num=6
I (312) I2S: DMA Malloc info, datalen=blocksize=1200, dma_desc_num=6
I (322) I2S: I2S0, MCLK output by GPIO0
I (332) DRV8311: ES8311 in Slave mode
I (342) gpio: GPIO[10]| InputEn: 0| OutputEn: 1| OpenDrain: 0| Pullup: 0| Pulldown: 0| Intr:0
```

If you have a logic analyzer, you can use a logic analyzer to grab GPIO signal directly. The following table describes the pins we use by default (Note that you can also use other pins for the same purpose).

| pin name| function | gpio_num |
|:---:|:---:|:---:|
| MCLK  |module clock   | GPIO_NUM_0|
| BCLK  |bit clock      | GPIO_NUM_4 |
| WS    |word select    | GPIO_NUM_5 |
| SDOUT |serial data out| GPIO_NUM_18/2 |
| SDIN  |serial data in | GPIO_NUM_19/3 |

Other pins like I2C please refer to `example_config.h`.

Please note that the power amplifier on some development boards (like P4 EV board) are disabled by default, you might need to set the PA_CTRL pin to high to play the music via a speaker.
The PA_CTRL pin can be configured by `idf.py menuconfig`, please check if the PA_CTRL pin is correct on your board if the audio can only be played from the earphones but not the speaker.

### Customize your own music

The example have contained a piece of music in canon.pcm, if you want to play your own music, you can follow these steps:

1. Choose the music in any format you want to play (e.g. a.mp3)
2. Install 'ffmpeg' tool
3. Check your music format using ```ffprobe a.mp3```, you can get the stream format (e.g. Stream #0.0: Audio: mp3, 44100Hz, stereo, s16p, 64kb/s)
4. Cut your music since there is no enough space for the whole piece of music. ```ffmpeg -i  a.mp3 -ss 00:00:00  -t  00:00:20  a_cut.mp3```
5. Transfer the music format into .pcm. ```ffmpeg -i a_cut.mp3 -f s16ls -ar 16000 -ac -1 -acodec pcm_s16le a.pcm```
6. Move 'a.pcm' under 'main' directory
7. Replace 'canon.pcm' with 'a.pcm' in 'CMakeLists.txt' under 'main' directory
8. Replace '_binary_canon_pcm_start' and '_binary_canon_pcm_end' with '_binary_a_pcm_start' and '_binary_a_pcm_end' in `i2s_es8311_example.c`
9. Download the example and enjoy your own music

## Troubleshooting

* Program upload failure

    * Hardware connection is not correct: run `idf.py -p PORT monitor`, and reboot your board to see if there are any output logs.
    * The baud rate for downloading is too high: lower your baud rate in the `menuconfig` menu, and try again.

* Failed to get audio from specker

    * The PA (Power Amplifier) on some dev-kits might be disabled by default, please check the schematic to see if PA_CTRL is connected to any GPIO or something.
    * Pull-up the PA_CTRL pin either by setting that GPIO to high or by connecting it to 3.3V with a jump wire should help.

For any technical queries, please open an [issue](https://github.com/espressif/esp-idf/issues) on GitHub. We will get back to you soon.

---

## El **ES8311** es un codec de audio que convierte:
- **Digital → Analógico** (DAC): Para reproducir audio en altavoces
- **Analógico → Digital** (ADC): Para capturar audio del micrófono

## Optimización de Audio

Los archivos PCM están optimizados para ESP32-S2:
- **Sample Rate**: 8kHz (vs 16kHz estándar)
- **Channels**: Mono (vs Estéreo)
- **Bit Depth**: 8-bit (vs 16-bit)
- **Resultado**: ~75% menos espacio que PCM estándar

### Comando de conversión usado:
```bash
ffmpeg -i input.mp3 -ar 8000 -ac 1 -f u8 output.pcm
```

## Funcionalidad

### Modo Actual: Alternancia de Canciones
- ▶️ Reproduce `victory8bit.pcm` por 4 segundos
- 🔄 Cambia automáticamente a `8bit.pcm` por 4 segundos
- 🔁 Ciclo continuo con transiciones suaves

### API del Audio Controller

```c
// Inicializar
audio_controller_config_t config = AUDIO_CONTROLLER_DEFAULT_CONFIG();
audio_controller_init(&config);

// Reproducir con loop
audio_controller_play_loop(data, size, delay_ms);

// Controlar volumen
audio_controller_set_volume(60); // 0-100%
