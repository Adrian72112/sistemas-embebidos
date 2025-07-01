#pragma once

#include "esp_err.h"
#include "esp_http_server.h"

/**
 * @brief Inicializa y arranca el servidor web
 * 
 * Este servidor web maneja:
 * - GET / -> Página principal de control
 * - GET /style.css -> Hoja de estilos
 * - GET /comando -> Procesamiento de comandos desde la web
 * 
 * @return ESP_OK si el servidor se inicia correctamente
 */
esp_err_t web_server_start(void);

/**
 * @brief Detiene el servidor web
 * 
 * @return ESP_OK si el servidor se detiene correctamente
 */
esp_err_t web_server_stop(void);

/**
 * @brief Verifica si el servidor web está funcionando
 * 
 * @return true si el servidor está activo, false en caso contrario
 */
bool web_server_is_running(void);
