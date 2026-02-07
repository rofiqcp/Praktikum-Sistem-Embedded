/*
 * STM32 Serial Basic Communication
 * Modul 13: Network Communication
 * 
 * Demonstrates:
 * - UART configuration
 * - Basic serial communication
 * - Command processing
 * - JSON message format
 */

#include <Arduino.h>

// Serial ports
// Serial1: PA9 (TX), PA10 (RX) - for ESP32 communication
// Serial: USB (for debugging)

// GPIO pins
const int LED_PIN = PC13;  // Built-in LED (active LOW)
const int LED2_PIN = PA0;  // External LED

// Configuration
#define ESP32_SERIAL Serial1
#define ESP32_BAUD 115200
#define DEBUG_SERIAL Serial
#define DEBUG_BAUD 115200

// System state
struct SystemState {
    bool led1;
    bool led2;
    unsigned long messagesSent;
    unsigned long messagesReceived;
    unsigned long errors;
    unsigned long uptime;
} state = {false, false, 0, 0, 0, 0};

// Buffer for receiving data
String receiveBuffer = "";

void sendToESP32(const String& message) {
    ESP32_SERIAL.println(message);
    state.messagesSent++;
    DEBUG_SERIAL.printf("[TX] %s\n", message.c_str());
}

void sendJSON(const char* type, const char* key, const char* value) {
    String json = "{\"type\":\"";
    json += type;
    json += "\",\"";
    json += key;
    json += "\":\"";
    json += value;
    json += "\"}";
    sendToESP32(json);
}

void sendStatus() {
    String json = "{\"type\":\"status\",";
    json += "\"led1\":" + String(state.led1 ? "true" : "false") + ",";
    json += "\"led2\":" + String(state.led2 ? "true" : "false") + ",";
    json += "\"sent\":" + String(state.messagesSent) + ",";
    json += "\"received\":" + String(state.messagesReceived) + ",";
    json += "\"uptime\":" + String(millis() / 1000) + "}";
    sendToESP32(json);
}

void sendAck(const String& command, bool success) {
    String json = "{\"type\":\"ack\",";
    json += "\"command\":\"" + command + "\",";
    json += "\"success\":" + String(success ? "true" : "false") + "}";
    sendToESP32(json);
}

void processCommand(const String& data) {
    state.messagesReceived++;
    DEBUG_SERIAL.printf("[RX] %s\n", data.c_str());
    
    // Simple command parsing (expecting JSON)
    if (data.indexOf("\"cmd\":\"led1_on\"") >= 0 || data == "led1_on") {
        state.led1 = true;
        digitalWrite(LED_PIN, LOW);  // Active LOW
        sendAck("led1_on", true);
        DEBUG_SERIAL.println("[CMD] LED1 ON");
        
    } else if (data.indexOf("\"cmd\":\"led1_off\"") >= 0 || data == "led1_off") {
        state.led1 = false;
        digitalWrite(LED_PIN, HIGH);
        sendAck("led1_off", true);
        DEBUG_SERIAL.println("[CMD] LED1 OFF");
        
    } else if (data.indexOf("\"cmd\":\"led2_on\"") >= 0 || data == "led2_on") {
        state.led2 = true;
        digitalWrite(LED2_PIN, HIGH);
        sendAck("led2_on", true);
        DEBUG_SERIAL.println("[CMD] LED2 ON");
        
    } else if (data.indexOf("\"cmd\":\"led2_off\"") >= 0 || data == "led2_off") {
        state.led2 = false;
        digitalWrite(LED2_PIN, LOW);
        sendAck("led2_off", true);
        DEBUG_SERIAL.println("[CMD] LED2 OFF");
        
    } else if (data.indexOf("\"cmd\":\"toggle\"") >= 0 || data == "toggle") {
        state.led1 = !state.led1;
        digitalWrite(LED_PIN, state.led1 ? LOW : HIGH);
        sendAck("toggle", true);
        DEBUG_SERIAL.printf("[CMD] Toggle LED1 -> %s\n", state.led1 ? "ON" : "OFF");
        
    } else if (data.indexOf("\"cmd\":\"status\"") >= 0 || data == "status") {
        sendStatus();
        DEBUG_SERIAL.println("[CMD] Status sent");
        
    } else if (data.indexOf("\"cmd\":\"ping\"") >= 0 || data == "ping") {
        String json = "{\"type\":\"pong\",\"timestamp\":" + String(millis()) + "}";
        sendToESP32(json);
        DEBUG_SERIAL.println("[CMD] Pong sent");
        
    } else if (data.indexOf("\"cmd\":\"info\"") >= 0 || data == "info") {
        String json = "{\"type\":\"info\",";
        json += "\"device\":\"STM32F103C8T6\",";
        json += "\"firmware\":\"1.0.0\",";
        json += "\"uptime\":" + String(millis() / 1000) + "}";
        sendToESP32(json);
        DEBUG_SERIAL.println("[CMD] Info sent");
        
    } else {
        state.errors++;
        sendJSON("error", "msg", "Unknown command");
        DEBUG_SERIAL.printf("[ERROR] Unknown: %s\n", data.c_str());
    }
}

