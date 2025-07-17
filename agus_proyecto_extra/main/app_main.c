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

// Declaraciones de funciones
static void mqtt_app_start(void);
static const char *TAG = "MQTT_MAIN";

// Cliente MQTT global
esp_mqtt_client_handle_t client = NULL;

// Declarar el semáforo globalmente en app_main.c
SemaphoreHandle_t uart_data_ready_semaphore;

// Semáforo para sincronizar la conexión WiFi
SemaphoreHandle_t wifi_connected_semaphore;

// Cola global para mensajes MQTT personalizados desde UART
QueueHandle_t mqtt_custom_message_queue;

// Variable para controlar el estado de conexión WiFi
static bool wifi_connected = false;

// Variable para controlar el estado de conexión MQTT
static bool mqtt_connected = false;

// Función de callback para mensajes MQTT
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32, base, event_id);
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;

    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT CONECTADO EXITOSAMENTE");
            mqtt_connected = true;
            
            // Suscribirse al tópico automáticamente al conectar
            int sub_id = esp_mqtt_client_subscribe(event->client, mqtt_topic, 0);
            if (sub_id != -1) {
                ESP_LOGI(TAG, "Suscrito al tópico: '%s' (msg_id: %d)", mqtt_topic, sub_id);
            } else {
                ESP_LOGE(TAG, "Error al suscribirse al tópico: '%s'", mqtt_topic);
            }
            break;

        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGW(TAG, "MQTT DESCONECTADO");
            mqtt_connected = false;
            break;

        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "SUSCRIPCIÓN CONFIRMADA - msg_id=%d", event->msg_id);
            break;

        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG, "DESUSCRIPCIÓN CONFIRMADA - msg_id=%d", event->msg_id);
            break;

        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "PUBLICACIÓN CONFIRMADA - msg_id=%d", event->msg_id);
            break;

        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "MENSAJE MQTT RECIBIDO ");
            ESP_LOGI(TAG, "Tópico: %.*s", event->topic_len, event->topic);
            ESP_LOGI(TAG, "Mensaje: %.*s", event->data_len, event->data);
            ESP_LOGI(TAG, "QoS: %d, Retain: %d", event->qos, event->retain);
            ESP_LOGI(TAG, "==========================================");
            
            // También mostrar en printf para mayor visibilidad
            printf("\n=== MENSAJE MQTT RECIBIDO ===\n");
            printf("Tópico: %.*s\n", event->topic_len, event->topic);
            printf("Mensaje: %.*s\n", event->data_len, event->data);
            printf("=============================\n\n");
            break;

        case MQTT_EVENT_ERROR:
            ESP_LOGE(TAG, "MQTT_EVENT_ERROR");
            mqtt_connected = false;
            if (event->error_handle) {
                if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
                    ESP_LOGE(TAG, "Error de transporte TCP: 0x%x", event->error_handle->connect_return_code);
                }
                else if (event->error_handle->error_type == MQTT_ERROR_TYPE_CONNECTION_REFUSED) {
                    ESP_LOGE(TAG, "Error de conexión MQTT: %d", event->error_handle->connect_return_code);
                }
                else if (event->error_handle->error_type == MQTT_ERROR_TYPE_NONE && event->error_handle->connect_return_code != 0) {
                     ESP_LOGE(TAG, "Error de protocolo MQTT (código de retorno): %d", event->error_handle->connect_return_code);
                }
                else if (event->error_handle->error_type == MQTT_ERROR_TYPE_ESP_TLS) {
                    ESP_LOGE(TAG, "Error TLS: 0x%x", event->error_handle->esp_tls_stack_err);
                }
                else {
                    ESP_LOGE(TAG, "Otro tipo de error MQTT. Código de retorno: %d, Tipo de error: %d",
                             event->error_handle->connect_return_code, event->error_handle->error_type);
                }
            } else {
                ESP_LOGE(TAG, "MQTT_EVENT_ERROR sin detalles de error (error_handle es NULL).");
            }
            break;

        default:
            ESP_LOGI(TAG, "Otro evento MQTT ID: %" PRIi32, event_id);
            break;
    }
}

