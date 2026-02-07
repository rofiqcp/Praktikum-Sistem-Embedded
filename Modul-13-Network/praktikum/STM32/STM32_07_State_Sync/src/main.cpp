/*
 * STM32 State Synchronization System
 * Modul 13: Network Communication
 * 
 * Demonstrates:
 * - Bidirectional state synchronization
 * - Conflict resolution
 * - Version tracking
 * - State persistence simulation
 */

#include <Arduino.h>
#include <ArduinoJson.h>

// Serial configuration
#define ESP32_SERIAL Serial1
#define ESP32_BAUD 115200
#define DEBUG_SERIAL Serial

// GPIO pins
const int LED_PIN = PC13;
const int LED1_PIN = PA4;
const int LED2_PIN = PA5;
const int LED3_PIN = PA6;
const int BUTTON_PIN = PA0;

// Device configuration
struct DeviceConfig {
    char name[32];
    int interval;      // Update interval ms
    int brightness;    // 0-100
    bool autoMode;
    int threshold;     // For sensors
    char timezone[8];  // UTC offset
    uint32_t version;  // Config version
    uint32_t checksum; // Validation
};

// Runtime state
struct DeviceState {
    bool led1;
    bool led2;
    bool led3;
    int pwmValue;      // 0-255
    int sensorValue;
    bool alarmActive;
    char mode[16];
    uint32_t version;  // State version
    unsigned long lastUpdate;
};

// Sync status
struct SyncStatus {
    bool syncing;
    bool dirty;        // Local changes pending
    unsigned long lastSyncTime;
    unsigned long lastRemoteUpdate;
    int syncCount;
    int conflictCount;
    char lastError[32];
};

DeviceConfig config = {"STM32_Node_01", 1000, 50, false, 512, "+7", 1, 0};
DeviceState localState = {false, false, false, 128, 0, false, "manual", 0, 0};
DeviceState remoteState = {false, false, false, 128, 0, false, "manual", 0, 0};
SyncStatus syncStatus = {false, false, 0, 0, 0, 0, ""};

// Buffer
String rxBuffer = "";

// Calculate simple checksum
uint32_t calculateChecksum(DeviceConfig& cfg) {
    uint32_t sum = 0;
    sum += cfg.interval;
    sum += cfg.brightness;
    sum += cfg.autoMode ? 1 : 0;
    sum += cfg.threshold;
    sum += cfg.version;
    for (int i = 0; i < strlen(cfg.name); i++) {
        sum += cfg.name[i];
    }
    return sum;
}

bool validateConfig(DeviceConfig& cfg) {
    return cfg.checksum == calculateChecksum(cfg);
}

void applyState() {
    // Apply local state to hardware
    digitalWrite(LED1_PIN, localState.led1);
    digitalWrite(LED2_PIN, localState.led2);
    digitalWrite(LED3_PIN, localState.led3);
    analogWrite(LED_PIN, 255 - localState.pwmValue);  // Inverted for PC13
    
    DEBUG_SERIAL.printf("[State] Applied - LED1:%d LED2:%d LED3:%d PWM:%d\n",
                       localState.led1, localState.led2, localState.led3, localState.pwmValue);
}

void markStateDirty() {
    localState.version++;
    localState.lastUpdate = millis();
    syncStatus.dirty = true;
}

void sendStateUpdate() {
    StaticJsonDocument<384> doc;
    
    doc["type"] = "state_update";
    doc["version"] = localState.version;
    
    JsonObject state = doc.createNestedObject("state");
    state["led1"] = localState.led1;
    state["led2"] = localState.led2;
    state["led3"] = localState.led3;
    state["pwm"] = localState.pwmValue;
    state["sensor"] = localState.sensorValue;
    state["alarm"] = localState.alarmActive;
    state["mode"] = localState.mode;
    
    doc["timestamp"] = millis();
    doc["dirty"] = syncStatus.dirty;
    
    String output;
    serializeJson(doc, output);
    ESP32_SERIAL.println(output);
    
    DEBUG_SERIAL.printf("[Sync] Sent state v%lu\n", localState.version);
}

