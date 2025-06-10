/**
 * @brief Crea y lanza la tarea A que hace parpadear el LED RGB.
 * 
 * Esta función debe llamarse una vez durante la inicialización del sistema,
 * típicamente desde `app_main` en `main.c`, luego de haber inicializado el
 * módulo `color` y el LED.
 * 
 * La tarea ejecutada se queda en bucle infinito parpadeando el LED en el color
 * actual definido en `color_get()`.
 */
void start_task_a(void);
