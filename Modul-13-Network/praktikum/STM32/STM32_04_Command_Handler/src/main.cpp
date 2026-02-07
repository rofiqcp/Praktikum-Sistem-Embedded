/*
 * STM32 Advanced Command Handler
 * Modul 13: Network Communication
 * 
 * Demonstrates:
 * - Command queue management
 * - State machine for command processing
 * - Error handling and validation
 * - Command acknowledgment
 */

#include <Arduino.h>
#include <ArduinoJson.h>

// Serial configuration
#define ESP32_SERIAL Serial1
#define ESP32_BAUD 115200
#define DEBUG_SERIAL Serial

// GPIO pins
const int LED_PINS[] = {PC13, PA0, PA1, PA2};
const int NUM_LEDS = 4;
const int RELAY_PINS[] = {PA3, PA4};
const int NUM_RELAYS = 2;
const int BUZZER_PIN = PA5;

// Command queue
const int QUEUE_SIZE = 10;
struct Command {
    String data;
    unsigned long timestamp;
    bool processed;
} commandQueue[QUEUE_SIZE];
int queueHead = 0;
int queueTail = 0;
int queueCount = 0;

// Device state
struct DeviceState {
    bool leds[4];
    bool relays[2];
    int pwmValues[4];
    bool buzzerActive;
    String mode;  // "normal", "test", "sleep"
} deviceState;

// Statistics
struct CommandStats {
    unsigned long received;
    unsigned long processed;
    unsigned long errors;
    unsigned long queued;
    unsigned long dropped;
} cmdStats = {0, 0, 0, 0, 0};

// Buffer
String rxBuffer = "";

// Command response codes
enum ResponseCode {
    RESP_OK = 0,
    RESP_ERROR = 1,
    RESP_INVALID_CMD = 2,
    RESP_INVALID_PARAM = 3,
    RESP_BUSY = 4,
    RESP_NOT_ALLOWED = 5
};

void sendResponse(const char* cmd, ResponseCode code, const char* message = nullptr, JsonObject* data = nullptr) {
    StaticJsonDocument<256> doc;
    
    doc["type"] = "response";
    doc["cmd"] = cmd;
    doc["code"] = code;
    doc["success"] = (code == RESP_OK);
    
    if (message) {
        doc["message"] = message;
    }
    
    if (data && !data->isNull()) {
        doc["data"] = *data;
    }
    
    doc["timestamp"] = millis();
    
    String output;
    serializeJson(doc, output);
    ESP32_SERIAL.println(output);
    DEBUG_SERIAL.printf("[TX] %s\n", output.c_str());
}

void sendEvent(const char* event, const char* details = nullptr) {
    StaticJsonDocument<128> doc;
    doc["type"] = "event";
    doc["event"] = event;
    if (details) {
        doc["details"] = details;
    }
    doc["timestamp"] = millis();
    
    String output;
    serializeJson(doc, output);
    ESP32_SERIAL.println(output);
}

bool enqueueCommand(const String& cmd) {
    if (queueCount >= QUEUE_SIZE) {
        cmdStats.dropped++;
        return false;
    }
    
    commandQueue[queueTail].data = cmd;
    commandQueue[queueTail].timestamp = millis();
    commandQueue[queueTail].processed = false;
    
    queueTail = (queueTail + 1) % QUEUE_SIZE;
    queueCount++;
    cmdStats.queued++;
    
    return true;
}

String dequeueCommand() {
    if (queueCount == 0) {
        return "";
    }
    
    String cmd = commandQueue[queueHead].data;
    queueHead = (queueHead + 1) % QUEUE_SIZE;
    queueCount--;
    
    return cmd;
}

