#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"    // Incluir FreeRTOS semaforos
#include "esp_log.h"
#include "esp_system.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "mqtt_client.h"
#include "esp_netif.h"
#include "esp_mac.h"
#include "sdkconfig.h"
#include "leido_uart.h" // Nuestro componente UART, que declara externas las variables

static const char *TAG = "MQTT_MAIN";

// Cliente MQTT global
esp_mqtt_client_handle_t client = NULL;

// Declarar el semáforo globalmente en app_main.c
SemaphoreHandle_t uart_data_ready_semaphore;

// Semáforo para sincronizar la conexión WiFi
SemaphoreHandle_t wifi_connected_semaphore;

// Variable para controlar el estado de conexión WiFi
static bool wifi_connected = false;

// Tarea para publicar mensajes MQTT periódicamente - Versión simplificada
static void mqtt_publish_task(void *pvParameters)
{
    char payload[100]; // Buffer para el mensaje a publicar
    // Puedes obtener un ID único para tu dispositivo, por ejemplo, usando la MAC address
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    // Formato el ID del dispositivo
    char device_id[13]; // 12 caracteres para la MAC + null terminator
    sprintf(device_id, "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    ESP_LOGI(TAG, "Tarea MQTT iniciada. Device ID: %s", device_id);

    // Esperar un tiempo para que MQTT se conecte
    vTaskDelay(pdMS_TO_TICKS(5000));

    while (true) {
        // Verificar estado de conexión WiFi y cliente MQTT
        if (!wifi_connected) {
            ESP_LOGW(TAG, "WiFi no conectado, esperando...");
            vTaskDelay(pdMS_TO_TICKS(5000));
            continue;
        }

        if (client != NULL) { 
            // Crea el mensaje a enviar. Por ejemplo: "<ID> activo"
            sprintf(payload, "%s activo", device_id);

            ESP_LOGI(TAG, "Publicando mensaje: '%s' al tópico: '%s'", payload, mqtt_topic);
            int msg_id = esp_mqtt_client_publish(client, mqtt_topic, payload, 0, 1, 0);
            if (msg_id != -1) {
                ESP_LOGI(TAG, "✓ Mensaje publicado al tópico '%s', ID: %d", mqtt_topic, msg_id);
                
                // También intentar suscribirse al tópico si aún no lo hemos hecho
                static bool subscribed = false;
                if (!subscribed) {
                    int sub_id = esp_mqtt_client_subscribe(client, mqtt_topic, 0);
                    if (sub_id != -1) {
                        ESP_LOGI(TAG, "✓ Suscrito al tópico '%s'", mqtt_topic);
                        subscribed = true;
                    }
                }
            } else {
                ESP_LOGW(TAG, "✗ Fallo al publicar mensaje. Cliente MQTT probablemente no conectado. Reintentando...");
            }
        } else {
            ESP_LOGW(TAG, "Cliente MQTT no inicializado, esperando...");
        }

        // Retraso de 30 segundos entre publicaciones (reducido para testing)
        vTaskDelay(pdMS_TO_TICKS(30000));
    }
}


// Event handler para eventos WiFi
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        ESP_LOGI(TAG, "WiFi iniciado, conectando...");
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        wifi_connected = false;
        ESP_LOGW(TAG, "WiFi desconectado, reintentando conexión...");
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "WiFi conectado! IP obtenida: " IPSTR, IP2STR(&event->ip_info.ip));
        wifi_connected = true;
        xSemaphoreGive(wifi_connected_semaphore);
    }
}

// Inicializar MQTT con el tópico - Enfoque básico compatible
static void mqtt_app_start(void)
{
    // Validar que tengamos URI y topic válidos
    if (strlen(mqtt_uri) == 0) {
        ESP_LOGE(TAG, "ERROR: URI MQTT vacío!");
        return;
    }

    if (strlen(mqtt_topic) == 0) {
        ESP_LOGE(TAG, "ERROR: Tópico MQTT vacío!");
        return;
    }

    ESP_LOGI(TAG, "Inicializando cliente MQTT con URI: '%s' y tópico: '%s'", mqtt_uri, mqtt_topic);

    // Configuración MQTT básica
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = mqtt_uri,
    };

    client = esp_mqtt_client_init(&mqtt_cfg);
    if (client == NULL) {
        ESP_LOGE(TAG, "Fallo al inicializar el cliente MQTT.");
        return;
    }
    
    ESP_LOGI(TAG, "Cliente MQTT inicializado correctamente, iniciando...");
    
    esp_err_t start_err = esp_mqtt_client_start(client);
    if (start_err != ESP_OK) {
        ESP_LOGE(TAG, "Error iniciando cliente MQTT: %s", esp_err_to_name(start_err));
        return;
    }
    
    ESP_LOGI(TAG, "Cliente MQTT iniciado exitosamente");
}

