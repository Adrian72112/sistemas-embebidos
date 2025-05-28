#include "touch_pad.h"
#include <stdio.h>
#include <inttypes.h>
#include "esp_log.h"

#include "driver/touch_pad.h"
#include "delay.h"

#define TOUCH_BUTTON_NUM    6
#define TOUCH_CHANGE_CONFIG 0

static const char *TAG = "touch read";
static const touch_pad_t button[TOUCH_BUTTON_NUM] = {
    TOUCH_PAD_NUM1, // VOL_UP
    TOUCH_PAD_NUM2, // PLAY/PAUSE
    TOUCH_PAD_NUM3, // VOL_DOWN
    TOUCH_PAD_NUM5, // RECORD
    TOUCH_PAD_NUM6, // PHOTO
    TOUCH_PAD_NUM11 // NETWORK
};

/*
  Read values sensed at all available touch pads.
 Print out values in a loop on a serial monitor.
 */
void tp_read(void)
{
    uint32_t touch_value;

    /* Wait touch sensor init done */
    delay_ms(100);
    printf("Touch Sensor read, the output format is: \nTouchpad num:[raw data]\n\n");

    while (1) {
        //TODO toca eliminar este for poner un switch case para cada boton y leer el touch_value{i} correspondiente y hacer cosas con el led
        for (int i = 0; i < TOUCH_BUTTON_NUM; i++) {
            touch_pad_read_raw_data(button[i], &touch_value);    // read raw data.
            printf("T%d: [%4"PRIu32"] ", button[i], touch_value);
        }
        printf("\n");
        delay_ms(200);
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