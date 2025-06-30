#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "audio_controler.h"
#include "mqtt_lib.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "wifi_connection.h"
#include <string.h>

#define BROKER_URI "mqtt://broker.hivemq.com"

static const char *TAG = "main";

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

void my_callback(const char *topic, const char *data, int len)
{
    ESP_LOGI(TAG, "📨 Mensaje MQTT recibido -> Topic: %s, Data: %s", topic, data);
    
    // Crear una copia de los datos para poder trabajar con ellos como string
    char command[len + 1];
    strncpy(command, data, len);
    command[len] = '\0';
    
    // Procesar comandos de audio
    if (strcmp(command, "play") == 0) {
        ESP_LOGI(TAG, "▶️ Comando: PLAY - Iniciando reproducción");
        esp_err_t ret = audio_controller_play();
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Error al reproducir: %s", esp_err_to_name(ret));
        }
    }
    else if (strcmp(command, "pause") == 0) {
        ESP_LOGI(TAG, "⏸️ Comando: PAUSE - Pausando reproducción");
        esp_err_t ret = audio_controller_pause();
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Error al pausar: %s", esp_err_to_name(ret));
        }
    }
    else if (strcmp(command, "next") == 0) {
        ESP_LOGI(TAG, "⏭️ Comando: NEXT - Siguiente pista");
        esp_err_t ret = audio_controller_next();
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Error al cambiar pista: %s", esp_err_to_name(ret));
        }
    }
    else if (strcmp(command, "previous") == 0) {
        ESP_LOGI(TAG, "⏮️ Comando: PREVIOUS - Pista anterior");
        esp_err_t ret = audio_controller_previous();
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Error al cambiar pista: %s", esp_err_to_name(ret));
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
    
    // Initialize audio controller
    ESP_LOGI(TAG, "🎵 Inicializando audio controller...");
    audio_controller_config_t config = AUDIO_CONTROLLER_DEFAULT_CONFIG();
    config.volume = 55;
    
    esp_err_t ret = audio_controller_init(&config);
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
    ESP_LOGI(TAG, "🎼 Playlist cargada con %d pistas:", 6);
    for (int i = 0; i < 6; i++) {
        ESP_LOGI(TAG, "  - %s", tracks[i].name);
    }
    
    // Mantener el programa funcionando
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));  // Delay de 1 segundo
    }
}