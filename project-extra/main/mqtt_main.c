#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_event.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "mqtt_client.h"

// 🧠 Variables globales
char wifi_ssid[64] = "defaultSSID";
char wifi_pass[64] = "defaultPASS";
char mqtt_uri[128] = "mqtt://broker.hivemq.com";
char mqtt_topic[64] = "test/topic";

// ⚙️ Grupo de eventos para manejar el estado de WiFi
EventGroupHandle_t wifi_event_group;
const int WIFI_CONNECTED_BIT = BIT0;

// 🛰️ Handler de eventos WiFi
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGI("WiFi", "Reintentando conexión...");
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

// 📶 Función para conectar al WiFi con las credenciales ingresadas
void connect_wifi(void) {
    wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL));

    wifi_config_t wifi_config = {
    .sta = {
        .ssid = "",
        .password = ""
    }
};

    strcpy((char *)wifi_config.sta.ssid, wifi_ssid);
    strcpy((char *)wifi_config.sta.password, wifi_pass);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI("WiFi", "Conectando a %s...", wifi_ssid);
    xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
    ESP_LOGI("WiFi", "Conectado con éxito.");
}

// 🔌 Callback del cliente MQTT
static esp_err_t mqtt_event_handler_cb(esp_mqtt_event_handle_t event) {
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI("MQTT", "Conectado");
            esp_mqtt_client_subscribe(event->client, mqtt_topic, 0);
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGI("MQTT", "Mensaje recibido: %.*s", event->data_len, event->data);
            break;
        default:
            break;
    }
    return ESP_OK;
}

// 🚀 Función para iniciar cliente MQTT
static void mqtt_app_start(void) {
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = mqtt_uri,
    };

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, MQTT_EVENT_ANY, mqtt_event_handler_cb, NULL);


    esp_mqtt_client_start(client);


    while (1) {
        esp_mqtt_client_publish(client, mqtt_topic, "Hola desde ESP32", 0, 1, 0);
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
void parse_command(char *line);

// 🧵 Tarea que escucha comandos por consola
void command_task(void *pvParameters) {
    char line[128];
    while (1) {
        if (fgets(line, sizeof(line), stdin)) {
            line[strcspn(line, "\r\n")] = 0;
            parse_command(line);
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    vTaskDelete(NULL);
}

// 🧠 Función para interpretar comandos de consola
void parse_command(char *line) {
    if (strncmp(line, "!wifi ", 6) == 0) {
        sscanf(line + 6, "%s %s", wifi_ssid, wifi_pass);
        printf("✅ WiFi actualizado: SSID=%s, PASS=%s\n", wifi_ssid, wifi_pass);
        connect_wifi();
    } else if (strncmp(line, "!broker ", 8) == 0) {
        sscanf(line + 8, "%s", mqtt_uri);
        printf("✅ Broker actualizado: %s\n", mqtt_uri);
    } else if (strncmp(line, "!topic ", 7) == 0) {
        sscanf(line + 7, "%s", mqtt_topic);
        printf("✅ Tópico actualizado: %s\n", mqtt_topic);
    } else {
        printf("❌ Comando desconocido: %s\n", line);
    }
}

// 🚩 Función principal
void app_main(void) {
    ESP_ERROR_CHECK(nvs_flash_init());

    // Arrancar WiFi
    connect_wifi();

    // Crear tareas
    xTaskCreate(command_task, "command_task", 4096, NULL, 5, NULL);
    xTaskCreate((TaskFunction_t)mqtt_app_start, "mqtt_app_task", 4096, NULL, 5, NULL);
}
