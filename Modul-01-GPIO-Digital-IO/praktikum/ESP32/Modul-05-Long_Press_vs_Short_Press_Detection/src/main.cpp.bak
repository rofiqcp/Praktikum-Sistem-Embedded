/**
 * @file main.cpp
 * @brief Program 05: Long Press vs Short Press Detection
 * 
 * Deskripsi:
 * Mendeteksi jenis penekanan button: short press, long press, very long press.
 * Masing-masing jenis penekanan memiliki aksi yang berbeda.
 * 
 * Aksi:
 * - Short Press (<500ms): Toggle LED 1
 * - Long Press (1-3s): Toggle LED 2
 * - Very Long Press (>3s): Reset kedua LED
 * 
 * @author Praktikum Sistem Embedded
 * @date 2026
 */

#include <Arduino.h>
#include "config.h"

// ==================== PRESS TYPE ====================
typedef enum {
    PRESS_NONE,
    PRESS_SHORT,
    PRESS_LONG,
    PRESS_VERY_LONG
} PressType_t;

// ==================== VARIABEL GLOBAL ====================
bool buttonPressed = false;
bool lastButtonState = HIGH;
unsigned long pressStartTime = 0;
unsigned long lastDebounceTime = 0;
bool led1State = false;
bool led2State = false;

// Statistics
uint32_t shortPressCount = 0;
uint32_t longPressCount = 0;
uint32_t veryLongPressCount = 0;

// ==================== FUNCTION PROTOTYPES ====================
PressType_t detectPressType(unsigned long duration);
void handlePress(PressType_t pressType);
void printStats();

// ==================== SETUP ====================
void setup() {
    Serial.begin(SERIAL_BAUD);
    delay(1000);
    
    Serial.println("\n========================================");
    Serial.println("Program 05: Long/Short Press Detection");
    Serial.println("Praktikum Sistem Embedded");
    Serial.println("========================================\n");
    
    // Konfigurasi pin
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    pinMode(LED_1, OUTPUT);
    pinMode(LED_2, OUTPUT);
    
    digitalWrite(LED_1, LOW);
    digitalWrite(LED_2, LOW);
    
    Serial.println("Press Types:");
    Serial.printf("  Short Press  : < %d ms → Toggle LED1\n", SHORT_PRESS_MAX);
    Serial.printf("  Long Press   : %d-%d ms → Toggle LED2\n", LONG_PRESS_MIN, VERY_LONG_MS);
    Serial.printf("  Very Long    : > %d ms → Reset ALL\n", VERY_LONG_MS);
    Serial.println("\nReady! Press the button...\n");
}

// ==================== LOOP ====================
void loop() {
    bool currentButtonState = digitalRead(BUTTON_PIN);
    
    // Debounce
    if (currentButtonState != lastButtonState) {
        lastDebounceTime = millis();
    }
    
    if ((millis() - lastDebounceTime) > DEBOUNCE_MS) {
        // Button baru saja ditekan
        if (currentButtonState == LOW && !buttonPressed) {
            buttonPressed = true;
            pressStartTime = millis();
            Serial.print("Button pressed... ");
        }
        
        // Button dilepas
        if (currentButtonState == HIGH && buttonPressed) {
            buttonPressed = false;
            unsigned long pressDuration = millis() - pressStartTime;
            
            Serial.printf("released after %lu ms\n", pressDuration);
            
            PressType_t pressType = detectPressType(pressDuration);
            handlePress(pressType);
        }
        
        // Feedback saat masih ditekan (real-time)
        if (buttonPressed) {
            unsigned long currentDuration = millis() - pressStartTime;
            static unsigned long lastFeedback = 0;
            
            // Feedback setiap 500ms
            if (currentDuration - lastFeedback >= 500 && currentDuration >= 500) {
                lastFeedback = currentDuration;
                
                if (currentDuration >= VERY_LONG_MS) {
                    Serial.println("  → VERY LONG detected!");
                } else if (currentDuration >= LONG_PRESS_MIN) {
                    Serial.printf("  → LONG press: %lu ms\n", currentDuration);
                }
            }
        }
    }
    
    lastButtonState = currentButtonState;
}

