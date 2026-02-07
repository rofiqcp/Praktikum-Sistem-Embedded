/*
 * ESP32 Bluetooth Classic (SPP) Demo
 * Modul 13: Network Communication
 * 
 * Demonstrates:
 * - Bluetooth Serial Port Profile (SPP)
 * - Device pairing
 * - Bidirectional data transfer
 * - Command processing via Bluetooth
 * - Sensor data streaming
 */

#include <Arduino.h>
#include "BluetoothSerial.h"
#include <ArduinoJson.h>

// Check if Bluetooth is enabled
#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` and enable it
#endif

// Bluetooth Serial
BluetoothSerial SerialBT;

// Device name
const char* deviceName = "ESP32_BT_Demo";

// GPIO pins
const int LED_PIN = 2;
const int LED2_PIN = 4;

// System state
struct SystemState {
    bool led1;
    bool led2;
    bool streaming;
    int streamInterval;
    unsigned long lastStream;
    unsigned long messageCount;
    bool connected;
} state = {false, false, false, 1000, 0, 0, false};

// Sensor data
struct SensorData {
    float temperature;
    float humidity;
    int light;
    int battery;
} sensors;

void updateSensors() {
    sensors.temperature = 20.0 + (random(0, 150) / 10.0);
    sensors.humidity = 40.0 + (random(0, 400) / 10.0);
    sensors.light = random(0, 1024);
    sensors.battery = random(70, 100);
}

// Bluetooth callback
void btCallback(esp_spp_cb_event_t event, esp_spp_cb_param_t *param) {
    switch (event) {
        case ESP_SPP_SRV_OPEN_EVT:
            Serial.println("[BT] Client connected!");
            state.connected = true;
            digitalWrite(LED_PIN, HIGH);
            
            // Send welcome message
            SerialBT.println("\n╔══════════════════════════════════════╗");
            SerialBT.println("║   Welcome to ESP32 Bluetooth Demo    ║");
            SerialBT.println("║   Type 'help' for commands           ║");
            SerialBT.println("╚══════════════════════════════════════╝\n");
            break;
            
        case ESP_SPP_CLOSE_EVT:
            Serial.println("[BT] Client disconnected!");
            state.connected = false;
            state.streaming = false;
            digitalWrite(LED_PIN, LOW);
            break;
            
        case ESP_SPP_DATA_IND_EVT:
            // Data received event
            break;
            
        default:
            break;
    }
}

void sendSensorDataText() {
    updateSensors();
    
    SerialBT.println("┌──────────────────────────────────────┐");
    SerialBT.println("│          Sensor Readings             │");
    SerialBT.println("├──────────────────────────────────────┤");
    SerialBT.printf("│ Temperature : %6.1f °C             │\n", sensors.temperature);
    SerialBT.printf("│ Humidity    : %6.1f %%              │\n", sensors.humidity);
    SerialBT.printf("│ Light Level : %6d                 │\n", sensors.light);
    SerialBT.printf("│ Battery     : %6d %%               │\n", sensors.battery);
    SerialBT.println("└──────────────────────────────────────┘");
}

void sendSensorDataJSON() {
    updateSensors();
    
    StaticJsonDocument<256> doc;
    doc["type"] = "sensors";
    doc["temperature"] = sensors.temperature;
    doc["humidity"] = sensors.humidity;
    doc["light"] = sensors.light;
    doc["battery"] = sensors.battery;
    doc["timestamp"] = millis();
    
    String output;
    serializeJson(doc, output);
    SerialBT.println(output);
}

