/**
 * @file main.cpp
 * @brief Program 07: GPIO Drive Strength Configuration
 * 
 * Konfigurasi drive strength GPIO ESP32 untuk berbagai kebutuhan arus.
 * ESP32 mendukung 4 level: 5mA, 10mA, 20mA, 40mA
 */

#include <Arduino.h>
#include "driver/gpio.h"

// ==================== KONFIGURASI ====================
#define LED_WEAK        2       // Low drive strength
#define LED_NORMAL      4       // Default drive strength
#define LED_STRONG      5       // High drive strength

#define BUTTON_PIN      0       // Untuk cycle drive strength

// ==================== VARIABEL ====================
uint8_t currentDrive = 0;
const char* driveNames[] = {"5mA (WEAK)", "10mA (DEFAULT)", "20mA (MEDIUM)", "40mA (STRONG)"};
gpio_drive_cap_t driveValues[] = {GPIO_DRIVE_CAP_0, GPIO_DRIVE_CAP_1, GPIO_DRIVE_CAP_2, GPIO_DRIVE_CAP_3};

volatile bool buttonFlag = false;
unsigned long lastPress = 0;

void IRAM_ATTR buttonISR() {
    if (millis() - lastPress > 200) {
        buttonFlag = true;
        lastPress = millis();
    }
}

// ==================== SETUP ====================
void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\n========================================");
    Serial.println("Program 07: GPIO Drive Strength Config");
    Serial.println("========================================\n");
    
    // Konfigurasi LED pins
    pinMode(LED_WEAK, OUTPUT);
    pinMode(LED_NORMAL, OUTPUT);
    pinMode(LED_STRONG, OUTPUT);
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    
    // Set berbagai drive strength
    gpio_set_drive_capability((gpio_num_t)LED_WEAK, GPIO_DRIVE_CAP_0);    // 5mA
    gpio_set_drive_capability((gpio_num_t)LED_NORMAL, GPIO_DRIVE_CAP_2);  // 20mA (default)
    gpio_set_drive_capability((gpio_num_t)LED_STRONG, GPIO_DRIVE_CAP_3);  // 40mA
    
    attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), buttonISR, FALLING);
    
    Serial.println("Drive Strength Levels:");
    Serial.println("  GPIO_DRIVE_CAP_0: ~5mA");
    Serial.println("  GPIO_DRIVE_CAP_1: ~10mA");
    Serial.println("  GPIO_DRIVE_CAP_2: ~20mA (default)");
    Serial.println("  GPIO_DRIVE_CAP_3: ~40mA");
    Serial.println("\nLED Configurations:");
    Serial.printf("  GPIO%d: 5mA (WEAK)\n", LED_WEAK);
    Serial.printf("  GPIO%d: 20mA (NORMAL)\n", LED_NORMAL);
    Serial.printf("  GPIO%d: 40mA (STRONG)\n", LED_STRONG);
    
    // Nyalakan semua LED
    digitalWrite(LED_WEAK, HIGH);
    digitalWrite(LED_NORMAL, HIGH);
    digitalWrite(LED_STRONG, HIGH);
    
    Serial.println("\nSemua LED menyala - perhatikan perbedaan brightness!");
    Serial.println("Tekan button untuk cycle drive strength pada GPIO2\n");
}

// ==================== LOOP ====================
void loop() {
    if (buttonFlag) {
        buttonFlag = false;
        
        currentDrive = (currentDrive + 1) % 4;
        gpio_set_drive_capability((gpio_num_t)LED_WEAK, driveValues[currentDrive]);
        
        Serial.printf("GPIO%d Drive Strength: %s\n", LED_WEAK, driveNames[currentDrive]);
    }
    
    // Demo brightness dengan PWM
    static unsigned long lastToggle = 0;
    if (millis() - lastToggle > 2000) {
        lastToggle = millis();
        
        gpio_drive_cap_t cap;
        gpio_get_drive_capability((gpio_num_t)LED_WEAK, &cap);
        Serial.printf("[%lu] GPIO%d current drive cap: %d\n", millis(), LED_WEAK, cap);
    }
}

/**
 * PENJELASAN DRIVE STRENGTH:
 * 
 * Drive strength menentukan berapa banyak arus yang dapat
 * disource/sink oleh GPIO pin.
 * 
 * ┌──────────────┬────────┬──────────────────┐
 * │     Level    │  Arus  │    Penggunaan    │
 * ├──────────────┼────────┼──────────────────┤
 * │ DRIVE_CAP_0  │  ~5mA  │ LED indicator    │
 * │ DRIVE_CAP_1  │ ~10mA  │ Small LED        │
 * │ DRIVE_CAP_2  │ ~20mA  │ Standard (default)│
 * │ DRIVE_CAP_3  │ ~40mA  │ High power LED   │
 * └──────────────┴────────┴──────────────────┘
 * 
 * PERHATIAN:
 * - Higher drive = more power consumption
 * - Higher drive = more heat generated
 * - Gunakan sesuai kebutuhan actual
 * 
 * WIRING:
 *   GPIO2 → 220Ω → LED1 → GND (5mA weak)
 *   GPIO4 → 220Ω → LED2 → GND (20mA normal)
 *   GPIO5 → 220Ω → LED3 → GND (40mA strong)
 *   GPIO0 → BOOT button
 */
