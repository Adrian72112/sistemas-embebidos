#include "wifistation.h"
#include "wifiap.h"        // ① incluir el header del AP
#include "esp_log.h"

void app_main(void) {
    //Lo movemos aquí para que no rompa al inicializar en ambos lados
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    const char* ssid     = "RedTese";
    const char* password = "Tituscan2022";

    ESP_LOGI("main", "Iniciando estación WiFi...");
    esp_err_t ret = wifi_init_station(ssid, password);
    if (ret == ESP_OK) {
        ESP_LOGI("main", "WiFi conectado correctamente");
    } else {
        ESP_LOGE("main", "No se pudo conectar al WiFi");
    }

    // ② arranca el SoftAP sin más cambios en main
    ESP_LOGI("main", "Iniciando punto de acceso WiFi...");
    wifi_init_softap(/*openWifi=*/0, /*hidden=*/0);
}
