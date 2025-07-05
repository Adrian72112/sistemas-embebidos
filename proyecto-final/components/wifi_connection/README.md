# Componente `wifi_connection`

## Descripción
Este componente gestiona la conexión WiFi de la placa ESP32-S2, incluyendo la configuración inicial, el almacenamiento de parámetros en NVS, y el manejo de eventos de conexión/desconexión.

## Funcionalidades principales
- **Configuración WiFi**: Permite guardar y cargar SSID y contraseña en NVS.
- **Modo Configuración**: Habilita un punto de acceso (AP) para configurar la red.
- **Reconexión automática**: Intenta reconectar hasta 5 veces en caso de desconexión.
- **Soporte IPv6**: Configura direcciones IPv6 y maneja tipos de direcciones.

## Uso de NVS
- **Claves utilizadas**:
  - `wifi_ssid`: SSID de la red.
  - `wifi_pass`: Contraseña de la red.
  - `wifi_configured`: Flag que indica si la configuración está completa.

## Eventos manejados
- `WIFI_EVENT_STA_DISCONNECTED`: Gestiona desconexiones y reconexiones.
- `IP_EVENT_STA_GOT_IP`: Maneja la obtención de direcciones IPv4.
- `IP_EVENT_GOT_IP6`: Maneja la obtención de direcciones IPv6.

## Ejemplo de uso
```c
wifi_config_nvs_t wifi_config;
if (wifi_load_config(&wifi_config) == ESP_OK) {
    ESP_LOGI("wifi_connection", "Conectado a SSID: %s", wifi_config.ssid);
} else {
    ESP_LOGW("wifi_connection", "No hay configuración WiFi guardada");
}
```

## Dependencias
- `nvs_flash`: Para almacenamiento persistente.
- `esp_netif`: Para manejo de interfaces de red.
- `freertos/FreeRTOS.h`: Para sincronización con semáforos.