void sendStatus() {
    SerialBT.println("\n┌──────────────────────────────────────┐");
    SerialBT.println("│            System Status             │");
    SerialBT.println("├──────────────────────────────────────┤");
    SerialBT.printf("│ LED 1 (GPIO %2d) : %-18s │\n", LED_PIN, state.led1 ? "ON" : "OFF");
    SerialBT.printf("│ LED 2 (GPIO %2d) : %-18s │\n", LED2_PIN, state.led2 ? "ON" : "OFF");
    SerialBT.printf("│ Streaming       : %-18s │\n", state.streaming ? "Active" : "Inactive");
    SerialBT.printf("│ Stream Interval : %-14d ms │\n", state.streamInterval);
    SerialBT.printf("│ Messages        : %-18lu │\n", state.messageCount);
    SerialBT.printf("│ Free Heap       : %-14lu B │\n", ESP.getFreeHeap());
    SerialBT.printf("│ Uptime          : %-14lu s │\n", millis() / 1000);
    SerialBT.println("└──────────────────────────────────────┘\n");
}

void sendHelp() {
    SerialBT.println("\n╔══════════════════════════════════════╗");
    SerialBT.println("║           Available Commands         ║");
    SerialBT.println("╠══════════════════════════════════════╣");
    SerialBT.println("║ Sensor Commands:                     ║");
    SerialBT.println("║   sensors    - Read sensor data      ║");
    SerialBT.println("║   json       - Sensor data as JSON   ║");
    SerialBT.println("║   stream     - Start data streaming  ║");
    SerialBT.println("║   stop       - Stop streaming        ║");
    SerialBT.println("║   interval N - Set stream interval   ║");
    SerialBT.println("╠══════════════════════════════════════╣");
    SerialBT.println("║ LED Commands:                        ║");
    SerialBT.println("║   led1 on/off - Control LED 1        ║");
    SerialBT.println("║   led2 on/off - Control LED 2        ║");
    SerialBT.println("║   toggle N    - Toggle LED N         ║");
    SerialBT.println("╠══════════════════════════════════════╣");
    SerialBT.println("║ System Commands:                     ║");
    SerialBT.println("║   status     - Show system status    ║");
    SerialBT.println("║   info       - Device information    ║");
    SerialBT.println("║   restart    - Restart ESP32         ║");
    SerialBT.println("║   help       - Show this help        ║");
    SerialBT.println("╚══════════════════════════════════════╝\n");
}

void sendDeviceInfo() {
    SerialBT.println("\n┌──────────────────────────────────────┐");
    SerialBT.println("│          Device Information          │");
    SerialBT.println("├──────────────────────────────────────┤");
    SerialBT.printf("│ Chip Model  : %-22s │\n", ESP.getChipModel());
    SerialBT.printf("│ Chip Rev    : %-22d │\n", ESP.getChipRevision());
    SerialBT.printf("│ CPU Freq    : %-19d MHz │\n", ESP.getCpuFreqMHz());
    SerialBT.printf("│ Flash Size  : %-19d KB │\n", ESP.getFlashChipSize() / 1024);
    SerialBT.printf("│ Free Heap   : %-19lu B │\n", ESP.getFreeHeap());
    SerialBT.printf("│ SDK Version : %-22s │\n", ESP.getSdkVersion());
    
    // Get Bluetooth MAC
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_BT);
    SerialBT.printf("│ BT MAC      : %02X:%02X:%02X:%02X:%02X:%02X       │\n",
                   mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    SerialBT.println("└──────────────────────────────────────┘\n");
}

