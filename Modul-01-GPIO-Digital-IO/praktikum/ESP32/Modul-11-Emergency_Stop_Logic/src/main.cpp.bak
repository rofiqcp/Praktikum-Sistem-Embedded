/**
 * @file main.cpp
 * @brief Program 11: Emergency Stop Logic (Fail-Safe Pattern)
 * 
 * Implementasi sistem emergency stop dengan:
 * - Button E-STOP yang langsung mematikan output
 * - LED indikator status
 * - Sistem harus di-reset manual setelah E-STOP
 */

#include <Arduino.h>

// ==================== KONFIGURASI ====================
#define ESTOP_PIN       0       // Emergency stop button (NC - Normally Closed)
#define RESET_PIN       4       // Reset button
#define OUTPUT_PIN      5       // Controlled output (motor/actuator simulation)
#define LED_RUN         2       // Green LED - Running
#define LED_STOP        18      // Red LED - Stopped

// ==================== STATE MACHINE ====================
typedef enum {
    STATE_STOPPED,      // System stopped (safe state)
    STATE_RUNNING,      // System running normally
    STATE_ESTOP         // Emergency stop active (latched)
} SystemState_t;

// ==================== VARIABEL ====================
volatile SystemState_t systemState = STATE_STOPPED;
volatile bool estopTriggered = false;
unsigned long runStartTime = 0;

// ==================== FORWARD DECLARATIONS ====================
void startSystem();
void stopSystem();
void resetEstop();

// ==================== ISR ====================
void IRAM_ATTR estopISR() {
    // Immediate action - don't wait for main loop!
    digitalWrite(OUTPUT_PIN, LOW);  // Kill output immediately
    digitalWrite(LED_RUN, LOW);
    digitalWrite(LED_STOP, HIGH);
    estopTriggered = true;
    systemState = STATE_ESTOP;
}

// ==================== SETUP ====================
void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\n========================================");
    Serial.println("Program 11: Emergency Stop System");
    Serial.println("========================================\n");
    
    // Configure pins
    pinMode(ESTOP_PIN, INPUT_PULLUP);
    pinMode(RESET_PIN, INPUT_PULLUP);
    pinMode(OUTPUT_PIN, OUTPUT);
    pinMode(LED_RUN, OUTPUT);
    pinMode(LED_STOP, OUTPUT);
    
    // Initial state: STOPPED (safe)
    digitalWrite(OUTPUT_PIN, LOW);
    digitalWrite(LED_RUN, LOW);
    digitalWrite(LED_STOP, HIGH);
    
    // Attach E-STOP interrupt (highest priority response)
    attachInterrupt(digitalPinToInterrupt(ESTOP_PIN), estopISR, FALLING);
    
    Serial.println("Emergency Stop System Initialized");
    Serial.println("================================");
    Serial.printf("  E-STOP Button : GPIO%d (press to stop)\n", ESTOP_PIN);
    Serial.printf("  Reset Button  : GPIO%d (press to reset)\n", RESET_PIN);
    Serial.printf("  Output Control: GPIO%d\n", OUTPUT_PIN);
    Serial.printf("  LED Running   : GPIO%d (Green)\n", LED_RUN);
    Serial.printf("  LED Stopped   : GPIO%d (Red)\n", LED_STOP);
    Serial.println();
    Serial.println("Commands:");
    Serial.println("  's' - Start system");
    Serial.println("  'x' - Stop system (normal)");
    Serial.println("  'r' - Reset after E-STOP");
    Serial.println();
    Serial.println("Current State: STOPPED");
    Serial.println();
}

