/**
 * ==========================================================================
 * PROGRAM 07: ADC Threshold Alert (Peringatan Ambang Batas ADC)
 * ==========================================================================
 * Modul 04 - ADC | Praktikum Sistem Embedded
 * 
 * Deskripsi:
 *   Program ini memonitor nilai ADC terhadap dua ambang batas (threshold):
 *   - LOW_THRESHOLD  = 1000 (sekitar 0.8V)
 *   - HIGH_THRESHOLD = 3000 (sekitar 2.4V)
 *   
 *   Ketika nilai ADC melebihi HIGH_THRESHOLD, LED pada GPIO2 menyala
 *   dan pesan peringatan ditampilkan. Program juga mengklasifikasikan
 *   level menjadi LOW, NORMAL, HIGH.
 * 
 * Koneksi Hardware:
 *   - Potensiometer: Wiper → GPIO34
 *   - LED: GPIO2 (built-in pada kebanyakan board ESP32)
 * ==========================================================================
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/adc.h"
#include "driver/gpio.h"
#include "esp_adc_cal.h"
#include "esp_log.h"

static const char *TAG = "ADC_ALERT";

/* Konfigurasi ADC */
#if CONFIG_IDF_TARGET_ESP32
    #define ADC_CHANNEL     ADC1_CHANNEL_6
    #define ADC_GPIO_NUM    34
#elif CONFIG_IDF_TARGET_ESP32S2 || CONFIG_IDF_TARGET_ESP32S3
    #define ADC_CHANNEL     ADC1_CHANNEL_3
    #define ADC_GPIO_NUM    4
#else
    #define ADC_CHANNEL     ADC1_CHANNEL_6
    #define ADC_GPIO_NUM    34
#endif

/* Konfigurasi LED */
#define LED_GPIO        GPIO_NUM_2      /* LED built-in */

/* Ambang batas */
#define LOW_THRESHOLD   1000            /* Ambang batas bawah */
#define HIGH_THRESHOLD  3000            /* Ambang batas atas */

#define ADC_WIDTH       ADC_WIDTH_BIT_12
#define ADC_ATTEN       ADC_ATTEN_DB_11
#define ADC_UNIT        ADC_UNIT_1
#define DEFAULT_VREF    1100
#define READ_INTERVAL_MS    200

/* Enumerasi status level */
typedef enum {
    LEVEL_LOW = 0,
    LEVEL_NORMAL,
    LEVEL_HIGH
} adc_level_t;

/* Variabel untuk mendeteksi perubahan status */
static adc_level_t prev_level = LEVEL_NORMAL;

/**
 * @brief Inisialisasi GPIO untuk LED output
 */
static void led_init(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << LED_GPIO),     /* Pilih pin LED */
        .mode = GPIO_MODE_OUTPUT,               /* Mode output */
        .pull_up_en = GPIO_PULLUP_DISABLE,      /* Tanpa pull-up */
        .pull_down_en = GPIO_PULLDOWN_DISABLE,  /* Tanpa pull-down */
        .intr_type = GPIO_INTR_DISABLE          /* Tanpa interrupt */
    };
    gpio_config(&io_conf);
    gpio_set_level(LED_GPIO, 0);  /* LED mati awal */
    ESP_LOGI(TAG, "LED pada GPIO%d dikonfigurasi sebagai output", LED_GPIO);
}

/**
 * @brief Tentukan level berdasarkan nilai ADC
 */
static adc_level_t get_level(int raw_value)
{
    if (raw_value < LOW_THRESHOLD) {
        return LEVEL_LOW;
    } else if (raw_value > HIGH_THRESHOLD) {
        return LEVEL_HIGH;
    } else {
        return LEVEL_NORMAL;
    }
}

/**
 * @brief Mengembalikan string label level
 */
static const char* get_level_label(adc_level_t level)
{
    switch (level) {
        case LEVEL_LOW:    return "LOW   ";
        case LEVEL_NORMAL: return "NORMAL";
        case LEVEL_HIGH:   return "HIGH  ";
        default:           return "???   ";
    }
}

