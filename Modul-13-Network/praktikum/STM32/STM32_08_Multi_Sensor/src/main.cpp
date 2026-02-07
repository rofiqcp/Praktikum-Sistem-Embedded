/*
 * STM32 Multi-Sensor Hub
 * Modul 13: Network Communication
 * 
 * Demonstrates:
 * - Multiple sensor aggregation
 * - Data fusion
 * - Calibration support
 * - Reporting modes (periodic, threshold, on-demand)
 */

#include <Arduino.h>
#include <ArduinoJson.h>

// Serial configuration
#define ESP32_SERIAL Serial1
#define ESP32_BAUD 115200
#define DEBUG_SERIAL Serial

// GPIO pins
const int LED_PIN = PC13;
const int TEMP_PIN = PA0;    // NTC thermistor / LM35
const int LIGHT_PIN = PA1;   // LDR
const int POT_PIN = PA2;     // Potentiometer
const int SOIL_PIN = PA3;    // Soil moisture sensor
const int GAS_PIN = PA4;     // Gas sensor (MQ series)
const int CURRENT_PIN = PA5; // Current sensor (ACS712)

// Sensor configuration
struct SensorConfig {
    const char* name;
    const char* unit;
    int pin;
    bool enabled;
    float minVal;
    float maxVal;
    float threshold;
    float calibOffset;
    float calibScale;
};

// Sensor definitions
const int SENSOR_COUNT = 6;
SensorConfig sensors[SENSOR_COUNT] = {
    {"temperature", "°C", TEMP_PIN, true, -40, 125, 40, 0.0, 0.1},  // LM35: 10mV/°C
    {"light", "lux", LIGHT_PIN, true, 0, 10000, 500, 0.0, 10.0},
    {"potentiometer", "%", POT_PIN, true, 0, 100, 50, 0.0, 0.0244},  // 100/4095
    {"soil_moisture", "%", SOIL_PIN, true, 0, 100, 30, 0.0, 0.0244},
    {"gas", "ppm", GAS_PIN, true, 0, 1000, 200, 0.0, 0.244},
    {"current", "mA", CURRENT_PIN, true, -5000, 5000, 1000, 2047.5, 13.51}  // ACS712 5A
};

// Sensor readings
struct SensorData {
    float raw;
    float calibrated;
    float filtered;       // Moving average
    float min;
    float max;
    float sum;
    int count;
    bool alertActive;
    unsigned long lastAlert;
};

SensorData sensorData[SENSOR_COUNT];

// Moving average filter
const int FILTER_SIZE = 10;
float filterBuffer[SENSOR_COUNT][FILTER_SIZE];
int filterIndex[SENSOR_COUNT] = {0};

// System configuration
struct SystemConfig {
    int reportMode;       // 0=periodic, 1=threshold, 2=on-demand
    int reportInterval;   // ms for periodic mode
    bool aggregateMode;   // Send aggregated or individual
    bool alertEnabled;
} sysConfig = {0, 2000, true, true};

// Statistics
struct Statistics {
    unsigned long readCount;
    unsigned long reportCount;
    unsigned long alertCount;
    unsigned long uptime;
} stats = {0, 0, 0, 0};

// Buffer
String rxBuffer = "";

// Forward declarations
float readSensor(int idx);
float applyCalibration(int idx, float raw);
float applyFilter(int idx, float value);
void sendSensorData(int idx);
void sendAggregatedData();

float readSensor(int idx) {
    if (idx < 0 || idx >= SENSOR_COUNT) return 0;
    if (!sensors[idx].enabled) return 0;
    
    int rawADC = analogRead(sensors[idx].pin);
    float voltage = rawADC * 3.3 / 4095.0;
    
    return (float)rawADC;
}

float applyCalibration(int idx, float raw) {
    // calibrated = (raw - offset) * scale
    float calibrated = (raw - sensors[idx].calibOffset) * sensors[idx].calibScale;
    
    // Clamp to valid range
    calibrated = constrain(calibrated, sensors[idx].minVal, sensors[idx].maxVal);
    
    return calibrated;
}

float applyFilter(int idx, float value) {
    // Moving average filter
    filterBuffer[idx][filterIndex[idx]] = value;
    filterIndex[idx] = (filterIndex[idx] + 1) % FILTER_SIZE;
    
    float sum = 0;
    for (int i = 0; i < FILTER_SIZE; i++) {
        sum += filterBuffer[idx][i];
    }
    
    return sum / FILTER_SIZE;
}

