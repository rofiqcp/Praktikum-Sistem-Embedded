/**
 * ==========================================================================
 *  config.h — Konfigurasi Pin untuk Toggle Latch (Edge Detection)
 * ==========================================================================
 *  Board Target:
 *    - ESP32 DevKit   : Button GPIO4, LED GPIO2
 *    - ESP32-S2       : Button GPIO9, LED GPIO15
 *    - ESP32-S3       : Button GPIO0, LED GPIO2
 *
 *  Hardware:
 *    - 1x Push button (active-LOW, pull-up)
 *    - 1x LED + 1x Resistor 220Ω
 * ==========================================================================
 */

#ifndef CONFIG_H
#define CONFIG_H

/* ── Pilih pin sesuai board ───────────────────────────────────────────── */
#if defined(CONFIG_IDF_TARGET_ESP32S2)
    #define BUTTON_PIN  GPIO_NUM_9
    #define LED_PIN     GPIO_NUM_15
#elif defined(CONFIG_IDF_TARGET_ESP32S3)
    #define BUTTON_PIN  GPIO_NUM_0
    #define LED_PIN     GPIO_NUM_2
#else
    /* ESP32 DevKit (default) */
    #define BUTTON_PIN  GPIO_NUM_4
    #define LED_PIN     GPIO_NUM_2
#endif

/* ── Debounce ─────────────────────────────────────────────────────────── */
#define DEBOUNCE_MS     50      /* Software debounce (ms) */

/* ── Polling interval ─────────────────────────────────────────────────── */
#define POLL_INTERVAL_MS  10

#endif /* CONFIG_H */
