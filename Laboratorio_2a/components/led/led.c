#include "led.h"
#include "delay.h"

esp_err_t led_init(led_strip_t **strip)
{
    return led_rgb_init(strip); // usa el init que nos dio el profe
}

void led_set_color(led_strip_t *strip, uint8_t r, uint8_t g, uint8_t b)
{
    strip->set_pixel(strip, 0, r, g, b);
    strip->refresh(strip, 100);
}

void led_off(led_strip_t *strip)
{
    strip->clear(strip, 100);
}

void led_blink_colors_loop(led_strip_t *strip)
{
    while (1)
    {
        led_set_color(strip, 255, 0, 0); // rojo
        delay_ms(500);
        led_off(strip);
        delay_ms(500);

        led_set_color(strip, 0, 255, 0); // verde
        delay_ms(500);
        led_off(strip);
        delay_ms(500);

        led_set_color(strip, 0, 0, 255); // azul
        delay_ms(500);
        led_off(strip);
        delay_ms(500);
    }
}
