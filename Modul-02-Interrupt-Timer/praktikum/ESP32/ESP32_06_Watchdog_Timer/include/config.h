/**
 * ============================================================
 *  ESP32_06_Watchdog_Timer - Konfigurasi
 * ============================================================
 *  Modul   : 02 - Interrupt & Timer
 *  Board   : ESP32 / ESP32-S2 / ESP32-S3
 *  Framework: ESP-IDF
 *
 *  Deskripsi:
 *    Demonstrasi Task Watchdog Timer (TWDT).
 *    Normal mode: feed setiap 1 detik (LED blink).
 *    Tekan tombol: simulasi hang (stop feeding → WDT reset).
 *
 *  Koneksi Hardware:
 *    - GPIO4  <- Push Button (ke GND, internal pull-up)
 *    - GPIO2  -> LED (+ resistor 220Ω ke GND)
 * ============================================================
 */

#ifndef CONFIG_H
#define CONFIG_H

/* ---- Pin Assignment ---- */
#define LED_PIN         2       // GPIO2 - Output, LED indikator
#define BUTTON_PIN      4       // GPIO4 - Input, trigger hang simulation

/* ---- Watchdog Setting ---- */
#define WDT_TIMEOUT_S   5      // Watchdog timeout (detik)

#endif /* CONFIG_H */
