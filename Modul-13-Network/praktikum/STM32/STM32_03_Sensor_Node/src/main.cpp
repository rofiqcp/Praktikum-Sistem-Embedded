/*
 * STM32 Sensor Node for IoT Gateway
 * Modul 13: Network Communication
 * 
 * Demonstrates:
 * - Sensor data collection
 * - Periodic data transmission
 * - Event-driven reporting
 * - Power management
 */

#include <Arduino.h>
#include <ArduinoJson.h>

// Serial ports
#define ESP32_SERIAL Serial1
#define ESP32_BAUD 115200
#define DEBUG_SERIAL Serial

// GPIO pins
const int LED_PIN = PC13;
const int BUTTON_PIN = PA0;
const int TEMP_PIN = PA1;
const int HUMIDITY_PIN = PA2;
const int LIGHT_PIN = PA3;
const int BATTERY_PIN = PA4;

// Node identification
const char* NODE_ID = "STM32_SENSOR_001";
const char* NODE_LOCATION = "Lab_Embedded";

// Configuration
struct NodeConfig {
    int reportInterval;      // ms
    int sampleInterval;      // ms
    int averageSamples;
    bool autoReport;
    float tempThreshold;     // Alert threshold
    float humThreshold;
    int lightThreshold;
} config = {5000, 100, 10, true, 35.0, 80.0, 200};

// Sensor data
struct SensorReadings {
    float temperature;
    float humidity;
    int light;
    int battery;
    bool buttonPressed;
    
    // Averaging
    float tempSum;
    float humSum;
    long lightSum;
    int sampleCount;
} sensors = {0, 0, 0, 100, false, 0, 0, 0, 0};

// Statistics
struct NodeStats {
    unsigned long reportsent;
    unsigned long alerts;
    unsigned long buttonEvents;
    unsigned long uptime;
} stats = {0, 0, 0, 0};

// Timing
unsigned long lastReport = 0;
unsigned long lastSample = 0;
unsigned long lastHeartbeat = 0;

// Buffer
String rxBuffer = "";

// Simulate temperature reading (replace with actual sensor)
float readTemperatureSensor() {
    int raw = analogRead(TEMP_PIN);
    // Simulated conversion: map ADC to temperature range
    return 15.0 + (raw / 4096.0 * 25.0) + (random(-10, 10) / 10.0);
}

// Simulate humidity reading
float readHumiditySensor() {
    int raw = analogRead(HUMIDITY_PIN);
    return 30.0 + (raw / 4096.0 * 50.0) + (random(-20, 20) / 10.0);
}

// Read light sensor
int readLightSensor() {
    return analogRead(LIGHT_PIN);
}

// Read battery level (simulated)
int readBatteryLevel() {
    int raw = analogRead(BATTERY_PIN);
    // Map to percentage (assuming 3.3V = 100%, 2.8V = 0%)
    int percentage = map(raw, 3482, 4095, 0, 100);
    return constrain(percentage, 0, 100);
}

void sampleSensors() {
    sensors.tempSum += readTemperatureSensor();
    sensors.humSum += readHumiditySensor();
    sensors.lightSum += readLightSensor();
    sensors.sampleCount++;
    
    if (sensors.sampleCount >= config.averageSamples) {
        sensors.temperature = sensors.tempSum / sensors.sampleCount;
        sensors.humidity = sensors.humSum / sensors.sampleCount;
        sensors.light = sensors.lightSum / sensors.sampleCount;
        sensors.battery = readBatteryLevel();
        
        // Reset averaging
        sensors.tempSum = 0;
        sensors.humSum = 0;
        sensors.lightSum = 0;
        sensors.sampleCount = 0;
    }
}

void sendMessage(JsonDocument& doc) {
    String output;
    serializeJson(doc, output);
    ESP32_SERIAL.println(output);
    DEBUG_SERIAL.printf("[TX] %s\n", output.c_str());
}

void sendSensorReport() {
    StaticJsonDocument<384> doc;
    
    doc["type"] = "sensors";
    doc["node_id"] = NODE_ID;
    doc["location"] = NODE_LOCATION;
    
    JsonObject data = doc.createNestedObject("data");
    data["temp"] = round(sensors.temperature * 10) / 10.0;
    data["hum"] = round(sensors.humidity * 10) / 10.0;
    data["light"] = sensors.light;
    data["bat"] = sensors.battery;
    data["button"] = sensors.buttonPressed;
    
    doc["timestamp"] = millis();
    doc["seq"] = stats.reportsent;
    
    sendMessage(doc);
    stats.reportsent++;
}

