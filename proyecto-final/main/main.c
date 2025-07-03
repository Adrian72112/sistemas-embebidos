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
#include "ntp_sync.h"
#include "touch_pad.h"
#define DEFAULT_BROKER_URI "mqtt://broker.hivemq.com"
#define DEFAULT_MQTT_TOPIC "/topic/qos1"
#define MQTT_EVENTS_TOPIC "/esp32/audio/events"

static const char *TAG = "main";

// Variables para control de sincronización de eventos
static bool mqtt_connection_established = false;
static TaskHandle_t sync_task_handle = NULL;
led_strip_t *strip = NULL;

// Variables para configuración dinámica
static char current_broker_uri[128] = DEFAULT_BROKER_URI;
static char current_mqtt_topic[64] = DEFAULT_MQTT_TOPIC;

// Forward declarations
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
            20,
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
    
    ESP_LOGI(TAG, "📋 Enviando %d eventos como array JSON", event_count);
    
    if (!mqtt_lib_is_connected()) {
        ESP_LOGW(TAG, "MQTT not connected, skipping sync");
        free(events);
        sync_task_handle = NULL;
        vTaskDelete(NULL);
        return;
    }
    
    // Crear buffer para el array JSON completo
    char *json_array = malloc(event_count * 512 + 100); // Buffer generoso
    if (!json_array) {
        ESP_LOGE(TAG, "Failed to allocate memory for JSON array");
        free(events);
        sync_task_handle = NULL;
        vTaskDelete(NULL);
        return;
    }
    
    strcpy(json_array, "[");
    
    // Convertir todos los eventos a JSON y concatenar
    for (int i = 0; i < event_count; i++) {
        char json_buffer[512];
        ret = logger_event_to_json(&events[i], json_buffer, sizeof(json_buffer));
        if (ret == ESP_OK) {
            strcat(json_array, json_buffer);
            if (i < event_count - 1) {
                strcat(json_array, ",");
            }
        }
    }
    
    strcat(json_array, "]");
    
    // Enviar array completo en una sola publicación
    ret = mqtt_lib_publish(MQTT_EVENTS_TOPIC, json_array, strlen(json_array), 1);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "📤 Array con %d eventos enviado exitosamente", event_count);
    } else {
        ESP_LOGE(TAG, "❌ Error enviando array de eventos: %s", esp_err_to_name(ret));
    }
    
    // Liberar memoria
    free(events);
    free(json_array);
    
    ESP_LOGI(TAG, "🎉 Sincronización completada");
    sync_task_handle = NULL;
    vTaskDelete(NULL);
}

