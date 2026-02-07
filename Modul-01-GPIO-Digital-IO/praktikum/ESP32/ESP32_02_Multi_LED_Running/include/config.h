/**
 * ==========================================================================
 *  config.h — Konfigurasi Pin untuk Multi LED Running Light
 * ==========================================================================
 *  Board Target:
 *    - ESP32 DevKit   : GPIO16, GPIO17, GPIO18, GPIO19
 *    - ESP32-S2 (Lolin): GPIO33, GPIO34, GPIO35, GPIO36
 *    - ESP32-S3       : GPIO4, GPIO5, GPIO6, GPIO7
 *
 *  Hardware:
 *    - 4x LED (warna bebas) + 4x Resistor 220Ω
 *    - LED anoda ke GPIO melalui resistor, katoda ke GND
 * ==========================================================================
 */

#ifndef CONFIG_H
#define CONFIG_H

/* ── Jumlah LED ───────────────────────────────────────────────────────── */
#define NUM_LEDS        4

/* ── Pilih pin sesuai board ───────────────────────────────────────────── */
#if defined(CONFIG_IDF_TARGET_ESP32S2)
    /* ESP32-S2 (Lolin S2 Mini) */
    #define LED_PIN_1   GPIO_NUM_33
    #define LED_PIN_2   GPIO_NUM_34
    #define LED_PIN_3   GPIO_NUM_35
    #define LED_PIN_4   GPIO_NUM_36
#elif defined(CONFIG_IDF_TARGET_ESP32S3)
    /* ESP32-S3 DevKitC-1 */
    #define LED_PIN_1   GPIO_NUM_4
    #define LED_PIN_2   GPIO_NUM_5
    #define LED_PIN_3   GPIO_NUM_6
    #define LED_PIN_4   GPIO_NUM_7
#else
    /* ESP32 DevKit (default) */
    #define LED_PIN_1   GPIO_NUM_16
    #define LED_PIN_2   GPIO_NUM_17
    #define LED_PIN_3   GPIO_NUM_18
    #define LED_PIN_4   GPIO_NUM_19
#endif

/* ── Delay antar perpindahan LED (ms) ─────────────────────────────────── */
#define RUNNING_DELAY_MS    250

#endif /* CONFIG_H */
