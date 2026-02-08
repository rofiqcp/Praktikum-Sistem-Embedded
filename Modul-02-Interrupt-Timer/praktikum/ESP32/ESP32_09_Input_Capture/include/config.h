/**
 * @file config.h
 * @brief Konfigurasi pin dan parameter untuk Input Capture
 * @details Multi-board support: ESP32 / ESP32-S2 / ESP32-S3
 */

#ifndef CONFIG_H
#define CONFIG_H

/* === Pin Definitions === */
#define INPUT_CAPTURE_PIN   GPIO_NUM_4    // Pin input untuk capture pulse
#define LED_STATUS_PIN      GPIO_NUM_2    // LED status indicator

/* === Parameters === */
#define MIN_PULSE_US        1000          // Minimum pulse width valid (1ms)
#define MAX_PULSE_US        5000000       // Maximum pulse width valid (5s)

#endif /* CONFIG_H */
