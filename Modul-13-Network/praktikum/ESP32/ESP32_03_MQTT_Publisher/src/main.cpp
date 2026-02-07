/*
 * ESP32 MQTT Publisher Demo
 * Modul 13: Network Communication
 * 
 * Demonstrates:
 * - MQTT connection to public broker
 * - Publishing sensor data as JSON
 * - QoS levels
 * - Retained messages
 * - Auto-reconnection
 */

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// WiFi credentials
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// MQTT Broker settings
const char* mqtt_server = "broker.hivemq.com";
const int mqtt_port = 1883;

// Device identification
const char* device_id = "ESP32_Publisher_001";
const char* location = "Lab_Embedded";

// MQTT Topics
const char* topic_sensor = "embedded/praktikum/sensors/data";
const char* topic_status = "embedded/praktikum/sensors/status";
const char* topic_heartbeat = "embedded/praktikum/sensors/heartbeat";

// Clients
WiFiClient espClient;
PubSubClient mqtt(espClient);

// Timing
unsigned long lastPublish = 0;
unsigned long lastHeartbeat = 0;
const int publishInterval = 5000;      // Publish every 5 seconds
const int heartbeatInterval = 30000;   // Heartbeat every 30 seconds

// Statistics
unsigned long publishCount = 0;
unsigned long publishSuccess = 0;
unsigned long publishFailed = 0;

// Simulated sensor functions
float readTemperature() {
    // Simulate temperature between 20-35°C
    return 20.0 + (random(0, 150) / 10.0);
}

float readHumidity() {
    // Simulate humidity between 40-80%
    return 40.0 + (random(0, 400) / 10.0);
}

float readPressure() {
    // Simulate pressure around 1013 hPa
    return 1000.0 + (random(0, 260) / 10.0);
}

int readLightLevel() {
    // Simulate light level 0-1023
    return random(0, 1024);
}

void publishSensorData() {
    StaticJsonDocument<512> doc;
    
    // Device info
    doc["device_id"] = device_id;
    doc["location"] = location;
    doc["timestamp"] = millis();
    
    // Sensor readings
    JsonObject sensors = doc.createNestedObject("sensors");
    sensors["temperature"] = readTemperature();
    sensors["humidity"] = readHumidity();
    sensors["pressure"] = readPressure();
    sensors["light"] = readLightLevel();
    
    // System info
    JsonObject system = doc.createNestedObject("system");
    system["heap"] = ESP.getFreeHeap();
    system["rssi"] = WiFi.RSSI();
    system["uptime"] = millis() / 1000;
    
    // Serialize to string
    char buffer[512];
    size_t len = serializeJson(doc, buffer);
    
    publishCount++;
    
    // Publish with QoS 1 (at least once delivery)
    if (mqtt.publish(topic_sensor, buffer, false)) {
        publishSuccess++;
        Serial.printf("[MQTT] Published (%lu bytes): ", len);
        Serial.println(buffer);
    } else {
        publishFailed++;
        Serial.println("[MQTT] Publish failed!");
    }
}

void publishStatus(bool online) {
    StaticJsonDocument<200> doc;
    
    doc["device_id"] = device_id;
    doc["status"] = online ? "online" : "offline";
    doc["ip"] = WiFi.localIP().toString();
    doc["rssi"] = WiFi.RSSI();
    doc["timestamp"] = millis();
    
    char buffer[200];
    serializeJson(doc, buffer);
    
    // Publish as retained message so new subscribers get current status
    if (mqtt.publish(topic_status, buffer, true)) {
        Serial.printf("[MQTT] Status published: %s\n", online ? "online" : "offline");
    }
}

void publishHeartbeat() {
    StaticJsonDocument<100> doc;
    
    doc["device_id"] = device_id;
    doc["heartbeat"] = millis() / 1000;
    doc["publishes"] = publishCount;
    
    char buffer[100];
    serializeJson(doc, buffer);
    
    mqtt.publish(topic_heartbeat, buffer);
}

void connectWiFi() {
    Serial.printf("[WiFi] Connecting to %s", ssid);
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 30) {
        delay(1000);
        Serial.print(".");
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println(" Connected!");
        Serial.printf("[WiFi] IP: %s\n", WiFi.localIP().toString().c_str());
    } else {
        Serial.println(" Failed!");
    }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
    // This publisher doesn't subscribe, but callback is required
    Serial.printf("[MQTT] Message received on %s\n", topic);
}

