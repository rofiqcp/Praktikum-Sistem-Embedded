/**
 * @file main.cpp
 * @brief Program 10: GPIO Matrix (Pin Reassignment)
 * 
 * Demonstrasi GPIO Matrix ESP32 - kemampuan memetakan
 * sinyal peripheral ke GPIO yang berbeda secara software.
 */

#include <Arduino.h>
#include "driver/gpio.h"
#include "driver/ledc.h"

// ==================== KONFIGURASI ====================
// Original pin mapping
#define LED_ORIGINAL    2

// Alternative pin mapping (via GPIO Matrix)
#define LED_ALT_1       4
#define LED_ALT_2       5
#define LED_ALT_3       18

#define BUTTON_PIN      0
#define PWM_FREQ        5000
#define PWM_RESOLUTION  8

// ==================== VARIABEL ====================
uint8_t currentPin = 0;
const uint8_t ledPins[] = {LED_ORIGINAL, LED_ALT_1, LED_ALT_2, LED_ALT_3};
const uint8_t numPins = sizeof(ledPins) / sizeof(ledPins[0]);

volatile bool switchPin = false;
unsigned long lastSwitch = 0;

void IRAM_ATTR buttonISR() {
    if (millis() - lastSwitch > 300) {
        switchPin = true;
        lastSwitch = millis();
    }
}

// ==================== SETUP ====================
void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\n========================================");
    Serial.println("Program 10: GPIO Matrix Demo");
    Serial.println("========================================\n");
    
    // Initialize all potential LED pins as output
    for (int i = 0; i < numPins; i++) {
        pinMode(ledPins[i], OUTPUT);
        digitalWrite(ledPins[i], LOW);
    }
    
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), buttonISR, FALLING);
    
    // Start with first pin
    #if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
        ledcAttach(ledPins[currentPin], PWM_FREQ, PWM_RESOLUTION);
    #else
        ledcSetup(0, PWM_FREQ, PWM_RESOLUTION);
        ledcAttachPin(ledPins[currentPin], 0);
    #endif
    
    Serial.println("ESP32 GPIO Matrix Features:");
    Serial.println("  - Peripheral signals can be mapped to almost any GPIO");
    Serial.println("  - PWM, SPI, I2C, UART can use different pins");
    Serial.println("  - Great for PCB routing flexibility");
    Serial.println();
    Serial.println("Available LED pins:");
    for (int i = 0; i < numPins; i++) {
        Serial.printf("  %d. GPIO%d\n", i+1, ledPins[i]);
    }
    Serial.printf("\nCurrently active: GPIO%d\n", ledPins[currentPin]);
    Serial.println("Press button to switch PWM to next pin\n");
}

// ==================== LOOP ====================
void loop() {
    // Handle pin switching
    if (switchPin) {
        switchPin = false;
        
        // Detach current pin
        #if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
            ledcDetach(ledPins[currentPin]);
        #else
            ledcDetachPin(ledPins[currentPin]);
        #endif
        digitalWrite(ledPins[currentPin], LOW);
        
        // Move to next pin
        currentPin = (currentPin + 1) % numPins;
        
        // Attach to new pin
        #if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
            ledcAttach(ledPins[currentPin], PWM_FREQ, PWM_RESOLUTION);
        #else
            ledcAttachPin(ledPins[currentPin], 0);
        #endif
        
        Serial.printf(">>> PWM switched to GPIO%d\n", ledPins[currentPin]);
    }
    
    // Breathing effect on current pin
    static int brightness = 0;
    static int fadeAmount = 5;
    
    #if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
        ledcWrite(ledPins[currentPin], brightness);
    #else
        ledcWrite(0, brightness);
    #endif
    
    brightness += fadeAmount;
    if (brightness <= 0 || brightness >= 255) {
        fadeAmount = -fadeAmount;
    }
    
    delay(20);
}

/**
 * ESP32 GPIO MATRIX EXPLAINED:
 * 
 * ESP32 memiliki GPIO Matrix yang memungkinkan:
 * 
 * 1. INPUT MATRIX:
 *    GPIO → Peripheral
 *    Misal: GPIO12 bisa jadi UART RX, SPI MISO, dll
 * 
 * 2. OUTPUT MATRIX:
 *    Peripheral → GPIO
 *    Misal: PWM channel 0 bisa output ke GPIO2, GPIO4, dll
 * 
 *    ┌──────────────────────────────────────────┐
 *    │              ESP32 Internal              │
 *    │  ┌─────────┐      ┌─────────────────┐   │
 *    │  │  LEDC   │      │   GPIO Matrix   │   │
 *    │  │ Channel │─────→│   (Crossbar)    │──→│ GPIO2
 *    │  │   0     │      │                 │──→│ GPIO4
 *    │  └─────────┘      │                 │──→│ GPIO5
 *    │                   │                 │──→│ GPIO18
 *    │  ┌─────────┐      │                 │   │
 *    │  │  UART   │─────→│                 │──→│ ...
 *    │  └─────────┘      └─────────────────┘   │
 *    └──────────────────────────────────────────┘
 * 
 * KEUNTUNGAN:
 * - Flexibilitas PCB design
 * - Bisa swap pin jika ada kerusakan
 * - Multiple peripherals berbagi pin (time-multiplexed)
 * 
 * BATASAN:
 * - GPIO 34-39 hanya INPUT
 * - Beberapa GPIO punya fungsi khusus saat boot
 * - GPIO 6-11 digunakan untuk Flash (jangan dipakai)
 * 
 * WIRING:
 *   Connect LEDs to: GPIO2, GPIO4, GPIO5, GPIO18
 *   Each with 220Ω resistor to GND
 */
