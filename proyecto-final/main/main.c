#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "audio_controller.h"
#include "mqtt_lib.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "wifi_connection.h"
#include "logger.h"
#include "web_server.h"
#include <string.h>
#include "led_strip.h"
#include "led.h"

#define BROKER_URI "mqtt://broker.hivemq.com"
#define MQTT_EVENTS_TOPIC "/esp32/audio/events"

static const char *TAG = "main";

// Variables para control de sincronización de eventos
static bool mqtt_connection_established = false;
static TaskHandle_t sync_task_handle = NULL;
led_strip_t *strip = NULL;
// Queue para manejar acknowledgments de mensajes publicados
#define PENDING_MESSAGES_QUEUE_SIZE 20
static QueueHandle_t pending_messages_queue = NULL;

typedef struct {
    int msg_id;
    uint32_t sequence_number;
    bool acknowledged;
} pending_message_t;

// Forward declarations
void mqtt_published_callback(int msg_id);
void mqtt_connected_callback(void);
void sync_events_task(void *pvParameters);

// External references to embedded audio data
extern const uint8_t music_pcm_start[] asm("_binary_victory8bit_pcm_start");
extern const uint8_t music_pcm_end[]   asm("_binary_victory8bit_pcm_end");

extern const uint8_t music2_pcm_start[] asm("_binary_8bit_pcm_start");
extern const uint8_t music2_pcm_end[]   asm("_binary_8bit_pcm_end");

extern const uint8_t music3_pcm_start[] asm("_binary_start_pcm_start");
extern const uint8_t music3_pcm_end[]   asm("_binary_start_pcm_end");

extern const uint8_t music4_pcm_start[] asm("_binary_whistle_pcm_start");
extern const uint8_t music4_pcm_end[]   asm("_binary_whistle_pcm_end");

extern const uint8_t music5_pcm_start[] asm("_binary_lose_pcm_start");
extern const uint8_t music5_pcm_end[]   asm("_binary_lose_pcm_end");

extern const uint8_t music6_pcm_start[] asm("_binary_buenass_pcm_start");
extern const uint8_t music6_pcm_end[]   asm("_binary_buenass_pcm_end");

// Callback para manejar acknowledgments de mensajes publicados
void mqtt_published_callback(int msg_id)
{
    ESP_LOGI(TAG, "📨 MQTT message acknowledged: msg_id=%d", msg_id);
    
    // Marcar el mensaje como confirmado en la cola de pendientes
    if (pending_messages_queue != NULL) {
        pending_message_t msg = { .msg_id = msg_id, .acknowledged = true };
        xQueueSend(pending_messages_queue, &msg, 0); // No bloquear
    }
}

// Callback para cuando se establece la conexión MQTT
void mqtt_connected_callback(void)
{
    ESP_LOGI(TAG, "🌐 Conexión MQTT establecida - Iniciando sincronización de eventos");
    mqtt_connection_established = true;
    
    // Crear tarea de sincronización si no existe
    if (sync_task_handle == NULL) {
        BaseType_t result = xTaskCreate(
            sync_events_task,
            "sync_events",
            4096,
            NULL,
            5,
            &sync_task_handle
        );
        
        if (result != pdPASS) {
            ESP_LOGE(TAG, "Failed to create sync events task");
        }
    }
}

