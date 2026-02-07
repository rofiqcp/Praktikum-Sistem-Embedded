/*
 * STM32 Event-Driven Communication System
 * Modul 13: Network Communication
 * 
 * Demonstrates:
 * - Event-driven architecture
 * - Interrupt-based detection
 * - Event prioritization
 * - Notification system
 */

#include <Arduino.h>
#include <ArduinoJson.h>

// Serial configuration
#define ESP32_SERIAL Serial1
#define ESP32_BAUD 115200
#define DEBUG_SERIAL Serial

// GPIO pins
const int LED_PIN = PC13;
const int BUTTON1_PIN = PA0;
const int BUTTON2_PIN = PA1;
const int MOTION_PIN = PA2;  // PIR sensor (simulated)
const int DOOR_PIN = PA3;    // Door sensor (simulated)
const int ALARM_PIN = PA4;   // Buzzer

// Event types
enum EventType {
    EVT_NONE = 0,
    EVT_BUTTON_PRESS,
    EVT_BUTTON_RELEASE,
    EVT_MOTION_DETECTED,
    EVT_MOTION_ENDED,
    EVT_DOOR_OPENED,
    EVT_DOOR_CLOSED,
    EVT_ALARM_TRIGGERED,
    EVT_ALARM_CLEARED,
    EVT_SENSOR_ALERT,
    EVT_HEARTBEAT,
    EVT_SYSTEM
};

// Event priority
enum EventPriority {
    PRIO_LOW = 0,
    PRIO_NORMAL = 1,
    PRIO_HIGH = 2,
    PRIO_CRITICAL = 3
};

// Event structure
struct Event {
    EventType type;
    EventPriority priority;
    unsigned long timestamp;
    int source;      // GPIO pin or source ID
    int value;       // Event-specific value
    char details[32];
};

// Event queue
const int EVENT_QUEUE_SIZE = 20;
Event eventQueue[EVENT_QUEUE_SIZE];
int eventQueueHead = 0;
int eventQueueTail = 0;
int eventQueueCount = 0;

// System state
struct SystemState {
    bool armed;              // Security system armed
    bool alarmActive;
    bool motionDetected;
    bool doorOpen;
    unsigned long lastMotion;
    unsigned long eventCount;
    unsigned long criticalCount;
} state = {false, false, false, false, 0, 0, 0};

// Debounce timers
volatile unsigned long lastButton1 = 0;
volatile unsigned long lastButton2 = 0;
const unsigned long DEBOUNCE_TIME = 50;

// Buffer
String rxBuffer = "";

// Forward declarations
void queueEvent(EventType type, EventPriority prio, int source, int value, const char* details);
void processEvent(Event& evt);

// ISR for Button 1
void button1ISR() {
    if (millis() - lastButton1 > DEBOUNCE_TIME) {
        bool pressed = !digitalRead(BUTTON1_PIN);
        queueEvent(pressed ? EVT_BUTTON_PRESS : EVT_BUTTON_RELEASE, 
                   PRIO_NORMAL, 1, pressed, "Button 1");
        lastButton1 = millis();
    }
}

// ISR for Button 2
void button2ISR() {
    if (millis() - lastButton2 > DEBOUNCE_TIME) {
        bool pressed = !digitalRead(BUTTON2_PIN);
        queueEvent(pressed ? EVT_BUTTON_PRESS : EVT_BUTTON_RELEASE, 
                   PRIO_NORMAL, 2, pressed, "Button 2");
        lastButton2 = millis();
    }
}

void queueEvent(EventType type, EventPriority prio, int source, int value, const char* details) {
    if (eventQueueCount >= EVENT_QUEUE_SIZE) {
        // Queue full - drop lowest priority or oldest
        return;
    }
    
    Event evt;
    evt.type = type;
    evt.priority = prio;
    evt.timestamp = millis();
    evt.source = source;
    evt.value = value;
    strncpy(evt.details, details, sizeof(evt.details) - 1);
    evt.details[sizeof(evt.details) - 1] = '\0';
    
    // Find insert position based on priority (higher priority first)
    int insertPos = eventQueueTail;
    
    // For simplicity, just append (FIFO within same priority)
    eventQueue[eventQueueTail] = evt;
    eventQueueTail = (eventQueueTail + 1) % EVENT_QUEUE_SIZE;
    eventQueueCount++;
    
    state.eventCount++;
    if (prio == PRIO_CRITICAL) {
        state.criticalCount++;
    }
}