void processCommand(const String& message) {
    cmdStats.processed++;
    
    DEBUG_SERIAL.printf("[PROC] %s\n", message.c_str());
    
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, message);
    
    if (error) {
        sendResponse("unknown", RESP_ERROR, "JSON parse error");
        cmdStats.errors++;
        return;
    }
    
    String cmd = doc["cmd"] | "";
    
    // LED control commands
    if (cmd.startsWith("led")) {
        int ledNum = -1;
        bool state = doc["state"] | false;
        
        if (cmd == "led_all") {
            for (int i = 0; i < NUM_LEDS; i++) {
                deviceState.leds[i] = state;
                digitalWrite(LED_PINS[i], (i == 0) ? !state : state);  // PC13 is active LOW
            }
            sendResponse(cmd.c_str(), RESP_OK, state ? "All LEDs ON" : "All LEDs OFF");
            
        } else if (sscanf(cmd.c_str(), "led%d", &ledNum) == 1) {
            if (ledNum >= 1 && ledNum <= NUM_LEDS) {
                int idx = ledNum - 1;
                deviceState.leds[idx] = state;
                digitalWrite(LED_PINS[idx], (idx == 0) ? !state : state);
                
                char msg[32];
                sprintf(msg, "LED %d %s", ledNum, state ? "ON" : "OFF");
                sendResponse(cmd.c_str(), RESP_OK, msg);
            } else {
                sendResponse(cmd.c_str(), RESP_INVALID_PARAM, "LED number 1-4");
            }
        }
        
    // Relay control
    } else if (cmd.startsWith("relay")) {
        int relayNum = -1;
        bool state = doc["state"] | false;
        
        if (sscanf(cmd.c_str(), "relay%d", &relayNum) == 1) {
            if (relayNum >= 1 && relayNum <= NUM_RELAYS) {
                int idx = relayNum - 1;
                deviceState.relays[idx] = state;
                digitalWrite(RELAY_PINS[idx], state ? HIGH : LOW);
                
                char msg[32];
                sprintf(msg, "Relay %d %s", relayNum, state ? "ON" : "OFF");
                sendResponse(cmd.c_str(), RESP_OK, msg);
            } else {
                sendResponse(cmd.c_str(), RESP_INVALID_PARAM, "Relay number 1-2");
            }
        }
        
    // PWM control
    } else if (cmd == "pwm") {
        int channel = doc["channel"] | 0;
        int value = doc["value"] | 0;
        
        if (channel >= 1 && channel <= NUM_LEDS && value >= 0 && value <= 255) {
            int idx = channel - 1;
            deviceState.pwmValues[idx] = value;
            analogWrite(LED_PINS[idx], value);
            
            char msg[32];
            sprintf(msg, "PWM channel %d = %d", channel, value);
            sendResponse(cmd.c_str(), RESP_OK, msg);
        } else {
            sendResponse(cmd.c_str(), RESP_INVALID_PARAM, "Channel 1-4, value 0-255");
        }
        
    // Buzzer control
    } else if (cmd == "buzzer") {
        bool state = doc["state"] | false;
        int duration = doc["duration"] | 0;
        int frequency = doc["frequency"] | 1000;
        
        if (duration > 0) {
            tone(BUZZER_PIN, frequency, duration);
            sendResponse(cmd.c_str(), RESP_OK, "Buzzer tone");
        } else {
            if (state) {
                tone(BUZZER_PIN, frequency);
            } else {
                noTone(BUZZER_PIN);
            }
            deviceState.buzzerActive = state;
            sendResponse(cmd.c_str(), RESP_OK, state ? "Buzzer ON" : "Buzzer OFF");
        }
        
    // Mode control
    } else if (cmd == "set_mode") {
        String mode = doc["mode"] | "normal";
        
        if (mode == "normal" || mode == "test" || mode == "sleep") {
            deviceState.mode = mode;
            sendEvent("mode_changed", mode.c_str());
            sendResponse(cmd.c_str(), RESP_OK, ("Mode: " + mode).c_str());
        } else {
            sendResponse(cmd.c_str(), RESP_INVALID_PARAM, "Modes: normal, test, sleep");
        }
        
    // Batch command
    } else if (cmd == "batch") {
        JsonArray commands = doc["commands"];
        int count = 0;
        
        for (JsonObject cmdObj : commands) {
            String subCmd;
            serializeJson(cmdObj, subCmd);
            if (enqueueCommand(subCmd)) {
                count++;
            }
        }
        
        char msg[32];
        sprintf(msg, "Queued %d commands", count);
        sendResponse(cmd.c_str(), RESP_OK, msg);
        
    // Get state
    } else if (cmd == "get_state") {
        StaticJsonDocument<256> stateDoc;
        
        JsonArray leds = stateDoc.createNestedArray("leds");
        for (int i = 0; i < NUM_LEDS; i++) {
            leds.add(deviceState.leds[i]);
        }
        
        JsonArray relays = stateDoc.createNestedArray("relays");
        for (int i = 0; i < NUM_RELAYS; i++) {
            relays.add(deviceState.relays[i]);
        }
        
        stateDoc["mode"] = deviceState.mode;
        stateDoc["buzzer"] = deviceState.buzzerActive;
        
        String output;
        serializeJson(stateDoc, output);
        
        StaticJsonDocument<64> resp;
        resp["type"] = "state";
        JsonObject stateObj = resp.createNestedObject("state");
        for (int i = 0; i < NUM_LEDS; i++) {
            stateObj["led" + String(i+1)] = deviceState.leds[i];
        }
        stateObj["mode"] = deviceState.mode;
        
        String respOutput;
        serializeJson(resp, respOutput);
        ESP32_SERIAL.println(respOutput);
        
    // Get stats
    } else if (cmd == "get_stats") {
        StaticJsonDocument<192> statsDoc;
        statsDoc["type"] = "stats";
        statsDoc["received"] = cmdStats.received;
        statsDoc["processed"] = cmdStats.processed;
        statsDoc["errors"] = cmdStats.errors;
        statsDoc["queued"] = cmdStats.queued;
        statsDoc["dropped"] = cmdStats.dropped;
        statsDoc["queue_size"] = queueCount;
        
        String output;
        serializeJson(statsDoc, output);
        ESP32_SERIAL.println(output);
        
    // Ping
    } else if (cmd == "ping") {
        sendResponse(cmd.c_str(), RESP_OK, "pong");
        
    // Test sequence
    } else if (cmd == "test") {
        sendEvent("test_start", "Running LED test");
        
        for (int i = 0; i < NUM_LEDS; i++) {
            digitalWrite(LED_PINS[i], (i == 0) ? LOW : HIGH);
            delay(200);
            digitalWrite(LED_PINS[i], (i == 0) ? HIGH : LOW);
        }
        
        sendResponse(cmd.c_str(), RESP_OK, "Test complete");
        sendEvent("test_complete", nullptr);
        
    // Reset
    } else if (cmd == "reset") {
        for (int i = 0; i < NUM_LEDS; i++) {
            deviceState.leds[i] = false;
            digitalWrite(LED_PINS[i], (i == 0) ? HIGH : LOW);
        }
        for (int i = 0; i < NUM_RELAYS; i++) {
            deviceState.relays[i] = false;
            digitalWrite(RELAY_PINS[i], LOW);
        }
        noTone(BUZZER_PIN);
        deviceState.buzzerActive = false;
        deviceState.mode = "normal";
        
        sendResponse(cmd.c_str(), RESP_OK, "Device reset");
        
    // Unknown command
    } else {
        sendResponse(cmd.c_str(), RESP_INVALID_CMD, "Unknown command");
        cmdStats.errors++;
    }
}

