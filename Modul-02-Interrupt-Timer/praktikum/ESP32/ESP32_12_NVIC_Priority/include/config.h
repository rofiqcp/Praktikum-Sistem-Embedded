/**
 * @file config.h
 * @brief Konfigurasi pin dan parameter untuk Interrupt Priority Demo
 * @details Multi-board support: ESP32 / ESP32-S2 / ESP32-S3
 */

#ifndef CONFIG_H
#define CONFIG_H

/* === Button Pins === */
#define BUTTON_HIGH_PIN     GPIO_NUM_18   // Button high priority interrupt
#define BUTTON_LOW_PIN      GPIO_NUM_19   // Button low priority interrupt

/* === LED Pins === */
#define LED_HIGH_PIN        GPIO_NUM_2    // LED for high-priority ISR
#define LED_LOW_PIN         GPIO_NUM_4    // LED for low-priority ISR

/* === Parameters === */
#define ISR_WORK_MS         500           // Simulated ISR work time (ms)
#define DEBOUNCE_US         200000        // 200ms debounce

#endif /* CONFIG_H */