// ==================== FUNCTIONS ====================

PressType_t detectPressType(unsigned long duration) {
    if (duration >= VERY_LONG_MS) {
        return PRESS_VERY_LONG;
    } else if (duration >= LONG_PRESS_MIN) {
        return PRESS_LONG;
    } else if (duration >= SHORT_PRESS_MIN) {
        return PRESS_SHORT;
    }
    return PRESS_NONE;
}

void handlePress(PressType_t pressType) {
    switch (pressType) {
        case PRESS_SHORT:
            shortPressCount++;
            led1State = !led1State;
            digitalWrite(LED_1, led1State);
            Serial.printf(">>> SHORT PRESS #%lu - LED1 %s\n\n", 
                         shortPressCount, led1State ? "ON" : "OFF");
            break;
            
        case PRESS_LONG:
            longPressCount++;
            led2State = !led2State;
            digitalWrite(LED_2, led2State);
            Serial.printf(">>> LONG PRESS #%lu - LED2 %s\n\n", 
                         longPressCount, led2State ? "ON" : "OFF");
            break;
            
        case PRESS_VERY_LONG:
            veryLongPressCount++;
            led1State = false;
            led2State = false;
            digitalWrite(LED_1, LOW);
            digitalWrite(LED_2, LOW);
            Serial.printf(">>> VERY LONG PRESS #%lu - ALL LEDs OFF (RESET)\n", 
                         veryLongPressCount);
            printStats();
            break;
            
        default:
            Serial.println(">>> Press too short, ignored\n");
            break;
    }
}

void printStats() {
    Serial.println("\n--- Statistics ---");
    Serial.printf("Short Press : %lu\n", shortPressCount);
    Serial.printf("Long Press  : %lu\n", longPressCount);
    Serial.printf("Very Long   : %lu\n", veryLongPressCount);
    Serial.println("------------------\n");
}

/**
 * TIMING DIAGRAM:
 * 
 * Short Press (< 500ms):
 *   Button: ──┐     ┌──
 *             │     │
 *             └─────┘
 *             ← <500ms →
 *   Action: Toggle LED1
 * 
 * Long Press (1-3s):
 *   Button: ──┐           ┌──
 *             │           │
 *             └───────────┘
 *             ← 1000-3000ms →
 *   Action: Toggle LED2
 * 
 * Very Long Press (> 3s):
 *   Button: ──┐                    ┌──
 *             │                    │
 *             └────────────────────┘
 *             ←     > 3000ms      →
 *   Action: Reset ALL LEDs
 * 
 * WIRING DIAGRAM:
 * 
 *   ESP32 DevKitC
 *   ┌─────────────┐
 *   │             │
 *   │        GPIO0├───[Button]───GND (BOOT button)
 *   │             │
 *   │        GPIO2├───[220Ω]───[LED1]───GND (Short press)
 *   │        GPIO4├───[220Ω]───[LED2]───GND (Long press)
 *   │             │
 *   └─────────────┘
 * 
 * EXPECTED OUTPUT:
 * Button pressed... released after 234 ms
 * >>> SHORT PRESS #1 - LED1 ON
 * 
 * Button pressed...   → LONG press: 1000 ms
 *   → LONG press: 1500 ms
 * released after 1823 ms
 * >>> LONG PRESS #1 - LED2 ON
 * 
 * Button pressed...   → LONG press: 1000 ms
 *   → LONG press: 1500 ms
 *   → LONG press: 2000 ms
 *   → LONG press: 2500 ms
 *   → VERY LONG detected!
 * released after 3456 ms
 * >>> VERY LONG PRESS #1 - ALL LEDs OFF (RESET)
 */