// Tarea de monitoreo de conexiones - Verifica y reestablece WiFi/MQTT
static void connection_monitor_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Tarea de monitoreo de conexiones iniciada");
    
    while (true) {
        // Monitorear cada 15 segundos
        vTaskDelay(pdMS_TO_TICKS(15000));
        
        // Verificar estado de WiFi
        if (!wifi_connected) {
            ESP_LOGW(TAG, "WiFi desconectado detectado, intentando reconectar...");
            esp_wifi_connect();
        }
        
        // Verificar estado de MQTT
        if (wifi_connected && !mqtt_connected && client != NULL) {
            ESP_LOGW(TAG, "MQTT desconectado detectado, intentando reconectar...");
            esp_mqtt_client_reconnect(client);
        }
        
        // Si no hay cliente MQTT pero hay WiFi, reinicializar MQTT
        if (wifi_connected && client == NULL) {
            ESP_LOGW(TAG, "Cliente MQTT no existe, reinicializando...");
            mqtt_app_start();
        }
        
        // Log de estado cada minuto (4 ciclos de 15 segundos)
        static int status_counter = 0;
        if (++status_counter >= 4) {
            ESP_LOGI(TAG, "Estado: WiFi=%s, MQTT=%s", 
                     wifi_connected ? "si" : "no", 
                     mqtt_connected ? "si" : "no");
            status_counter = 0;
        }
    }
}

// Tarea para publicar mensajes MQTT periódicamente - Versión completa con reconexión
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

    // Esperar un tiempo inicial para que MQTT se conecte
    vTaskDelay(pdMS_TO_TICKS(5000));

    // Contador para publicaciones periódicas (cada 50 segundos)
    TickType_t last_periodic_publish = xTaskGetTickCount();

    while (true) {
        // Verificar estado de conexión WiFi
        if (!wifi_connected) {
            ESP_LOGW(TAG, "WiFi no conectado, esperando reconexión...");
            vTaskDelay(pdMS_TO_TICKS(5000));
            continue;
        }

        // Verificar estado de conexión MQTT
        if (!mqtt_connected) {
            ESP_LOGW(TAG, "MQTT no conectado, esperando reconexión...");
            vTaskDelay(pdMS_TO_TICKS(5000));
            continue;
        }

        // Verificar si hay mensajes personalizados en la cola (sin bloqueo)
        mqtt_custom_message_t custom_msg;
        if (xQueueReceive(mqtt_custom_message_queue, &custom_msg, 0) == pdTRUE) {
            if (client != NULL) {
                ESP_LOGI(TAG, "Publicando mensaje personalizado: '%s' al tópico: '%s'", 
                         custom_msg.message, mqtt_topic);
                
                int msg_id = esp_mqtt_client_publish(client, mqtt_topic, custom_msg.message, 0, 1, 0);
                
                if (msg_id != -1) {
                    ESP_LOGI(TAG, "Mensaje personalizado publicado exitosamente - ID: %d", msg_id);
                } else {
                    ESP_LOGW(TAG, "Fallo al publicar mensaje personalizado");
                    mqtt_connected = false;
                }
            }
        }

        // Verificar si es tiempo de enviar el mensaje periódico (cada 50 segundos)
        TickType_t current_time = xTaskGetTickCount();
        if ((current_time - last_periodic_publish) >= pdMS_TO_TICKS(50000)) {
            if (client != NULL) { 
                // Crear el mensaje periódico
                sprintf(payload, "%s activo", device_id);

                ESP_LOGI(TAG, "Publicando mensaje periódico: '%s' al tópico: '%s'", payload, mqtt_topic);
                int msg_id = esp_mqtt_client_publish(client, mqtt_topic, payload, 0, 1, 0);
                
                if (msg_id != -1) {
                    ESP_LOGI(TAG, "Mensaje periódico publicado exitosamente - ID: %d", msg_id);
                    last_periodic_publish = current_time;
                } else {
                    ESP_LOGW(TAG, "Fallo al publicar mensaje periódico. Verificando conexión...");
                    mqtt_connected = false; // Marcar como desconectado para intentar reconectar
                }
            } else {
                ESP_LOGW(TAG, "Cliente MQTT no inicializado, reintentando inicialización...");
                // Intentar reinicializar MQTT
                mqtt_app_start();
            }
        }

        // Esperar un poco antes del siguiente ciclo
        vTaskDelay(pdMS_TO_TICKS(1000)); // Verificar cola cada segundo
    }
}


// Event handler para eventos WiFi - Con lógica de reconexión mejorada
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        ESP_LOGI(TAG, "WiFi iniciado, conectando...");
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        wifi_connected = false;
        mqtt_connected = false; // También marcar MQTT como desconectado
        ESP_LOGW(TAG, "WiFi desconectado, reintentando conexión en 5 segundos...");
        
        // Esperar un poco antes de reconectar para evitar loops muy rápidos
        vTaskDelay(pdMS_TO_TICKS(5000));
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "WiFi conectado! IP obtenida: " IPSTR, IP2STR(&event->ip_info.ip));
        wifi_connected = true;
        xSemaphoreGive(wifi_connected_semaphore);
        
        // Si WiFi se reconectó, intentar reconectar MQTT también
        if (client != NULL && !mqtt_connected) {
            ESP_LOGI(TAG, "WiFi reconectado, reintentando conexión MQTT...");
            esp_mqtt_client_reconnect(client);
        }
    }
}

