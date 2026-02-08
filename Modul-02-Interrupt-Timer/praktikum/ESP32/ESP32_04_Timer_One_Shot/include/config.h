/**
 * ============================================================
 *  ESP32_04_Timer_One_Shot - Konfigurasi
 * ============================================================
 *  Modul   : 02 - Interrupt & Timer
 *  Board   : ESP32 / ESP32-S2 / ESP32-S3
 *  Framework: ESP-IDF
 *
 *  Deskripsi:
 *    One-shot timer untuk auto-shutoff LED.
 *    Tekan tombol → LED ON → setelah 3 detik → LED OFF otomatis.
 *
 *  Koneksi Hardware:
 *    - GPIO4  <- Push Button (ke GND, internal pull-up)
 *    - GPIO2  -> LED (+ resistor 220Ω ke GND)
 * ============================================================
 */

#ifndef CONFIG_H
#define CONFIG_H

/* ---- Pin Assignment ---- */
#define BUTTON_PIN      4       // GPIO4 - Input, trigger one-shot
#define LED_PIN         2       // GPIO2 - Output, LED indikator

/* ---- Timer Setting ---- */
#define DELAY_MS        3000    // Delay sebelum LED mati otomatis (ms)

#endif /* CONFIG_H */
