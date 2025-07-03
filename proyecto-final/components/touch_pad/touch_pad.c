#include "touch_pad.h"
#include <stdio.h>
#include <inttypes.h>
#include "esp_log.h"
#include "driver/touch_pad.h"
#include "freertos/FreeRTOS.h"
#include "led.h"
#include "esp_timer.h"
#include "audio_controller.h"
#define TOUCH_BUTTON_NUM    2
#define TOUCH_THRESHOLD     60000  // ajustar según calibración
static const char *TAG = "touch read";

static const touch_pad_t button[TOUCH_BUTTON_NUM] = {
    //TOUCH_PAD_NUM1, // VOL_UP
    TOUCH_PAD_NUM2, // PLAY/PAUSE
    //TOUCH_PAD_NUM3, // VOL_DOWN
    TOUCH_PAD_NUM5, // RECORD
    //TOUCH_PAD_NUM6, // PHOTO
    //TOUCH_PAD_NUM11 // NETWORK
};

// Estado global del LED
static led_strip_t *s_strip = NULL;
static uint8_t    s_brightness    = 128;  // 0–255
static uint8_t    s_color_r       = 255;
static uint8_t    s_color_g       = 255;
static uint8_t    s_color_b       = 255;

// Debounce muy básico (evitamos así lecturas repetidas)
static uint32_t last_time[TOUCH_BUTTON_NUM] = {0};
#define DEBOUNCE_MS 300

// Llamar desde main, justo después de led_init():
void tp_set_led_strip(led_strip_t *strip) {
    s_strip = strip;
}

void tp_read(void *pvParameters)
{
    if (!s_strip) {
        ESP_LOGE(TAG, "tp_set_led_strip() NO fue llamado antes de tp_read()");
        return;
    }

    uint32_t touch_value;
    uint64_t now;

    vTaskDelay(100);
    printf("Touch Sensor read:\n");

    while (1) {
        now = esp_timer_get_time() / 1000;

        for (int i = 0; i < TOUCH_BUTTON_NUM; i++) {
            touch_pad_read_raw_data(button[i], &touch_value);

            // Si supera el umbral y pasó DEBOUNCE_MS desde la última vez...
            if (touch_value > TOUCH_THRESHOLD &&
                now - last_time[i] > DEBOUNCE_MS)
            {
                last_time[i] = now;
                esp_err_t ret;
                switch (button[i]) {
                    /*case TOUCH_PAD_NUM1:  // VOL_UP
                        s_brightness = (s_brightness + 20 > 255) ? 255 : s_brightness + 20;
                        ESP_LOGI(TAG, "VOL_UP: brillo=%d", s_brightness);
                        led_set_color(s_strip,
                            (s_color_r * s_brightness) / 255,
                            (s_color_g * s_brightness) / 255,
                            (s_color_b * s_brightness) / 255
                        );
                        break;*/

                    /*case TOUCH_PAD_NUM3:  // VOL_DOWN
                        s_brightness = (s_brightness < 20) ? 0 : s_brightness - 20;
                        ESP_LOGI(TAG, "VOL_DOWN: brillo=%d", s_brightness);
                        led_set_color(s_strip,
                            (s_color_r * s_brightness) / 255,
                            (s_color_g * s_brightness) / 255,
                            (s_color_b * s_brightness) / 255
                        );
                        break;*/

                    case TOUCH_PAD_NUM2:  // PLAY/PAUSE
                        ESP_LOGI(TAG, "▶ TOUCH PLAY: enviando evento PLAY");
                        ret = audio_controller_send_event(AUDIO_EVENT_PLAY);
                        if (ret != ESP_OK) {
                            ESP_LOGE(TAG, "Error al enviar evento PLAY desde touch: %s", esp_err_to_name(ret));
                        } 
                        break;

                    case TOUCH_PAD_NUM5:  // RECORD -> rojo
                        ESP_LOGI(TAG, "TOUCH STOP: enviando evento STOP");
                        ret = audio_controller_send_event(AUDIO_EVENT_STOP);
                        if (ret != ESP_OK) {
                            ESP_LOGE(TAG, "Error al enviar evento STOP desde touch: %s", esp_err_to_name(ret));
                        }
                        break;


                    /*case TOUCH_PAD_NUM11: // NETWORK -> azul
                        ESP_LOGI(TAG, "NETWORK: color=Azul");
                        s_color_r = 0; s_color_g = 0; s_color_b = 255;
                        led_set_color(s_strip,
                            0, 0, (255 * s_brightness) / 255
                        );
                        break;*/

                    default:
                        break;
                }
            }
        }
        vTaskDelay(100);
    }
}

void configure_touch_pad(void)
{
    /* Initialize touch pad peripheral. */
    touch_pad_init();
    for (int i = 0; i < TOUCH_BUTTON_NUM; i++) {
        touch_pad_config(button[i]);
    }
#if TOUCH_CHANGE_CONFIG
    /* If you want change the touch sensor default setting, please write here(after initialize). There are examples: */
    touch_pad_set_measurement_interval(TOUCH_PAD_SLEEP_CYCLE_DEFAULT);
    touch_pad_set_charge_discharge_times(TOUCH_PAD_MEASURE_CYCLE_DEFAULT);
    touch_pad_set_voltage(TOUCH_PAD_HIGH_VOLTAGE_THRESHOLD, TOUCH_PAD_LOW_VOLTAGE_THRESHOLD, TOUCH_PAD_ATTEN_VOLTAGE_THRESHOLD);
    touch_pad_set_idle_channel_connect(TOUCH_PAD_IDLE_CH_CONNECT_DEFAULT);
    for (int i = 0; i < TOUCH_BUTTON_NUM; i++) {
        touch_pad_set_cnt_mode(button[i], TOUCH_PAD_SLOPE_DEFAULT, TOUCH_PAD_TIE_OPT_DEFAULT);
    }
#endif
    /* Denoise setting at TouchSensor 0. */
    touch_pad_denoise_t denoise = {
        /* The bits to be cancelled are determined according to the noise level. */
        .grade = TOUCH_PAD_DENOISE_BIT4,
        .cap_level = TOUCH_PAD_DENOISE_CAP_L4,
    };
    touch_pad_denoise_set_config(&denoise);
    touch_pad_denoise_enable();
    ESP_LOGI(TAG, "Denoise function init");

    /* Enable touch sensor clock. Work mode is "timer trigger". */
    touch_pad_set_fsm_mode(TOUCH_FSM_MODE_TIMER);
    touch_pad_fsm_start();
}