// Tarea para sincronizar eventos del logger con MQTT
void sync_events_task(void *pvParameters)
{
    ESP_LOGI(TAG, "🔄 Iniciando tarea de sincronización de eventos");
    
    // Crear cola para mensajes pendientes
    pending_messages_queue = xQueueCreate(PENDING_MESSAGES_QUEUE_SIZE, sizeof(pending_message_t));
    if (pending_messages_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create pending messages queue");
        vTaskDelete(NULL);
        return;
    }
    
    // Esperar a que esté inicializado el logger
    while (!logger_get_event_count()) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    
    // Obtener todos los eventos en orden cronológico
    logger_event_t *events = NULL;
    uint8_t event_count = 0;
    
    esp_err_t ret = logger_get_all_events_chronological(&events, &event_count);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get events from logger: %s", esp_err_to_name(ret));
        vTaskDelete(NULL);
        return;
    }
    
    ESP_LOGI(TAG, "📋 Sincronizando %d eventos con MQTT broker", event_count);
    
    // Enviar cada evento y esperar confirmación
    for (int i = 0; i < event_count; i++) {
        if (!mqtt_lib_is_connected()) {
            ESP_LOGW(TAG, "MQTT connection lost during sync");
            break;
        }
        
        // Convertir evento a JSON
        char json_buffer[512];
        ret = logger_event_to_json(&events[i], json_buffer, sizeof(json_buffer));
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to convert event %d to JSON", i);
            continue;
        }
        
        // Publicar con QoS 1 (garantía de entrega)
        ret = mqtt_lib_publish(MQTT_EVENTS_TOPIC, json_buffer, strlen(json_buffer), 1);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to publish event %d", i);
            continue;
        }
        
        ESP_LOGI(TAG, "📤 Evento %d/%d enviado: %s (seq: %lu)", 
                 i + 1, event_count, 
                 logger_event_type_to_string(events[i].type),
                 (unsigned long)events[i].sequence_number);
        
        // Esperar confirmación con timeout
        pending_message_t ack_msg;
        bool acknowledged = false;
        int timeout_ms = 5000; // 5 segundos timeout
        int wait_time = 0;
        
        while (wait_time < timeout_ms && !acknowledged) {
            if (xQueueReceive(pending_messages_queue, &ack_msg, pdMS_TO_TICKS(100))) {
                if (ack_msg.acknowledged) {
                    acknowledged = true;
                    ESP_LOGI(TAG, "✅ Evento %d confirmado (msg_id: %d)", i + 1, ack_msg.msg_id);
                }
            }
            wait_time += 100;
        }
        
        if (!acknowledged) {
            ESP_LOGW(TAG, "⚠️ Timeout esperando confirmación para evento %d", i + 1);
        }
        
        // Pequeña pausa entre envíos para no saturar
        vTaskDelay(pdMS_TO_TICKS(500));
    }
    
    // Liberar memoria
    if (events) {
        free(events);
    }
    
    ESP_LOGI(TAG, "🎉 Sincronización de eventos completada");
    
    // Limpiar cola y tarea
    if (pending_messages_queue) {
        vQueueDelete(pending_messages_queue);
        pending_messages_queue = NULL;
    }
    
    sync_task_handle = NULL;
    vTaskDelete(NULL);
}

void my_callback(const char *topic, const char *data, int len)
{
    ESP_LOGI(TAG, "📨 Mensaje MQTT recibido -> Topic: %s, Data: %s", topic, data);
    
    // Crear una copia de los datos para poder trabajar con ellos como string
    char command[len + 1];
    strncpy(command, data, len);
    command[len] = '\0';
    
    // Procesar comandos de audio
    if (strcmp(command, "play") == 0) {
        ESP_LOGI(TAG, "▶️ Comando: PLAY - Enviando evento");
        esp_err_t ret = audio_controller_send_event(AUDIO_EVENT_PLAY);
        while(1) {
            led_set_color(strip, 0, 0, 255); // azul
            vTaskDelay(100);
            led_off(strip);
            vTaskDelay(100);
        }
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Error al enviar evento PLAY: %s", esp_err_to_name(ret));
        }
    }
    else if (strcmp(command, "pause") == 0) {
        ESP_LOGI(TAG, "⏸️ Comando: PAUSE - Enviando evento");
        esp_err_t ret = audio_controller_send_event(AUDIO_EVENT_PAUSE);
        led_off(strip);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Error al enviar evento PAUSE: %s", esp_err_to_name(ret));
        }
    }
    else if (strcmp(command, "next") == 0) {
        ESP_LOGI(TAG, "⏭️ Comando: NEXT - Enviando evento");
        esp_err_t ret = audio_controller_send_event(AUDIO_EVENT_NEXT);
        led_set_color(strip, 0, 0, 255); // azul
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Error al enviar evento NEXT: %s", esp_err_to_name(ret));
        }
    }
    else if (strcmp(command, "previous") == 0) {
        ESP_LOGI(TAG, "⏮️ Comando: PREVIOUS - Enviando evento");
        esp_err_t ret = audio_controller_send_event(AUDIO_EVENT_PREVIOUS);
        led_set_color(strip, 0, 0, 255); // azul
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Error al enviar evento PREVIOUS: %s", esp_err_to_name(ret));
        }
    }
    else {
        ESP_LOGW(TAG, "⚠️ Comando desconocido: %s", command);
        ESP_LOGI(TAG, "Comandos válidos: play, pause, next, previous");
    }
}

