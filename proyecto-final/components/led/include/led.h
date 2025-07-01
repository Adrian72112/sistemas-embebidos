#include "led_strip.h"
#include "esp_err.h"

/**
 * @brief Inicializa el LED RGB embebido de la placa ESP32-S2-Kaluga-1
 * 
 * Esta función configura el periférico RMT y crea una instancia de la estructura
 * led_strip_t para controlar el LED RGB embebido en la placa.
 * 
 * @param[out] strip Puntero al puntero donde se almacenará la instancia del LED
 * @return
 *      - ESP_OK si se inicializó correctamente
 *      - ESP_FAIL si ocurrió un error en la inicialización
 */
esp_err_t led_init(led_strip_t **strip);

/**
 * @brief Establece el color del LED RGB
 * 
 * Esta función enciende el LED RGB con los valores de color indicados para los
 * canales rojo, verde y azul. El cambio se refleja inmediatamente.
 * 
 * @param[in] strip Instancia del LED previamente inicializada con led_init
 * @param[in] r Valor del canal rojo (0–255)
 * @param[in] g Valor del canal verde (0–255)
 * @param[in] b Valor del canal azul (0–255)
 */
void led_set_color(led_strip_t *strip, uint8_t r, uint8_t g, uint8_t b);

/**
 * @brief Apaga el LED RGB
 * 
 * Esta función apaga el LED RGB estableciendo todos los canales en 0 (negro)
 * y actualizando el estado de la tira.
 * 
 * @param[in] strip Instancia del LED previamente inicializada con led_init
 */
void led_off(led_strip_t *strip);

/**
 * @brief Parpadea el LED RGB indefinidamente cambiando de color
 * 
 * Esta función hace que el LED RGB parpadee en los colores rojo, verde y azul,
 * con un intervalo fijo entre encendido y apagado. El parpadeo es bloqueante
 * y se repite en bucle infinito.
 * 
 * @param[in] strip Instancia del LED previamente inicializada con led_init
 */
void led_blink_colors_loop(led_strip_t *strip);