void printStatus() {
    DEBUG_SERIAL.println("\n╔══════════════════════════════════════╗");
    DEBUG_SERIAL.println("║       Command Handler Status         ║");
    DEBUG_SERIAL.println("╠══════════════════════════════════════╣");
    DEBUG_SERIAL.printf("║ Mode           : %-19s ║\n", deviceState.mode.c_str());
    DEBUG_SERIAL.printf("║ Commands Recv  : %-19lu ║\n", cmdStats.received);
    DEBUG_SERIAL.printf("║ Commands Proc  : %-19lu ║\n", cmdStats.processed);
    DEBUG_SERIAL.printf("║ Errors         : %-19lu ║\n", cmdStats.errors);
    DEBUG_SERIAL.printf("║ Queue Size     : %-19d ║\n", queueCount);
    DEBUG_SERIAL.printf("║ Dropped        : %-19lu ║\n", cmdStats.dropped);
    DEBUG_SERIAL.println("╠══════════════════════════════════════╣");
    DEBUG_SERIAL.print("║ LEDs: ");
    for (int i = 0; i < NUM_LEDS; i++) {
        DEBUG_SERIAL.printf("%d:%s ", i+1, deviceState.leds[i] ? "ON" : "OFF");
    }
    DEBUG_SERIAL.println("        ║");
    DEBUG_SERIAL.print("║ Relays: ");
    for (int i = 0; i < NUM_RELAYS; i++) {
        DEBUG_SERIAL.printf("%d:%s ", i+1, deviceState.relays[i] ? "ON" : "OFF");
    }
    DEBUG_SERIAL.println("                      ║");
    DEBUG_SERIAL.println("╚══════════════════════════════════════╝\n");
}

