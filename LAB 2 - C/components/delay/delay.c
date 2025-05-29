#include <stdio.h>
#include "delay.h"
#include "esp_rom_sys.h"

void delay_ms(uint32_t ms)
{
    if (ms > UINT32_MAX / 1000) {
        // valor demasiado grande
        return;
    }
    esp_rom_delay_us(ms * 1000);
}
