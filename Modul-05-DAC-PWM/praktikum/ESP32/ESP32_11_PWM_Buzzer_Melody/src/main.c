/**
 * ==========================================================================
 * PROGRAM 11: PWM Buzzer Melody (Melodi Buzzer dengan PWM)
 * ==========================================================================
 * Modul 05 - DAC & PWM | Praktikum Sistem Embedded
 *
 * Deskripsi:
 *   Memainkan melodi sederhana ("Twinkle Twinkle Little Star") menggunakan
 *   buzzer pasif yang dikontrol dengan PWM. Frekuensi PWM diubah sesuai
 *   frekuensi nada musik, duty cycle 50% untuk volume maksimum.
 *
 * Koneksi Hardware:
 *   - Buzzer pasif: + → GPIO18, - → GND
 *   - PENTING: Gunakan buzzer PASIF (bukan aktif)
 *
 * Frekuensi Nada:
 *   C4=262Hz, D4=294Hz, E4=330Hz, F4=349Hz,
 *   G4=392Hz, A4=440Hz, B4=494Hz, C5=523Hz
 *
 * API yang digunakan:
 *   - ledc_timer_config()  : Konfigurasi timer PWM
 *   - ledc_channel_config(): Konfigurasi channel
 *   - ledc_set_freq()      : Ubah frekuensi (nada)
 *   - ledc_set_duty()      : Ubah duty (volume, 50% = max)
 * ==========================================================================
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include "esp_log.h"

static const char *TAG = "PWM_MELODY";

/* Pin buzzer */
#define BUZZER_PIN       18

/* Konfigurasi LEDC */
#define LEDC_TIMER       LEDC_TIMER_0
#define LEDC_MODE        LEDC_LOW_SPEED_MODE
#define LEDC_CH          LEDC_CHANNEL_0
#define LEDC_DUTY_RES    LEDC_TIMER_10_BIT   /* 10-bit (0-1023) */
#define LEDC_HALF_DUTY   512                  /* 50% duty = volume max */

/* Definisi frekuensi nada musik (Hz) */
#define NOTE_C4   262
#define NOTE_D4   294
#define NOTE_E4   330
#define NOTE_F4   349
#define NOTE_G4   392
#define NOTE_A4   440
#define NOTE_B4   494
#define NOTE_C5   523
#define NOTE_D5   587
#define NOTE_E5   659
#define NOTE_REST 0       /* Istirahat (senyap) */

/* Durasi nada (dalam milidetik) */
#define WHOLE     1600    /* Not penuh */
#define HALF      800     /* Setengah */
#define QUARTER   400     /* Seperempat */
#define EIGHTH    200     /* Seperdelapan */

/* Struktur untuk satu nada */
typedef struct {
    uint32_t frequency;   /* Frekuensi nada (Hz), 0 = istirahat */
    uint32_t duration_ms; /* Durasi nada (ms) */
} note_t;

/*
 * Melodi: "Twinkle Twinkle Little Star"
 * C C G G A A G - F F E E D D C -
 * G G F F E E D - G G F F E E D -
 * C C G G A A G - F F E E D D C -
 */
static const note_t melody[] = {
    /* Baris 1: "Twinkle twinkle little star" */
    {NOTE_C4, QUARTER}, {NOTE_C4, QUARTER},
    {NOTE_G4, QUARTER}, {NOTE_G4, QUARTER},
    {NOTE_A4, QUARTER}, {NOTE_A4, QUARTER},
    {NOTE_G4, HALF},

    /* Baris 2: "How I wonder what you are" */
    {NOTE_F4, QUARTER}, {NOTE_F4, QUARTER},
    {NOTE_E4, QUARTER}, {NOTE_E4, QUARTER},
    {NOTE_D4, QUARTER}, {NOTE_D4, QUARTER},
    {NOTE_C4, HALF},

    /* Baris 3: "Up above the world so high" */
    {NOTE_G4, QUARTER}, {NOTE_G4, QUARTER},
    {NOTE_F4, QUARTER}, {NOTE_F4, QUARTER},
    {NOTE_E4, QUARTER}, {NOTE_E4, QUARTER},
    {NOTE_D4, HALF},

    /* Baris 4: "Like a diamond in the sky" */
    {NOTE_G4, QUARTER}, {NOTE_G4, QUARTER},
    {NOTE_F4, QUARTER}, {NOTE_F4, QUARTER},
    {NOTE_E4, QUARTER}, {NOTE_E4, QUARTER},
    {NOTE_D4, HALF},

    /* Baris 5: "Twinkle twinkle little star" (ulang) */
    {NOTE_C4, QUARTER}, {NOTE_C4, QUARTER},
    {NOTE_G4, QUARTER}, {NOTE_G4, QUARTER},
    {NOTE_A4, QUARTER}, {NOTE_A4, QUARTER},
    {NOTE_G4, HALF},

    /* Baris 6: "How I wonder what you are" (ulang) */
    {NOTE_F4, QUARTER}, {NOTE_F4, QUARTER},
    {NOTE_E4, QUARTER}, {NOTE_E4, QUARTER},
    {NOTE_D4, QUARTER}, {NOTE_D4, QUARTER},
    {NOTE_C4, HALF},
};

