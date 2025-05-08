#include <stdio.h>
#include <stdlib.h>
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "esp_rom_sys.h"

// --- Parámetros ADC fijos ---
#define ADC_WIDTH_DEFAULT  ADC_WIDTH_BIT_13   // para ESP32-S2 en IDF v5.x
#define ADC_UNIT           ADC_UNIT_1
#define ADC_CH             ADC1_CHANNEL_5     // ajusta al canal AU_BT_ADC
#define ADC_ATTEN          ADC_ATTEN_DB_11    // rango 0–3.3 V
#define DEFAULT_VREF       1100               // mV (valor típico)

// Umbrales en mV para cada botón (0 = ninguno, 1..6 botones)
static const uint32_t btn_mv[6] = {
    2450,  // 1 = REC
    1980,  // 2 = MODE
    1650,  // 3 = PLAY
    1110,  // 4 = SET
     820,  // 5 = VOL−
     380   // 6 = VOL+
};

static esp_adc_cal_characteristics_t *adc_chars;

// Inicializa ADC1 (ancho, atenuación) y calibra Vref
static void init_adc(void)
{
    esp_err_t err;

    // 1) Configura resolución a 13 bits
    err = adc1_config_width(ADC_WIDTH_DEFAULT);
    if (err != ESP_OK) {
        printf("Error en adc1_config_width(): %d\n", err);
    }

    // 2) Atenuación para 0–3.3 V
    adc1_config_channel_atten(ADC_CH, ADC_ATTEN);

    // 3) Reserva y calibra Vref
    adc_chars = calloc(1, sizeof(*adc_chars));
    esp_adc_cal_characterize(
        ADC_UNIT,
        ADC_ATTEN,
        ADC_WIDTH_DEFAULT,
        DEFAULT_VREF,
        adc_chars
    );
}

// Lee raw → mV → devuelve 0 si ninguno, o 1..6 según umbral
static int read_button_adc(void) {
    uint32_t raw = adc1_get_raw(ADC_CH);
    uint32_t voltage = esp_adc_cal_raw_to_voltage(raw, adc_chars);

    int   best_btn  = 0;
    uint32_t best_d = UINT32_MAX;
    for (int i = 0; i < 6; i++) {
        uint32_t diff = (voltage > btn_mv[i])
                        ? voltage - btn_mv[i]
                        : btn_mv[i] - voltage;
        if (diff < best_d) {
            best_d    = diff;
            best_btn  = i + 1;  // +1 para que vaya de 1 a 6
        }
    }
    // opcional: si best_d es muy grande (ej. >300 mV), devuelve 0
    if (best_d > 300) {
        return 0;
    }
    return best_btn;
}

void app_main(void) {
    init_adc();
    int last = 0;
    while (true) {
        int btn = read_button_adc();
        if (btn != 0 && btn != last) {
            printf("¡Pulsado botón %d!\r\n", btn);
            last = btn;
        }
        if (btn == 0) {
            last = 0;
        }
        esp_rom_delay_us(50 * 1000);
    }
}