void sendAlert(const char* alertType, float value, float threshold) {
    StaticJsonDocument<256> doc;
    
    doc["type"] = "alert";
    doc["node_id"] = NODE_ID;
    doc["alert"] = alertType;
    doc["value"] = value;
    doc["threshold"] = threshold;
    doc["timestamp"] = millis();
    
    sendMessage(doc);
    stats.alerts++;
    
    // Visual alert
    for (int i = 0; i < 3; i++) {
        digitalWrite(LED_PIN, LOW);
        delay(100);
        digitalWrite(LED_PIN, HIGH);
        delay(100);
    }
}

void sendHeartbeat() {
    StaticJsonDocument<192> doc;
    
    doc["type"] = "heartbeat";
    doc["node_id"] = NODE_ID;
    doc["uptime"] = millis() / 1000;
    doc["reports"] = stats.reportsent;
    doc["alerts"] = stats.alerts;
    doc["battery"] = sensors.battery;
    
    sendMessage(doc);
}

void sendStatus() {
    StaticJsonDocument<384> doc;
    
    doc["type"] = "status";
    doc["node_id"] = NODE_ID;
    doc["location"] = NODE_LOCATION;
    
    JsonObject cfg = doc.createNestedObject("config");
    cfg["report_interval"] = config.reportInterval;
    cfg["sample_interval"] = config.sampleInterval;
    cfg["avg_samples"] = config.averageSamples;
    cfg["auto_report"] = config.autoReport;
    
    JsonObject thresholds = doc.createNestedObject("thresholds");
    thresholds["temp"] = config.tempThreshold;
    thresholds["hum"] = config.humThreshold;
    thresholds["light"] = config.lightThreshold;
    
    JsonObject statistics = doc.createNestedObject("stats");
    statistics["reports"] = stats.reportsent;
    statistics["alerts"] = stats.alerts;
    statistics["uptime"] = millis() / 1000;
    
    sendMessage(doc);
}

void checkAlerts() {
    static unsigned long lastTempAlert = 0;
    static unsigned long lastHumAlert = 0;
    static unsigned long lastLightAlert = 0;
    
    unsigned long now = millis();
    const unsigned long ALERT_COOLDOWN = 30000;  // 30 seconds between same alerts
    
    if (sensors.temperature > config.tempThreshold && 
        now - lastTempAlert > ALERT_COOLDOWN) {
        sendAlert("high_temperature", sensors.temperature, config.tempThreshold);
        lastTempAlert = now;
    }
    
    if (sensors.humidity > config.humThreshold && 
        now - lastHumAlert > ALERT_COOLDOWN) {
        sendAlert("high_humidity", sensors.humidity, config.humThreshold);
        lastHumAlert = now;
    }
    
    if (sensors.light < config.lightThreshold && 
        now - lastLightAlert > ALERT_COOLDOWN) {
        sendAlert("low_light", sensors.light, config.lightThreshold);
        lastLightAlert = now;
    }
}

void processCommand(const String& message) {
    DEBUG_SERIAL.printf("[RX] %s\n", message.c_str());
    
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, message);
    
    if (error) {
        DEBUG_SERIAL.printf("[ERROR] JSON: %s\n", error.c_str());
        return;
    }
    
    String cmd = doc["cmd"] | "";
    
    if (cmd == "read_sensors" || cmd == "get_data") {
        sendSensorReport();
        
    } else if (cmd == "get_status") {
        sendStatus();
        
    } else if (cmd == "set_interval") {
        config.reportInterval = doc["value"] | config.reportInterval;
        DEBUG_SERIAL.printf("[Config] Report interval: %d ms\n", config.reportInterval);
        
    } else if (cmd == "set_threshold") {
        String which = doc["which"] | "";
        float value = doc["value"];
        
        if (which == "temp") config.tempThreshold = value;
        else if (which == "hum") config.humThreshold = value;
        else if (which == "light") config.lightThreshold = value;
        
    } else if (cmd == "start_report") {
        config.autoReport = true;
        DEBUG_SERIAL.println("[Config] Auto report started");
        
    } else if (cmd == "stop_report") {
        config.autoReport = false;
        DEBUG_SERIAL.println("[Config] Auto report stopped");
        
    } else if (cmd == "led_on") {
        digitalWrite(LED_PIN, LOW);
        
    } else if (cmd == "led_off") {
        digitalWrite(LED_PIN, HIGH);
        
    } else if (cmd == "ping") {
        StaticJsonDocument<64> pong;
        pong["type"] = "pong";
        pong["node_id"] = NODE_ID;
        sendMessage(pong);
    }
}

