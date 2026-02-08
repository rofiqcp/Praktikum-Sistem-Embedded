/**
 * @file config.h
 * @brief Konfigurasi pin dan parameter untuk Encoder Interface
 * @details Multi-board support: ESP32 / ESP32-S2 / ESP32-S3
 */

#ifndef CONFIG_H
#define CONFIG_H

/* === Encoder Pins (KY-040 Rotary Encoder) === */
#define ENCODER_A_PIN       GPIO_NUM_18   // Encoder channel A (CLK)
#define ENCODER_B_PIN       GPIO_NUM_19   // Encoder channel B (DT)
#define ENCODER_SW_PIN      GPIO_NUM_4    // Encoder push button (SW)

/* === LED Pin === */
#define LED_PIN             GPIO_NUM_2    // LED indicator

/* === Parameters === */
#define DEBOUNCE_US         5000          // Debounce 5ms untuk encoder
#define POSITION_MIN        0             // Posisi minimum
#define POSITION_MAX        100           // Posisi maximum

#endif /* CONFIG_H */