// ==================== LOOP ====================
void loop() {
    // Handle serial commands
    if (Serial.available() > 0) {
        char cmd = Serial.read();
        
        switch (cmd) {
            case 's':
            case 'S':
                if (systemState == STATE_STOPPED) {
                    startSystem();
                } else if (systemState == STATE_ESTOP) {
                    Serial.println("!!! Cannot start - E-STOP active! Reset first.");
                } else {
                    Serial.println("System already running");
                }
                break;
                
            case 'x':
            case 'X':
                if (systemState == STATE_RUNNING) {
                    stopSystem();
                }
                break;
                
            case 'r':
            case 'R':
                if (systemState == STATE_ESTOP) {
                    resetEstop();
                } else {
                    Serial.println("No E-STOP to reset");
                }
                break;
        }
    }
    
    // Handle physical reset button
    static unsigned long lastReset = 0;
    if (digitalRead(RESET_PIN) == LOW && millis() - lastReset > 500) {
        lastReset = millis();
        if (systemState == STATE_ESTOP) {
            resetEstop();
        }
    }
    
    // Handle E-STOP event
    if (estopTriggered) {
        estopTriggered = false;
        unsigned long runDuration = millis() - runStartTime;
        
        Serial.println("\n!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
        Serial.println("!!! EMERGENCY STOP ACTIVATED !!!");
        Serial.println("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
        Serial.printf("System was running for %lu ms\n", runDuration);
        Serial.println("Output KILLED immediately");
        Serial.println("Press 'r' or Reset button to clear\n");
    }
    
    // Running indicator (blink output to show activity)
    if (systemState == STATE_RUNNING) {
        static unsigned long lastBlink = 0;
        if (millis() - lastBlink > 500) {
            lastBlink = millis();
            static bool blink = false;
            blink = !blink;
            
            // Simulate some output activity
            Serial.printf("[RUNNING] Uptime: %lu ms\n", millis() - runStartTime);
        }
    }
    
    // E-STOP state indicator (fast blink red LED)
    if (systemState == STATE_ESTOP) {
        static unsigned long lastBlink = 0;
        if (millis() - lastBlink > 200) {
            lastBlink = millis();
            digitalWrite(LED_STOP, !digitalRead(LED_STOP));
        }
    }
}

// ==================== FUNCTIONS ====================

void startSystem() {
    systemState = STATE_RUNNING;
    runStartTime = millis();
    
    digitalWrite(OUTPUT_PIN, HIGH);
    digitalWrite(LED_RUN, HIGH);
    digitalWrite(LED_STOP, LOW);
    
    Serial.println("\n>>> SYSTEM STARTED");
    Serial.println("Output enabled, monitoring for E-STOP...\n");
}

void stopSystem() {
    systemState = STATE_STOPPED;
    
    digitalWrite(OUTPUT_PIN, LOW);
    digitalWrite(LED_RUN, LOW);
    digitalWrite(LED_STOP, HIGH);
    
    unsigned long runDuration = millis() - runStartTime;
    Serial.printf("\n>>> SYSTEM STOPPED (normal)\n");
    Serial.printf("Run duration: %lu ms\n\n", runDuration);
}

void resetEstop() {
    systemState = STATE_STOPPED;
    estopTriggered = false;
    
    digitalWrite(LED_STOP, HIGH);  // Solid red (stopped, not E-STOP)
    
    Serial.println("\n>>> E-STOP RESET");
    Serial.println("System in STOPPED state");
    Serial.println("Press 's' to start again\n");
}

/**
 * STATE DIAGRAM:
 * 
 *                    ┌─────────┐
 *          ┌────────→│ STOPPED │←────────┐
 *          │  reset  └────┬────┘  stop   │
 *          │              │ start        │
 *          │              ↓              │
 *     ┌────┴────┐    ┌─────────┐         │
 *     │ E-STOP  │←───│ RUNNING │─────────┘
 *     └─────────┘    └─────────┘
 *        ↑ E-STOP button (interrupt)
 * 
 * FAIL-SAFE PRINCIPLES:
 * 1. E-STOP harus menggunakan interrupt (immediate response)
 * 2. Output di-kill PERTAMA, baru proses lain
 * 3. Sistem harus di-reset manual (no auto-restart)
 * 4. E-STOP button idealnya NC (Normally Closed)
 *    - Wire break = E-STOP triggered (fail-safe)
 * 
 * WIRING:
 *   GPIO0  (E-STOP) → Push Button (NC) → GND
 *   GPIO4  (Reset)  → Push Button → GND
 *   GPIO5  (Output) → Relay/Motor driver
 *   GPIO2  (Green)  → 220Ω → LED → GND
 *   GPIO18 (Red)    → 220Ω → LED → GND
 */