void sendConfigUpdate() {
    StaticJsonDocument<256> doc;
    
    doc["type"] = "config_update";
    doc["version"] = config.version;
    
    JsonObject cfg = doc.createNestedObject("config");
    cfg["name"] = config.name;
    cfg["interval"] = config.interval;
    cfg["brightness"] = config.brightness;
    cfg["auto_mode"] = config.autoMode;
    cfg["threshold"] = config.threshold;
    cfg["timezone"] = config.timezone;
    
    config.checksum = calculateChecksum(config);
    cfg["checksum"] = config.checksum;
    
    String output;
    serializeJson(doc, output);
    ESP32_SERIAL.println(output);
    
    DEBUG_SERIAL.printf("[Sync] Sent config v%lu\n", config.version);
}

void requestSync() {
    StaticJsonDocument<64> doc;
    doc["type"] = "sync_request";
    doc["local_version"] = localState.version;
    doc["config_version"] = config.version;
    
    String output;
    serializeJson(doc, output);
    ESP32_SERIAL.println(output);
    
    syncStatus.syncing = true;
    DEBUG_SERIAL.println("[Sync] Requested sync from ESP32");
}

void sendSyncAck(bool success, const char* message) {
    StaticJsonDocument<128> doc;
    doc["type"] = "sync_ack";
    doc["success"] = success;
    doc["message"] = message;
    doc["local_version"] = localState.version;
    
    String output;
    serializeJson(doc, output);
    ESP32_SERIAL.println(output);
}

// Conflict resolution: Last Write Wins or Custom
void resolveConflict(DeviceState& remote) {
    // Check versions
    if (remote.version > localState.version) {
        // Remote is newer - accept remote
        localState = remote;
        applyState();
        sendSyncAck(true, "Accepted remote state");
        
    } else if (remote.version == localState.version) {
        // Same version but different - use timestamp
        if (remote.lastUpdate > localState.lastUpdate) {
            localState = remote;
            applyState();
            sendSyncAck(true, "Accepted remote (newer timestamp)");
        } else {
            // Keep local, send update
            sendSyncAck(true, "Kept local (newer timestamp)");
            sendStateUpdate();
        }
        
    } else {
        // Local is newer - send update
        sendSyncAck(true, "Local version newer");
        sendStateUpdate();
    }
    
    syncStatus.conflictCount++;
}

