# Componente `web_server`

## Descripción
Este componente implementa un servidor web para la configuración y control del sistema, permitiendo interactuar con la placa ESP32-S2 desde un navegador.

## Funcionalidades principales
- **Interfaz web**: Sirve páginas HTML embebidas para configuración y estado del sistema.
- **Procesamiento de comandos**: Recibe comandos desde la interfaz web y los publica vía MQTT.
- **Manejo de configuraciones**: Permite guardar configuraciones WiFi y MQTT desde el navegador.
- **Estado dinámico**: Muestra información actualizada sobre la conexión WiFi, MQTT, y tiempo activo.

## Páginas servidas
- `/`: Página principal con el modo actual del sistema.
- `/config`: Página de configuración.
- `/save_config`: Procesa y guarda configuraciones.
- `/status`: Muestra el estado del sistema.
- `/clear_config`: Borra configuraciones y reinicia el sistema.
- `/style.css`: Hoja de estilos.

## Ejemplo de uso
```c
httpd_handle_t server = start_web_server();
if (server) {
    ESP_LOGI("web_server", "Servidor web iniciado");
} else {
    ESP_LOGE("web_server", "Error iniciando servidor web");
}
```

## Dependencias
- `esp_http_server`: Para manejo de solicitudes HTTP.
- `wifi_connection`: Para cargar configuraciones WiFi.
- `mqtt_lib`: Para publicar comandos vía MQTT.
- `freertos/FreeRTOS.h`: Para cálculo de tiempo activo.