// Inicializar WiFi en modo estación
static void wifi_init_sta(void)
{
    // Validar que tengamos datos WiFi válidos
    if (strlen(wifi_ssid) == 0 || strlen(wifi_pass) == 0) {
        ESP_LOGE(TAG, "ERROR: SSID o contraseña WiFi vacíos. SSID len: %d, Pass len: %d", 
                 strlen(wifi_ssid), strlen(wifi_pass));
        return;
    }

    ESP_LOGI(TAG, "Configurando WiFi con SSID: '%s', Password: [%d caracteres]", 
             wifi_ssid, strlen(wifi_pass));

    // Inicializar configuración WiFi
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = {0}, // Inicializa a cero para strncpy
            .password = {0}, // Inicializa a cero para strncpy
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = {
                .capable = true,
                .required = false,
            },
        },
    };

    // Copiar valores desde las variables globales (definidas en leido_uart.c)
    strncpy((char *)wifi_config.sta.ssid, wifi_ssid, sizeof(wifi_config.sta.ssid) - 1);
    wifi_config.sta.ssid[sizeof(wifi_config.sta.ssid) - 1] = '\0';
    strncpy((char *)wifi_config.sta.password, wifi_pass, sizeof(wifi_config.sta.password) - 1);
    wifi_config.sta.password[sizeof(wifi_config.sta.password) - 1] = '\0';

    ESP_LOGI(TAG, "Configuración WiFi aplicada. SSID configurado: '%s'", wifi_config.sta.ssid);

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "WiFi iniciado con SSID: %s", wifi_ssid);
}

void app_main(void)
{
    ESP_LOGI(TAG, "=== INICIANDO APLICACIÓN ESP32-S2 ===");
    
    // Inicializar NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_LOGI(TAG, "NVS inicializado correctamente");

    // Crear los semáforos binarios
    uart_data_ready_semaphore = xSemaphoreCreateBinary();
    wifi_connected_semaphore = xSemaphoreCreateBinary();
    
    if (uart_data_ready_semaphore == NULL || wifi_connected_semaphore == NULL) {
        ESP_LOGE(TAG, "Fallo al crear los semáforos. Reiniciando...");
        esp_restart();
    }
    ESP_LOGI(TAG, "Semáforos creados correctamente");

    // 1. Iniciar UART para recibir comandos (SSID, Contraseña, Tópico MQTT)
    ESP_LOGI(TAG, "Inicializando UART...");
    leido_uart_init(uart_data_ready_semaphore);

    // 2. Inicializar WiFi y Event Loop antes de esperar
    ESP_LOGI(TAG, "Inicializando subsistemas de red...");
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    // Registrar event handlers WiFi
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL));
    ESP_LOGI(TAG, "Event handlers WiFi registrados");

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_LOGI(TAG, "WiFi inicializado");

    // Esperar aquí hasta que los datos de UART sean recibidos y el semáforo sea liberado
    ESP_LOGI(TAG, "=== ESPERANDO COMANDOS UART ===");
    ESP_LOGI(TAG, "Envía los siguientes comandos:");
    ESP_LOGI(TAG, "  !wifi <ssid> <password>");
    ESP_LOGI(TAG, "  !topic <topico>");
    ESP_LOGI(TAG, "  !uri <uri_broker>  (opcional, por defecto: mqtt://broker.hivemq.com:1883)");
    ESP_LOGI(TAG, "  !done");
    
    if (xSemaphoreTake(uart_data_ready_semaphore, portMAX_DELAY) == pdTRUE) {
        ESP_LOGI(TAG, "=== COMANDOS UART RECIBIDOS ===");
        ESP_LOGI(TAG, "WiFi SSID: '%s'", wifi_ssid);
        ESP_LOGI(TAG, "WiFi Password: [%d caracteres]", strlen(wifi_pass));
        ESP_LOGI(TAG, "MQTT Topic: '%s'", mqtt_topic);
        ESP_LOGI(TAG, "MQTT URI: '%s'", mqtt_uri);
    } else {
        ESP_LOGE(TAG, "Error: El semáforo no se liberó inesperadamente.");
        return;
    }

    // 3. Inicializar WiFi usando los datos recibidos del UART
    ESP_LOGI(TAG, "=== INICIANDO CONEXIÓN WIFI ===");
    wifi_init_sta();

    // Esperar a que WiFi se conecte
    ESP_LOGI(TAG, "Esperando conexión WiFi...");
    if (xSemaphoreTake(wifi_connected_semaphore, pdMS_TO_TICKS(30000)) == pdTRUE) {
        ESP_LOGI(TAG, "=== WIFI CONECTADO EXITOSAMENTE ===");
    } else {
        ESP_LOGE(TAG, "=== TIMEOUT: NO SE PUDO CONECTAR AL WIFI ===");
        ESP_LOGE(TAG, "Verifique SSID y contraseña. SSID: '%s'", wifi_ssid);
        return;
    }

    // 4. Iniciar conexión al broker MQTT
    ESP_LOGI(TAG, "=== INICIANDO CONEXIÓN MQTT ===");
    mqtt_app_start();

    // Esperar un poco para que MQTT se conecte
    vTaskDelay(pdMS_TO_TICKS(2000));

    // 5. Crear la tarea de publicación MQTT después de iniciar el cliente
    ESP_LOGI(TAG, "=== CREANDO TAREA DE PUBLICACIÓN MQTT ===");
    xTaskCreate(mqtt_publish_task, "mqtt_pub_task", 4096, NULL, 5, NULL);

    ESP_LOGI(TAG, "=== APLICACIÓN INICIADA COMPLETAMENTE ===");
    // La tarea principal de app_main continuará ejecutándose.
}