void processCommand(const String& message) {
    DEBUG_SERIAL.printf("[RX] %s\n", message.c_str());
    
    StaticJsonDocument<512> doc;
    if (deserializeJson(doc, message)) {
        DEBUG_SERIAL.println("[Error] JSON parse failed");
        return;
    }
    
    String msgType = doc["type"] | "";
    
    if (msgType == "state_update") {
        // Incoming state from ESP32
        uint32_t remoteVersion = doc["version"];
        
        remoteState.led1 = doc["state"]["led1"] | false;
        remoteState.led2 = doc["state"]["led2"] | false;
        remoteState.led3 = doc["state"]["led3"] | false;
        remoteState.pwmValue = doc["state"]["pwm"] | 128;
        remoteState.sensorValue = doc["state"]["sensor"] | 0;
        remoteState.alarmActive = doc["state"]["alarm"] | false;
        strlcpy(remoteState.mode, doc["state"]["mode"] | "manual", sizeof(remoteState.mode));
        remoteState.version = remoteVersion;
        remoteState.lastUpdate = doc["timestamp"] | millis();
        
        syncStatus.lastRemoteUpdate = millis();
        
        // Check for conflict
        if (syncStatus.dirty && remoteVersion != localState.version) {
            DEBUG_SERIAL.println("[Sync] Conflict detected!");
            resolveConflict(remoteState);
        } else {
            // No conflict - apply remote state
            localState = remoteState;
            applyState();
            sendSyncAck(true, "State applied");
        }
        
        syncStatus.dirty = false;
        syncStatus.syncing = false;
        syncStatus.syncCount++;
        syncStatus.lastSyncTime = millis();
        
    } else if (msgType == "config_update") {
        // Incoming config from ESP32
        uint32_t remoteVersion = doc["version"];
        
        if (remoteVersion > config.version) {
            strlcpy(config.name, doc["config"]["name"] | "STM32", sizeof(config.name));
            config.interval = doc["config"]["interval"] | 1000;
            config.brightness = doc["config"]["brightness"] | 50;
            config.autoMode = doc["config"]["auto_mode"] | false;
            config.threshold = doc["config"]["threshold"] | 512;
            strlcpy(config.timezone, doc["config"]["timezone"] | "+7", sizeof(config.timezone));
            config.version = remoteVersion;
            config.checksum = calculateChecksum(config);
            
            DEBUG_SERIAL.printf("[Config] Updated to v%lu\n", config.version);
            sendSyncAck(true, "Config applied");
        } else {
            sendSyncAck(false, "Config version not newer");
        }
        
    } else if (msgType == "sync_request") {
        // ESP32 requesting sync
        sendStateUpdate();
        sendConfigUpdate();
        
    } else if (msgType == "sync_ack") {
        bool success = doc["success"];
        const char* msg = doc["message"] | "";
        DEBUG_SERIAL.printf("[Sync ACK] %s: %s\n", success ? "OK" : "FAIL", msg);
        
    } else if (msgType == "cmd") {
        // Direct command
        String cmd = doc["cmd"] | "";
        
        if (cmd == "led1_on") {
            localState.led1 = true;
            markStateDirty();
            applyState();
        } else if (cmd == "led1_off") {
            localState.led1 = false;
            markStateDirty();
            applyState();
        } else if (cmd == "led2_toggle") {
            localState.led2 = !localState.led2;
            markStateDirty();
            applyState();
        } else if (cmd == "led3_toggle") {
            localState.led3 = !localState.led3;
            markStateDirty();
            applyState();
        } else if (cmd == "set_pwm") {
            localState.pwmValue = doc["value"] | 128;
            markStateDirty();
            applyState();
        } else if (cmd == "set_mode") {
            strlcpy(localState.mode, doc["value"] | "manual", sizeof(localState.mode));
            markStateDirty();
        } else if (cmd == "get_state") {
            sendStateUpdate();
        } else if (cmd == "get_config") {
            sendConfigUpdate();
        } else if (cmd == "force_sync") {
            requestSync();
        }
    }
}

void sendStatus() {
    StaticJsonDocument<256> doc;
    
    doc["type"] = "sync_status";
    doc["syncing"] = syncStatus.syncing;
    doc["dirty"] = syncStatus.dirty;
    doc["last_sync"] = (millis() - syncStatus.lastSyncTime) / 1000;
    doc["sync_count"] = syncStatus.syncCount;
    doc["conflicts"] = syncStatus.conflictCount;
    doc["local_version"] = localState.version;
    doc["config_version"] = config.version;
    
    String output;
    serializeJson(doc, output);
    ESP32_SERIAL.println(output);
}

void printStatus() {
    DEBUG_SERIAL.println("\n╔═══════════════════════════════════════════╗");
    DEBUG_SERIAL.println("║         State Synchronization Status      ║");
    DEBUG_SERIAL.println("╠═══════════════════════════════════════════╣");
    DEBUG_SERIAL.printf("║ Device Name    : %-24s ║\n", config.name);
    DEBUG_SERIAL.printf("║ State Version  : %-24lu ║\n", localState.version);
    DEBUG_SERIAL.printf("║ Config Version : %-24lu ║\n", config.version);
    DEBUG_SERIAL.printf("║ Dirty (Pending): %-24s ║\n", syncStatus.dirty ? "YES" : "NO");
    DEBUG_SERIAL.printf("║ Syncing        : %-24s ║\n", syncStatus.syncing ? "YES" : "NO");
    DEBUG_SERIAL.printf("║ Sync Count     : %-24d ║\n", syncStatus.syncCount);
    DEBUG_SERIAL.printf("║ Conflicts      : %-24d ║\n", syncStatus.conflictCount);
    DEBUG_SERIAL.println("╠═══════════════════════════════════════════╣");
    DEBUG_SERIAL.printf("║ LED1: %-3s  LED2: %-3s  LED3: %-3s  PWM: %-3d ║\n",
                       localState.led1 ? "ON" : "OFF",
                       localState.led2 ? "ON" : "OFF",
                       localState.led3 ? "ON" : "OFF",
                       localState.pwmValue);
    DEBUG_SERIAL.printf("║ Mode: %-10s  Alarm: %-14s ║\n",
                       localState.mode,
                       localState.alarmActive ? "ACTIVE" : "Inactive");
    DEBUG_SERIAL.println("╚═══════════════════════════════════════════╝\n");
}

