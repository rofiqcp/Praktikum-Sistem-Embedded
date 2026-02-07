/*
 * STM32 Data Logger with ESP32 Upload
 * Modul 13: Network Communication
 * 
 * Demonstrates:
 * - Local data logging in RAM buffer
 * - Batch upload to ESP32 gateway
 * - Circular buffer management
 * - Data compression (delta encoding)
 */

#include <Arduino.h>
#include <ArduinoJson.h>

// Serial configuration
#define ESP32_SERIAL Serial1
#define ESP32_BAUD 115200
#define DEBUG_SERIAL Serial

// GPIO pins
const int LED_PIN = PC13;
const int TEMP_PIN = PA0;
const int LIGHT_PIN = PA1;

// Logging configuration
const int LOG_BUFFER_SIZE = 100;  // Number of entries
const int UPLOAD_BATCH_SIZE = 10; // Entries per upload

// Log entry structure
struct LogEntry {
    unsigned long timestamp;
    float temperature;
    float humidity;
    int light;
    uint8_t flags;  // Bit flags for events
};

// Circular buffer for logs
LogEntry logBuffer[LOG_BUFFER_SIZE];
int logHead = 0;  // Write position
int logTail = 0;  // Read position (for upload)
int logCount = 0; // Number of entries

// Logger state
struct LoggerState {
    bool logging;
    int logInterval;
    unsigned long lastLog;
    unsigned long totalLogs;
    unsigned long uploadedLogs;
    bool pendingUpload;
} logger = {true, 1000, 0, 0, 0, false};

// Delta encoding for compression
struct DeltaState {
    float lastTemp;
    float lastHum;
    int lastLight;
    bool initialized;
} delta = {0, 0, 0, false};

// Buffer
String rxBuffer = "";

// Simulate sensor readings
float readTemperature() {
    int raw = analogRead(TEMP_PIN);
    return 15.0 + (raw / 4096.0 * 25.0) + (random(-10, 10) / 10.0);
}

float readHumidity() {
    return 40.0 + (random(0, 400) / 10.0);
}

int readLight() {
    return analogRead(LIGHT_PIN);
}

void addLogEntry() {
    LogEntry entry;
    entry.timestamp = millis();
    entry.temperature = readTemperature();
    entry.humidity = readHumidity();
    entry.light = readLight();
    entry.flags = 0;
    
    // Check for alert conditions
    if (entry.temperature > 35.0) entry.flags |= 0x01;
    if (entry.humidity > 80.0) entry.flags |= 0x02;
    if (entry.light < 100) entry.flags |= 0x04;
    
    // Store in circular buffer
    logBuffer[logHead] = entry;
    logHead = (logHead + 1) % LOG_BUFFER_SIZE;
    
    if (logCount < LOG_BUFFER_SIZE) {
        logCount++;
    } else {
        // Buffer full, advance tail
        logTail = (logTail + 1) % LOG_BUFFER_SIZE;
    }
    
    logger.totalLogs++;
    
    // LED blink on log
    digitalWrite(LED_PIN, LOW);
    delay(10);
    digitalWrite(LED_PIN, HIGH);
}

void sendMessage(JsonDocument& doc) {
    String output;
    serializeJson(doc, output);
    ESP32_SERIAL.println(output);
    DEBUG_SERIAL.printf("[TX] %s\n", output.c_str());
}

void uploadBatch() {
    if (logCount == 0) {
        DEBUG_SERIAL.println("[Upload] No data to upload");
        return;
    }
    
    int batchSize = min(UPLOAD_BATCH_SIZE, logCount);
    
    StaticJsonDocument<1024> doc;
    doc["type"] = "log_batch";
    doc["batch_size"] = batchSize;
    doc["total_pending"] = logCount;
    doc["timestamp"] = millis();
    
    JsonArray entries = doc.createNestedArray("entries");
    
    int idx = logTail;
    for (int i = 0; i < batchSize; i++) {
        JsonObject entry = entries.createNestedObject();
        entry["ts"] = logBuffer[idx].timestamp;
        entry["t"] = round(logBuffer[idx].temperature * 10) / 10.0;
        entry["h"] = round(logBuffer[idx].humidity * 10) / 10.0;
        entry["l"] = logBuffer[idx].light;
        entry["f"] = logBuffer[idx].flags;
        
        idx = (idx + 1) % LOG_BUFFER_SIZE;
    }
    
    sendMessage(doc);
    
    // Update tail after successful send
    logTail = idx;
    logCount -= batchSize;
    logger.uploadedLogs += batchSize;
    
    DEBUG_SERIAL.printf("[Upload] Sent %d entries, %d remaining\n", batchSize, logCount);
}

