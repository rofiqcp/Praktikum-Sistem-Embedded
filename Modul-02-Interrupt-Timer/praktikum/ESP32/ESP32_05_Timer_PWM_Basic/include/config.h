/**
 * ============================================================
 *  ESP32_05_Timer_PWM_Basic - Konfigurasi
 * ============================================================
 *  Modul   : 02 - Interrupt & Timer
 *  Board   : ESP32 / ESP32-S2 / ESP32-S3
 *  Framework: ESP-IDF
 *
 *  Deskripsi:
 *    Demonstrasi konfigurasi timer untuk mode PWM
 *    menggunakan LEDC peripheral. Fokus pada setup timer,
 *    bukan aplikasi PWM.
 *
 *  Koneksi Hardware:
 *    - GPIO2  -> LED (+ resistor 220Ω ke GND)
 *    - Opsional: Oscilloscope pada GPIO2 untuk melihat sinyal
 * ============================================================
 */

#ifndef CONFIG_H
#define CONFIG_H

/* ---- Pin Assignment ---- */
#define PWM_PIN         2       // GPIO2 - Output PWM

/* ---- PWM/Timer Setting ---- */
#define PWM_FREQ        50      // Frekuensi PWM (Hz)

#endif /* CONFIG_H */
