#include "ntp_sync.h"
#include <stdio.h>
#include "esp_log.h"
#include <string.h>
#include "esp_sntp.h"
#include <time.h>

#define TAG "NTP_SYNC"

/*Inicializa el cliente sntp que es el encargado de consultar la hora actual a un servidor NTP (como pool.ntp.org)
 usando internet*/
void ntp_initialize(void) {
    ESP_LOGI(TAG, "Inicializando SNTP...");
    sntp_setoperatingmode(SNTP_OPMODE_POLL); // CONSULTA LA HORA PERIODICAMENTE con el modo POLL
    sntp_setservername(0, "pool.ntp.org"); // Le indicamos al servidor publico pool.ntp.org, 0 indice del servidor
    sntp_init();
}

void ntp_wait_for_sync(void) {
    time_t now = 0;
    struct tm timeinfo = { 0 };

    int retry = 0;
    const int retry_count = 10;

    ESP_LOGI(TAG, "⏳ Esperando sincronizacion NTP...");
    
    while (timeinfo.tm_year < (2016 - 1900) && ++retry < retry_count) {
        ESP_LOGI(TAG, "🔄 Intento de sincronizacion NTP... (%d/%d)", retry, retry_count);
        vTaskDelay(pdMS_TO_TICKS(2000));
        time(&now);
        localtime_r(&now, &timeinfo);
        
        // Log del tiempo actual para debug
        ESP_LOGI(TAG, "⏰ Tiempo actual: %lld segundos desde epoch, año: %d", 
                 (long long)now, timeinfo.tm_year + 1900);
    }

    if (timeinfo.tm_year >= (2016 - 1900)) {
        // Configurar zona horaria de Uruguay (UTC-3)
        // En el formato TZ, el signo es invertido: UTC-3 se escribe como "UYT3"
        setenv("TZ", "UYT3", 1);  // Uruguay Time (UTC-3)
        tzset();                   // Aplica la zona horaria al sistema
        
        // Actualizar tiempo después de configurar zona horaria
        time(&now);
        localtime_r(&now, &timeinfo);
        
        ESP_LOGI(TAG, "✅ Hora sincronizada exitosamente!");
        
        // Mostrar hora UTC
        struct tm utc_time;
        gmtime_r(&now, &utc_time);
        ESP_LOGI(TAG, "🌍 Hora UTC: %04d-%02d-%02d %02d:%02d:%02d", 
                 utc_time.tm_year + 1900, utc_time.tm_mon + 1, utc_time.tm_mday,
                 utc_time.tm_hour, utc_time.tm_min, utc_time.tm_sec);
        
        // Mostrar hora local (Uruguay)
        ESP_LOGI(TAG, "🏠 Hora local Uruguay: %04d-%02d-%02d %02d:%02d:%02d", 
                 timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
                 timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
        
        ESP_LOGI(TAG, "🕐 Timestamp Unix: %lld", (long long)now);
        ESP_LOGI(TAG, "🌍 Zona horaria: Uruguay (UTC-3)");
        ESP_LOGI(TAG, "🔍 Verificacion: UTC timestamp vs local time diferencia esperada: 3 horas");
    } else {
        ESP_LOGW(TAG, "⚠️ No se logro sincronizar la hora via NTP");
        ESP_LOGW(TAG, "⚠️ Los timestamps del logger usaran el tiempo del sistema");
        
        // Log del tiempo actual aunque no esté sincronizado
        time(&now);
        ESP_LOGW(TAG, "⏰ Tiempo no sincronizado: %lld segundos", (long long)now);
    }
}