void my_callback(const char *topic, const char *data, int len)
{
    ESP_LOGI(TAG, "📨 Mensaje MQTT recibido -> Topic: %s, Data: %s", topic, data);
    
    // Mostrar timestamp actual para debug
    time_t now;
    time(&now);
    ESP_LOGI(TAG, "🕐 Timestamp actual al recibir comando: %lld", (long long)now);
    
    // Crear una copia de los datos para poder trabajar con ellos como string
    char command[len + 1];
    strncpy(command, data, len);
    command[len] = '\0';
    
    // Procesar comandos de audio
    if (strcmp(command, "play") == 0) {
        ESP_LOGI(TAG, "▶️ Comando: PLAY - Enviando evento");
        esp_err_t ret = audio_controller_send_event(AUDIO_EVENT_PLAY);
        
        // Cambiar estado del LED a PLAY (parpadeo azul)
        led_set_state(LED_STATE_PLAY);
        
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Error al enviar evento PLAY: %s", esp_err_to_name(ret));
            led_set_state(LED_STATE_ERROR);
        } else {
            ESP_LOGI(TAG, "✅ Evento PLAY enviado correctamente");
        }
    }
    else if (strcmp(command, "pause") == 0) {
        ESP_LOGI(TAG, "⏸️ Comando: PAUSE - Enviando evento");
        esp_err_t ret = audio_controller_send_event(AUDIO_EVENT_PAUSE);
        
        // Cambiar estado del LED a PAUSE (apagado)
        led_set_state(LED_STATE_PAUSE);
        
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Error al enviar evento PAUSE: %s", esp_err_to_name(ret));
            led_set_state(LED_STATE_ERROR);
        } else {
            ESP_LOGI(TAG, "✅ Evento PAUSE enviado correctamente");
        }
    }
    else if (strcmp(command, "next") == 0) {
        ESP_LOGI(TAG, "⏭️ Comando: NEXT - Enviando evento");
        esp_err_t ret = audio_controller_send_event(AUDIO_EVENT_NEXT);
        
        // Cambiar estado del LED a NEXT (azul sólido)
        led_set_state(LED_STATE_NEXT);
        
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Error al enviar evento NEXT: %s", esp_err_to_name(ret));
            led_set_state(LED_STATE_ERROR);
        } else {
            ESP_LOGI(TAG, "✅ Evento NEXT enviado correctamente");
        }
    }
    else if (strcmp(command, "previous") == 0) {
        ESP_LOGI(TAG, "⏮️ Comando: PREVIOUS - Enviando evento");
        esp_err_t ret = audio_controller_send_event(AUDIO_EVENT_PREVIOUS);
        
        // Cambiar estado del LED a PREVIOUS (azul sólido)
        led_set_state(LED_STATE_PREVIOUS);
        
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Error al enviar evento PREVIOUS: %s", esp_err_to_name(ret));
            led_set_state(LED_STATE_ERROR);
        } else {
            ESP_LOGI(TAG, "✅ Evento PREVIOUS enviado correctamente");
        }
    }
    else if (strcmp(command, "volumeup") == 0) {
        ESP_LOGI(TAG, "🔊 Comando: VOLUME UP - Enviando evento");
        esp_err_t ret = audio_controller_send_event(AUDIO_EVENT_VOLUME_UP);
        
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Error al enviar evento VOLUME UP: %s", esp_err_to_name(ret));
            led_set_state(LED_STATE_ERROR);
        } else {
            ESP_LOGI(TAG, "✅ Evento VOLUME UP enviado correctamente");
        }
    }
    else if (strcmp(command, "volumedown") == 0) {
        ESP_LOGI(TAG, "🔉 Comando: VOLUME DOWN - Enviando evento");
        esp_err_t ret = audio_controller_send_event(AUDIO_EVENT_VOLUME_DOWN);
        
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Error al enviar evento VOLUME DOWN: %s", esp_err_to_name(ret));
            led_set_state(LED_STATE_ERROR);
        } else {
            ESP_LOGI(TAG, "✅ Evento VOLUME DOWN enviado correctamente");
        }
    }
    else {
        ESP_LOGW(TAG, "⚠️ Comando desconocido: %s", command);
        ESP_LOGI(TAG, "📋 Comandos válidos: play, pause, next, previous, volumeup, volumedown");
        
        // LED de error por comando desconocido
        led_set_state(LED_STATE_ERROR);
    }
}

esp_err_t init_mqtt_with_config(void)
{
    // Cargar configuración MQTT desde NVS
    mqtt_config_nvs_t mqtt_config;
    esp_err_t ret = mqtt_load_config(&mqtt_config);
    
    if (ret == ESP_OK && mqtt_config.configured) {
        ESP_LOGI(TAG, "🌐 Configuración MQTT encontrada, usando: %s", mqtt_config.broker_uri);
        strcpy(current_broker_uri, mqtt_config.broker_uri);
        strcpy(current_mqtt_topic, mqtt_config.topic);
    } else {
        ESP_LOGI(TAG, "📋 Usando configuración MQTT por defecto: %s", current_broker_uri);
    }
    
    // Inicializar MQTT con la configuración cargada
    ret = mqtt_lib_init(current_broker_uri, my_callback);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "❌ Error inicializando MQTT: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Configurar callback para conexión
    ESP_ERROR_CHECK(mqtt_lib_set_connected_callback(mqtt_connected_callback));
    
    return ESP_OK;
}

