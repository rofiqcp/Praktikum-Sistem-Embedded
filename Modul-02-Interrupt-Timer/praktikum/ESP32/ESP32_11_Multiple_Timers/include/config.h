/**
 * @file config.h
 * @brief Konfigurasi pin dan parameter untuk Multiple Timers
 * @details Multi-board support: ESP32 / ESP32-S2 / ESP32-S3
 */

#ifndef CONFIG_H
#define CONFIG_H

/* === LED Pins (3 LED untuk 3 timer) === */
#define LED1_PIN            GPIO_NUM_2    // LED 1 - timer cepat
#define LED2_PIN            GPIO_NUM_4    // LED 2 - timer medium
#define LED3_PIN            GPIO_NUM_5    // LED 3 - timer lambat

/* === Timer Intervals (ms) === */
#define TIMER1_INTERVAL_MS  200           // Timer 1: 200ms (5 Hz)
#define TIMER2_INTERVAL_MS  500           // Timer 2: 500ms (2 Hz)
#define TIMER3_INTERVAL_MS  1000          // Timer 3: 1000ms (1 Hz)

/* === Timer Resolution === */
#define TIMER_RESOLUTION_HZ 1000000       // 1 MHz

#endif /* CONFIG_H */