/**
 * @brief Mengembalikan string visual bar
 */
static void print_bar(int raw_value, int width)
{
    int filled = raw_value * width / 4095;
    int low_mark = LOW_THRESHOLD * width / 4095;
    int high_mark = HIGH_THRESHOLD * width / 4095;

    printf("[");
    for (int i = 0; i < width; i++) {
        if (i == low_mark || i == high_mark) {
            printf("|");  /* Marker threshold */
        } else if (i < filled) {
            printf("#");
        } else {
            printf(".");
        }
    }
    printf("]");
}

void app_main(void)
{
    /* ====== INISIALISASI ====== */
    led_init();

    adc1_config_width(ADC_WIDTH);
    adc1_config_channel_atten(ADC_CHANNEL, ADC_ATTEN);

    /* Kalibrasi */
    esp_adc_cal_characteristics_t *adc_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));
    esp_adc_cal_characterize(ADC_UNIT, ADC_ATTEN, ADC_WIDTH, DEFAULT_VREF, adc_chars);

    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  ADC Threshold Alert Monitor");
    ESP_LOGI(TAG, "  Channel : ADC1_CH%d (GPIO%d)", ADC_CHANNEL, ADC_GPIO_NUM);
    ESP_LOGI(TAG, "  LED     : GPIO%d", LED_GPIO);
    ESP_LOGI(TAG, "  Low TH  : %d (~%.0f mV)", LOW_THRESHOLD, LOW_THRESHOLD * 3300.0 / 4095);
    ESP_LOGI(TAG, "  High TH : %d (~%.0f mV)", HIGH_THRESHOLD, HIGH_THRESHOLD * 3300.0 / 4095);
    ESP_LOGI(TAG, "========================================");

    int counter = 0;
    int alert_count = 0;

    /* ====== LOOP MONITORING ====== */
    while (1) {
        /* Baca nilai ADC */
        int raw = adc1_get_raw(ADC_CHANNEL);
        uint32_t voltage = esp_adc_cal_raw_to_voltage(raw, adc_chars);

        /* Tentukan level */
        adc_level_t current_level = get_level(raw);

        /* Kontrol LED berdasarkan level */
        if (current_level == LEVEL_HIGH) {
            gpio_set_level(LED_GPIO, 1);  /* LED NYALA */
            alert_count++;
        } else {
            gpio_set_level(LED_GPIO, 0);  /* LED MATI */
        }

        /* Tampilkan data */
        counter++;
        printf("[%04d] Raw: %4d | %4lu mV | %s ", 
               counter, raw, (unsigned long)voltage, 
               get_level_label(current_level));
        print_bar(raw, 40);

        /* Deteksi perubahan level (transisi) */
        if (current_level != prev_level) {
            printf(" *** TRANSISI: %s → %s ***",
                   get_level_label(prev_level),
                   get_level_label(current_level));
            
            if (current_level == LEVEL_HIGH) {
                printf("\n  ⚠️  PERINGATAN: Nilai ADC melebihi ambang batas tinggi! "
                       "(Raw=%d > %d) LED=ON", raw, HIGH_THRESHOLD);
            } else if (current_level == LEVEL_LOW) {
                printf("\n  ℹ️  INFO: Nilai ADC di bawah ambang batas rendah "
                       "(Raw=%d < %d)", raw, LOW_THRESHOLD);
            }
        }

        printf("\n");

        /* Update status sebelumnya */
        prev_level = current_level;

        /* Tampilkan ringkasan setiap 50 pembacaan */
        if (counter % 50 == 0) {
            printf("\n--- Ringkasan: %d pembacaan, %d alert (%.1f%%) ---\n\n",
                   counter, alert_count,
                   (float)alert_count / counter * 100.0f);
        }

        vTaskDelay(pdMS_TO_TICKS(READ_INTERVAL_MS));
    }
}