void printDebugInfo() {
    DEBUG_SERIAL.println("\n╔══════════════════════════════════════╗");
    DEBUG_SERIAL.println("║         Sensor Node Status           ║");
    DEBUG_SERIAL.println("╠══════════════════════════════════════╣");
    DEBUG_SERIAL.printf("║ Node ID     : %-22s ║\n", NODE_ID);
    DEBUG_SERIAL.printf("║ Temperature : %6.1f °C              ║\n", sensors.temperature);
    DEBUG_SERIAL.printf("║ Humidity    : %6.1f %%               ║\n", sensors.humidity);
    DEBUG_SERIAL.printf("║ Light       : %6d                 ║\n", sensors.light);
    DEBUG_SERIAL.printf("║ Battery     : %6d %%               ║\n", sensors.battery);
    DEBUG_SERIAL.printf("║ Reports Sent: %-22lu ║\n", stats.reportsent);
    DEBUG_SERIAL.printf("║ Alerts      : %-22lu ║\n", stats.alerts);
    DEBUG_SERIAL.printf("║ Uptime      : %-20lu s ║\n", millis() / 1000);
    DEBUG_SERIAL.println("╚══════════════════════════════════════╝\n");
}

void setup() {
    // Initialize GPIOs
    pinMode(LED_PIN, OUTPUT);
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    digitalWrite(LED_PIN, HIGH);
    
    // Initialize ADC pins
    pinMode(TEMP_PIN, INPUT_ANALOG);
    pinMode(HUMIDITY_PIN, INPUT_ANALOG);
    pinMode(LIGHT_PIN, INPUT_ANALOG);
    pinMode(BATTERY_PIN, INPUT_ANALOG);
    
    // Initialize serial
    Serial.begin(115200);
    ESP32_SERIAL.begin(ESP32_BAUD);
    
    delay(2000);
    
    DEBUG_SERIAL.println("\n╔══════════════════════════════════════╗");
    DEBUG_SERIAL.println("║   STM32 Sensor Node for IoT Gateway  ║");
    DEBUG_SERIAL.println("║   Modul 13: Network Communication    ║");
    DEBUG_SERIAL.println("╚══════════════════════════════════════╝\n");
    
    DEBUG_SERIAL.printf("[Node] ID: %s\n", NODE_ID);
    DEBUG_SERIAL.printf("[Node] Location: %s\n", NODE_LOCATION);
    DEBUG_SERIAL.println("[Ready] Sensor node active\n");
    
    // Send startup message
    StaticJsonDocument<128> startup;
    startup["type"] = "startup";
    startup["node_id"] = NODE_ID;
    startup["location"] = NODE_LOCATION;
    sendMessage(startup);
}

void loop() {
    unsigned long now = millis();
    
    // Sample sensors
    if (now - lastSample >= config.sampleInterval) {
        sampleSensors();
        lastSample = now;
    }
    
    // Send periodic report
    if (config.autoReport && (now - lastReport >= config.reportInterval)) {
        sendSensorReport();
        checkAlerts();
        lastReport = now;
    }
    
    // Send heartbeat every 30 seconds
    if (now - lastHeartbeat >= 30000) {
        sendHeartbeat();
        lastHeartbeat = now;
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
    
    // Button handling
    static bool lastButtonState = true;
    bool currentButtonState = digitalRead(BUTTON_PIN);
    
    if (currentButtonState != lastButtonState) {
        delay(50);  // Debounce
        if (digitalRead(BUTTON_PIN) != lastButtonState) {
            lastButtonState = currentButtonState;
            sensors.buttonPressed = !currentButtonState;
            
            if (sensors.buttonPressed) {
                stats.buttonEvents++;
                
                // Send immediate report on button press
                sendSensorReport();
            }
        }
    }
    
    // Debug serial commands
    if (DEBUG_SERIAL.available()) {
        String cmd = DEBUG_SERIAL.readStringUntil('\n');
        cmd.trim();
        
        if (cmd == "status" || cmd == "info") {
            printDebugInfo();
        } else if (cmd == "data") {
            sendSensorReport();
        } else if (cmd == "start") {
            config.autoReport = true;
            DEBUG_SERIAL.println("[Auto] Started");
        } else if (cmd == "stop") {
            config.autoReport = false;
            DEBUG_SERIAL.println("[Auto] Stopped");
        }
    }
}
