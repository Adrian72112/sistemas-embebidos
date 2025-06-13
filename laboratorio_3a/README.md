# Lab - Control de LEDs por UART en ESP32

Este proyecto implementa un sistema de control de LEDs en un ESP32 mediante comandos recibidos por UART. Fue desarrollado como parte de un laboratorio de Sistemas Embebidos y demuestra el uso de FreeRTOS, UART, colas y tareas concurrentes.

## 📋 Descripción general

El programa recibe comandos por UART en formato `color-tiempo` (por ejemplo: `rojo-1000`) y enciende un LED del color indicado luego del tiempo especificado en milisegundos (ya que el mismo dispara un timer con un callback a la tarea de cambiar el color). 

Se pueden enviar múltiples comandos separados por comas, y se ejecutarán en orden.

### Ejemplo de input por UART:
rojo-1000,verde-500,azul-1500


Este comando:
1. Enciende el LED rojo luego de 1000 ms  
2. Enciende el LED verde luego de 500 ms  
3. Enciende el LED azul luego de 1500 ms

En este caso se encenderían los LEDs en este orden: **verde → rojo → azul**

---

## 🧠 Arquitectura

El sistema se organiza en tres tareas:

### 🔴 `task_a`: Parpadeo del LED

- Se encarga de leer periódicamente el color actual desde el módulo `color`.
- Hace parpadear el LED RGB con ese color.
- El efecto es un ON/OFF intermitente cada 100 ms.

### 🟡 `task_b`: Recepción de comandos UART

- Recibe texto desde UART (soporta comandos largos).
- Guarda la entrada parcial hasta recibir un `\n`.
- Cuando se completa una línea, la parsea en estructuras `color_command_t`.
- Intenta encolar cada comando individualmente en la `Queue`.
- Si la cola está llena, **reintenta hasta 5 veces antes de descartar** el comando y mostrar un error.

### 🟢 `task_c`: Procesamiento de comandos

- Lee comandos encolados.
- Cambia el color global activo.
- Inicia un timer que, al completarse, indica volver al estado anterior.

---

## 🔄 Múltiples comandos y robustez del encolado

Se pueden enviar **comandos largos y múltiples** como:

rojo-500,azul-1000,verde-500,rojo-5000,verde-2000,rojo-500,azul-1000,verde-500,rojo-5000,verde-2000,rojo-500,azul-1000,verde-500,rojo-5000,verde-2000,rojo-500,azul-1000,verde-500,rojo-5000,verde-2000,rojo-500,azul-1000,verde-500,rojo-5000,verde-2000,rojo-500,azul-1000,verde-500,rojo-5000,verde-2000,rojo-500,azul-1000,verde-500,rojo-5000,verde-2000,rojo-500,azul-1000,verde-500,rojo-5000,verde-2000,rojo-500,azul-1000,verde-500,rojo-5000,verde-2000,rojo-500,azul-1000,verde-500,rojo-5000,verde-2000,rojo-500,azul-1000,verde-500,rojo-5000,verde-2000.


> 📌 **Importante**: El mensaje debe terminar en `.` (punto) para indicar fin de entrada y evitar pérdida de comandos por buffer.

---

## 🧪 Formato del comando

Cada comando debe seguir este formato:
<color>-<tiempo_ms>

- `color`: puede ser `rojo`, `verde` o `azul`
- `tiempo_ms`: número entero con la duración en milisegundos

### ✅ Entrada válida

### ❌ Entradas inválidas
- `rojo1000` ← falta el guión
- `amarillo-500` ← color no reconocido
- `verde-dosmil` ← tiempo no numérico

Las entradas inválidas son descartadas y se muestra un mensaje por consola.

> Puedes ajustar estos valores en `color.c` según tu hardware.

---

## ⚙️ Reintentos y cola

Cuando se reciben muchos comandos y la cola está llena:

- El sistema reintenta encolar hasta 5 veces con espera.
- Si luego de esos intentos no hay espacio, el comando es descartado y se muestra en el log.

Podés aumentar el tamaño de la cola si esperás muchos comandos seguidos:
```c
#define QUEUE_LENGTH 30  // por ejemplo
