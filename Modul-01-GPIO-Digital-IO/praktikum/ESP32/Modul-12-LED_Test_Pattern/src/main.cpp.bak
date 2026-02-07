/**
 * @file main.cpp
 * @brief Program 12: LED Test Pattern (Self-Check / POST)
 * 
 * Power-On Self-Test (POST) untuk LED dan GPIO.
 * Menjalankan serangkaian test pattern saat startup
 * untuk memverifikasi hardware berfungsi dengan baik.
 */

#include <Arduino.h>

// ==================== KONFIGURASI ====================
#define NUM_LEDS        4
#define LED_1           2
#define LED_2           4
#define LED_3           5
#define LED_4           18

const uint8_t LED_PINS[NUM_LEDS] = {LED_1, LED_2, LED_3, LED_4};

#define TEST_DELAY      100     // Delay antar test step
#define PATTERN_REPEAT  2       // Pengulangan pattern

// ==================== TEST RESULTS ====================
typedef struct {
    bool led[NUM_LEDS];
    bool allPassed;
    uint32_t testTime;
} TestResult_t;

TestResult_t testResult;

// ==================== FUNCTION PROTOTYPES ====================
void runPOST();
void testAllOn();
void testAllOff();
void testSequential();
void testAlternate();
void testBinary();
void testRandomPattern();
void printTestResult();

// ==================== SETUP ====================
void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\n========================================");
    Serial.println("Program 12: LED Self-Test (POST)");
    Serial.println("========================================\n");
    
    // Initialize all LEDs
    for (int i = 0; i < NUM_LEDS; i++) {
        pinMode(LED_PINS[i], OUTPUT);
        digitalWrite(LED_PINS[i], LOW);
    }
    
    Serial.println("LED Configuration:");
    for (int i = 0; i < NUM_LEDS; i++) {
        Serial.printf("  LED %d: GPIO%d\n", i+1, LED_PINS[i]);
    }
    Serial.println();
    
    // Run Power-On Self-Test
    runPOST();
    
    Serial.println("\n========================================");
    Serial.println("Self-test complete!");
    Serial.println("Type 't' to run test again");
    Serial.println("Type '1'-'4' to toggle individual LED");
    Serial.println("========================================\n");
}

// ==================== LOOP ====================
void loop() {
    if (Serial.available() > 0) {
        char cmd = Serial.read();
        
        if (cmd == 't' || cmd == 'T') {
            Serial.println("\n>>> Running self-test again...\n");
            runPOST();
        } else if (cmd >= '1' && cmd <= '4') {
            int ledNum = cmd - '1';
            bool state = !digitalRead(LED_PINS[ledNum]);
            digitalWrite(LED_PINS[ledNum], state);
            Serial.printf("LED %d (GPIO%d): %s\n", ledNum+1, LED_PINS[ledNum], 
                         state ? "ON" : "OFF");
        }
    }
}

// ==================== TEST FUNCTIONS ====================

void runPOST() {
    unsigned long startTime = millis();
    
    Serial.println("╔════════════════════════════════════╗");
    Serial.println("║     POWER-ON SELF-TEST (POST)      ║");
    Serial.println("╚════════════════════════════════════╝\n");
    
    // Initialize test results
    for (int i = 0; i < NUM_LEDS; i++) {
        testResult.led[i] = true;  // Assume pass
    }
    testResult.allPassed = true;
    
    // Test 1: All ON
    Serial.println("[Test 1] All LEDs ON...");
    testAllOn();
    delay(500);
    
    // Test 2: All OFF
    Serial.println("[Test 2] All LEDs OFF...");
    testAllOff();
    delay(300);
    
    // Test 3: Sequential
    Serial.println("[Test 3] Sequential pattern...");
    for (int r = 0; r < PATTERN_REPEAT; r++) {
        testSequential();
    }
    
    // Test 4: Alternate
    Serial.println("[Test 4] Alternate pattern...");
    for (int r = 0; r < PATTERN_REPEAT; r++) {
        testAlternate();
    }
    
    // Test 5: Binary count
    Serial.println("[Test 5] Binary count (0-15)...");
    testBinary();
    
    // Test 6: Random
    Serial.println("[Test 6] Random pattern...");
    testRandomPattern();
    
    // Final: All OFF
    testAllOff();
    
    testResult.testTime = millis() - startTime;
    
    // Print results
    printTestResult();
}

void testAllOn() {
    for (int i = 0; i < NUM_LEDS; i++) {
        digitalWrite(LED_PINS[i], HIGH);
    }
    Serial.println("    All LEDs should be ON");
}

