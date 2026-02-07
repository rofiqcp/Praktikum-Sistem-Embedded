/**
 * @file main.cpp
 * @brief Program 08: DIP Switch Reader (Multiple Inputs)
 * 
 * Membaca 4-bit DIP switch dan menampilkan nilai dalam
 * format binary, decimal, dan hexadecimal.
 */

#include <Arduino.h>

// ==================== KONFIGURASI ====================
#define NUM_SWITCHES    4
#define SW1_PIN         32      // Bit 0 (LSB)
#define SW2_PIN         33      // Bit 1
#define SW3_PIN         25      // Bit 2
#define SW4_PIN         26      // Bit 3 (MSB)

#define LED_PIN         2       // Status LED

const uint8_t SWITCH_PINS[NUM_SWITCHES] = {SW1_PIN, SW2_PIN, SW3_PIN, SW4_PIN};

// ==================== VARIABEL ====================
uint8_t lastValue = 0xFF;   // Nilai sebelumnya (init berbeda)
uint8_t currentValue = 0;

// ==================== FUNCTION PROTOTYPES ====================
uint8_t readDipSwitch();
void printBinary(uint8_t value);

// ==================== SETUP ====================
void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\n========================================");
    Serial.println("Program 08: DIP Switch Reader");
    Serial.println("========================================\n");
    
    // Konfigurasi pin sebagai input dengan pull-up
    for (int i = 0; i < NUM_SWITCHES; i++) {
        pinMode(SWITCH_PINS[i], INPUT_PULLUP);
    }
    pinMode(LED_PIN, OUTPUT);
    
    Serial.println("DIP Switch Configuration:");
    Serial.printf("  SW1 (Bit 0): GPIO%d\n", SW1_PIN);
    Serial.printf("  SW2 (Bit 1): GPIO%d\n", SW2_PIN);
    Serial.printf("  SW3 (Bit 2): GPIO%d\n", SW3_PIN);
    Serial.printf("  SW4 (Bit 3): GPIO%d\n", SW4_PIN);
    Serial.println("\nSwitch ON = 0 (grounded)");
    Serial.println("Switch OFF = 1 (pull-up)\n");
    Serial.println("Monitoring... (output on change)\n");
}

// ==================== LOOP ====================
void loop() {
    currentValue = readDipSwitch();
    
    // Hanya tampilkan jika ada perubahan
    if (currentValue != lastValue) {
        Serial.print("DIP Switch: ");
        printBinary(currentValue);
        Serial.printf("  Dec: %2d  Hex: 0x%X\n", currentValue, currentValue);
        
        // LED indicator berdasarkan nilai
        if (currentValue > 0) {
            digitalWrite(LED_PIN, HIGH);
        } else {
            digitalWrite(LED_PIN, LOW);
        }
        
        // Interpretasi nilai
        switch (currentValue) {
            case 0:
                Serial.println("  → All OFF");
                break;
            case 15:
                Serial.println("  → All ON (Max value)");
                break;
            default:
                Serial.printf("  → Mode %d selected\n", currentValue);
                break;
        }
        Serial.println();
        
        lastValue = currentValue;
    }
    
    delay(50);  // Small delay untuk debounce
}

// ==================== FUNCTIONS ====================

uint8_t readDipSwitch() {
    uint8_t value = 0;
    
    for (int i = 0; i < NUM_SWITCHES; i++) {
        // Baca pin (inverted karena pull-up: ON=LOW=1, OFF=HIGH=0)
        if (digitalRead(SWITCH_PINS[i]) == LOW) {
            value |= (1 << i);  // Set bit ke-i
        }
    }
    
    return value;
}

void printBinary(uint8_t value) {
    Serial.print("B");
    for (int i = NUM_SWITCHES - 1; i >= 0; i--) {
        Serial.print((value >> i) & 1);
    }
}

/**
 * TRUTH TABLE DIP SWITCH 4-BIT:
 * 
 * ┌─────┬─────┬─────┬─────┬─────────┬─────┐
 * │ SW4 │ SW3 │ SW2 │ SW1 │ Binary  │ Dec │
 * ├─────┼─────┼─────┼─────┼─────────┼─────┤
 * │ OFF │ OFF │ OFF │ OFF │  B0000  │  0  │
 * │ OFF │ OFF │ OFF │ ON  │  B0001  │  1  │
 * │ OFF │ OFF │ ON  │ OFF │  B0010  │  2  │
 * │ OFF │ OFF │ ON  │ ON  │  B0011  │  3  │
 * │ ... │ ... │ ... │ ... │   ...   │ ... │
 * │ ON  │ ON  │ ON  │ ON  │  B1111  │ 15  │
 * └─────┴─────┴─────┴─────┴─────────┴─────┘
 * 
 * WIRING (dengan Pull-up internal):
 * 
 *   ESP32 DevKitC
 *   ┌─────────────┐      ┌─────────────┐
 *   │       GPIO32├──────┤ SW1 ├───┐
 *   │       GPIO33├──────┤ SW2 ├───┤
 *   │       GPIO25├──────┤ SW3 ├───┤
 *   │       GPIO26├──────┤ SW4 ├───┤
 *   │             │      └─────────────┘   │
 *   │         GND ├───────────────────────┘
 *   └─────────────┘
 * 
 *   Atau gunakan 4 push button sebagai simulasi
 * 
 * EXPECTED OUTPUT:
 * DIP Switch: B0001  Dec:  1  Hex: 0x1
 *   → Mode 1 selected
 * 
 * DIP Switch: B1010  Dec: 10  Hex: 0xA
 *   → Mode 10 selected
 */
