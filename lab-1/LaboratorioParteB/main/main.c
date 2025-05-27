#include "led.h"

void app_main(void)
{
    led_strip_t *strip;
    if (led_init(&strip) != ESP_OK) {
        return;
    }

    led_blink_colors_loop(strip);
}