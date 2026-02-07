/**
 * @file main.cpp
 * @brief Program 06: Toggle LED dengan Latch Behavior
 * 
 * Implementasi latch/flip-flop behavior menggunakan button dan LED.
 * Satu tekan = ON, tekan lagi = OFF (toggle persistent)
 */

#include <Arduino.h>

// ==================== KONFIGURASI ====================
#define BUTTON_PIN      0       // BOOT button
#define LED_PIN         2       // Built-in LED
#define DEBOUNCE_MS     50

// ==================== VARIABEL ====================
volatile bool latchState = false;   // Latch state (persistent)
volatile bool buttonFlag = false;   // Flag dari ISR
unsigned long lastInterrupt = 0;

// ==================== ISR ====================
void IRAM_ATTR buttonISR() {
    unsigned long now = millis();
    if (now - lastInterrupt > DEBOUNCE_MS) {
        buttonFlag = true;
        lastInterrupt = now;
    }
}

// ==================== SETUP ====================
void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\n========================================");
    Serial.println("Program 06: Toggle Latch Behavior");
    Serial.println("========================================\n");
    
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);
    
    // Attach interrupt pada falling edge (button ditekan)
    attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), buttonISR, FALLING);
    
    Serial.println("Latch initialized: OFF");
    Serial.println("Press button to toggle latch state\n");
}

// ==================== LOOP ====================
void loop() {
    if (buttonFlag) {
        buttonFlag = false;
        
        // Toggle latch state
        latchState = !latchState;
        digitalWrite(LED_PIN, latchState);
        
        Serial.printf("[%lu ms] LATCH = %s\n", 
                      millis(),
                      latchState ? "ON (SET)" : "OFF (RESET)");
    }
}

/**
 * KONSEP LATCH/FLIP-FLOP:
 * 
 * Set-Reset Latch Truth Table:
 * ┌─────────┬─────────┬────────┐
 * │  Input  │  State  │ Output │
 * ├─────────┼─────────┼────────┤
 * │  Press  │   OFF   │   ON   │
 * │  Press  │   ON    │   OFF  │
 * │ No Press│   ON    │   ON   │ ← Persistent!
 * │ No Press│   OFF   │   OFF  │ ← Persistent!
 * └─────────┴─────────┴────────┘
 * 
 * WIRING:
 *   GPIO0 (BOOT button) → Internal Pull-up
 *   GPIO2 → 220Ω → LED → GND
 * 
 * EXPECTED OUTPUT:
 * [1234 ms] LATCH = ON (SET)
 * [2456 ms] LATCH = OFF (RESET)
 * [3789 ms] LATCH = ON (SET)
 */
