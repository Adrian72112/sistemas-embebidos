#include <stdio.h>
#include <inttypes.h>
#include "touch_pad.h"
#include "esp_task_wdt.h"

void app_main(void)
{
    //desactivamos el watchdog
    esp_task_wdt_deinit();
    configure_touch_pad();
    tp_read();
}