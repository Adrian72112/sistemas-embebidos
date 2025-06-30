#include "ntp_sync.h"
#include "esp_log.h"
#include "esp_sntp.h"
#include <time.h>

#define TAG "NTP_SYNC"

/*Inicializa el cliente sntp que es el encargado de consultar la hora actual a un servidor NTP (como pool.ntp.org)
 usando internet*/
void ntp_initialize(void) {
    ESP_LOGI(TAG, "Inicializando SNTP...");
    sntp_setoperatingmode(SNTP_OPMODE_POLL); // CONSULTA LA HORA PERIODICAMENTE con el modo POLL
    sntp_setservername(0, "pool.ntp.org"); // Le indicamos al servidor publico pool.ntg.org, 0 indice del servidor
    sntp_init();
}

void ntp_wait_for_sync(void) {
    time_t now = 0;
    struct tm timeinfo = { 0 };

    int retry = 0;
    const int retry_count = 10;

    while (timeinfo.tm_year < (2016 - 1900) && ++retry < retry_count) {
        ESP_LOGI(TAG, "Esperando sincronización NTP... (%d/%d)", retry, retry_count);
        vTaskDelay(pdMS_TO_TICKS(2000));
        time(&now);
        localtime_r(&now, &timeinfo);
    }

    if (timeinfo.tm_year >= (2016 - 1900)) {
        ESP_LOGI(TAG, "Hora sincronizada: %s", asctime(&timeinfo));
    } else {
        ESP_LOGW(TAG, "No se logró sincronizar la hora vía NTP");
    }
}
