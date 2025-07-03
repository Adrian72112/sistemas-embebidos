#include "led_strip.h"
/**
 * @brief Inicializa y configura el módulo de Touch Pad.
 *
 * Esta función realiza la configuración de los canales táctiles, habilita
 * la función de denoise, arranca el FSM de muestreo y deja listo el sistema
 * para la lectura de toques.
 *
 * @note Antes de llamar a esta función, el LED RGB debe estar inicializado
 *       si se va a utilizar la funcionalidad de control de iluminación.
 */
void configure_touch_pad(void);

/**
 * @brief Bucle de lectura de valores de Touch Pad y control del LED.
 *
 * Entra en un bucle infinito donde se lee periódicamente el valor bruto de
 * cada touch pad. Cuando se detecta un toque (valor supera umbral y se cumple
 * el tiempo de debounce), se dispara la acción correspondiente sobre el LED
 * RGB integrado (cambio de color, ajuste de brillo o parpadeo).
 *
 * @warning Función bloqueante que itera sin fin.
 */
void tp_read(void *pvParameters);
void tp_set_led_strip(led_strip_t *strip);