void setup() {
    // Initialize outputs
    pinMode(LED_PIN, OUTPUT);
    pinMode(LED1_PIN, OUTPUT);
    pinMode(LED2_PIN, OUTPUT);
    pinMode(LED3_PIN, OUTPUT);
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    
    // Initial state
    applyState();
    
    // Initialize serial
    Serial.begin(115200);
    ESP32_SERIAL.begin(ESP32_BAUD);
    
    delay(2000);
    
    DEBUG_SERIAL.println("\n╔═══════════════════════════════════════════╗");
    DEBUG_SERIAL.println("║    STM32 State Synchronization System     ║");
    DEBUG_SERIAL.println("║    Modul 13: Network Communication        ║");
    DEBUG_SERIAL.println("╚═══════════════════════════════════════════╝\n");
    
    DEBUG_SERIAL.println("[Commands] status, sync, led1, led2, led3, pwm <0-255>\n");
    
    // Initial sync request
    delay(1000);
    requestSync();
}

void loop() {
    static unsigned long lastAutoSync = 0;
    static unsigned long lastButtonCheck = 0;
    static bool lastButtonState = true;
    
    // Auto sync every interval (from config)
    if (millis() - lastAutoSync >= config.interval) {
        if (syncStatus.dirty) {
            sendStateUpdate();
        }
        lastAutoSync = millis();
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
    
    // Button handling (toggle LED1)
    if (millis() - lastButtonCheck >= 50) {
        bool buttonState = digitalRead(BUTTON_PIN);
        if (buttonState == LOW && lastButtonState == HIGH) {
            localState.led1 = !localState.led1;
            markStateDirty();
            applyState();
            DEBUG_SERIAL.println("[Button] LED1 toggled locally");
        }
        lastButtonState = buttonState;
        lastButtonCheck = millis();
    }
    
    // Simulate sensor reading
    static unsigned long lastSensor = 0;
    if (millis() - lastSensor >= 1000) {
        localState.sensorValue = analogRead(PA7);
        
        // Check threshold for alarm
        if (config.autoMode && localState.sensorValue > config.threshold) {
            if (!localState.alarmActive) {
                localState.alarmActive = true;
                markStateDirty();
                DEBUG_SERIAL.printf("[Alarm] Triggered - sensor %d > threshold %d\n",
                                   localState.sensorValue, config.threshold);
            }
        } else if (localState.alarmActive && localState.sensorValue <= config.threshold) {
            localState.alarmActive = false;
            markStateDirty();
            DEBUG_SERIAL.println("[Alarm] Cleared");
        }
        
        lastSensor = millis();
    }
    
    // Debug serial commands
    if (DEBUG_SERIAL.available()) {
        String cmd = DEBUG_SERIAL.readStringUntil('\n');
        cmd.trim();
        
        if (cmd == "status") {
            printStatus();
        } else if (cmd == "sync") {
            requestSync();
        } else if (cmd == "led1") {
            localState.led1 = !localState.led1;
            markStateDirty();
            applyState();
        } else if (cmd == "led2") {
            localState.led2 = !localState.led2;
            markStateDirty();
            applyState();
        } else if (cmd == "led3") {
            localState.led3 = !localState.led3;
            markStateDirty();
            applyState();
        } else if (cmd.startsWith("pwm ")) {
            int val = cmd.substring(4).toInt();
            localState.pwmValue = constrain(val, 0, 255);
            markStateDirty();
            applyState();
        } else if (cmd == "send") {
            sendStateUpdate();
        }
    }
    
    // LED status indicator
    static unsigned long lastLed = 0;
    int interval = syncStatus.syncing ? 100 : (syncStatus.dirty ? 300 : 1000);
    if (millis() - lastLed >= interval) {
        digitalWrite(LED_PIN, !digitalRead(LED_PIN));
        lastLed = millis();
    }
    
    delay(1);
}