void printHelp() {
    DEBUG_SERIAL.println("\n╔══════════════════════════════════════╗");
    DEBUG_SERIAL.println("║        Available Commands            ║");
    DEBUG_SERIAL.println("╠══════════════════════════════════════╣");
    DEBUG_SERIAL.println("║  led1_on/off - Control built-in LED  ║");
    DEBUG_SERIAL.println("║  led2_on/off - Control external LED  ║");
    DEBUG_SERIAL.println("║  toggle      - Toggle LED1           ║");
    DEBUG_SERIAL.println("║  status      - Send status           ║");
    DEBUG_SERIAL.println("║  ping        - Test connection       ║");
    DEBUG_SERIAL.println("║  info        - Device info           ║");
    DEBUG_SERIAL.println("║  test        - Send test message     ║");
    DEBUG_SERIAL.println("║  help        - Show this help        ║");
    DEBUG_SERIAL.println("╚══════════════════════════════════════╝\n");
}

void setup() {
    // Initialize GPIOs
    pinMode(LED_PIN, OUTPUT);
    pinMode(LED2_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH);   // LED OFF (active LOW)
    digitalWrite(LED2_PIN, LOW);   // LED OFF
    
    // Initialize serial ports
    DEBUG_SERIAL.begin(DEBUG_BAUD);
    ESP32_SERIAL.begin(ESP32_BAUD);
    
    delay(2000);  // Wait for serial
    
    DEBUG_SERIAL.println("\n╔══════════════════════════════════════╗");
    DEBUG_SERIAL.println("║   STM32 Serial Basic Communication   ║");
    DEBUG_SERIAL.println("║   Modul 13: Network Communication    ║");
    DEBUG_SERIAL.println("╚══════════════════════════════════════╝\n");
    
    DEBUG_SERIAL.printf("[Serial1] TX=PA9, RX=PA10, Baud=%d\n", ESP32_BAUD);
    DEBUG_SERIAL.println("[Ready] Waiting for commands...\n");
    
    // Send startup message to ESP32
    String startupMsg = "{\"type\":\"startup\",\"device\":\"STM32\",\"version\":\"1.0\"}";
    sendToESP32(startupMsg);
    
    printHelp();
}

void loop() {
    // Read from ESP32
    while (ESP32_SERIAL.available()) {
        char c = ESP32_SERIAL.read();
        
        if (c == '\n') {
            if (receiveBuffer.length() > 0) {
                processCommand(receiveBuffer);
                receiveBuffer = "";
            }
        } else if (c != '\r') {
            receiveBuffer += c;
        }
    }
    
    // Read from debug serial (USB)
    if (DEBUG_SERIAL.available()) {
        String cmd = DEBUG_SERIAL.readStringUntil('\n');
        cmd.trim();
        
        if (cmd.length() > 0) {
            if (cmd == "help") {
                printHelp();
            } else if (cmd == "test") {
                String json = "{\"type\":\"test\",\"message\":\"Hello from STM32!\"}";
                sendToESP32(json);
            } else if (cmd.startsWith("send ")) {
                String msg = cmd.substring(5);
                sendToESP32(msg);
            } else if (cmd == "stats") {
                DEBUG_SERIAL.println("\n[Statistics]");
                DEBUG_SERIAL.printf("  Messages Sent    : %lu\n", state.messagesSent);
                DEBUG_SERIAL.printf("  Messages Received: %lu\n", state.messagesReceived);
                DEBUG_SERIAL.printf("  Errors           : %lu\n", state.errors);
                DEBUG_SERIAL.printf("  Uptime           : %lu seconds\n", millis() / 1000);
            } else {
                // Forward command to processing (simulate from ESP32)
                processCommand(cmd);
            }
        }
    }
    
    // Heartbeat LED blink
    static unsigned long lastBlink = 0;
    if (millis() - lastBlink >= 1000) {
        // Brief blink if not controlled
        if (!state.led1) {
            digitalWrite(LED_PIN, LOW);
            delay(50);
            digitalWrite(LED_PIN, HIGH);
        }
        lastBlink = millis();
    }
}
