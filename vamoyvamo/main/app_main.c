#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h> // Agregado para strlen
#include "esp_system.h"
#include "esp_wifi.h"       // Agregado para funciones y constantes de WiFi como ESP_MAC_WIFI_STA y WIFI_SAE_MODE_PWE_BOTH
#include "esp_system.h"     // Agregado para esp_read_mac
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "driver/gpio.h"    // Agregado para funciones GPIO (gpio_set_level, gpio_reset_pin, gpio_set_direction, GPIO_MODE_OUTPUT)
#include "esp_log.h"
#include "mqtt_client.h"    // Agregado para funciones del cliente MQTT (esp_mqtt_client_is_connected)
#include "esp_tls.h"

#ifdef CONFIG_BUILD_TARGET_ESP32S2
#define LED_GPIO_PIN GPIO_NUM_2 // Ejemplo de pin LED para ESP32-S2, ajustar si es necesario
#define LED_GPIO_PIN GPIO_NUM_2 // Por defecto para otros chips, ajustar si es necesario

static const char *TAG = "MQTT_EXAMPLE";

// Variable global para el manejador del cliente MQTT
static esp_mqtt_client_handle_t s_mqtt_client = NULL;
static const char *s_mqtt_topic = "/vamoyvamo/data"; // Ejemplo de tópico MQTT, ajustar si es necesario

// Función para obtener la dirección MAC como ID
void get_mac_as_id(char *id_buffer) {
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA); // Corregido: Usando ESP_MAC_WIFI_STA
    sprintf(id_buffer, "%02x%02x%02x%02x%02x%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

static void mqtt5_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32, base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;

    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            gpio_set_level(LED_GPIO_PIN, 1); // Encender LED
            esp_mqtt_client_subscribe(client, s_mqtt_topic, 0); // Suscribirse al tópico
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            gpio_set_level(LED_GPIO_PIN, 0); // Apagar LED
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "MQTT_EVENT_DATA");
            printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
            printf("DATA=%.*s\r\n", event->data_len, event->data);
            gpio_set_level(LED_GPIO_PIN, (gpio_get_level(LED_GPIO_PIN) == 0) ? 1 : 0); // Alternar LED al recibir datos
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
            if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
                ESP_LOGI(TAG, "Last error code reported from mqtt event: %" PRIi32, (long int)event->error_handle->connect_return_code);
            } else if (event->error_handle->error_type == MQTT_ERROR_TYPE_CONNECTION_REFUSED) {
                ESP_LOGI(TAG, "MQTT Connection Refused Error: %" PRIi32, (long int)event->error_handle->connect_return_code);
            } else {
                ESP_LOGI(TAG, "MQTT Other Error type: %d", event->error_handle->error_type);
            }
            break;
        default:
            ESP_LOGI(TAG, "Other event id:%d", event->event_id);
            break;
    }
}

static void periodic_publish_task(void *pvParameters) {
    char mac_id[13];
    get_mac_as_id(mac_id);

    while (1) {
        // Corregido: esp_mqtt_client_is_connected ahora está declarado por mqtt_client.h
        if (s_mqtt_client && esp_mqtt_client_is_connected(s_mqtt_client)) {
            char message[50];
            sprintf(message, "%s activo", mac_id);
            // Corregido: pasando s_mqtt_client como primer argumento, y strlen(message) para la longitud
            int msg_id = esp_mqtt_client_publish(s_mqtt_client, s_mqtt_topic, message, strlen(message), 1, 0);
            ESP_LOGI(TAG, "Sent publish successful, msg_id=%d, message: %s", msg_id, message);
        }
        vTaskDelay(pdMS_TO_TICKS(50000)); // Publicar cada 50 segundos
    }
}

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGI(TAG, "Wi-Fi disconnected, trying to reconnect...");
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "Got IP address:" IPSTR, IP2STR(&event->ip_info.ip));
        if (s_mqtt_client == NULL) {
            ESP_LOGI(TAG, "Starting MQTT client after Wi-Fi connection.");
            esp_mqtt_client_config_t mqtt_cfg = {
                .broker.address.uri = "mqtt://broker.hivemq.com:1883", // URI de ejemplo
                .session.protocol_ver = MQTT_PROTOCOL_V_3_1_1, // Cambiado a V3.1.1 para amplia compatibilidad
                .network.reconnect_timeout_ms = 5000,
            };
            s_mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
            esp_mqtt_client_register_event(s_mqtt_client, ESP_EVENT_ANY_ID, mqtt5_event_handler, NULL);
            esp_mqtt_client_start(s_mqtt_client);
            xTaskCreate(&periodic_publish_task, "periodic_publish_task", 4096, NULL, 5, NULL);
        }
    }
}


static void wifi_init_sta(void) {
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = "YOUR_WIFI_SSID",     // Reemplazar con tu SSID de WiFi
            .password = "YOUR_WIFI_PASS", // Reemplazar con tu contraseña de WiFi
            .threshold.authmode = WIFI_AUTH_WPA2_PSK, // Por defecto para transición WPA2/WPA3
            .sae_pwe_h2e = WIFI_SAE_MODE_PWE_BOTH, // Corregido: Usar WIFI_SAE_MODE_PWE_BOTH
            // Eliminado: .max_tx_power = 20, ya que 'max_tx_power' no es un miembro de wifi_sta_config_t
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_sta finished.");
}


void app_main(void) {
    ESP_LOGI(TAG, "[APP] Startup..");
    ESP_LOGI(TAG, "[APP] Free memory: %" PRIu32 " bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());

    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("MQTT_CLIENT", ESP_LOG_VERBOSE);
    esp_log_level_set("MQTT_EXAMPLE", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_BASE", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_TCP", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_SSL", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT", ESP_LOG_VERBOSE);
    esp_log_level_set("OUTBOX", ESP_LOG_VERBOSE);

    // Inicializar NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Inicializar GPIO para LED
    gpio_reset_pin(LED_GPIO_PIN); // Corregido: gpio_reset_pin ahora está declarado
    gpio_set_direction(LED_GPIO_PIN, GPIO_MODE_OUTPUT); // Corregido: gpio_set_direction y GPIO_MODE_OUTPUT están declarados

    wifi_init_sta();

    // El cliente MQTT se inicializa y se inicia dentro de wifi_event_handler una vez que se obtiene la IP.
}