Event* dequeueEvent() {
    if (eventQueueCount == 0) {
        return nullptr;
    }
    
    Event* evt = &eventQueue[eventQueueHead];
    eventQueueHead = (eventQueueHead + 1) % EVENT_QUEUE_SIZE;
    eventQueueCount--;
    
    return evt;
}

const char* eventTypeToString(EventType type) {
    switch (type) {
        case EVT_BUTTON_PRESS: return "button_press";
        case EVT_BUTTON_RELEASE: return "button_release";
        case EVT_MOTION_DETECTED: return "motion_detected";
        case EVT_MOTION_ENDED: return "motion_ended";
        case EVT_DOOR_OPENED: return "door_opened";
        case EVT_DOOR_CLOSED: return "door_closed";
        case EVT_ALARM_TRIGGERED: return "alarm_triggered";
        case EVT_ALARM_CLEARED: return "alarm_cleared";
        case EVT_SENSOR_ALERT: return "sensor_alert";
        case EVT_HEARTBEAT: return "heartbeat";
        case EVT_SYSTEM: return "system";
        default: return "unknown";
    }
}

const char* priorityToString(EventPriority prio) {
    switch (prio) {
        case PRIO_LOW: return "low";
        case PRIO_NORMAL: return "normal";
        case PRIO_HIGH: return "high";
        case PRIO_CRITICAL: return "critical";
        default: return "unknown";
    }
}

void sendEvent(Event& evt) {
    StaticJsonDocument<256> doc;
    
    doc["type"] = "event";
    doc["event"] = eventTypeToString(evt.type);
    doc["priority"] = priorityToString(evt.priority);
    doc["source"] = evt.source;
    doc["value"] = evt.value;
    doc["details"] = evt.details;
    doc["timestamp"] = evt.timestamp;
    doc["seq"] = state.eventCount;
    
    String output;
    serializeJson(doc, output);
    ESP32_SERIAL.println(output);
    
    DEBUG_SERIAL.printf("[EVT] %s (prio=%s): %s\n", 
                       eventTypeToString(evt.type),
                       priorityToString(evt.priority),
                       evt.details);
}

void processEvent(Event& evt) {
    // Send to ESP32
    sendEvent(evt);
    
    // Local processing based on event type
    switch (evt.type) {
        case EVT_BUTTON_PRESS:
            if (evt.source == 1) {
                // Button 1: Toggle armed state
                state.armed = !state.armed;
                queueEvent(EVT_SYSTEM, PRIO_NORMAL, 0, state.armed, 
                          state.armed ? "System armed" : "System disarmed");
            } else if (evt.source == 2) {
                // Button 2: Clear alarm
                if (state.alarmActive) {
                    state.alarmActive = false;
                    noTone(ALARM_PIN);
                    queueEvent(EVT_ALARM_CLEARED, PRIO_HIGH, 0, 0, "Manual clear");
                }
            }
            break;
            
        case EVT_MOTION_DETECTED:
            if (state.armed && !state.alarmActive) {
                // Trigger alarm
                state.alarmActive = true;
                tone(ALARM_PIN, 2000);
                queueEvent(EVT_ALARM_TRIGGERED, PRIO_CRITICAL, MOTION_PIN, 1, "Motion while armed");
            }
            break;
            
        case EVT_DOOR_OPENED:
            if (state.armed && !state.alarmActive) {
                state.alarmActive = true;
                tone(ALARM_PIN, 1500);
                queueEvent(EVT_ALARM_TRIGGERED, PRIO_CRITICAL, DOOR_PIN, 1, "Door opened while armed");
            }
            break;
            
        case EVT_ALARM_TRIGGERED:
            // Flash LED rapidly
            for (int i = 0; i < 5; i++) {
                digitalWrite(LED_PIN, LOW);
                delay(100);
                digitalWrite(LED_PIN, HIGH);
                delay(100);
            }
            break;
            
        default:
            break;
    }
}

