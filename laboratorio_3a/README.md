# Lab - Control de LEDs por UART en ESP32

Este proyecto implementa un sistema de control de LEDs en un ESP32 mediante comandos recibidos por UART. Fue desarrollado como parte de un laboratorio de Sistemas Embebidos y demuestra el uso de FreeRTOS, UART, colas y tareas concurrentes.

## 📋 Descripción general

El programa recibe comandos por UART en formato `color-tiempo` (por ejemplo: `rojo-1000`) y enciende un LED del color indicado luego de el tiempo especificado en milisegundos (ya que el mismo dispara un timer con un callback a la tarea de cambiar el color). 

Se pueden enviar múltiples comandos separados por comas, y se ejecutarán en orden.
### Ejemplo de input por UART:
rojo-1000,verde-500,azul-1500

Este comando:
1. Enciende el LED rojo luego de 1000 ms  
2. Enciende el LED verde luego de 500 ms  
3. Enciende el LED azul luego de 1500 ms

En este caso se encenderían la led en este órden: verde --> rojo --> azul

## 🧠 Arquitectura

El programa se compone de dos tareas principales:

- **task_b**: Recibe cadenas por UART, las parsea y convierte en estructuras `color_command_t`. Cada comando es encolado en una `Queue`.
- **task_c**: Lee los comandos de la cola y enciende el LED correspondiente durante el tiempo especificado.

## 🧪 Formato del comando

Cada comando debe seguir este formato:
<color>-<tiempo_ms>

- `color`: puede ser `rojo`, `verde` o `azul`
- `tiempo_ms`: número entero con la duración en milisegundos

### Entrada válida
rojo-1000,verde-2000

### Entradas inválidas
rojo1000 ← falta el guión
amarillo-500 ← color no reconocido
verde-dosmil ← tiempo no numérico

Las entradas inválidas son descartadas y se muestra un mensaje por consola.

> Puedes ajustar estos valores en `color.c` según tu hardware.