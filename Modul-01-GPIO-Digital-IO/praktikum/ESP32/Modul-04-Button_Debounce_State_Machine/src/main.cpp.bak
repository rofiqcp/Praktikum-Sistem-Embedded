/**
 * @file main.cpp
 * @brief Program 04: Button Debounce menggunakan State Machine
 * 
 * Deskripsi:
 * Implementasi debounce pushbutton menggunakan state machine.
 * Menghindari pembacaan ganda akibat bouncing mekanis switch.
 * 
 * Hardware:
 * - ESP32 DevKitC
 * - Pushbutton (atau gunakan BOOT button di GPIO0)
 * - LED indicator
 * 
 * @author Praktikum Sistem Embedded
 * @date 2026
 */

#include <Arduino.h>
#include "config.h"

// ==================== DEBOUNCE STATE MACHINE ====================
typedef enum {
    BTN_IDLE,           // Button tidak ditekan
    BTN_DEBOUNCE,       // Sedang debounce (menunggu stabil)
    BTN_PRESSED,        // Button ditekan (stabil)
    BTN_RELEASED        // Button dilepas
} ButtonState_t;

// ==================== VARIABEL GLOBAL ====================
ButtonState_t buttonState = BTN_IDLE;
bool lastButtonRead = HIGH;         // Button dengan pull-up (HIGH = tidak ditekan)
bool currentButtonRead = HIGH;
unsigned long debounceStartTime = 0;
unsigned long pressStartTime = 0;
uint32_t pressCount = 0;
bool ledState = false;

// ==================== FUNCTION PROTOTYPES ====================
void updateButtonState();
void handleButtonPress();
void handleButtonRelease();

// ==================== SETUP ====================
void setup() {
    Serial.begin(SERIAL_BAUD);
    delay(1000);
    
    Serial.println("\n========================================");
    Serial.println("Program 04: Button Debounce");
    Serial.println("Praktikum Sistem Embedded");
    Serial.println("========================================\n");
    
    // Konfigurasi pin
    pinMode(BUTTON_PIN, INPUT_PULLUP);  // Internal pull-up
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);
    
    Serial.printf("Button Pin: GPIO%d (aktif LOW)\n", BUTTON_PIN);
    Serial.printf("LED Pin: GPIO%d\n", LED_PIN);
    Serial.printf("Debounce Time: %d ms\n", DEBOUNCE_MS);
    Serial.println("\nTekan button untuk toggle LED...\n");
}

// ==================== LOOP ====================
void loop() {
    updateButtonState();
}

// ==================== BUTTON STATE MACHINE ====================
void updateButtonState() {
    currentButtonRead = digitalRead(BUTTON_PIN);
    
    switch (buttonState) {
        case BTN_IDLE:
            // Menunggu button ditekan (transisi HIGH → LOW)
            if (currentButtonRead == LOW && lastButtonRead == HIGH) {
                buttonState = BTN_DEBOUNCE;
                debounceStartTime = millis();
            }
            break;
            
        case BTN_DEBOUNCE:
            // Tunggu debounce time
            if (millis() - debounceStartTime >= DEBOUNCE_MS) {
                // Cek apakah masih ditekan setelah debounce
                if (currentButtonRead == LOW) {
                    buttonState = BTN_PRESSED;
                    pressStartTime = millis();
                    handleButtonPress();
                } else {
                    // False trigger, kembali ke IDLE
                    buttonState = BTN_IDLE;
                }
            }
            break;
            
        case BTN_PRESSED:
            // Tunggu button dilepas
            if (currentButtonRead == HIGH) {
                buttonState = BTN_RELEASED;
                debounceStartTime = millis();
            }
            break;
            
        case BTN_RELEASED:
            // Debounce release
            if (millis() - debounceStartTime >= DEBOUNCE_MS) {
                if (currentButtonRead == HIGH) {
                    handleButtonRelease();
                    buttonState = BTN_IDLE;
                } else {
                    // Masih ditekan
                    buttonState = BTN_PRESSED;
                }
            }
            break;
    }
    
    lastButtonRead = currentButtonRead;
}

void handleButtonPress() {
    pressCount++;
    
    // Toggle LED
    ledState = !ledState;
    digitalWrite(LED_PIN, ledState);
    
    Serial.printf("[%lu ms] Button PRESSED #%lu - LED %s\n", 
                  millis(), 
                  pressCount,
                  ledState ? "ON" : "OFF");
}

void handleButtonRelease() {
    unsigned long pressDuration = millis() - pressStartTime;
    
    Serial.printf("[%lu ms] Button RELEASED - Duration: %lu ms", 
                  millis(), 
                  pressDuration);
    
    if (pressDuration >= LONG_PRESS_MS) {
        Serial.println(" (LONG PRESS)");
    } else {
        Serial.println(" (short press)");
    }
    Serial.println();
}

/**
 * PENJELASAN DEBOUNCE:
 * 
 * Bouncing adalah fenomena dimana kontak mekanis switch
 * menghasilkan pulsa ON/OFF berulang dalam waktu singkat
 * sebelum stabil.
 * 
 * Tanpa Debounce:
 *   Button Press  ──┐┌┐┌┐┌┐┌──────────┐┌┐┌┐┌┐
 *                   └┘└┘└┘└┘          └┘└┘└┘└──
 *                   ←──────── bouncing ────────→
 *   Detected Press: Multiple (salah!)
 * 
 * Dengan Debounce:
 *   Button Press  ──┐┌┐┌┐┌┐┌──────────┐┌┐┌┐┌┐
 *                   └┘└┘└┘└┘          └┘└┘└┘└──
 *                   ←wait→↓           ←wait→↓
 *   Detected:            1 press          1 release
 * 
 * STATE MACHINE:
 * 
 *   ┌─────────┐  pressed   ┌───────────┐
 *   │  IDLE   │───────────→│ DEBOUNCE  │
 *   └─────────┘            └───────────┘
 *        ↑                       │
 *        │                       │ stable
 *        │                       ↓
 *   ┌──────────┐ released  ┌───────────┐
 *   │ RELEASED │←──────────│  PRESSED  │
 *   └──────────┘           └───────────┘
 * 
 * WIRING DIAGRAM:
 * 
 *   ESP32 DevKitC
 *   ┌─────────────┐
 *   │             │
 *   │        GPIO0├───┬───[Button]───GND
 *   │             │   │
 *   │             │   └──[10kΩ]───3.3V (optional, ada internal pull-up)
 *   │             │
 *   │        GPIO2├───[220Ω]───[LED]───GND
 *   │             │
 *   └─────────────┘
 * 
 * Note: GPIO0 adalah BOOT button pada ESP32 DevKitC
 * 
 * EXPECTED OUTPUT:
 * [1523 ms] Button PRESSED #1 - LED ON
 * [1698 ms] Button RELEASED - Duration: 175 ms (short press)
 * 
 * [3241 ms] Button PRESSED #2 - LED OFF
 * [4589 ms] Button RELEASED - Duration: 1348 ms (LONG PRESS)
 */
