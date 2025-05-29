#include "esp_err.h"

/**
 * @brief Inicializa y conecta a una red WiFi en modo estación.
 *
 * Esta función inicializa NVS, la interfaz de red, el driver WiFi,
 * registra los manejadores de eventos y conecta a la SSID y contraseña
 * especificados.
 *
 * @param ssid     Cadena C con el nombre de la red WiFi.
 * @param password Cadena C con la contraseña de la red WiFi.
 * @return esp_err_t ESP_OK si tiene éxito, o un código de error ESP.
 */
esp_err_t wifi_init_station(const char* ssid, const char* password);
