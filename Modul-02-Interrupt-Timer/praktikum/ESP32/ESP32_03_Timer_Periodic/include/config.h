/**
 * ============================================================
 *  ESP32_03_Timer_Periodic - Konfigurasi
 * ============================================================
 *  Modul   : 02 - Interrupt & Timer
 *  Board   : ESP32 / ESP32-S2 / ESP32-S3
 *  Framework: ESP-IDF
 *
 *  Deskripsi:
 *    Hardware GP Timer dengan alarm periodik.
 *    Timer interrupt toggle LED setiap interval tertentu.
 *
 *  Koneksi Hardware:
 *    - GPIO2  -> LED (+ resistor 220Ω ke GND)
 * ============================================================
 */

#ifndef CONFIG_H
#define CONFIG_H

/* ---- Pin Assignment ---- */
#define LED_PIN             2       // GPIO2 - Output, LED indikator

/* ---- Timer Setting ---- */
#define TIMER_INTERVAL_MS   1000    // Interval alarm periodik (ms)

#endif /* CONFIG_H */