void uploadDelta() {
    // Upload using delta encoding (more compact)
    if (logCount == 0) return;
    
    int batchSize = min(UPLOAD_BATCH_SIZE, logCount);
    
    StaticJsonDocument<512> doc;
    doc["type"] = "log_delta";
    doc["batch_size"] = batchSize;
    
    if (!delta.initialized) {
        // Send first entry as baseline
        LogEntry& first = logBuffer[logTail];
        doc["baseline"]["ts"] = first.timestamp;
        doc["baseline"]["t"] = first.temperature;
        doc["baseline"]["h"] = first.humidity;
        doc["baseline"]["l"] = first.light;
        
        delta.lastTemp = first.temperature;
        delta.lastHum = first.humidity;
        delta.lastLight = first.light;
        delta.initialized = true;
        
        logTail = (logTail + 1) % LOG_BUFFER_SIZE;
        logCount--;
        batchSize--;
    }
    
    JsonArray deltas = doc.createNestedArray("deltas");
    
    int idx = logTail;
    for (int i = 0; i < batchSize; i++) {
        JsonObject d = deltas.createNestedObject();
        
        // Delta values (differences from last)
        float dTemp = logBuffer[idx].temperature - delta.lastTemp;
        float dHum = logBuffer[idx].humidity - delta.lastHum;
        int dLight = logBuffer[idx].light - delta.lastLight;
        
        d["dt"] = (int)(dTemp * 10);  // Delta temp * 10 (as int)
        d["dh"] = (int)(dHum * 10);   // Delta humidity * 10
        d["dl"] = dLight;              // Delta light
        d["f"] = logBuffer[idx].flags;
        
        // Update last values
        delta.lastTemp = logBuffer[idx].temperature;
        delta.lastHum = logBuffer[idx].humidity;
        delta.lastLight = logBuffer[idx].light;
        
        idx = (idx + 1) % LOG_BUFFER_SIZE;
    }
    
    sendMessage(doc);
    
    logTail = idx;
    logCount -= batchSize;
    logger.uploadedLogs += batchSize;
}

void uploadAll() {
    DEBUG_SERIAL.printf("[Upload] Starting bulk upload of %d entries\n", logCount);
    
    while (logCount > 0) {
        uploadBatch();
        delay(100);  // Small delay between batches
    }
    
    DEBUG_SERIAL.println("[Upload] Complete");
}

void sendStatus() {
    StaticJsonDocument<256> doc;
    
    doc["type"] = "logger_status";
    doc["logging"] = logger.logging;
    doc["interval"] = logger.logInterval;
    doc["buffer_used"] = logCount;
    doc["buffer_size"] = LOG_BUFFER_SIZE;
    doc["buffer_pct"] = (logCount * 100) / LOG_BUFFER_SIZE;
    doc["total_logs"] = logger.totalLogs;
    doc["uploaded"] = logger.uploadedLogs;
    doc["uptime"] = millis() / 1000;
    
    sendMessage(doc);
}

void processCommand(const String& message) {
    DEBUG_SERIAL.printf("[RX] %s\n", message.c_str());
    
    StaticJsonDocument<128> doc;
    DeserializationError error = deserializeJson(doc, message);
    
    if (error) return;
    
    String cmd = doc["cmd"] | "";
    
    if (cmd == "start_log") {
        logger.logging = true;
        DEBUG_SERIAL.println("[Logger] Started");
        
    } else if (cmd == "stop_log") {
        logger.logging = false;
        DEBUG_SERIAL.println("[Logger] Stopped");
        
    } else if (cmd == "upload" || cmd == "upload_batch") {
        uploadBatch();
        
    } else if (cmd == "upload_all") {
        uploadAll();
        
    } else if (cmd == "upload_delta") {
        uploadDelta();
        
    } else if (cmd == "get_status") {
        sendStatus();
        
    } else if (cmd == "set_interval") {
        logger.logInterval = doc["value"] | logger.logInterval;
        DEBUG_SERIAL.printf("[Logger] Interval: %d ms\n", logger.logInterval);
        
    } else if (cmd == "clear_buffer") {
        logHead = 0;
        logTail = 0;
        logCount = 0;
        delta.initialized = false;
        DEBUG_SERIAL.println("[Logger] Buffer cleared");
        
    } else if (cmd == "get_latest") {
        if (logCount > 0) {
            int lastIdx = (logHead - 1 + LOG_BUFFER_SIZE) % LOG_BUFFER_SIZE;
            
            StaticJsonDocument<128> latest;
            latest["type"] = "latest";
            latest["ts"] = logBuffer[lastIdx].timestamp;
            latest["t"] = logBuffer[lastIdx].temperature;
            latest["h"] = logBuffer[lastIdx].humidity;
            latest["l"] = logBuffer[lastIdx].light;
            
            sendMessage(latest);
        }
    }
}

