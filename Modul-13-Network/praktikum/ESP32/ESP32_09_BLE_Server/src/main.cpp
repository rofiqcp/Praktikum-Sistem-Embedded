/*
 * ESP32 BLE Server (GATT) Demo
 * Modul 13: Network Communication
 * 
 * Demonstrates:
 * - BLE GATT Server
 * - Custom Services and Characteristics
 * - Notifications
 * - Read/Write characteristics
 * - Environmental sensing profile
 */

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <ArduinoJson.h>

// BLE Server name
#define DEVICE_NAME "ESP32_BLE_Sensor"

// Custom Service UUID
#define SERVICE_UUID           "4fafc201-1fb5-459e-8fcc-c5c9c331914b"

// Characteristic UUIDs
#define CHAR_TEMPERATURE_UUID  "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define CHAR_HUMIDITY_UUID     "beb5483e-36e1-4688-b7f5-ea07361b26a9"
#define CHAR_LED_UUID          "beb5483e-36e1-4688-b7f5-ea07361b26aa"
#define CHAR_CONFIG_UUID       "beb5483e-36e1-4688-b7f5-ea07361b26ab"
#define CHAR_DATA_UUID         "beb5483e-36e1-4688-b7f5-ea07361b26ac"

// Environmental Sensing Service (standard BLE)
#define ENV_SENSE_UUID         "181A"
#define TEMP_CHAR_UUID         "2A6E"  // Temperature
#define HUM_CHAR_UUID          "2A6F"  // Humidity

// GPIO pins
const int LED_PIN = 2;

// BLE Server objects
BLEServer* pServer = NULL;
BLECharacteristic* pTempChar = NULL;
BLECharacteristic* pHumChar = NULL;
BLECharacteristic* pLedChar = NULL;
BLECharacteristic* pConfigChar = NULL;
BLECharacteristic* pDataChar = NULL;

// State
bool deviceConnected = false;
bool oldDeviceConnected = false;
unsigned long lastNotify = 0;
int notifyInterval = 1000;
bool notificationsEnabled = true;
unsigned long connectionCount = 0;

// Sensor data
float temperature = 25.0;
float humidity = 50.0;
bool ledState = false;

// Update sensor values (simulated)
void updateSensors() {
    temperature = 20.0 + (random(0, 150) / 10.0);
    humidity = 40.0 + (random(0, 400) / 10.0);
}

// Server callbacks
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
        deviceConnected = true;
        connectionCount++;
        Serial.println("[BLE] Client connected!");
        digitalWrite(LED_PIN, HIGH);
    }
    
    void onDisconnect(BLEServer* pServer) {
        deviceConnected = false;
        Serial.println("[BLE] Client disconnected!");
        digitalWrite(LED_PIN, LOW);
    }
};

// LED characteristic callbacks
class LedCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic* pCharacteristic) {
        std::string value = pCharacteristic->getValue();
        
        if (value.length() > 0) {
            ledState = (value[0] == 0x01);
            digitalWrite(LED_PIN, ledState ? HIGH : LOW);
            Serial.printf("[BLE] LED set to %s\n", ledState ? "ON" : "OFF");
        }
    }
    
    void onRead(BLECharacteristic* pCharacteristic) {
        uint8_t val = ledState ? 0x01 : 0x00;
        pCharacteristic->setValue(&val, 1);
        Serial.println("[BLE] LED state read");
    }
};

// Config characteristic callbacks
class ConfigCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic* pCharacteristic) {
        std::string value = pCharacteristic->getValue();
        
        if (value.length() >= 2) {
            // First byte: notification enable (0/1)
            // Second byte: interval in 100ms units (1-100)
            notificationsEnabled = (value[0] == 0x01);
            if (value.length() > 1 && value[1] >= 1 && value[1] <= 100) {
                notifyInterval = value[1] * 100;
            }
            
            Serial.printf("[BLE] Config: Notifications=%s, Interval=%dms\n",
                         notificationsEnabled ? "ON" : "OFF", notifyInterval);
        }
    }
    
    void onRead(BLECharacteristic* pCharacteristic) {
        uint8_t config[4];
        config[0] = notificationsEnabled ? 0x01 : 0x00;
        config[1] = notifyInterval / 100;
        config[2] = (connectionCount >> 8) & 0xFF;
        config[3] = connectionCount & 0xFF;
        pCharacteristic->setValue(config, 4);
    }
};

