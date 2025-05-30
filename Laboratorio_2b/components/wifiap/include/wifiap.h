#include "esp_event.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "lwip/err.h"
#include "lwip/sys.h"

/** SSID por defecto del punto de acceso */
#define EXAMPLE_ESP_WIFI_SSID      "chorizo_seco"
/** Contraseña por defecto del punto de acceso */
#define EXAMPLE_ESP_WIFI_PASS      "conpancasero"
/** Canal WiFi por defecto (configurable en sdkconfig) */
#define EXAMPLE_ESP_WIFI_CHANNEL   CONFIG_ESP_WIFI_CHANNEL
/** Número máximo de estaciones permitidas */
#define EXAMPLE_MAX_STA_CONN       CONFIG_ESP_MAX_STA_CONN



/**
 * @brief Manejador de eventos del SoftAP.
 *
 * Procesa los eventos de conexión y desconexión de estaciones en modo AP:
 *  - WIFI_EVENT_AP_STACONNECTED: una estación se ha conectado.
 *  - WIFI_EVENT_AP_STADISCONNECTED: una estación se ha desconectado.
 *
 * @param arg         Parámetro de usuario (no utilizado).
 * @param event_base  Base del evento (debe ser WIFI_EVENT).
 * @param event_id    Identificador del evento (ej. WIFI_EVENT_AP_STACONNECTED).
 * @param event_data  Puntero a datos específicos del evento.
 */
void wifi_event_handler(void* arg,
                        esp_event_base_t event_base,
                        int32_t event_id,
                        void* event_data);

/**
 * @brief Inicializa y arranca el punto de acceso WiFi (SoftAP).
 *
 * Configura la interfaz de red, el driver WiFi y el manejador de eventos,
 * y arranca el AP con los parámetros definidos por defecto (SSID,
 * contraseña, canal y número máximo de conexiones).
 *
 * @param openWifi  Si es 1, el AP estará en modo abierto (WIFI_AUTH_OPEN).
 *                  Si es 0, usará la contraseña definida en EXAMPLE_ESP_WIFI_PASS.
 * @param hidden    Si es 1, ocultará el SSID; si es 0, el SSID será visible.
 */
void wifi_init_softap(int openWifi, int hidden);

/**
 * @brief Inicializa la memoria no volátil (NVS).
 *
 * Arranca el subsistema de NVS; si detecta que está lleno o hay
 * una versión nueva, borra y vuelve a inicializar el espacio NVS.
 */
void init_nvs(void);