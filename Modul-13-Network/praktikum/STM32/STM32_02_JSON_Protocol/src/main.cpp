/*
 * STM32 JSON Protocol Communication
 * Modul 13: Network Communication
 * 
 * Demonstrates:
 * - ArduinoJson library usage
 * - Structured data exchange
 * - Message framing
 * - Protocol handling
 */

#include <Arduino.h>
#include <ArduinoJson.h>

// Serial ports
#define ESP32_SERIAL Serial1
#define ESP32_BAUD 115200
#define DEBUG_SERIAL Serial
#define DEBUG_BAUD 115200

// GPIO pins
const int LED_PIN = PC13;
const int BUTTON_PIN = PA0;

// Message types
enum MessageType {
    MSG_DATA,
    MSG_COMMAND,
    MSG_RESPONSE,
    MSG_EVENT,
    MSG_ERROR
};

// System configuration
struct Config {
    int reportInterval;
    bool autoReport;
    int ledBrightness;
    String deviceName;
} config = {1000, false, 100, "STM32_Node"};

// Statistics
struct Stats {
    unsigned long txMessages;
    unsigned long rxMessages;
    unsigned long txBytes;
    unsigned long rxBytes;
    unsigned long errors;
    unsigned long startTime;
} stats = {0, 0, 0, 0, 0, 0};

// Buffer
String rxBuffer = "";
unsigned long lastReport = 0;

// Simulated sensors
float readTemperature() {
    return 20.0 + (analogRead(PA1) / 4096.0 * 15.0);
}

float readHumidity() {
    return 40.0 + (analogRead(PA2) / 4096.0 * 40.0);
}

int readLight() {
    return analogRead(PA3);
}

void sendMessage(JsonDocument& doc) {
    String output;
    serializeJson(doc, output);
    
    ESP32_SERIAL.println(output);
    
    stats.txMessages++;
    stats.txBytes += output.length();
    
    DEBUG_SERIAL.printf("[TX] %s\n", output.c_str());
}

void sendData() {
    StaticJsonDocument<256> doc;
    
    doc["type"] = "sensors";
    doc["device"] = config.deviceName;
    doc["timestamp"] = millis();
    
    JsonObject data = doc.createNestedObject("data");
    data["temp"] = readTemperature();
    data["hum"] = readHumidity();
    data["light"] = readLight();
    data["button"] = !digitalRead(BUTTON_PIN);
    
    sendMessage(doc);
}

void sendResponse(const char* command, bool success, const char* message = nullptr) {
    StaticJsonDocument<128> doc;
    
    doc["type"] = "response";
    doc["cmd"] = command;
    doc["success"] = success;
    if (message) {
        doc["message"] = message;
    }
    doc["timestamp"] = millis();
    
    sendMessage(doc);
}

void sendEvent(const char* event, JsonObject& data) {
    StaticJsonDocument<256> doc;
    
    doc["type"] = "event";
    doc["event"] = event;
    doc["device"] = config.deviceName;
    doc["data"] = data;
    doc["timestamp"] = millis();
    
    sendMessage(doc);
}

void sendError(const char* error, int code = 0) {
    StaticJsonDocument<128> doc;
    
    doc["type"] = "error";
    doc["error"] = error;
    doc["code"] = code;
    doc["timestamp"] = millis();
    
    sendMessage(doc);
    stats.errors++;
}

void sendStatus() {
    StaticJsonDocument<512> doc;
    
    doc["type"] = "status";
    doc["device"] = config.deviceName;
    
    JsonObject state = doc.createNestedObject("state");
    state["led"] = !digitalRead(LED_PIN);
    state["uptime"] = millis() / 1000;
    
    JsonObject cfg = doc.createNestedObject("config");
    cfg["interval"] = config.reportInterval;
    cfg["autoReport"] = config.autoReport;
    cfg["brightness"] = config.ledBrightness;
    
    JsonObject statistics = doc.createNestedObject("stats");
    statistics["tx"] = stats.txMessages;
    statistics["rx"] = stats.rxMessages;
    statistics["errors"] = stats.errors;
    
    sendMessage(doc);
}

void processMessage(const String& message) {
    stats.rxMessages++;
    stats.rxBytes += message.length();
    
    DEBUG_SERIAL.printf("[RX] %s\n", message.c_str());
    
    // Parse JSON
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, message);
    
    if (error) {
        DEBUG_SERIAL.printf("[ERROR] JSON: %s\n", error.c_str());
        sendError("JSON parse error", 1);
        return;
    }
    
    // Get message type and command
    String type = doc["type"] | "unknown";
    String cmd = doc["cmd"] | "";
    
    if (type == "command" || !cmd.isEmpty()) {
        // Process command
        if (cmd == "led_on") {
            digitalWrite(LED_PIN, LOW);
            sendResponse("led_on", true, "LED turned on");
            
        } else if (cmd == "led_off") {
            digitalWrite(LED_PIN, HIGH);
            sendResponse("led_off", true, "LED turned off");
            
        } else if (cmd == "led_toggle") {
            digitalWrite(LED_PIN, !digitalRead(LED_PIN));
            sendResponse("led_toggle", true);
            
        } else if (cmd == "read_sensors" || cmd == "get_data") {
            sendData();
            
        } else if (cmd == "get_status") {
            sendStatus();
            
        } else if (cmd == "set_config") {
            // Update configuration
            if (doc.containsKey("interval")) {
                config.reportInterval = doc["interval"];
            }
            if (doc.containsKey("autoReport")) {
                config.autoReport = doc["autoReport"];
            }
            if (doc.containsKey("name")) {
                config.deviceName = doc["name"].as<String>();
            }
            sendResponse("set_config", true, "Config updated");
            
        } else if (cmd == "start_report") {
            config.autoReport = true;
            sendResponse("start_report", true);
            
        } else if (cmd == "stop_report") {
            config.autoReport = false;
            sendResponse("stop_report", true);
            
        } else if (cmd == "ping") {
            StaticJsonDocument<64> pong;
            pong["type"] = "pong";
            pong["timestamp"] = millis();
            sendMessage(pong);
            
        } else if (cmd == "reset_stats") {
            stats.txMessages = 0;
            stats.rxMessages = 0;
            stats.txBytes = 0;
            stats.rxBytes = 0;
            stats.errors = 0;
            sendResponse("reset_stats", true);
            
        } else if (cmd == "restart") {
            sendResponse("restart", true, "Restarting...");
            delay(500);
            NVIC_SystemReset();
            
        } else {
            sendError("Unknown command", 2);
        }
    }
}

