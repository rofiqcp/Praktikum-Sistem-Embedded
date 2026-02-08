/**
 * @file config.h
 * @brief Konfigurasi pin dan parameter untuk Timer Cascade
 * @details Multi-board support: ESP32 / ESP32-S2 / ESP32-S3
 */

#ifndef CONFIG_H
#define CONFIG_H

/* === LED Pin === */
#define LED_FAST_PIN        GPIO_NUM_2    // LED toggle cepat (timer 1)
#define LED_SLOW_PIN        GPIO_NUM_4    // LED toggle lambat (cascaded)

/* === Timer Parameters === */
#define TIMER_FAST_MS       100           // Timer cepat: 100ms interval
#define CASCADE_COUNT       10            // Cascade: setiap 10x timer cepat = 1s
#define TIMER_RESOLUTION_HZ 1000000       // 1 MHz = 1 us per tick

#endif /* CONFIG_H */
