# ESP32-S2 Kaluga-1 Touch Pad LED Control 🚀

> *Ejercicio 1 del Laboratorio 2 de Sistemas Embebidos*
>
> En esta primera parte haremos uso de la placa de extensión ESP32-S2-Kaluga-1 que cuenta con 6 botones capacitivos.
>
> *Ejercicios Primera Parte:*
>
> 1. Rehacer la configuración del LED del laboratorio anterior:
>
>    * En VSCode crear un proyecto nuevo y llamarlo Laboratorio_2a.
>    * Agregar al proyecto las librerías del LED RGB creadas en el Laboratorio_1.
>    * Crear un programa sencillo y probar que el laboratorio 1 sigue funcionando correctamente.
> 2. Lectura de botones mediante polling:
>
>    * Leer referencias de Espressif sobre touch pads y describir su funcionamiento.
>    * Revisar esquemáticos de la placa base y TouchPad: ¿necesita modificarse algo? ¿Qué problemas conocidos existen?
>    * Diseñar pseudocódigo para detectar estado de botones (presionado/no presionado).
>    * Crear librería nueva que inicialice TouchPad e implemente detección de botones; modificar código anterior para que al presionar un botón cambie el estado del LED.

## Características 🔥 🔥

* *Deshabilita el Watchdog* (WDT) para evitar resets por bucles largos. 😅
* *Configura 6 touch pads* (VOL\_UP, PLAY/PAUSE, VOL\_DOWN, RECORD, PHOTO, NETWORK).
* *Lee* continuamente el valor bruto de cada pad y detecta toques usando umbral y debounce.
* *Controla el LED RGB*:

  * *VOL\_UP/DOWN*: aumenta o reduce el brillo.
  * *PLAY/PAUSE*: parpadea el color actual dos veces.
  * *RECORD: cambia el color a **rojo*.
  * *PHOTO: cambia el color a **verde*.
  * *NETWORK: cambia el color a **azul*.

---

## Estructura de archivos


├── components/
│   ├── delay/            # Control del delay que ya utilizamos en lab1
│   ├── led/            # Control de LED RGB que ya utilizamos en lab1
│   └── touch_pad/      # Lógica de touch pads (touch_pad.c, touch_pad.h)
├── main/
│   ├── main.c         # app_main: deshabilita WDT, configura touch y arranca lectura
│   └── CMakeLists.txt
└── README.md        


---

## Requisitos ⚙

* *ESP-IDF v5.4.1* (o compatible)
* *ESP32-S2 Kaluga-1*
* PC con entorno de desarrollo ESP-IDF configurado

---

## Instalación y uso

1. Clonar el repositorio:

2. Configurar el proyecto:

3. Compilar flashear y monitorear:

4. Una vez en el monitor serial, toca los pads:

   * *Pad 1 (VOL\_UP)*: aumenta brillo
   * *Pad 2 (PLAY/PAUSE)*: parpadea
   * *Pad 3 (VOL\_DOWN)*: baja brillo
   * *Pad 5 (RECORD)*: color rojo
   * *Pad 6 (PHOTO)*: color verde
   * *Pad 11 (NETWORK)*: color azul

---

## Detalles de implementación

* *main.c*:

  * Llama a esp_task_wdt_deinit() para desactivar el watchdog.
  * Invoca configure_touch_pad() para inicializar los pads y el LED.
  * Ejecuta tp_read(), bucle infinito de lectura y control LED.

* *touch\_pad.c / touch\_pad.h*:

  * configure_touch_pad(): arranca el periférico touch, configura canales, denoise y FSM.
  * tp_read(): lee raw data, aplica umbral (TOUCH_THRESHOLD) y debounce (DEBOUNCE_MS), mapea cada pad a una acción LED.
  * tp_set_led_strip(): recibe el puntero al led_strip_t tras inicializar el LED.

* *led.c / led.h*:

  * led_init(): inicializa el periférico RMT y crea instancia led_strip_t.
  * led_set_color(), led_off(): control básico de color.

---

## Ajustes y calibración

* *TOUCH\_THRESHOLD* en touch_pad.c: valor mínimo de raw data para detectar toque. Ajustar según pruebas.
* *DEBOUNCE\_MS*: tiempo mínimo entre toques sucesivos para cada botón.

---

## Posibles mejoras con FreeRTOS 🛠

* Ejecutar tp_read() en una tarea dedicada para permitir otras tareas concurrentes.
* Utilizar temporizadores de software en lugar de bucles de delay_ms() para un control más preciso.
* Implementar colas o semáforos para comunicar eventos de touch a la tarea LED sin bloquear.
* Aprovechar el Watchdog de tarea para monitorizar la salud de cada tarea.
* Mover la lógica de debounce a un timer o interrupción para reducir CPU time.

---

## Problemas conocidos 🐞

* Al entrar en tp_read() el código es bloqueante; no hay multitarea.
* Si se requieren otras tareas, refactorizar fuera de bucle infinito.
