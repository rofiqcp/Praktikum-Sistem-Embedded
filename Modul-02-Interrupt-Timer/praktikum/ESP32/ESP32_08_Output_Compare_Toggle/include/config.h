/**
 * @file config.h
 * @brief Konfigurasi pin dan parameter untuk Output Compare Toggle
 * @details Multi-board support: ESP32 / ESP32-S2 / ESP32-S3
 */

#ifndef CONFIG_H
#define CONFIG_H

/* === LED/Output Pin === */
#define OUTPUT_PIN          GPIO_NUM_2    // Pin output toggle (LED/oscilloscope)

/* === Timer Parameters === */
#define TOGGLE_FREQ_HZ      1            // Frekuensi toggle: 1 Hz (0.5s ON, 0.5s OFF)
#define TIMER_RESOLUTION_HZ 1000000      // 1 MHz = 1 us per tick

#endif /* CONFIG_H */
