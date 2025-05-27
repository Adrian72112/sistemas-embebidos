#include <string.h>
#include "wifiap.h"
#include "esp_log.h"


void app_main(void)
{
    init_nvs();
    ESP_LOGI(TAG, "ESP_WIFI_MODE_AP");

    // Set to 1 for open WiFi, 0 for secured WiFi
    wifi_init_softap(1, 1); 
}