void updateStatistics(int idx, float value) {
    SensorData& data = sensorData[idx];
    
    if (data.count == 0) {
        data.min = value;
        data.max = value;
    } else {
        if (value < data.min) data.min = value;
        if (value > data.max) data.max = value;
    }
    
    data.sum += value;
    data.count++;
}

void resetStatistics(int idx) {
    if (idx == -1) {
        // Reset all
        for (int i = 0; i < SENSOR_COUNT; i++) {
            sensorData[i].min = 0;
            sensorData[i].max = 0;
            sensorData[i].sum = 0;
            sensorData[i].count = 0;
        }
    } else {
        sensorData[idx].min = 0;
        sensorData[idx].max = 0;
        sensorData[idx].sum = 0;
        sensorData[idx].count = 0;
    }
}

void checkThreshold(int idx) {
    if (!sysConfig.alertEnabled) return;
    
    SensorData& data = sensorData[idx];
    float threshold = sensors[idx].threshold;
    
    bool shouldAlert = data.calibrated > threshold;
    
    if (shouldAlert && !data.alertActive) {
        // Alert triggered
        data.alertActive = true;
        data.lastAlert = millis();
        stats.alertCount++;
        
        // Send alert
        StaticJsonDocument<192> doc;
        doc["type"] = "alert";
        doc["sensor"] = sensors[idx].name;
        doc["value"] = data.calibrated;
        doc["threshold"] = threshold;
        doc["unit"] = sensors[idx].unit;
        doc["timestamp"] = millis();
        
        String output;
        serializeJson(doc, output);
        ESP32_SERIAL.println(output);
        
        DEBUG_SERIAL.printf("[ALERT] %s: %.2f > %.2f %s\n",
                           sensors[idx].name, data.calibrated, threshold, sensors[idx].unit);
                           
    } else if (!shouldAlert && data.alertActive) {
        // Alert cleared
        data.alertActive = false;
        
        StaticJsonDocument<128> doc;
        doc["type"] = "alert_clear";
        doc["sensor"] = sensors[idx].name;
        doc["value"] = data.calibrated;
        
        String output;
        serializeJson(doc, output);
        ESP32_SERIAL.println(output);
    }
}

void readAllSensors() {
    for (int i = 0; i < SENSOR_COUNT; i++) {
        if (!sensors[i].enabled) continue;
        
        SensorData& data = sensorData[i];
        
        data.raw = readSensor(i);
        data.calibrated = applyCalibration(i, data.raw);
        data.filtered = applyFilter(i, data.calibrated);
        
        updateStatistics(i, data.calibrated);
        
        if (sysConfig.reportMode == 1) {  // Threshold mode
            checkThreshold(i);
        }
    }
    
    stats.readCount++;
}

void sendSensorData(int idx) {
    if (idx < 0 || idx >= SENSOR_COUNT) return;
    
    StaticJsonDocument<256> doc;
    SensorData& data = sensorData[idx];
    
    doc["type"] = "sensor";
    doc["name"] = sensors[idx].name;
    doc["raw"] = data.raw;
    doc["value"] = data.calibrated;
    doc["filtered"] = data.filtered;
    doc["unit"] = sensors[idx].unit;
    doc["min"] = data.min;
    doc["max"] = data.max;
    doc["avg"] = data.count > 0 ? data.sum / data.count : 0;
    doc["alert"] = data.alertActive;
    doc["timestamp"] = millis();
    
    String output;
    serializeJson(doc, output);
    ESP32_SERIAL.println(output);
    
    stats.reportCount++;
}

void sendAggregatedData() {
    StaticJsonDocument<512> doc;
    
    doc["type"] = "sensors";
    doc["timestamp"] = millis();
    
    JsonObject values = doc.createNestedObject("values");
    JsonObject alerts = doc.createNestedObject("alerts");
    
    for (int i = 0; i < SENSOR_COUNT; i++) {
        if (!sensors[i].enabled) continue;
        
        values[sensors[i].name] = sensorData[i].filtered;
        if (sensorData[i].alertActive) {
            alerts[sensors[i].name] = true;
        }
    }
    
    doc["read_count"] = stats.readCount;
    doc["alert_count"] = stats.alertCount;
    
    String output;
    serializeJson(doc, output);
    ESP32_SERIAL.println(output);
    
    stats.reportCount++;
}

