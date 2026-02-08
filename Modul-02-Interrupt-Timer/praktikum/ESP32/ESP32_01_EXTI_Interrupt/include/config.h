/**
 * ============================================================
 *  ESP32_01_EXTI_Interrupt - Konfigurasi Pin
 * ============================================================
 *  Modul   : 02 - Interrupt & Timer
 *  Board   : ESP32 / ESP32-S2 / ESP32-S3
 *  Framework: ESP-IDF
 *
 *  Deskripsi:
 *    External Interrupt (EXTI) pada GPIO.
 *    Tombol ditekan → ISR set flag → main loop toggle LED.
 *
 *  Koneksi Hardware:
 *    - GPIO4  <- Push Button (ke GND, internal pull-up)
 *    - GPIO2  -> LED (+ resistor 220Ω ke GND)
 * ============================================================
 */

#ifndef CONFIG_H
#define CONFIG_H

/* ---- Pin Assignment ---- */
#define BUTTON_PIN      4       // GPIO4 - Input, external interrupt
#define LED_PIN         2       // GPIO2 - Output, LED indikator

#endif /* CONFIG_H */