void setup() {
    // Initialize LEDs
    for (int i = 0; i < NUM_LEDS; i++) {
        pinMode(LED_PINS[i], OUTPUT);
        digitalWrite(LED_PINS[i], (i == 0) ? HIGH : LOW);  // All OFF
        deviceState.leds[i] = false;
        deviceState.pwmValues[i] = 0;
    }
    
    // Initialize relays
    for (int i = 0; i < NUM_RELAYS; i++) {
        pinMode(RELAY_PINS[i], OUTPUT);
        digitalWrite(RELAY_PINS[i], LOW);
        deviceState.relays[i] = false;
    }
    
    // Initialize buzzer
    pinMode(BUZZER_PIN, OUTPUT);
    deviceState.buzzerActive = false;
    deviceState.mode = "normal";
    
    // Initialize serial
    Serial.begin(115200);
    ESP32_SERIAL.begin(ESP32_BAUD);
    
    delay(2000);
    
    DEBUG_SERIAL.println("\n╔══════════════════════════════════════╗");
    DEBUG_SERIAL.println("║   STM32 Advanced Command Handler     ║");
    DEBUG_SERIAL.println("║   Modul 13: Network Communication    ║");
    DEBUG_SERIAL.println("╚══════════════════════════════════════╝\n");
    
    // Send startup event
    sendEvent("startup", "STM32 Command Handler ready");
    
    DEBUG_SERIAL.println("[Ready] Waiting for commands...");
}

void loop() {
    // Read from ESP32
    while (ESP32_SERIAL.available()) {
        char c = ESP32_SERIAL.read();
        
        if (c == '\n') {
            if (rxBuffer.length() > 0) {
                cmdStats.received++;
                
                // Try to process immediately or queue
                if (queueCount > 0) {
                    enqueueCommand(rxBuffer);
                } else {
                    processCommand(rxBuffer);
                }
                rxBuffer = "";
            }
        } else if (c != '\r') {
            rxBuffer += c;
        }
    }
    
    // Process queued commands
    if (queueCount > 0) {
        String cmd = dequeueCommand();
        if (cmd.length() > 0) {
            processCommand(cmd);
        }
    }
    
    // Debug serial
    if (DEBUG_SERIAL.available()) {
        String cmd = DEBUG_SERIAL.readStringUntil('\n');
        cmd.trim();
        
        if (cmd == "status") {
            printStatus();
        } else if (cmd == "help") {
            DEBUG_SERIAL.println("\nCommands: status, test, reset, help");
            DEBUG_SERIAL.println("Or send JSON command to process");
        } else if (cmd.length() > 0) {
            processCommand(cmd);
        }
    }
    
    delay(1);
}