static const int melody_length = sizeof(melody) / sizeof(melody[0]);

/**
 * Mendapatkan nama nada dari frekuensi
 */
static const char* freq_to_note_name(uint32_t freq)
{
    switch (freq) {
        case NOTE_C4: return "C4";
        case NOTE_D4: return "D4";
        case NOTE_E4: return "E4";
        case NOTE_F4: return "F4";
        case NOTE_G4: return "G4";
        case NOTE_A4: return "A4";
        case NOTE_B4: return "B4";
        case NOTE_C5: return "C5";
        case NOTE_D5: return "D5";
        case NOTE_E5: return "E5";
        case NOTE_REST: return "REST";
        default: return "??";
    }
}

/**
 * Mainkan satu nada
 */
static void play_tone(uint32_t freq, uint32_t duration_ms)
{
    if (freq == NOTE_REST || freq == 0) {
        /* Istirahat: matikan buzzer */
        ledc_set_duty(LEDC_MODE, LEDC_CH, 0);
        ledc_update_duty(LEDC_MODE, LEDC_CH);
    } else {
        /* Set frekuensi nada */
        ledc_set_freq(LEDC_MODE, LEDC_TIMER, freq);
        /* Set duty cycle 50% untuk volume maksimum */
        ledc_set_duty(LEDC_MODE, LEDC_CH, LEDC_HALF_DUTY);
        ledc_update_duty(LEDC_MODE, LEDC_CH);
    }

    /* Tunggu selama durasi nada (90% berbunyi, 10% senyap untuk artikulasi) */
    vTaskDelay(pdMS_TO_TICKS(duration_ms * 9 / 10));

    /* Jeda singkat antar nada untuk artikulasi */
    ledc_set_duty(LEDC_MODE, LEDC_CH, 0);
    ledc_update_duty(LEDC_MODE, LEDC_CH);
    vTaskDelay(pdMS_TO_TICKS(duration_ms / 10));
}

void app_main(void)
{
    ESP_LOGI(TAG, "=== ESP32 PWM Buzzer Melody Player ===");
    ESP_LOGI(TAG, "Buzzer Pin: GPIO%d", BUZZER_PIN);
    ESP_LOGI(TAG, "Melodi: Twinkle Twinkle Little Star");
    ESP_LOGI(TAG, "Jumlah nada: %d", melody_length);

    /* Konfigurasi timer LEDC dengan frekuensi awal */
    ledc_timer_config_t timer_conf = {
        .speed_mode      = LEDC_MODE,
        .timer_num        = LEDC_TIMER,
        .duty_resolution = LEDC_DUTY_RES,
        .freq_hz         = NOTE_C4,      /* Frekuensi awal C4 */
        .clk_cfg         = LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timer_conf));

    /* Konfigurasi channel LEDC */
    ledc_channel_config_t ch_conf = {
        .gpio_num   = BUZZER_PIN,
        .speed_mode = LEDC_MODE,
        .channel    = LEDC_CH,
        .timer_sel  = LEDC_TIMER,
        .duty       = 0,       /* Mulai senyap */
        .hpoint     = 0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ch_conf));

    ESP_LOGI(TAG, "LEDC dikonfigurasi untuk buzzer");
    ESP_LOGI(TAG, "-------------------------------------------");

    int play_count = 0;
    while (1) {
        play_count++;
        ESP_LOGI(TAG, "♪ Pemutaran ke-%d dimulai...", play_count);

        /* Mainkan seluruh melodi */
        for (int i = 0; i < melody_length; i++) {
            const note_t *note = &melody[i];
            const char *name = freq_to_note_name(note->frequency);

            /* Format: DATA:<indeks>,<nama_nada>,<frekuensi>,<durasi>,<pemutaran> */
            printf("DATA:%d,%s,%lu,%lu,%d\n",
                   i, name,
                   (unsigned long)note->frequency,
                   (unsigned long)note->duration_ms,
                   play_count);

            ESP_LOGI(TAG, "Nada %2d/%d: %-4s (%4luHz) %lums",
                     i + 1, melody_length, name,
                     (unsigned long)note->frequency,
                     (unsigned long)note->duration_ms);

            play_tone(note->frequency, note->duration_ms);
        }

        ESP_LOGI(TAG, "♪ Pemutaran ke-%d selesai", play_count);

        /* Jeda antar pemutaran */
        ESP_LOGI(TAG, "Jeda 3 detik sebelum pemutaran berikutnya...");
        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}