// Data characteristic callbacks (JSON)
class DataCallbacks: public BLECharacteristicCallbacks {
    void onRead(BLECharacteristic* pCharacteristic) {
        updateSensors();
        
        StaticJsonDocument<256> doc;
        doc["temp"] = temperature;
        doc["hum"] = humidity;
        doc["led"] = ledState;
        doc["uptime"] = millis() / 1000;
        doc["heap"] = ESP.getFreeHeap();
        
        char buffer[256];
        serializeJson(doc, buffer);
        pCharacteristic->setValue(buffer);
        
        Serial.println("[BLE] Data characteristic read");
    }
};

void setupBLE() {
    // Initialize BLE
    BLEDevice::init(DEVICE_NAME);
    
    // Create BLE Server
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());
    
    // Create Custom Service
    BLEService* pService = pServer->createService(SERVICE_UUID);
    
    // Temperature Characteristic (notify)
    pTempChar = pService->createCharacteristic(
        CHAR_TEMPERATURE_UUID,
        BLECharacteristic::PROPERTY_READ |
        BLECharacteristic::PROPERTY_NOTIFY
    );
    pTempChar->addDescriptor(new BLE2902());
    
    // Humidity Characteristic (notify)
    pHumChar = pService->createCharacteristic(
        CHAR_HUMIDITY_UUID,
        BLECharacteristic::PROPERTY_READ |
        BLECharacteristic::PROPERTY_NOTIFY
    );
    pHumChar->addDescriptor(new BLE2902());
    
    // LED Characteristic (read/write)
    pLedChar = pService->createCharacteristic(
        CHAR_LED_UUID,
        BLECharacteristic::PROPERTY_READ |
        BLECharacteristic::PROPERTY_WRITE
    );
    pLedChar->setCallbacks(new LedCallbacks());
    
    // Config Characteristic (read/write)
    pConfigChar = pService->createCharacteristic(
        CHAR_CONFIG_UUID,
        BLECharacteristic::PROPERTY_READ |
        BLECharacteristic::PROPERTY_WRITE
    );
    pConfigChar->setCallbacks(new ConfigCallbacks());
    
    // Data Characteristic (read - JSON)
    pDataChar = pService->createCharacteristic(
        CHAR_DATA_UUID,
        BLECharacteristic::PROPERTY_READ
    );
    pDataChar->setCallbacks(new DataCallbacks());
    
    // Start service
    pService->start();
    
    // Start advertising
    BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);  // iPhone connection issue fix
    pAdvertising->setMinPreferred(0x12);
    BLEDevice::startAdvertising();
    
    Serial.println("[BLE] Service started, advertising...");
}

void notifySensorData() {
    if (!deviceConnected || !notificationsEnabled) return;
    
    updateSensors();
    
    // Temperature as 16-bit signed integer (0.01 °C resolution)
    int16_t tempValue = (int16_t)(temperature * 100);
    pTempChar->setValue((uint8_t*)&tempValue, 2);
    pTempChar->notify();
    
    // Humidity as 16-bit unsigned integer (0.01% resolution)
    uint16_t humValue = (uint16_t)(humidity * 100);
    pHumChar->setValue((uint8_t*)&humValue, 2);
    pHumChar->notify();
    
    Serial.printf("[BLE] Notified: Temp=%.1f°C, Hum=%.1f%%\n", temperature, humidity);
}