// Inicializar MQTT con event handler y configuración completa
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

    // Si ya existe un cliente, destruirlo primero
    if (client != NULL) {
        ESP_LOGI(TAG, "Destruyendo cliente MQTT existente...");
        esp_mqtt_client_stop(client);
        esp_mqtt_client_destroy(client);
        client = NULL;
    }

    // Configuración MQTT mejorada con timeouts y reconexión automática
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker = {
            .address.uri = mqtt_uri,
        },
        .session = {
            .keepalive = 60,
            .disable_clean_session = false,
        },
        .network = {
            .timeout_ms = 10000,
            .refresh_connection_after_ms = 20000,
            .disable_auto_reconnect = false, // Habilitar reconexión automática
        },
        .task = {
            .priority = 5,
            .stack_size = 6144,
        },
    };

    client = esp_mqtt_client_init(&mqtt_cfg);
    if (client == NULL) {
        ESP_LOGE(TAG, "Fallo al inicializar el cliente MQTT.");
        return;
    }
    
    ESP_LOGI(TAG, "Cliente MQTT inicializado, registrando event handler...");
    
    // Registrar el event handler
    esp_err_t err = esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error registrando event handler MQTT: %s", esp_err_to_name(err));
        return;
    }
    
    ESP_LOGI(TAG, "Iniciando cliente MQTT...");
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
    ESP_LOGI(TAG, "INICIANDO APLICACIÓN ESP32-S2");
    
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
    
    // Crear cola para mensajes personalizados MQTT (capacidad para 10 mensajes)
    mqtt_custom_message_queue = xQueueCreate(10, sizeof(mqtt_custom_message_t));
    
    if (uart_data_ready_semaphore == NULL || wifi_connected_semaphore == NULL || mqtt_custom_message_queue == NULL) {
        ESP_LOGE(TAG, "Fallo al crear los semáforos o colas. Reiniciando...");
        esp_restart();
    }
    ESP_LOGI(TAG, "Semáforos y colas creados correctamente");

    // 1. Iniciar UART para recibir comandos (SSID, Contraseña, Tópico MQTT)
    ESP_LOGI(TAG, "Inicializando UART...");
    leido_uart_init(uart_data_ready_semaphore, mqtt_custom_message_queue);

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
    ESP_LOGI(TAG, "INICIANDO CONEXIÓN WIFI");
    wifi_init_sta();

    // Esperar a que WiFi se conecte
    ESP_LOGI(TAG, "Esperando conexión WiFi...");
    if (xSemaphoreTake(wifi_connected_semaphore, pdMS_TO_TICKS(30000)) == pdTRUE) {
        ESP_LOGI(TAG, "WIFI CONECTADO EXITOSAMENTE");
    } else {
        ESP_LOGE(TAG, "TIMEOUT: NO SE PUDO CONECTAR AL WIFI");
        ESP_LOGE(TAG, "Verifique SSID y contraseña. SSID: '%s'", wifi_ssid);
        return;
    }

    // 4. Iniciar conexión al broker MQTT
    ESP_LOGI(TAG, "INICIANDO CONEXIÓN MQTT");
    mqtt_app_start();

    // Esperar un poco para que MQTT se conecte
    vTaskDelay(pdMS_TO_TICKS(3000));

    // 5. Crear la tarea de monitoreo de conexiones
    ESP_LOGI(TAG, "CREANDO TAREA DE MONITOREO");
    xTaskCreate(connection_monitor_task, "conn_monitor", 3072, NULL, 4, NULL);

    // 6. Crear la tarea de publicación MQTT después de iniciar el cliente
    ESP_LOGI(TAG, "CREANDO TAREA DE PUBLICACIÓN MQTT");
    xTaskCreate(mqtt_publish_task, "mqtt_pub_task", 4096, NULL, 5, NULL);

    ESP_LOGI(TAG, "APLICACIÓN INICIADA COMPLETAMENTE");
    ESP_LOGI(TAG, "Tareas activas:");
    ESP_LOGI(TAG, "UART: Escuchando comandos (!pub <mensaje> para enviar por MQTT)");
    ESP_LOGI(TAG, "WiFi: Monitoreando conexión");
    ESP_LOGI(TAG, "MQTT: Publicando cada 50 segundos + mensajes personalizados");
    ESP_LOGI(TAG, "Monitor: Verificando conexiones cada 15 segundos");
    ESP_LOGI(TAG, "Los mensajes MQTT recibidos se mostrarán automáticamente");
    ESP_LOGI(TAG, "Para enviar mensaje personalizado: !pub <tu_mensaje>");
    
    // La tarea principal de app_main continuará ejecutándose.
}