void sendStatistics() {
    StaticJsonDocument<384> doc;
    
    doc["type"] = "statistics";
    
    JsonArray arr = doc.createNestedArray("sensors");
    
    for (int i = 0; i < SENSOR_COUNT; i++) {
        if (!sensors[i].enabled) continue;
        
        JsonObject sensor = arr.createNestedObject();
        sensor["name"] = sensors[i].name;
        sensor["min"] = sensorData[i].min;
        sensor["max"] = sensorData[i].max;
        sensor["avg"] = sensorData[i].count > 0 ? sensorData[i].sum / sensorData[i].count : 0;
        sensor["count"] = sensorData[i].count;
    }
    
    doc["total_reads"] = stats.readCount;
    doc["total_reports"] = stats.reportCount;
    doc["total_alerts"] = stats.alertCount;
    doc["uptime"] = millis() / 1000;
    
    String output;
    serializeJson(doc, output);
    ESP32_SERIAL.println(output);
}

void processCommand(const String& message) {
    DEBUG_SERIAL.printf("[RX] %s\n", message.c_str());
    
    StaticJsonDocument<256> doc;
    if (deserializeJson(doc, message)) return;
    
    String cmd = doc["cmd"] | "";
    
    if (cmd == "read_all") {
        readAllSensors();
        sendAggregatedData();
        
    } else if (cmd == "read") {
        String sensor = doc["sensor"] | "";
        for (int i = 0; i < SENSOR_COUNT; i++) {
            if (sensor == sensors[i].name) {
                sensorData[i].raw = readSensor(i);
                sensorData[i].calibrated = applyCalibration(i, sensorData[i].raw);
                sendSensorData(i);
                break;
            }
        }
        
    } else if (cmd == "get_stats") {
        sendStatistics();
        
    } else if (cmd == "reset_stats") {
        resetStatistics(-1);
        stats.readCount = 0;
        stats.reportCount = 0;
        stats.alertCount = 0;
        DEBUG_SERIAL.println("[Stats] Reset");
        
    } else if (cmd == "set_mode") {
        int mode = doc["mode"] | 0;
        sysConfig.reportMode = constrain(mode, 0, 2);
        DEBUG_SERIAL.printf("[Config] Report mode: %d\n", sysConfig.reportMode);
        
    } else if (cmd == "set_interval") {
        int interval = doc["interval"] | 2000;
        sysConfig.reportInterval = constrain(interval, 100, 60000);
        DEBUG_SERIAL.printf("[Config] Interval: %d ms\n", sysConfig.reportInterval);
        
    } else if (cmd == "set_threshold") {
        String sensor = doc["sensor"] | "";
        float threshold = doc["threshold"] | 0;
        for (int i = 0; i < SENSOR_COUNT; i++) {
            if (sensor == sensors[i].name) {
                sensors[i].threshold = threshold;
                DEBUG_SERIAL.printf("[Config] %s threshold: %.2f\n", sensor.c_str(), threshold);
                break;
            }
        }
        
    } else if (cmd == "calibrate") {
        String sensor = doc["sensor"] | "";
        float offset = doc["offset"] | 0;
        float scale = doc["scale"] | 1;
        for (int i = 0; i < SENSOR_COUNT; i++) {
            if (sensor == sensors[i].name) {
                sensors[i].calibOffset = offset;
                sensors[i].calibScale = scale;
                DEBUG_SERIAL.printf("[Calib] %s: offset=%.2f scale=%.4f\n", 
                                   sensor.c_str(), offset, scale);
                break;
            }
        }
        
    } else if (cmd == "enable") {
        String sensor = doc["sensor"] | "";
        bool enable = doc["enable"] | true;
        for (int i = 0; i < SENSOR_COUNT; i++) {
            if (sensor == sensors[i].name) {
                sensors[i].enabled = enable;
                DEBUG_SERIAL.printf("[Config] %s: %s\n", sensor.c_str(), 
                                   enable ? "enabled" : "disabled");
                break;
            }
        }
        
    } else if (cmd == "get_config") {
        StaticJsonDocument<512> resp;
        resp["type"] = "config";
        resp["mode"] = sysConfig.reportMode;
        resp["interval"] = sysConfig.reportInterval;
        resp["aggregate"] = sysConfig.aggregateMode;
        resp["alerts"] = sysConfig.alertEnabled;
        
        JsonArray arr = resp.createNestedArray("sensors");
        for (int i = 0; i < SENSOR_COUNT; i++) {
            JsonObject s = arr.createNestedObject();
            s["name"] = sensors[i].name;
            s["enabled"] = sensors[i].enabled;
            s["threshold"] = sensors[i].threshold;
            s["unit"] = sensors[i].unit;
        }
        
        String output;
        serializeJson(resp, output);
        ESP32_SERIAL.println(output);
    }
}

