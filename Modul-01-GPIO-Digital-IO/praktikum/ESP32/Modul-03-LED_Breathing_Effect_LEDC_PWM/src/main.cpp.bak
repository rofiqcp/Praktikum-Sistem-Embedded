/**
 * @file main.cpp
 * @brief Program 03: LED Breathing Effect menggunakan LEDC PWM
 * 
 * Deskripsi:
 * Menghasilkan efek breathing (fade in/out) pada LED menggunakan
 * peripheral LEDC (LED Control) ESP32 yang memiliki hardware PWM.
 * 
 * Hardware:
 * - ESP32 DevKitC
 * - LED dengan resistor 220Ω
 * 
 * @author Praktikum Sistem Embedded
 * @date 2026
 */

#include <Arduino.h>
#include "config.h"

// ==================== VARIABEL GLOBAL ====================
int brightness = BREATH_MIN;
int fadeAmount = BREATH_STEP;
unsigned long previousMillis = 0;

// ==================== SETUP ====================
void setup() {
    Serial.begin(SERIAL_BAUD);
    delay(1000);
    
    Serial.println("\n========================================");
    Serial.println("Program 03: LED Breathing (LEDC PWM)");
    Serial.println("Praktikum Sistem Embedded");
    Serial.println("========================================\n");
    
    // Konfigurasi LEDC PWM
    // ledcSetup(channel, freq, resolution) - deprecated di ESP32 Arduino 3.x
    // Menggunakan ledcAttach() untuk versi terbaru
    
    #if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
        // ESP32 Arduino Core 3.x
        ledcAttach(LED_PIN, LEDC_FREQ, LEDC_RESOLUTION);
        Serial.println("Using ledcAttach() - ESP32 Core 3.x");
    #else
        // ESP32 Arduino Core 2.x
        ledcSetup(LEDC_CHANNEL, LEDC_FREQ, LEDC_RESOLUTION);
        ledcAttachPin(LED_PIN, LEDC_CHANNEL);
        Serial.println("Using ledcSetup() - ESP32 Core 2.x");
    #endif
    
    Serial.printf("LED Pin: GPIO%d\n", LED_PIN);
    Serial.printf("PWM Frequency: %d Hz\n", LEDC_FREQ);
    Serial.printf("PWM Resolution: %d-bit (0-%d)\n", LEDC_RESOLUTION, (1 << LEDC_RESOLUTION) - 1);
    Serial.println("\nBreathing effect started...\n");
}

// ==================== LOOP ====================
void loop() {
    unsigned long currentMillis = millis();
    
    if (currentMillis - previousMillis >= BREATH_DELAY) {
        previousMillis = currentMillis;
        
        // Set PWM duty cycle
        #if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
            ledcWrite(LED_PIN, brightness);
        #else
            ledcWrite(LEDC_CHANNEL, brightness);
        #endif
        
        // Update brightness
        brightness += fadeAmount;
        
        // Reverse direction at limits
        if (brightness <= BREATH_MIN || brightness >= BREATH_MAX) {
            fadeAmount = -fadeAmount;
            
            // Debug output at peaks
            if (brightness >= BREATH_MAX) {
                Serial.printf("[%lu ms] Peak brightness: %d\n", currentMillis, brightness);
            } else {
                Serial.printf("[%lu ms] Minimum brightness: %d\n", currentMillis, brightness);
            }
        }
    }
}

/**
 * PENJELASAN LEDC ESP32:
 * 
 * ESP32 memiliki 16 channel PWM (LEDC - LED Control)
 * - Channel 0-7: High Speed (80MHz clock)
 * - Channel 8-15: Low Speed (8MHz clock)
 * 
 * Resolusi:
 * - 1-bit: 0-1 (2 levels)
 * - 8-bit: 0-255 (256 levels)
 * - 10-bit: 0-1023 (1024 levels)
 * - 12-bit: 0-4095 (4096 levels)
 * - 16-bit: 0-65535 (65536 levels)
 * 
 * Formula frekuensi max:
 * freq_max = 80MHz / (2^resolution)
 * Contoh: 8-bit → 80MHz/256 = 312.5kHz max
 * 
 * WIRING DIAGRAM:
 * 
 *   ESP32 DevKitC
 *   ┌─────────────┐
 *   │             │
 *   │        GPIO2├───[220Ω]───[LED+]
 *   │             │              │
 *   │         GND ├────────────[LED-]
 *   │             │
 *   └─────────────┘
 * 
 * EXPECTED OUTPUT:
 * ========================================
 * Program 03: LED Breathing (LEDC PWM)
 * ========================================
 * 
 * LED Pin: GPIO2
 * PWM Frequency: 5000 Hz
 * PWM Resolution: 8-bit (0-255)
 * 
 * Breathing effect started...
 * 
 * [1020 ms] Peak brightness: 255
 * [2040 ms] Minimum brightness: 0
 * [3060 ms] Peak brightness: 255
 * ...
 */