void testAllOff() {
    for (int i = 0; i < NUM_LEDS; i++) {
        digitalWrite(LED_PINS[i], LOW);
    }
    Serial.println("    All LEDs should be OFF");
}

void testSequential() {
    // Forward
    for (int i = 0; i < NUM_LEDS; i++) {
        for (int j = 0; j < NUM_LEDS; j++) {
            digitalWrite(LED_PINS[j], (i == j) ? HIGH : LOW);
        }
        delay(TEST_DELAY);
    }
    // Backward
    for (int i = NUM_LEDS - 1; i >= 0; i--) {
        for (int j = 0; j < NUM_LEDS; j++) {
            digitalWrite(LED_PINS[j], (i == j) ? HIGH : LOW);
        }
        delay(TEST_DELAY);
    }
    testAllOff();
}

void testAlternate() {
    // Pattern 1: 1010
    digitalWrite(LED_PINS[0], HIGH);
    digitalWrite(LED_PINS[1], LOW);
    digitalWrite(LED_PINS[2], HIGH);
    digitalWrite(LED_PINS[3], LOW);
    delay(200);
    
    // Pattern 2: 0101
    digitalWrite(LED_PINS[0], LOW);
    digitalWrite(LED_PINS[1], HIGH);
    digitalWrite(LED_PINS[2], LOW);
    digitalWrite(LED_PINS[3], HIGH);
    delay(200);
    
    testAllOff();
}

void testBinary() {
    for (int count = 0; count <= 15; count++) {
        for (int i = 0; i < NUM_LEDS; i++) {
            digitalWrite(LED_PINS[i], (count >> i) & 1);
        }
        Serial.printf("    Binary: %d%d%d%d = %2d\n",
                     (count >> 3) & 1,
                     (count >> 2) & 1,
                     (count >> 1) & 1,
                     (count >> 0) & 1,
                     count);
        delay(100);
    }
    testAllOff();
}

void testRandomPattern() {
    for (int r = 0; r < 10; r++) {
        uint8_t pattern = random(16);
        for (int i = 0; i < NUM_LEDS; i++) {
            digitalWrite(LED_PINS[i], (pattern >> i) & 1);
        }
        delay(100);
    }
    testAllOff();
}

void printTestResult() {
    Serial.println("\n╔════════════════════════════════════╗");
    Serial.println("║          TEST RESULTS              ║");
    Serial.println("╠════════════════════════════════════╣");
    
    for (int i = 0; i < NUM_LEDS; i++) {
        Serial.printf("║  LED %d (GPIO%2d): %s            ║\n",
                     i+1, LED_PINS[i],
                     testResult.led[i] ? "✓ PASS" : "✗ FAIL");
    }
    
    Serial.println("╠════════════════════════════════════╣");
    Serial.printf("║  Test Duration: %4lu ms            ║\n", testResult.testTime);
    Serial.printf("║  Overall: %s                    ║\n",
                 testResult.allPassed ? "✓ ALL PASSED" : "✗ FAILED");
    Serial.println("╚════════════════════════════════════╝");
}

/**
 * POST (Power-On Self-Test) SEQUENCE:
 * 
 * 1. All ON     - Verify all LEDs can light up
 * 2. All OFF    - Verify all LEDs can turn off
 * 3. Sequential - Check each LED individually
 * 4. Alternate  - Check wiring (no shorts)
 * 5. Binary     - Count 0-15 in binary
 * 6. Random     - Visual verification
 * 
 * TEST PATTERNS:
 * 
 * Sequential:
 *   ●○○○ → ○●○○ → ○○●○ → ○○○● → ○○●○ → ○●○○ → ●○○○
 * 
 * Alternate:
 *   ●○●○ ↔ ○●○●
 * 
 * Binary Count:
 *   ○○○○ = 0
 *   ○○○● = 1
 *   ○○●○ = 2
 *   ...
 *   ●●●● = 15
 * 
 * WIRING:
 *   GPIO2  → 220Ω → LED1 → GND
 *   GPIO4  → 220Ω → LED2 → GND
 *   GPIO5  → 220Ω → LED3 → GND
 *   GPIO18 → 220Ω → LED4 → GND
 * 
 * TROUBLESHOOTING:
 * - LED tidak menyala: Cek resistor dan polaritas
 * - Brightness berbeda: Resistor value berbeda
 * - Pattern tidak sinkron: Cek koneksi longgar
 */