void printStatus() {
    DEBUG_SERIAL.println("\n╔════════════════════════════════════════════════════╗");
    DEBUG_SERIAL.println("║             Multi-Sensor Hub Status                ║");
    DEBUG_SERIAL.println("╠════════════════════════════════════════════════════╣");
    
    for (int i = 0; i < SENSOR_COUNT; i++) {
        if (!sensors[i].enabled) continue;
        DEBUG_SERIAL.printf("║ %-12s: %7.2f %-5s (raw: %4.0f) %s ║\n",
                           sensors[i].name,
                           sensorData[i].calibrated,
                           sensors[i].unit,
                           sensorData[i].raw,
                           sensorData[i].alertActive ? "ALERT!" : "");
    }
    
    DEBUG_SERIAL.println("╠════════════════════════════════════════════════════╣");
    DEBUG_SERIAL.printf("║ Report Mode: %d  Interval: %d ms                    ║\n",
                       sysConfig.reportMode, sysConfig.reportInterval);
    DEBUG_SERIAL.printf("║ Reads: %-8lu  Reports: %-8lu  Alerts: %-6lu ║\n",
                       stats.readCount, stats.reportCount, stats.alertCount);
    DEBUG_SERIAL.println("╚════════════════════════════════════════════════════╝\n");
}

void setup() {
    // Initialize outputs
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH);
    
    // Initialize analog inputs
    for (int i = 0; i < SENSOR_COUNT; i++) {
        pinMode(sensors[i].pin, INPUT_ANALOG);
    }
    
    // Initialize filter buffers
    for (int i = 0; i < SENSOR_COUNT; i++) {
        for (int j = 0; j < FILTER_SIZE; j++) {
            filterBuffer[i][j] = 0;
        }
    }
    
    // Initialize serial
    Serial.begin(115200);
    ESP32_SERIAL.begin(ESP32_BAUD);
    
    delay(2000);
    
    DEBUG_SERIAL.println("\n╔════════════════════════════════════════════════════╗");
    DEBUG_SERIAL.println("║          STM32 Multi-Sensor Hub                    ║");
    DEBUG_SERIAL.println("║          Modul 13: Network Communication           ║");
    DEBUG_SERIAL.println("╚════════════════════════════════════════════════════╝\n");
    
    DEBUG_SERIAL.println("[Commands] status, read, stats, mode <0/1/2>, help\n");
    
    // Initial read
    readAllSensors();
}

void loop() {
    static unsigned long lastRead = 0;
    static unsigned long lastReport = 0;
    
    // Read sensors at fixed interval
    if (millis() - lastRead >= 100) {
        readAllSensors();
        lastRead = millis();
    }
    
    // Report based on mode
    if (sysConfig.reportMode == 0) {  // Periodic
        if (millis() - lastReport >= sysConfig.reportInterval) {
            if (sysConfig.aggregateMode) {
                sendAggregatedData();
            } else {
                for (int i = 0; i < SENSOR_COUNT; i++) {
                    if (sensors[i].enabled) sendSensorData(i);
                }
            }
            lastReport = millis();
        }
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
    
    // Debug serial commands
    if (DEBUG_SERIAL.available()) {
        String cmd = DEBUG_SERIAL.readStringUntil('\n');
        cmd.trim();
        
        if (cmd == "status") {
            printStatus();
        } else if (cmd == "read") {
            readAllSensors();
            printStatus();
        } else if (cmd == "stats") {
            sendStatistics();
        } else if (cmd.startsWith("mode ")) {
            sysConfig.reportMode = cmd.substring(5).toInt();
            DEBUG_SERIAL.printf("[Mode] Set to %d\n", sysConfig.reportMode);
        } else if (cmd == "send") {
            sendAggregatedData();
        }
    }
    
    // LED indicator
    static unsigned long lastLed = 0;
    int blinkInterval = stats.alertCount > 0 ? 200 : 1000;
    if (millis() - lastLed >= blinkInterval) {
        digitalWrite(LED_PIN, !digitalRead(LED_PIN));
        lastLed = millis();
    }
    
    delay(1);
}