void printStatus() {
    DEBUG_SERIAL.println("\n╔══════════════════════════════════════╗");
    DEBUG_SERIAL.println("║          Data Logger Status          ║");
    DEBUG_SERIAL.println("╠══════════════════════════════════════╣");
    DEBUG_SERIAL.printf("║ Logging        : %-19s ║\n", logger.logging ? "Active" : "Stopped");
    DEBUG_SERIAL.printf("║ Interval       : %-16d ms ║\n", logger.logInterval);
    DEBUG_SERIAL.printf("║ Buffer Used    : %d / %d (%d%%)        ║\n", 
                       logCount, LOG_BUFFER_SIZE, (logCount * 100) / LOG_BUFFER_SIZE);
    DEBUG_SERIAL.printf("║ Total Logged   : %-19lu ║\n", logger.totalLogs);
    DEBUG_SERIAL.printf("║ Total Uploaded : %-19lu ║\n", logger.uploadedLogs);
    DEBUG_SERIAL.printf("║ Uptime         : %-17lu s ║\n", millis() / 1000);
    DEBUG_SERIAL.println("╚══════════════════════════════════════╝\n");
}

void setup() {
    // Initialize GPIO
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH);
    
    // Initialize serial
    Serial.begin(115200);
    ESP32_SERIAL.begin(ESP32_BAUD);
    
    delay(2000);
    
    DEBUG_SERIAL.println("\n╔══════════════════════════════════════╗");
    DEBUG_SERIAL.println("║   STM32 Data Logger with Upload      ║");
    DEBUG_SERIAL.println("║   Modul 13: Network Communication    ║");
    DEBUG_SERIAL.println("╚══════════════════════════════════════╝\n");
    
    DEBUG_SERIAL.printf("[Logger] Buffer size: %d entries\n", LOG_BUFFER_SIZE);
    DEBUG_SERIAL.printf("[Logger] Batch size: %d entries\n", UPLOAD_BATCH_SIZE);
    DEBUG_SERIAL.println("[Commands] status, start, stop, upload, clear, help");
    
    // Notify ESP32
    StaticJsonDocument<64> startup;
    startup["type"] = "logger_startup";
    startup["buffer_size"] = LOG_BUFFER_SIZE;
    sendMessage(startup);
}

void loop() {
    unsigned long now = millis();
    
    // Log data periodically
    if (logger.logging && (now - logger.lastLog >= logger.logInterval)) {
        addLogEntry();
        logger.lastLog = now;
        
        // Auto-upload when buffer is 80% full
        if (logCount > (LOG_BUFFER_SIZE * 80 / 100)) {
            DEBUG_SERIAL.println("[Logger] Buffer 80% full, auto-uploading...");
            uploadBatch();
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
    
    // Debug serial
    if (DEBUG_SERIAL.available()) {
        String cmd = DEBUG_SERIAL.readStringUntil('\n');
        cmd.trim();
        
        if (cmd == "status") {
            printStatus();
        } else if (cmd == "start") {
            logger.logging = true;
            DEBUG_SERIAL.println("[Logger] Started");
        } else if (cmd == "stop") {
            logger.logging = false;
            DEBUG_SERIAL.println("[Logger] Stopped");
        } else if (cmd == "upload") {
            uploadBatch();
        } else if (cmd == "uploadall") {
            uploadAll();
        } else if (cmd == "clear") {
            logHead = logTail = logCount = 0;
            delta.initialized = false;
            DEBUG_SERIAL.println("[Logger] Buffer cleared");
        } else if (cmd == "help") {
            DEBUG_SERIAL.println("\nCommands: status, start, stop, upload, uploadall, clear");
        }
    }
}