void checkSensors() {
    static bool lastMotion = false;
    static bool lastDoor = false;
    
    // Check motion sensor (simulated with digital input)
    bool motion = !digitalRead(MOTION_PIN);  // Active LOW
    if (motion != lastMotion) {
        if (motion) {
            state.motionDetected = true;
            state.lastMotion = millis();
            queueEvent(EVT_MOTION_DETECTED, state.armed ? PRIO_HIGH : PRIO_NORMAL, 
                      MOTION_PIN, 1, "Motion detected");
        } else {
            state.motionDetected = false;
            queueEvent(EVT_MOTION_ENDED, PRIO_LOW, MOTION_PIN, 0, "Motion ended");
        }
        lastMotion = motion;
    }
    
    // Check door sensor (simulated)
    bool door = !digitalRead(DOOR_PIN);
    if (door != lastDoor) {
        if (door) {
            state.doorOpen = true;
            queueEvent(EVT_DOOR_OPENED, state.armed ? PRIO_CRITICAL : PRIO_NORMAL,
                      DOOR_PIN, 1, "Door opened");
        } else {
            state.doorOpen = false;
            queueEvent(EVT_DOOR_CLOSED, PRIO_LOW, DOOR_PIN, 0, "Door closed");
        }
        lastDoor = door;
    }
}

void sendStatus() {
    StaticJsonDocument<256> doc;
    
    doc["type"] = "status";
    doc["armed"] = state.armed;
    doc["alarm_active"] = state.alarmActive;
    doc["motion"] = state.motionDetected;
    doc["door_open"] = state.doorOpen;
    doc["total_events"] = state.eventCount;
    doc["critical_events"] = state.criticalCount;
    doc["queue_size"] = eventQueueCount;
    doc["uptime"] = millis() / 1000;
    
    String output;
    serializeJson(doc, output);
    ESP32_SERIAL.println(output);
}

void processCommand(const String& message) {
    DEBUG_SERIAL.printf("[RX] %s\n", message.c_str());
    
    StaticJsonDocument<128> doc;
    if (deserializeJson(doc, message)) return;
    
    String cmd = doc["cmd"] | "";
    
    if (cmd == "arm") {
        state.armed = true;
        queueEvent(EVT_SYSTEM, PRIO_NORMAL, 0, 1, "System armed remotely");
        
    } else if (cmd == "disarm") {
        state.armed = false;
        if (state.alarmActive) {
            state.alarmActive = false;
            noTone(ALARM_PIN);
        }
        queueEvent(EVT_SYSTEM, PRIO_NORMAL, 0, 0, "System disarmed remotely");
        
    } else if (cmd == "clear_alarm") {
        state.alarmActive = false;
        noTone(ALARM_PIN);
        queueEvent(EVT_ALARM_CLEARED, PRIO_HIGH, 0, 0, "Remote clear");
        
    } else if (cmd == "test_alarm") {
        tone(ALARM_PIN, 1000, 500);
        queueEvent(EVT_SYSTEM, PRIO_LOW, 0, 0, "Alarm test");
        
    } else if (cmd == "get_status") {
        sendStatus();
        
    } else if (cmd == "simulate_motion") {
        queueEvent(EVT_MOTION_DETECTED, state.armed ? PRIO_HIGH : PRIO_NORMAL,
                  MOTION_PIN, 1, "Simulated motion");
                  
    } else if (cmd == "simulate_door") {
        queueEvent(EVT_DOOR_OPENED, state.armed ? PRIO_CRITICAL : PRIO_NORMAL,
                  DOOR_PIN, 1, "Simulated door open");
    }
}

void printStatus() {
    DEBUG_SERIAL.println("\n╔══════════════════════════════════════╗");
    DEBUG_SERIAL.println("║        Event System Status           ║");
    DEBUG_SERIAL.println("╠══════════════════════════════════════╣");
    DEBUG_SERIAL.printf("║ Armed          : %-19s ║\n", state.armed ? "YES" : "NO");
    DEBUG_SERIAL.printf("║ Alarm Active   : %-19s ║\n", state.alarmActive ? "YES" : "NO");
    DEBUG_SERIAL.printf("║ Motion         : %-19s ║\n", state.motionDetected ? "Detected" : "Clear");
    DEBUG_SERIAL.printf("║ Door           : %-19s ║\n", state.doorOpen ? "Open" : "Closed");
    DEBUG_SERIAL.printf("║ Total Events   : %-19lu ║\n", state.eventCount);
    DEBUG_SERIAL.printf("║ Critical Events: %-19lu ║\n", state.criticalCount);
    DEBUG_SERIAL.printf("║ Queue Size     : %-19d ║\n", eventQueueCount);
    DEBUG_SERIAL.println("╚══════════════════════════════════════╝\n");
}

