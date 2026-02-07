/**
 * ==========================================================================
 *  config.h — Konfigurasi Pin untuk LED Binary Counter
 * ==========================================================================
 *  Board Target:
 *    - ESP32 DevKit   : GPIO16 (BIT0), GPIO17 (BIT1), GPIO18 (BIT2), GPIO19 (BIT3)
 *    - ESP32-S2       : GPIO33-36
 *    - ESP32-S3       : GPIO4-7
 *
 *  Hardware:
 *    - 4x LED + 4x Resistor 220Ω
 *    - LED BIT0 = LSB (paling kanan), LED BIT3 = MSB (paling kiri)
 * ==========================================================================
 */

#ifndef CONFIG_H
#define CONFIG_H

/* ── Jumlah bit (LED) ─────────────────────────────────────────────────── */
#define NUM_BITS        4
#define MAX_COUNT       ((1 << NUM_BITS) - 1)   /* 15 untuk 4-bit */

/* ── Pilih pin sesuai board ───────────────────────────────────────────── */
#if defined(CONFIG_IDF_TARGET_ESP32S2)
    #define LED_BIT0    GPIO_NUM_33     /* LSB */
    #define LED_BIT1    GPIO_NUM_34
    #define LED_BIT2    GPIO_NUM_35
    #define LED_BIT3    GPIO_NUM_36     /* MSB */
#elif defined(CONFIG_IDF_TARGET_ESP32S3)
    #define LED_BIT0    GPIO_NUM_4      /* LSB */
    #define LED_BIT1    GPIO_NUM_5
    #define LED_BIT2    GPIO_NUM_6
    #define LED_BIT3    GPIO_NUM_7      /* MSB */
#else
    #define LED_BIT0    GPIO_NUM_16     /* LSB */
    #define LED_BIT1    GPIO_NUM_17
    #define LED_BIT2    GPIO_NUM_18
    #define LED_BIT3    GPIO_NUM_19     /* MSB */
#endif

/* ── Delay antar increment (ms) ───────────────────────────────────────── */
#define COUNT_DELAY_MS  500

#endif /* CONFIG_H */