void printDebugStats() {
    DEBUG_SERIAL.println("\n╔══════════════════════════════════════╗");
    DEBUG_SERIAL.println("║        Communication Statistics      ║");
    DEBUG_SERIAL.println("╠══════════════════════════════════════╣");
    DEBUG_SERIAL.printf("║ TX Messages    : %-19lu ║\n", stats.txMessages);
    DEBUG_SERIAL.printf("║ RX Messages    : %-19lu ║\n", stats.rxMessages);
    DEBUG_SERIAL.printf("║ TX Bytes       : %-19lu ║\n", stats.txBytes);
    DEBUG_SERIAL.printf("║ RX Bytes       : %-19lu ║\n", stats.rxBytes);
    DEBUG_SERIAL.printf("║ Errors         : %-19lu ║\n", stats.errors);
    DEBUG_SERIAL.printf("║ Uptime         : %-17lu s ║\n", millis() / 1000);
    DEBUG_SERIAL.println("╚══════════════════════════════════════╝\n");
}

void setup() {
    // Initialize GPIOs
    pinMode(LED_PIN, OUTPUT);
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    digitalWrite(LED_PIN, HIGH);  // LED OFF
    
    // Initialize serial
    DEBUG_SERIAL.begin(DEBUG_BAUD);
    ESP32_SERIAL.begin(ESP32_BAUD);
    
    delay(2000);
    
    DEBUG_SERIAL.println("\n╔══════════════════════════════════════╗");
    DEBUG_SERIAL.println("║    STM32 JSON Protocol Communication ║");
    DEBUG_SERIAL.println("║   Modul 13: Network Communication    ║");
    DEBUG_SERIAL.println("╚══════════════════════════════════════╝\n");
    
    stats.startTime = millis();
    
    // Send startup notification
    StaticJsonDocument<128> startup;
    startup["type"] = "startup";
    startup["device"] = config.deviceName;
    startup["version"] = "2.0";
    startup["timestamp"] = millis();
    sendMessage(startup);
    
    DEBUG_SERIAL.println("[Ready] Commands: data, status, stats, start, stop, help");
}

void loop() {
    // Read from ESP32
    while (ESP32_SERIAL.available()) {
        char c = ESP32_SERIAL.read();
        
        if (c == '\n') {
            if (rxBuffer.length() > 0) {
                processMessage(rxBuffer);
                rxBuffer = "";
            }
        } else if (c != '\r') {
            rxBuffer += c;
        }
    }
    
    // Debug serial commands
    if (DEBUG_SERIAL.available()) {
        String cmd = DEBUG_SERIAL.readStringUntil('\n');
        cmd.trim();
        
        if (cmd == "data") {
            sendData();
        } else if (cmd == "status") {
            sendStatus();
        } else if (cmd == "stats") {
            printDebugStats();
        } else if (cmd == "start") {
            config.autoReport = true;
            DEBUG_SERIAL.println("[Auto] Started");
        } else if (cmd == "stop") {
            config.autoReport = false;
            DEBUG_SERIAL.println("[Auto] Stopped");
        } else if (cmd.startsWith("interval ")) {
            config.reportInterval = cmd.substring(9).toInt();
            DEBUG_SERIAL.printf("[Config] Interval: %d ms\n", config.reportInterval);
        } else if (cmd == "help") {
            DEBUG_SERIAL.println("\nCommands: data, status, stats, start, stop, interval N");
        } else if (cmd.length() > 0) {
            // Try as JSON command
            processMessage(cmd);
        }
    }
    
    // Auto report
    if (config.autoReport && (millis() - lastReport >= config.reportInterval)) {
        sendData();
        lastReport = millis();
    }
    
    // Button event detection
    static bool lastButton = true;
    bool currentButton = digitalRead(BUTTON_PIN);
    if (currentButton != lastButton) {
        delay(50);  // Debounce
        if (digitalRead(BUTTON_PIN) != lastButton) {
            lastButton = currentButton;
            
            StaticJsonDocument<64> eventData;
            eventData["pressed"] = !currentButton;
            JsonObject obj = eventData.as<JsonObject>();
            sendEvent("button", obj);
        }
    }
}
