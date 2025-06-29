#include "mqtt_lib.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"
#include "esp_log.h"

#define BROKER_URI "mqtt://localhost"

static const char *TAG = "main";

void my_callback(const char *topic, const char *data, int len)
{
    ESP_LOGI(TAG, "Mensaje recibido -> Topic: %s, Data: %s", topic, data);
}

void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(example_connect());

    ESP_ERROR_CHECK(mqtt_lib_init(BROKER_URI, my_callback));
}