void processCommand(String cmd) {
    cmd.trim();
    cmd.toLowerCase();
    state.messageCount++;
    
    Serial.printf("[BT] Command: %s\n", cmd.c_str());
    
    if (cmd == "help" || cmd == "?") {
        sendHelp();
    } else if (cmd == "sensors" || cmd == "read") {
        sendSensorDataText();
    } else if (cmd == "json") {
        sendSensorDataJSON();
    } else if (cmd == "stream") {
        state.streaming = true;
        SerialBT.println("[OK] Streaming started");
        Serial.println("[BT] Streaming started");
    } else if (cmd == "stop") {
        state.streaming = false;
        SerialBT.println("[OK] Streaming stopped");
        Serial.println("[BT] Streaming stopped");
    } else if (cmd.startsWith("interval ")) {
        int interval = cmd.substring(9).toInt();
        if (interval >= 100 && interval <= 10000) {
            state.streamInterval = interval;
            SerialBT.printf("[OK] Interval set to %d ms\n", interval);
        } else {
            SerialBT.println("[ERROR] Interval must be 100-10000 ms");
        }
    } else if (cmd == "led1 on") {
        state.led1 = true;
        digitalWrite(LED_PIN, HIGH);
        SerialBT.println("[OK] LED 1 ON");
    } else if (cmd == "led1 off") {
        state.led1 = false;
        digitalWrite(LED_PIN, LOW);
        SerialBT.println("[OK] LED 1 OFF");
    } else if (cmd == "led2 on") {
        state.led2 = true;
        digitalWrite(LED2_PIN, HIGH);
        SerialBT.println("[OK] LED 2 ON");
    } else if (cmd == "led2 off") {
        state.led2 = false;
        digitalWrite(LED2_PIN, LOW);
        SerialBT.println("[OK] LED 2 OFF");
    } else if (cmd == "toggle 1") {
        state.led1 = !state.led1;
        digitalWrite(LED_PIN, state.led1 ? HIGH : LOW);
        SerialBT.printf("[OK] LED 1 %s\n", state.led1 ? "ON" : "OFF");
    } else if (cmd == "toggle 2") {
        state.led2 = !state.led2;
        digitalWrite(LED2_PIN, state.led2 ? HIGH : LOW);
        SerialBT.printf("[OK] LED 2 %s\n", state.led2 ? "ON" : "OFF");
    } else if (cmd == "status") {
        sendStatus();
    } else if (cmd == "info") {
        sendDeviceInfo();
    } else if (cmd == "restart") {
        SerialBT.println("[OK] Restarting...");
        delay(500);
        ESP.restart();
    } else if (cmd.length() > 0) {
        SerialBT.printf("[ERROR] Unknown command: %s\n", cmd.c_str());
        SerialBT.println("Type 'help' for available commands");
    }
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    // Initialize GPIOs
    pinMode(LED_PIN, OUTPUT);
    pinMode(LED2_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);
    digitalWrite(LED2_PIN, LOW);
    
    Serial.println("\n╔══════════════════════════════════════╗");
    Serial.println("║   ESP32 Bluetooth Classic Demo       ║");
    Serial.println("║   Modul 13: Network Communication    ║");
    Serial.println("╚══════════════════════════════════════╝\n");
    
    // Initialize Bluetooth
    SerialBT.register_callback(btCallback);
    
    if (!SerialBT.begin(deviceName)) {
        Serial.println("[ERROR] Bluetooth initialization failed!");
        while (1);
    }
    
    Serial.printf("[BT] Device name: %s\n", deviceName);
    Serial.println("[BT] Waiting for connection...");
    Serial.println("[BT] Pair with your phone or computer");
    
    // Get Bluetooth MAC address
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_BT);
    Serial.printf("[BT] MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

void loop() {
    // Handle Bluetooth data
    if (SerialBT.available()) {
        String incoming = "";
        while (SerialBT.available()) {
            char c = SerialBT.read();
            if (c == '\n' || c == '\r') {
                if (incoming.length() > 0) {
                    processCommand(incoming);
                    incoming = "";
                }
            } else {
                incoming += c;
            }
        }
        // Process any remaining data
        if (incoming.length() > 0) {
            processCommand(incoming);
        }
    }
    
    // Handle serial data (for testing/debugging)
    if (Serial.available()) {
        String cmd = Serial.readStringUntil('\n');
        cmd.trim();
        
        if (cmd == "status") {
            Serial.printf("[Status] Connected: %s | Messages: %lu | Streaming: %s\n",
                         state.connected ? "Yes" : "No",
                         state.messageCount,
                         state.streaming ? "Yes" : "No");
        } else if (cmd.startsWith("send ")) {
            String msg = cmd.substring(5);
            SerialBT.println(msg);
            Serial.printf("[Sent] %s\n", msg.c_str());
        }
    }
    
    // Stream sensor data if enabled
    if (state.streaming && state.connected) {
        if (millis() - state.lastStream >= state.streamInterval) {
            sendSensorDataJSON();
            state.lastStream = millis();
        }
    }
    
    delay(10);
}
