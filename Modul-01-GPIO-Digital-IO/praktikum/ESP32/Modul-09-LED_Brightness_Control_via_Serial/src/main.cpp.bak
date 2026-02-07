/**
 * @file main.cpp
 * @brief Program 09: LED Brightness Control via Serial
 * 
 * Mengontrol kecerahan LED menggunakan LEDC PWM.
 * Input dari Serial: angka 0-100 untuk persentase brightness.
 */

#include <Arduino.h>

// ==================== KONFIGURASI ====================
#define LED_PIN         2
#define LEDC_FREQ       5000
#define LEDC_RESOLUTION 10      // 10-bit: 0-1023

#define BTN_UP          0       // Button increase
#define BTN_DOWN        4       // Button decrease

// ==================== VARIABEL ====================
int brightness = 50;            // Brightness 0-100%
int pwmValue = 0;

// ==================== FUNCTION PROTOTYPES ====================
void setBrightness(int percent);
void printBrightnessBar();

// ==================== SETUP ====================
void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\n========================================");
    Serial.println("Program 09: LED Brightness Control");
    Serial.println("========================================\n");
    
    // Setup LEDC PWM
    #if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
        ledcAttach(LED_PIN, LEDC_FREQ, LEDC_RESOLUTION);
    #else
        ledcSetup(0, LEDC_FREQ, LEDC_RESOLUTION);
        ledcAttachPin(LED_PIN, 0);
    #endif
    
    pinMode(BTN_UP, INPUT_PULLUP);
    pinMode(BTN_DOWN, INPUT_PULLUP);
    
    Serial.println("Commands:");
    Serial.println("  Type 0-100 : Set brightness percentage");
    Serial.println("  Type 'u'   : Increase by 10%");
    Serial.println("  Type 'd'   : Decrease by 10%");
    Serial.println("  Type 'm'   : Set to max (100%)");
    Serial.println("  Type 'n'   : Set to min (0%)");
    Serial.println("\nOr use buttons:");
    Serial.printf("  GPIO%d: Increase\n", BTN_UP);
    Serial.printf("  GPIO%d: Decrease\n", BTN_DOWN);
    Serial.println();
    
    setBrightness(brightness);
}

// ==================== LOOP ====================
void loop() {
    // Handle serial input
    if (Serial.available() > 0) {
        String input = Serial.readStringUntil('\n');
        input.trim();
        
        if (input.length() > 0) {
            char cmd = input.charAt(0);
            
            if (cmd == 'u' || cmd == 'U') {
                brightness = min(100, brightness + 10);
            } else if (cmd == 'd' || cmd == 'D') {
                brightness = max(0, brightness - 10);
            } else if (cmd == 'm' || cmd == 'M') {
                brightness = 100;
            } else if (cmd == 'n' || cmd == 'N') {
                brightness = 0;
            } else if (isDigit(cmd)) {
                int value = input.toInt();
                brightness = constrain(value, 0, 100);
            }
            
            setBrightness(brightness);
        }
    }
    
    // Handle button input
    static unsigned long lastBtnPress = 0;
    if (millis() - lastBtnPress > 200) {
        if (digitalRead(BTN_UP) == LOW) {
            brightness = min(100, brightness + 5);
            setBrightness(brightness);
            lastBtnPress = millis();
        }
        if (digitalRead(BTN_DOWN) == LOW) {
            brightness = max(0, brightness - 5);
            setBrightness(brightness);
            lastBtnPress = millis();
        }
    }
}

// ==================== FUNCTIONS ====================

void setBrightness(int percent) {
    // Convert percentage to PWM value (0-100 → 0-1023)
    pwmValue = map(percent, 0, 100, 0, 1023);
    
    // Apply gamma correction for perceived linear brightness
    // Human eye perceives brightness logarithmically
    float gamma = 2.2;
    int correctedValue = (int)(pow((float)percent / 100.0, gamma) * 1023);
    
    #if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
        ledcWrite(LED_PIN, correctedValue);
    #else
        ledcWrite(0, correctedValue);
    #endif
    
    Serial.printf("Brightness: %3d%% (PWM: %4d, Gamma: %4d)\n", 
                  percent, pwmValue, correctedValue);
    printBrightnessBar();
}

void printBrightnessBar() {
    Serial.print("[");
    int bars = brightness / 5;  // 20 bars max
    for (int i = 0; i < 20; i++) {
        if (i < bars) {
            Serial.print("█");
        } else {
            Serial.print("░");
        }
    }
    Serial.println("]");
    Serial.println();
}

/**
 * GAMMA CORRECTION:
 * 
 * Human eye perceives brightness logarithmically, not linearly.
 * Without gamma correction:
 *   50% PWM ≠ 50% perceived brightness (too bright!)
 * 
 * With gamma correction (γ = 2.2):
 *   output = input^γ
 *   50% input → ~22% actual PWM → 50% perceived brightness
 * 
 * ┌──────────┬─────────────┬───────────────┐
 * │ Input %  │ Linear PWM  │ Gamma PWM     │
 * ├──────────┼─────────────┼───────────────┤
 * │    10%   │    102      │      4        │
 * │    25%   │    256      │     47        │
 * │    50%   │    512      │    223        │
 * │    75%   │    768      │    564        │
 * │   100%   │   1023      │   1023        │
 * └──────────┴─────────────┴───────────────┘
 * 
 * WIRING:
 *   GPIO2 → 220Ω → LED → GND
 *   GPIO0 → Button Up → GND (internal pull-up)
 *   GPIO4 → Button Down → GND (internal pull-up)
 * 
 * EXPECTED OUTPUT:
 * Brightness:  50% (PWM:  512, Gamma:  223)
 * [██████████░░░░░░░░░░]
 */