void printStatus() {
    Serial.println("\n╔══════════════════════════════════════╗");
    Serial.println("║          BLE Server Status           ║");
    Serial.println("╠══════════════════════════════════════╣");
    Serial.printf("║ Device Name    : %-19s ║\n", DEVICE_NAME);
    Serial.printf("║ Connected      : %-19s ║\n", deviceConnected ? "Yes" : "No");
    Serial.printf("║ Connections    : %-19lu ║\n", connectionCount);
    Serial.printf("║ Notifications  : %-19s ║\n", notificationsEnabled ? "Enabled" : "Disabled");
    Serial.printf("║ Interval       : %-16d ms ║\n", notifyInterval);
    Serial.printf("║ Temperature    : %-18.1f°C ║\n", temperature);
    Serial.printf("║ Humidity       : %-18.1f%% ║\n", humidity);
    Serial.printf("║ LED State      : %-19s ║\n", ledState ? "ON" : "OFF");
    Serial.printf("║ Free Heap      : %-15lu B ║\n", ESP.getFreeHeap());
    Serial.println("╚══════════════════════════════════════╝");
}

void printUUIDs() {
    Serial.println("\n[BLE UUIDs]");
    Serial.printf("  Service     : %s\n", SERVICE_UUID);
    Serial.printf("  Temperature : %s\n", CHAR_TEMPERATURE_UUID);
    Serial.printf("  Humidity    : %s\n", CHAR_HUMIDITY_UUID);
    Serial.printf("  LED         : %s\n", CHAR_LED_UUID);
    Serial.printf("  Config      : %s\n", CHAR_CONFIG_UUID);
    Serial.printf("  Data (JSON) : %s\n", CHAR_DATA_UUID);
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    // Initialize GPIO
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);
    
    Serial.println("\n╔══════════════════════════════════════╗");
    Serial.println("║      ESP32 BLE Server Demo           ║");
    Serial.println("║   Modul 13: Network Communication    ║");
    Serial.println("╚══════════════════════════════════════╝\n");
    
    // Setup BLE
    setupBLE();
    
    printUUIDs();
    
    Serial.println("\n[Ready] Waiting for BLE connection...");
    Serial.println("[Tip] Use nRF Connect app to connect\n");
    Serial.println("Commands: status, uuid, led on/off, help");
}

void loop() {
    // Notify sensor data periodically
    if (millis() - lastNotify >= notifyInterval) {
        notifySensorData();
        lastNotify = millis();
    }
    
    // Handle disconnection
    if (!deviceConnected && oldDeviceConnected) {
        delay(500);  // Give the bluetooth stack time
        pServer->startAdvertising();
        Serial.println("[BLE] Advertising restarted");
        oldDeviceConnected = deviceConnected;
    }
    
    // Handle new connection
    if (deviceConnected && !oldDeviceConnected) {
        oldDeviceConnected = deviceConnected;
    }
    
    // Handle serial commands
    if (Serial.available()) {
        String cmd = Serial.readStringUntil('\n');
        cmd.trim();
        cmd.toLowerCase();
        
        if (cmd == "status") {
            printStatus();
        } else if (cmd == "uuid") {
            printUUIDs();
        } else if (cmd == "led on") {
            ledState = true;
            digitalWrite(LED_PIN, HIGH);
            Serial.println("[LED] ON");
        } else if (cmd == "led off") {
            ledState = false;
            digitalWrite(LED_PIN, LOW);
            Serial.println("[LED] OFF");
        } else if (cmd == "notify on") {
            notificationsEnabled = true;
            Serial.println("[Notify] Enabled");
        } else if (cmd == "notify off") {
            notificationsEnabled = false;
            Serial.println("[Notify] Disabled");
        } else if (cmd.startsWith("interval ")) {
            int interval = cmd.substring(9).toInt();
            if (interval >= 100 && interval <= 10000) {
                notifyInterval = interval;
                Serial.printf("[Interval] Set to %d ms\n", interval);
            }
        } else if (cmd == "help") {
            Serial.println("\nCommands:");
            Serial.println("  status     - Show status");
            Serial.println("  uuid       - Show service UUIDs");
            Serial.println("  led on/off - Control LED");
            Serial.println("  notify on/off - Enable/disable notifications");
            Serial.println("  interval N - Set notify interval (100-10000 ms)");
        }
    }
    
    delay(10);
}