void app_main(void)
{
    printf("ESP32-S2 Kaluga Kit - Audio Player con Control MQTT\n");
    printf("=====================================================\n");
    ESP_ERROR_CHECK( led_init(&strip) );
    // Inicializar NVS, networking y WiFi
    ESP_LOGI(TAG, "🔧 Inicializando sistema...");
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    
    ESP_LOGI(TAG, "📶 Conectando WiFi...");
    ESP_ERROR_CHECK(wifi_connect());
    
    // Inicializar MQTT
    ESP_LOGI(TAG, "🌐 Inicializando MQTT...");
    ESP_ERROR_CHECK(mqtt_lib_init(BROKER_URI, my_callback));
    
    // Configurar callbacks para conexión y confirmación de mensajes
    ESP_ERROR_CHECK(mqtt_lib_set_connected_callback(mqtt_connected_callback));
    ESP_ERROR_CHECK(mqtt_lib_set_published_callback(mqtt_published_callback));
    
    // Inicializar servidor web
    ESP_LOGI(TAG, "🌐 Iniciando servidor web...");
    esp_err_t ret = web_server_start();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error iniciando servidor web: %s", esp_err_to_name(ret));
        // Continuar sin servidor web si falla
    }
    
    // Initialize audio controller
    ESP_LOGI(TAG, "🎵 Inicializando audio controller...");
    audio_controller_config_t config = AUDIO_CONTROLLER_DEFAULT_CONFIG();
    config.volume = 55;
    
    ret = audio_controller_init(&config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error inicializando audio: %s", esp_err_to_name(ret));
        return;
    }
    
    // Prepare playlist
    audio_track_t tracks[] = {
        { music_pcm_start, music_pcm_end - music_pcm_start, "Victory" },
        { music2_pcm_start, music2_pcm_end - music2_pcm_start, "8bit Classic" },
        { music3_pcm_start, music3_pcm_end - music3_pcm_start, "Game Start" },
        {
            .data = music4_pcm_start,
            .size = music4_pcm_end - music4_pcm_start,
            .name = "Whistle"
        },
        {
            .data = music5_pcm_start,
            .size = music5_pcm_end - music5_pcm_start,
            .name = "Lose"
        },
        {
            .data = music6_pcm_start,
            .size = music6_pcm_end - music6_pcm_start,
            .name = "Bueenass"
        }
    };
    
    // Load playlist
    ret = audio_controller_load_playlist(tracks, 6);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error cargando playlist: %s", esp_err_to_name(ret));
        return;
    }
    
    ESP_LOGI(TAG, "✅ Sistema listo! Esperando comandos MQTT...");
    ESP_LOGI(TAG, "📋 Comandos disponibles: play, pause, next, previous");
    ESP_LOGI(TAG, "🌐 Control web disponible en:");
    ESP_LOGI(TAG, "   - AP: http://192.168.4.1/ (red 'ConfiguradorESP')");
    ESP_LOGI(TAG, "   - Si conectado a STA: http://[IP_LOCAL]/");
    ESP_LOGI(TAG, "🎼 Playlist cargada con %d pistas:", 6);
    for (int i = 0; i < 6; i++) {
        ESP_LOGI(TAG, "  - %s", tracks[i].name);
    }
    
    // Mantener el programa funcionando
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));  // Delay de 1 segundo
    }
}