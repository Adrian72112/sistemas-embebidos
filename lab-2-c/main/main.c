#include "wifistation.h"
#include "esp_log.h"

void app_main(void) {
    const char* ssid = "RedTese";
    const char* password = "Tituscan2022";

    ESP_LOGI("main", "Iniciando estación WiFi...");
    esp_err_t ret = wifi_init_station(ssid, password);
    if (ret == ESP_OK) {
        ESP_LOGI("main", "WiFi conectado correctamente");
    } else {
        ESP_LOGE("main", "No se pudo conectar al WiFi");
    }
}