void app_main(void)
{
    printf("ESP32-S2 Kaluga Kit - Audio Player con Control MQTT\n");
    printf("=====================================================\n");
    
    // Inicializar LED básico
    ESP_ERROR_CHECK(led_init(&strip));
    
    // Inicializar controlador del LED con máquina de estados
    ESP_ERROR_CHECK(led_controller_init(strip));
    
    // Indicar inicio con LED azul
    led_set_state(LED_STATE_OFF);
    
    // Inicializar NVS, networking y WiFi
    ESP_LOGI(TAG, "🔧 Inicializando sistema...");
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    
    // Inicializar WiFi (con configuración automática desde NVS)
    ESP_LOGI(TAG, "📶 Inicializando WiFi...");
    esp_err_t wifi_ret = wifi_connect();
    if (wifi_ret != ESP_OK) {
        ESP_LOGW(TAG, "⚠️ WiFi no conectado completamente: %s", esp_err_to_name(wifi_ret));
        ESP_LOGI(TAG, "📡 AP disponible para configuración en http://192.168.4.1/");
        // Continuar ejecución - AP sigue disponible para configuración
    }
    
    // Inicializar servidor web (siempre disponible)
    ESP_LOGI(TAG, "🌐 Iniciando servidor web...");
    esp_err_t ret = web_server_start();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "❌ Error iniciando servidor web: %s", esp_err_to_name(ret));
        led_set_state(LED_STATE_ERROR);
        // Continuar sin servidor web
    }
    
    // Verificar si tenemos configuración WiFi para intentar MQTT
    wifi_config_nvs_t wifi_config;
    if (wifi_load_config(&wifi_config) == ESP_OK && wifi_config.configured) {
        ESP_LOGI(TAG, "✅ WiFi configurado, iniciando servicios completos...");
        
        // Inicializar MQTT con configuración guardada
        ret = init_mqtt_with_config();
        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "⚠️ Error con MQTT, continuando sin él");
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
    
    ESP_LOGI(TAG, "🎼 Playlist cargada con %d pistas:", 6);
        for (int i = 0; i < 6; i++) {
            ESP_LOGI(TAG, "   %d. %s (%d bytes)", i+1, tracks[i].name, tracks[i].size);
        }
        
        // Inicializar y sincronizar NTP ANTES de inicializar el logger
        ESP_LOGI(TAG, "🕐 Configurando sincronización de tiempo...");
        ntp_initialize();
        ntp_wait_for_sync();  // Esperar a que se sincronice el tiempo
        
        // Inicializar logger DESPUÉS de sincronizar el tiempo
        ESP_LOGI(TAG, "📝 Inicializando logger con tiempo sincronizado...");
        ret = logger_init();
        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "⚠️ Error inicializando logger: %s", esp_err_to_name(ret));
            // Continuar sin logger si falla
        }

        configure_touch_pad();
        tp_set_led_strip(strip); 
        xTaskCreate(tp_read, "tp_read_task", 4096, NULL, 5, NULL);
        
        ESP_LOGI(TAG, "✅ Sistema completo iniciado!");
        ESP_LOGI(TAG, "📋 Comandos disponibles: play, pause, next, previous");
        ESP_LOGI(TAG, "🌐 Control web disponible en http://[IP_LOCAL]/");
        
    } else {
        ESP_LOGI(TAG, "⚙️ Modo configuración - WiFi no configurado");
        ESP_LOGI(TAG, "🌐 Conecta a la red 'ESP32-Config' (password: config123)");
        ESP_LOGI(TAG, "🔧 Accede a http://192.168.4.1/ para configurar WiFi y MQTT");
        led_set_state(LED_STATE_PAUSE); // LED apagado en modo configuración
    }
    
    ESP_LOGI(TAG, "🚀 Sistema listo!");
    
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}