void setup() {
    // Initialize outputs
    pinMode(LED_PIN, OUTPUT);
    pinMode(ALARM_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH);
    
    // Initialize inputs
    pinMode(BUTTON1_PIN, INPUT_PULLUP);
    pinMode(BUTTON2_PIN, INPUT_PULLUP);
    pinMode(MOTION_PIN, INPUT_PULLUP);
    pinMode(DOOR_PIN, INPUT_PULLUP);
    
    // Attach interrupts
    attachInterrupt(digitalPinToInterrupt(BUTTON1_PIN), button1ISR, CHANGE);
    attachInterrupt(digitalPinToInterrupt(BUTTON2_PIN), button2ISR, CHANGE);
    
    // Initialize serial
    Serial.begin(115200);
    ESP32_SERIAL.begin(ESP32_BAUD);
    
    delay(2000);
    
    DEBUG_SERIAL.println("\n╔══════════════════════════════════════╗");
    DEBUG_SERIAL.println("║   STM32 Event-Driven System          ║");
    DEBUG_SERIAL.println("║   Modul 13: Network Communication    ║");
    DEBUG_SERIAL.println("╚══════════════════════════════════════╝\n");
    
    DEBUG_SERIAL.println("[Button 1] Toggle arm/disarm");
    DEBUG_SERIAL.println("[Button 2] Clear alarm");
    DEBUG_SERIAL.println("[Commands] status, arm, disarm, test, help\n");
    
    // Startup event
    queueEvent(EVT_SYSTEM, PRIO_NORMAL, 0, 0, "Event system started");
}

void loop() {
    // Check sensors
    static unsigned long lastSensorCheck = 0;
    if (millis() - lastSensorCheck >= 100) {
        checkSensors();
        lastSensorCheck = millis();
    }
    
    // Process event queue
    Event* evt = dequeueEvent();
    if (evt != nullptr) {
        processEvent(*evt);
    }
    
    // Send heartbeat every 30 seconds
    static unsigned long lastHeartbeat = 0;
    if (millis() - lastHeartbeat >= 30000) {
        queueEvent(EVT_HEARTBEAT, PRIO_LOW, 0, state.eventCount, "Heartbeat");
        lastHeartbeat = millis();
    }
    
    // Read from ESP32
    while (ESP32_SERIAL.available()) {
        char c = ESP32_SERIAL.read();
        
        if (c == '\n') {
            if (rxBuffer.length() > 0) {
                processCommand(rxBuffer);
                rxBuffer = "";
            }
        } else if (c != '\r') {
            rxBuffer += c;
        }
    }
    
    // Debug serial
    if (DEBUG_SERIAL.available()) {
        String cmd = DEBUG_SERIAL.readStringUntil('\n');
        cmd.trim();
        
        if (cmd == "status") {
            printStatus();
        } else if (cmd == "arm") {
            state.armed = true;
            DEBUG_SERIAL.println("[System] Armed");
        } else if (cmd == "disarm") {
            state.armed = false;
            state.alarmActive = false;
            noTone(ALARM_PIN);
            DEBUG_SERIAL.println("[System] Disarmed");
        } else if (cmd == "test") {
            tone(ALARM_PIN, 1000, 500);
            DEBUG_SERIAL.println("[System] Alarm test");
        }
    }
    
    // LED indicator
    static unsigned long lastLedToggle = 0;
    int blinkInterval = state.alarmActive ? 100 : (state.armed ? 500 : 1000);
    if (millis() - lastLedToggle >= blinkInterval) {
        digitalWrite(LED_PIN, !digitalRead(LED_PIN));
        lastLedToggle = millis();
    }
    
    delay(1);
}
