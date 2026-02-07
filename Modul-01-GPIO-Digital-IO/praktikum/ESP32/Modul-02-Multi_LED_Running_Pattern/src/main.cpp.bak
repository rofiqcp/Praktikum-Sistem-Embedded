/**
 * @file main.cpp
 * @brief Program 02: Multi-LED Running Pattern
 * 
 * Deskripsi:
 * Mengendalikan beberapa LED dengan berbagai pola running.
 * Termasuk pola: running, bounce, fill, dan blink all.
 * 
 * Hardware:
 * - ESP32 DevKitC
 * - 4x LED dengan resistor 220Ω
 * 
 * @author Praktikum Sistem Embedded
 * @date 2026
 */

#include <Arduino.h>
#include "config.h"

// ==================== VARIABEL GLOBAL ====================
uint8_t currentLed = 0;
uint8_t currentPattern = PATTERN_RUNNING;
bool direction = true;  // true = forward, false = backward
unsigned long previousMillis = 0;
uint8_t fillCount = 0;

// ==================== FUNCTION PROTOTYPES ====================
void initLEDs();
void allLEDsOff();
void allLEDsOn();
void runningPattern();
void bouncePattern();
void fillPattern();
void blinkAllPattern();
void showCurrentPattern();

// ==================== SETUP ====================
void setup() {
    Serial.begin(SERIAL_BAUD);
    delay(1000);
    
    Serial.println("\n========================================");
    Serial.println("Program 02: Multi-LED Running Pattern");
    Serial.println("Praktikum Sistem Embedded");
    Serial.println("========================================\n");
    
    initLEDs();
    
    Serial.println("Patterns available:");
    Serial.println("0 - Running (satu arah)");
    Serial.println("1 - Bounce (bolak-balik)");
    Serial.println("2 - Fill (mengisi)");
    Serial.println("3 - Blink All");
    Serial.println("\nKetik angka 0-3 untuk ganti pattern\n");
}

// ==================== LOOP ====================
void loop() {
    unsigned long currentMillis = millis();
    
    // Cek input serial untuk ganti pattern
    if (Serial.available() > 0) {
        char input = Serial.read();
        if (input >= '0' && input <= '3') {
            currentPattern = input - '0';
            currentLed = 0;
            fillCount = 0;
            direction = true;
            allLEDsOff();
            showCurrentPattern();
        }
    }
    
    // Update LED berdasarkan pattern
    if (currentMillis - previousMillis >= PATTERN_DELAY) {
        previousMillis = currentMillis;
        
        switch (currentPattern) {
            case PATTERN_RUNNING:
                runningPattern();
                break;
            case PATTERN_BOUNCE:
                bouncePattern();
                break;
            case PATTERN_FILL:
                fillPattern();
                break;
            case PATTERN_BLINK_ALL:
                blinkAllPattern();
                break;
        }
    }
}

// ==================== FUNCTIONS ====================

void initLEDs() {
    Serial.print("Initializing LEDs on GPIO: ");
    for (int i = 0; i < NUM_LEDS; i++) {
        pinMode(LED_PINS[i], OUTPUT);
        digitalWrite(LED_PINS[i], LOW);
        Serial.printf("%d ", LED_PINS[i]);
    }
    Serial.println("\n");
}

void allLEDsOff() {
    for (int i = 0; i < NUM_LEDS; i++) {
        digitalWrite(LED_PINS[i], LOW);
    }
}

void allLEDsOn() {
    for (int i = 0; i < NUM_LEDS; i++) {
        digitalWrite(LED_PINS[i], HIGH);
    }
}

void runningPattern() {
    allLEDsOff();
    digitalWrite(LED_PINS[currentLed], HIGH);
    
    Serial.printf("Running: LED %d ON\n", currentLed + 1);
    
    currentLed++;
    if (currentLed >= NUM_LEDS) {
        currentLed = 0;
    }
}

void bouncePattern() {
    allLEDsOff();
    digitalWrite(LED_PINS[currentLed], HIGH);
    
    Serial.printf("Bounce: LED %d ON (%s)\n", 
                  currentLed + 1, 
                  direction ? "→" : "←");
    
    if (direction) {
        currentLed++;
        if (currentLed >= NUM_LEDS - 1) {
            direction = false;
        }
    } else {
        currentLed--;
        if (currentLed <= 0) {
            direction = true;
        }
    }
}

void fillPattern() {
    // Nyalakan LED dari 0 sampai fillCount
    for (int i = 0; i <= fillCount; i++) {
        digitalWrite(LED_PINS[i], HIGH);
    }
    // Matikan sisanya
    for (int i = fillCount + 1; i < NUM_LEDS; i++) {
        digitalWrite(LED_PINS[i], LOW);
    }
    
    Serial.printf("Fill: %d LED(s) ON\n", fillCount + 1);
    
    fillCount++;
    if (fillCount >= NUM_LEDS) {
        fillCount = 0;
        allLEDsOff();
        delay(200);  // Pause sebelum restart
    }
}

void blinkAllPattern() {
    static bool state = false;
    state = !state;
    
    if (state) {
        allLEDsOn();
        Serial.println("Blink: ALL ON");
    } else {
        allLEDsOff();
        Serial.println("Blink: ALL OFF");
    }
}

void showCurrentPattern() {
    const char* patterns[] = {"RUNNING", "BOUNCE", "FILL", "BLINK ALL"};
    Serial.printf("\n>>> Pattern changed to: %s\n\n", patterns[currentPattern]);
}

/**
 * WIRING DIAGRAM:
 * 
 *   ESP32 DevKitC
 *   ┌─────────────┐
 *   │             │
 *   │        GPIO2├───[220Ω]───[LED1]───┐
 *   │        GPIO4├───[220Ω]───[LED2]───┤
 *   │        GPIO5├───[220Ω]───[LED3]───┤
 *   │       GPIO18├───[220Ω]───[LED4]───┤
 *   │             │                     │
 *   │         GND ├─────────────────────┘
 *   │             │
 *   └─────────────┘
 * 
 * EXPECTED OUTPUT:
 * Running: LED 1 ON
 * Running: LED 2 ON
 * Running: LED 3 ON
 * Running: LED 4 ON
 * Running: LED 1 ON
 * ...
 */