void connectMQTT() {
    int attempts = 0;
    
    while (!mqtt.connected() && attempts < 5) {
        Serial.printf("[MQTT] Connecting to %s:%d...", mqtt_server, mqtt_port);
        
        // Create unique client ID
        String clientId = String(device_id) + "_" + String(random(0xffff), HEX);
        
        // Last Will Testament - will be published if we disconnect unexpectedly
        StaticJsonDocument<100> lwt;
        lwt["device_id"] = device_id;
        lwt["status"] = "offline";
        lwt["timestamp"] = millis();
        
        char lwtBuffer[100];
        serializeJson(lwt, lwtBuffer);
        
        // Connect with LWT
        if (mqtt.connect(clientId.c_str(), NULL, NULL, topic_status, 1, true, lwtBuffer)) {
            Serial.println(" Connected!");
            
            // Publish online status
            publishStatus(true);
            
            return;
        }
        
        Serial.printf(" Failed (rc=%d)\n", mqtt.state());
        attempts++;
        delay(2000);
    }
    
    Serial.println("[MQTT] Connection failed after 5 attempts");
}

void printMQTTError(int state) {
    switch (state) {
        case -4: Serial.println("Connection timeout"); break;
        case -3: Serial.println("Connection lost"); break;
        case -2: Serial.println("Connect failed"); break;
        case -1: Serial.println("Disconnected"); break;
        case 0: Serial.println("Connected"); break;
        case 1: Serial.println("Bad protocol"); break;
        case 2: Serial.println("Bad client ID"); break;
        case 3: Serial.println("Unavailable"); break;
        case 4: Serial.println("Bad credentials"); break;
        case 5: Serial.println("Unauthorized"); break;
        default: Serial.println("Unknown error");
    }
}

void printStats() {
    Serial.println("\n╔══════════════════════════════════════╗");
    Serial.println("║        Publisher Statistics          ║");
    Serial.println("╠══════════════════════════════════════╣");
    Serial.printf("║ Total Publishes  : %-18lu ║\n", publishCount);
    Serial.printf("║ Successful       : %-18lu ║\n", publishSuccess);
    Serial.printf("║ Failed           : %-18lu ║\n", publishFailed);
    Serial.printf("║ Success Rate     : %-17.1f%% ║\n", 
                 publishCount > 0 ? (publishSuccess * 100.0 / publishCount) : 0);
    Serial.printf("║ MQTT Connected   : %-18s ║\n", mqtt.connected() ? "Yes" : "No");
    Serial.printf("║ Free Heap        : %-14lu bytes ║\n", ESP.getFreeHeap());
    Serial.println("╚══════════════════════════════════════╝\n");
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\n╔══════════════════════════════════════╗");
    Serial.println("║     ESP32 MQTT Publisher Demo        ║");
    Serial.println("║   Modul 13: Network Communication    ║");
    Serial.println("╚══════════════════════════════════════╝\n");
    
    // Connect to WiFi
    connectWiFi();
    
    // Configure MQTT
    mqtt.setServer(mqtt_server, mqtt_port);
    mqtt.setCallback(mqttCallback);
    mqtt.setBufferSize(1024);  // Increase buffer for larger messages
    
    // Connect to MQTT
    connectMQTT();
    
    Serial.println("\n[Ready] Publishing sensor data...");
    Serial.printf("[Config] Topic: %s\n", topic_sensor);
    Serial.printf("[Config] Interval: %d ms\n", publishInterval);
}

void loop() {
    // Maintain MQTT connection
    if (!mqtt.connected()) {
        Serial.println("[MQTT] Connection lost, reconnecting...");
        connectMQTT();
    }
    mqtt.loop();
    
    // Publish sensor data periodically
    if (millis() - lastPublish >= publishInterval) {
        if (mqtt.connected()) {
            publishSensorData();
        }
        lastPublish = millis();
    }
    
    // Send heartbeat periodically
    if (millis() - lastHeartbeat >= heartbeatInterval) {
        if (mqtt.connected()) {
            publishHeartbeat();
        }
        lastHeartbeat = millis();
    }
    
    // Handle serial commands
    if (Serial.available()) {
        String cmd = Serial.readStringUntil('\n');
        cmd.trim();
        cmd.toLowerCase();
        
        if (cmd == "publish") {
            Serial.println("[Manual] Publishing sensor data...");
            publishSensorData();
        } else if (cmd == "status") {
            Serial.printf("[Status] MQTT: %s | WiFi: %s\n",
                         mqtt.connected() ? "Connected" : "Disconnected",
                         WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected");
        } else if (cmd == "stats") {
            printStats();
        } else if (cmd == "reconnect") {
            Serial.println("[Manual] Reconnecting MQTT...");
            mqtt.disconnect();
            connectMQTT();
        } else if (cmd == "help") {
            Serial.println("\nCommands:");
            Serial.println("  publish   - Send sensor data now");
            Serial.println("  status    - Show connection status");
            Serial.println("  stats     - Show publish statistics");
            Serial.println("  reconnect - Reconnect to MQTT broker");
            Serial.println("  help      - Show this help");
        }
    }
}
