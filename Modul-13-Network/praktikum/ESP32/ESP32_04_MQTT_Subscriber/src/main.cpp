/*
 * ESP32 MQTT Subscriber Demo
 * Modul 13: Network Communication
 * 
 * Demonstrates:
 * - MQTT subscription to multiple topics
 * - Message parsing with JSON
 * - Wildcard subscriptions
 * - QoS handling
 * - Command processing
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
const char* device_id = "ESP32_Subscriber_001";

// MQTT Topics to subscribe
const char* topic_sensors = "embedded/praktikum/sensors/#";  // Wildcard - all sensor topics
const char* topic_commands = "embedded/praktikum/commands";
const char* topic_broadcast = "embedded/praktikum/broadcast";

// Specific topics to listen for
const char* topic_sensor_data = "embedded/praktikum/sensors/data";
const char* topic_sensor_status = "embedded/praktikum/sensors/status";

// Response topic
const char* topic_response = "embedded/praktikum/responses";

// GPIO pins for LED indicators
const int LED_BUILTIN_PIN = 2;
const int LED_MESSAGE = 4;

// Clients
WiFiClient espClient;
PubSubClient mqtt(espClient);

// Statistics
unsigned long messageCount = 0;
unsigned long sensorMessages = 0;
unsigned long commandMessages = 0;
unsigned long broadcastMessages = 0;
unsigned long parseErrors = 0;

// Last sensor data received
struct SensorData {
    String device_id;
    float temperature;
    float humidity;
    float pressure;
    int light;
    long rssi;
    unsigned long timestamp;
    bool valid;
} lastSensorData;

void handleSensorData(const char* payload) {
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, payload);
    
    if (error) {
        Serial.printf("[ERROR] JSON parse failed: %s\n", error.c_str());
        parseErrors++;
        return;
    }
    
    sensorMessages++;
    
    // Extract data
    lastSensorData.device_id = doc["device_id"].as<String>();
    lastSensorData.timestamp = doc["timestamp"];
    
    if (doc.containsKey("sensors")) {
        JsonObject sensors = doc["sensors"];
        lastSensorData.temperature = sensors["temperature"];
        lastSensorData.humidity = sensors["humidity"];
        lastSensorData.pressure = sensors["pressure"];
        lastSensorData.light = sensors["light"];
    }
    
    if (doc.containsKey("system")) {
        lastSensorData.rssi = doc["system"]["rssi"];
    }
    
    lastSensorData.valid = true;
    
    // Print received data
    Serial.println("\n┌─────────────────────────────────────────┐");
    Serial.println("│          Sensor Data Received           │");
    Serial.println("├─────────────────────────────────────────┤");
    Serial.printf("│ Device     : %-26s │\n", lastSensorData.device_id.c_str());
    Serial.printf("│ Temperature: %-22.1f °C │\n", lastSensorData.temperature);
    Serial.printf("│ Humidity   : %-22.1f %% │\n", lastSensorData.humidity);
    Serial.printf("│ Pressure   : %-20.1f hPa │\n", lastSensorData.pressure);
    Serial.printf("│ Light      : %-26d │\n", lastSensorData.light);
    Serial.printf("│ RSSI       : %-23ld dBm │\n", lastSensorData.rssi);
    Serial.println("└─────────────────────────────────────────┘");
    
    // Blink LED to indicate received data
    digitalWrite(LED_MESSAGE, HIGH);
    delay(100);
    digitalWrite(LED_MESSAGE, LOW);
}

void handleStatusUpdate(const char* payload) {
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, payload);
    
    if (error) {
        parseErrors++;
        return;
    }
    
    String device = doc["device_id"].as<String>();
    String status = doc["status"].as<String>();
    String ip = doc["ip"].as<String>();
    
    Serial.printf("\n[STATUS] Device '%s' is now %s", device.c_str(), status.c_str());
    if (status == "online") {
        Serial.printf(" at IP: %s", ip.c_str());
    }
    Serial.println();
}

void handleCommand(const char* payload) {
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, payload);
    
    commandMessages++;
    
    if (error) {
        Serial.printf("[ERROR] Command parse failed: %s\n", error.c_str());
        parseErrors++;
        return;
    }
    
    String command = doc["command"].as<String>();
    String target = doc["target"].as<String>();
    
    // Check if this command is for us
    if (target != device_id && target != "all") {
        return;
    }
    
    Serial.printf("\n[COMMAND] Received: %s\n", command.c_str());
    
    // Process command
    StaticJsonDocument<256> response;
    response["device_id"] = device_id;
    response["command"] = command;
    
    if (command == "ping") {
        response["status"] = "pong";
        response["uptime"] = millis() / 1000;
    } else if (command == "status") {
        response["status"] = "active";
        response["heap"] = ESP.getFreeHeap();
        response["rssi"] = WiFi.RSSI();
        response["messages"] = messageCount;
    } else if (command == "led_on") {
        digitalWrite(LED_BUILTIN_PIN, HIGH);
        response["status"] = "LED turned ON";
    } else if (command == "led_off") {
        digitalWrite(LED_BUILTIN_PIN, LOW);
        response["status"] = "LED turned OFF";
    } else if (command == "restart") {
        response["status"] = "Restarting...";
        char buffer[256];
        serializeJson(response, buffer);
        mqtt.publish(topic_response, buffer);
        delay(1000);
        ESP.restart();
    } else {
        response["status"] = "unknown_command";
    }
    
    // Publish response
    char buffer[256];
    serializeJson(response, buffer);
    mqtt.publish(topic_response, buffer);
    Serial.printf("[RESPONSE] %s\n", buffer);
}

void handleBroadcast(const char* payload) {
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, payload);
    
    broadcastMessages++;
    
    if (error) {
        parseErrors++;
        Serial.printf("[BROADCAST] Raw: %s\n", payload);
        return;
    }
    
    String message = doc["message"].as<String>();
    String from = doc["from"].as<String>();
    
    Serial.println("\n╔══════════════════════════════════════╗");
    Serial.println("║          BROADCAST MESSAGE           ║");
    Serial.println("╠══════════════════════════════════════╣");
    Serial.printf("║ From: %-30s ║\n", from.c_str());
    Serial.printf("║ Message: %-27s ║\n", message.c_str());
    Serial.println("╚══════════════════════════════════════╝");
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
    messageCount++;
    
    // Create null-terminated string
    char message[length + 1];
    memcpy(message, payload, length);
    message[length] = '\0';
    
    Serial.printf("\n[MQTT] Topic: %s\n", topic);
    
    // Route message based on topic
    String topicStr = String(topic);
    
    if (topicStr == topic_sensor_data) {
        handleSensorData(message);
    } else if (topicStr == topic_sensor_status) {
        handleStatusUpdate(message);
    } else if (topicStr == topic_commands) {
        handleCommand(message);
    } else if (topicStr == topic_broadcast) {
        handleBroadcast(message);
    } else if (topicStr.startsWith("embedded/praktikum/sensors/")) {
        // Other sensor topics (heartbeat, etc.)
        Serial.printf("[SENSOR] %s: %s\n", topic, message);
    } else {
        Serial.printf("[OTHER] %s\n", message);
    }
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
    }
}

void connectMQTT() {
    int attempts = 0;
    
    while (!mqtt.connected() && attempts < 5) {
        Serial.printf("[MQTT] Connecting to %s:%d...", mqtt_server, mqtt_port);
        
        String clientId = String(device_id) + "_" + String(random(0xffff), HEX);
        
        if (mqtt.connect(clientId.c_str())) {
            Serial.println(" Connected!");
            
            // Subscribe to topics
            mqtt.subscribe(topic_sensors, 1);      // QoS 1 for sensors
            mqtt.subscribe(topic_commands, 1);     // QoS 1 for commands
            mqtt.subscribe(topic_broadcast, 0);    // QoS 0 for broadcasts
            
            Serial.println("\n[Subscribed Topics]");
            Serial.printf("  - %s (QoS 1)\n", topic_sensors);
            Serial.printf("  - %s (QoS 1)\n", topic_commands);
            Serial.printf("  - %s (QoS 0)\n", topic_broadcast);
            
            // Announce presence
            StaticJsonDocument<100> doc;
            doc["device_id"] = device_id;
            doc["event"] = "subscriber_online";
            doc["ip"] = WiFi.localIP().toString();
            
            char buffer[100];
            serializeJson(doc, buffer);
            mqtt.publish(topic_response, buffer);
            
            return;
        }
        
        Serial.printf(" Failed (rc=%d)\n", mqtt.state());
        attempts++;
        delay(2000);
    }
}

void printStats() {
    Serial.println("\n╔══════════════════════════════════════╗");
    Serial.println("║       Subscriber Statistics          ║");
    Serial.println("╠══════════════════════════════════════╣");
    Serial.printf("║ Total Messages   : %-18lu ║\n", messageCount);
    Serial.printf("║ Sensor Messages  : %-18lu ║\n", sensorMessages);
    Serial.printf("║ Command Messages : %-18lu ║\n", commandMessages);
    Serial.printf("║ Broadcasts       : %-18lu ║\n", broadcastMessages);
    Serial.printf("║ Parse Errors     : %-18lu ║\n", parseErrors);
    Serial.printf("║ MQTT Connected   : %-18s ║\n", mqtt.connected() ? "Yes" : "No");
    Serial.printf("║ Free Heap        : %-14lu bytes ║\n", ESP.getFreeHeap());
    Serial.println("╚══════════════════════════════════════╝");
    
    if (lastSensorData.valid) {
        Serial.println("\n[Last Sensor Data]");
        Serial.printf("  Device: %s\n", lastSensorData.device_id.c_str());
        Serial.printf("  Temp: %.1f°C, Humidity: %.1f%%\n", 
                     lastSensorData.temperature, lastSensorData.humidity);
    }
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    // Initialize GPIOs
    pinMode(LED_BUILTIN_PIN, OUTPUT);
    pinMode(LED_MESSAGE, OUTPUT);
    
    Serial.println("\n╔══════════════════════════════════════╗");
    Serial.println("║    ESP32 MQTT Subscriber Demo        ║");
    Serial.println("║   Modul 13: Network Communication    ║");
    Serial.println("╚══════════════════════════════════════╝\n");
    
    // Initialize last sensor data
    lastSensorData.valid = false;
    
    // Connect to WiFi
    connectWiFi();
    
    // Configure MQTT
    mqtt.setServer(mqtt_server, mqtt_port);
    mqtt.setCallback(mqttCallback);
    mqtt.setBufferSize(1024);
    
    // Connect to MQTT
    connectMQTT();
    
    Serial.println("\n[Ready] Listening for messages...");
    Serial.println("[Tip] Type 'help' for available commands");
}

void loop() {
    // Maintain MQTT connection
    if (!mqtt.connected()) {
        Serial.println("[MQTT] Connection lost, reconnecting...");
        connectMQTT();
    }
    mqtt.loop();
    
    // Handle serial commands
    if (Serial.available()) {
        String cmd = Serial.readStringUntil('\n');
        cmd.trim();
        cmd.toLowerCase();
        
        if (cmd == "stats") {
            printStats();
        } else if (cmd == "status") {
            Serial.printf("[Status] MQTT: %s | WiFi: %s | Messages: %lu\n",
                         mqtt.connected() ? "Connected" : "Disconnected",
                         WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected",
                         messageCount);
        } else if (cmd == "last") {
            if (lastSensorData.valid) {
                Serial.printf("[Last Data] %s: Temp=%.1f°C, Hum=%.1f%%, Light=%d\n",
                             lastSensorData.device_id.c_str(),
                             lastSensorData.temperature,
                             lastSensorData.humidity,
                             lastSensorData.light);
            } else {
                Serial.println("[Last Data] No sensor data received yet");
            }
        } else if (cmd == "reconnect") {
            Serial.println("[Manual] Reconnecting MQTT...");
            mqtt.disconnect();
            connectMQTT();
        } else if (cmd == "clear") {
            messageCount = 0;
            sensorMessages = 0;
            commandMessages = 0;
            broadcastMessages = 0;
            parseErrors = 0;
            Serial.println("[Stats] Cleared");
        } else if (cmd == "help") {
            Serial.println("\nCommands:");
            Serial.println("  stats     - Show message statistics");
            Serial.println("  status    - Show connection status");
            Serial.println("  last      - Show last sensor data");
            Serial.println("  reconnect - Reconnect to MQTT broker");
            Serial.println("  clear     - Clear statistics");
            Serial.println("  help      - Show this help");
        }
    }
    
    // Periodic status LED blink
    static unsigned long lastBlink = 0;
    if (millis() - lastBlink >= 1000) {
        digitalWrite(LED_BUILTIN_PIN, !digitalRead(LED_BUILTIN_PIN));
        lastBlink = millis();
    }
}
