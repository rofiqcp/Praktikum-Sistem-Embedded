/*
 * ESP32 IoT Gateway - STM32 Bridge
 * Modul 13: Network Communication
 * 
 * Demonstrates:
 * - Serial communication with STM32
 * - WiFi connectivity
 * - MQTT data publishing
 * - Web dashboard
 * - Protocol bridging (Serial <-> WiFi)
 */

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// WiFi credentials
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// MQTT Broker settings
const char* mqtt_server = "broker.hivemq.com";
const int mqtt_port = 1883;
const char* mqtt_topic_data = "embedded/gateway/sensors";
const char* mqtt_topic_status = "embedded/gateway/status";
const char* mqtt_topic_commands = "embedded/gateway/commands";

// Serial2 for STM32 communication
#define STM32_RX 16  // ESP32 RX <- STM32 TX
#define STM32_TX 17  // ESP32 TX -> STM32 RX
#define STM32_BAUD 115200

// LED pins
const int LED_WIFI = 2;
const int LED_MQTT = 4;
const int LED_DATA = 5;

// Clients
WiFiClient espClient;
PubSubClient mqtt(espClient);
WebServer server(80);

// System state
struct GatewayState {
    bool wifiConnected;
    bool mqttConnected;
    bool stm32Connected;
    unsigned long dataReceived;
    unsigned long dataPublished;
    unsigned long mqttMessages;
    unsigned long lastHeartbeat;
    unsigned long uptime;
} state = {false, false, false, 0, 0, 0, 0, 0};

// Latest sensor data from STM32
struct SensorData {
    float temperature;
    float humidity;
    float pressure;
    int light;
    int battery;
    bool valid;
    unsigned long timestamp;
} sensorData = {0, 0, 0, 0, 0, false, 0};

// Buffer for serial data
String serialBuffer = "";

void processSTM32Data(String data) {
    state.dataReceived++;
    
    // Parse JSON from STM32
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, data);
    
    if (error) {
        Serial.printf("[STM32] JSON parse error: %s\n", error.c_str());
        return;
    }
    
    // Extract sensor data
    String type = doc["type"].as<String>();
    
    if (type == "sensors") {
        sensorData.temperature = doc["temp"];
        sensorData.humidity = doc["hum"];
        sensorData.pressure = doc["press"] | 1013.0;
        sensorData.light = doc["light"] | 0;
        sensorData.battery = doc["bat"] | 100;
        sensorData.valid = true;
        sensorData.timestamp = millis();
        
        state.stm32Connected = true;
        state.lastHeartbeat = millis();
        
        Serial.printf("[STM32] Temp=%.1f°C, Hum=%.1f%%, Light=%d\n",
                     sensorData.temperature, sensorData.humidity, sensorData.light);
        
        // Blink data LED
        digitalWrite(LED_DATA, HIGH);
        
        // Publish to MQTT if connected
        if (mqtt.connected()) {
            publishSensorData();
        }
        
        delay(50);
        digitalWrite(LED_DATA, LOW);
        
    } else if (type == "status") {
        Serial.printf("[STM32] Status: %s\n", doc["msg"].as<const char*>());
        state.stm32Connected = true;
        state.lastHeartbeat = millis();
        
    } else if (type == "event") {
        String event = doc["event"].as<String>();
        Serial.printf("[STM32] Event: %s\n", event.c_str());
        
        // Forward event to MQTT
        if (mqtt.connected()) {
            char buffer[256];
            serializeJson(doc, buffer);
            mqtt.publish(mqtt_topic_status, buffer);
        }
    }
}

void publishSensorData() {
    if (!sensorData.valid) return;
    
    StaticJsonDocument<512> doc;
    doc["device"] = "ESP32_Gateway";
    doc["source"] = "STM32";
    doc["timestamp"] = millis();
    
    JsonObject sensors = doc.createNestedObject("sensors");
    sensors["temperature"] = sensorData.temperature;
    sensors["humidity"] = sensorData.humidity;
    sensors["pressure"] = sensorData.pressure;
    sensors["light"] = sensorData.light;
    sensors["battery"] = sensorData.battery;
    
    JsonObject gateway = doc.createNestedObject("gateway");
    gateway["wifi_rssi"] = WiFi.RSSI();
    gateway["uptime"] = millis() / 1000;
    gateway["heap"] = ESP.getFreeHeap();
    
    char buffer[512];
    serializeJson(doc, buffer);
    
    if (mqtt.publish(mqtt_topic_data, buffer)) {
        state.dataPublished++;
        Serial.println("[MQTT] Published sensor data");
    }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
    state.mqttMessages++;
    
    char message[length + 1];
    memcpy(message, payload, length);
    message[length] = '\0';
    
    Serial.printf("[MQTT] Topic: %s\n", topic);
    Serial.printf("[MQTT] Message: %s\n", message);
    
    // Parse command
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, message);
    
    if (!error) {
        String cmd = doc["command"].as<String>();
        
        // Forward command to STM32
        Serial2.println(message);
        Serial.printf("[STM32] Forwarded command: %s\n", cmd.c_str());
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
        state.wifiConnected = true;
        digitalWrite(LED_WIFI, HIGH);
    } else {
        Serial.println(" Failed!");
        state.wifiConnected = false;
    }
}

void connectMQTT() {
    if (!state.wifiConnected) return;
    
    int attempts = 0;
    while (!mqtt.connected() && attempts < 3) {
        Serial.printf("[MQTT] Connecting to %s...", mqtt_server);
        
        String clientId = "ESP32_Gateway_" + String(random(0xffff), HEX);
        
        if (mqtt.connect(clientId.c_str())) {
            Serial.println(" Connected!");
            state.mqttConnected = true;
            digitalWrite(LED_MQTT, HIGH);
            
            // Subscribe to command topic
            mqtt.subscribe(mqtt_topic_commands);
            
            // Publish online status
            StaticJsonDocument<128> doc;
            doc["device"] = "ESP32_Gateway";
            doc["status"] = "online";
            doc["ip"] = WiFi.localIP().toString();
            
            char buffer[128];
            serializeJson(doc, buffer);
            mqtt.publish(mqtt_topic_status, buffer, true);
            
            return;
        }
        
        Serial.printf(" Failed (rc=%d)\n", mqtt.state());
        attempts++;
        delay(2000);
    }
}

// Web server handlers
const char* getHTML() {
    return R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>IoT Gateway Dashboard</title>
    <style>
        * { box-sizing: border-box; margin: 0; padding: 0; }
        body {
            font-family: 'Segoe UI', Arial, sans-serif;
            background: linear-gradient(135deg, #1a1a2e 0%, #16213e 100%);
            min-height: 100vh;
            color: white;
            padding: 20px;
        }
        .container { max-width: 1200px; margin: 0 auto; }
        h1 { text-align: center; margin-bottom: 30px; color: #00ff88; }
        .grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(280px, 1fr));
            gap: 20px;
        }
        .card {
            background: rgba(255,255,255,0.1);
            border-radius: 15px;
            padding: 20px;
            backdrop-filter: blur(10px);
        }
        .card h2 { color: #00ff88; margin-bottom: 15px; font-size: 1.2em; }
        .status { display: flex; justify-content: space-between; padding: 10px 0; }
        .status-value { font-weight: bold; color: #00ff88; }
        .status-indicator {
            display: inline-block;
            width: 12px;
            height: 12px;
            border-radius: 50%;
            margin-right: 10px;
        }
        .status-on { background: #00ff88; }
        .status-off { background: #ff4757; }
        .sensor-big {
            text-align: center;
            padding: 20px;
        }
        .sensor-big .value {
            font-size: 3em;
            font-weight: bold;
            color: #00ff88;
        }
        .sensor-big .label { color: #aaa; }
        button {
            width: 100%;
            padding: 12px;
            border: none;
            border-radius: 8px;
            background: #00ff88;
            color: #1a1a2e;
            font-weight: bold;
            cursor: pointer;
            margin-top: 10px;
        }
        button:hover { background: #00cc6a; }
    </style>
</head>
<body>
    <div class="container">
        <h1>🌐 IoT Gateway Dashboard</h1>
        
        <div class="grid">
            <div class="card">
                <h2>📡 Connection Status</h2>
                <div class="status">
                    <span><span class="status-indicator" id="wifi-led"></span>WiFi</span>
                    <span class="status-value" id="wifi-status">--</span>
                </div>
                <div class="status">
                    <span><span class="status-indicator" id="mqtt-led"></span>MQTT</span>
                    <span class="status-value" id="mqtt-status">--</span>
                </div>
                <div class="status">
                    <span><span class="status-indicator" id="stm32-led"></span>STM32</span>
                    <span class="status-value" id="stm32-status">--</span>
                </div>
            </div>
            
            <div class="card sensor-big">
                <div class="label">🌡️ Temperature</div>
                <div class="value" id="temp">--</div>
            </div>
            
            <div class="card sensor-big">
                <div class="label">💧 Humidity</div>
                <div class="value" id="hum">--</div>
            </div>
            
            <div class="card">
                <h2>📊 Sensor Data</h2>
                <div class="status">
                    <span>Pressure</span>
                    <span class="status-value" id="pressure">--</span>
                </div>
                <div class="status">
                    <span>Light Level</span>
                    <span class="status-value" id="light">--</span>
                </div>
                <div class="status">
                    <span>Battery</span>
                    <span class="status-value" id="battery">--</span>
                </div>
            </div>
            
            <div class="card">
                <h2>📈 Statistics</h2>
                <div class="status">
                    <span>Data Received</span>
                    <span class="status-value" id="received">--</span>
                </div>
                <div class="status">
                    <span>Data Published</span>
                    <span class="status-value" id="published">--</span>
                </div>
                <div class="status">
                    <span>MQTT Messages</span>
                    <span class="status-value" id="mqtt-msgs">--</span>
                </div>
                <div class="status">
                    <span>Uptime</span>
                    <span class="status-value" id="uptime">--</span>
                </div>
            </div>
            
            <div class="card">
                <h2>🔧 Commands</h2>
                <button onclick="sendCommand('led_on')">STM32 LED ON</button>
                <button onclick="sendCommand('led_off')">STM32 LED OFF</button>
                <button onclick="sendCommand('read_sensors')">Request Sensors</button>
            </div>
        </div>
    </div>
    
    <script>
        async function fetchData() {
            try {
                const res = await fetch('/api/status');
                const data = await res.json();
                
                // Connection status
                document.getElementById('wifi-status').textContent = data.wifi ? 'Connected' : 'Disconnected';
                document.getElementById('wifi-led').className = 'status-indicator ' + (data.wifi ? 'status-on' : 'status-off');
                document.getElementById('mqtt-status').textContent = data.mqtt ? 'Connected' : 'Disconnected';
                document.getElementById('mqtt-led').className = 'status-indicator ' + (data.mqtt ? 'status-on' : 'status-off');
                document.getElementById('stm32-status').textContent = data.stm32 ? 'Connected' : 'No Data';
                document.getElementById('stm32-led').className = 'status-indicator ' + (data.stm32 ? 'status-on' : 'status-off');
                
                // Sensors
                if (data.sensors.valid) {
                    document.getElementById('temp').textContent = data.sensors.temp.toFixed(1) + '°C';
                    document.getElementById('hum').textContent = data.sensors.hum.toFixed(1) + '%';
                    document.getElementById('pressure').textContent = data.sensors.press.toFixed(1) + ' hPa';
                    document.getElementById('light').textContent = data.sensors.light;
                    document.getElementById('battery').textContent = data.sensors.bat + '%';
                }
                
                // Stats
                document.getElementById('received').textContent = data.stats.received;
                document.getElementById('published').textContent = data.stats.published;
                document.getElementById('mqtt-msgs').textContent = data.stats.mqtt;
                document.getElementById('uptime').textContent = formatUptime(data.uptime);
            } catch(e) {
                console.error('Fetch error:', e);
            }
        }
        
        function formatUptime(seconds) {
            const h = Math.floor(seconds / 3600);
            const m = Math.floor((seconds % 3600) / 60);
            const s = seconds % 60;
            return h + 'h ' + m + 'm ' + s + 's';
        }
        
        async function sendCommand(cmd) {
            try {
                await fetch('/api/command', {
                    method: 'POST',
                    headers: {'Content-Type': 'application/json'},
                    body: JSON.stringify({command: cmd})
                });
            } catch(e) {
                console.error('Command error:', e);
            }
        }
        
        fetchData();
        setInterval(fetchData, 2000);
    </script>
</body>
</html>
)rawliteral";
}

void handleRoot() {
    server.send(200, "text/html", getHTML());
}

void handleApiStatus() {
    StaticJsonDocument<512> doc;
    
    doc["wifi"] = state.wifiConnected;
    doc["mqtt"] = state.mqttConnected;
    doc["stm32"] = state.stm32Connected;
    doc["uptime"] = millis() / 1000;
    
    JsonObject sensors = doc.createNestedObject("sensors");
    sensors["valid"] = sensorData.valid;
    sensors["temp"] = sensorData.temperature;
    sensors["hum"] = sensorData.humidity;
    sensors["press"] = sensorData.pressure;
    sensors["light"] = sensorData.light;
    sensors["bat"] = sensorData.battery;
    
    JsonObject stats = doc.createNestedObject("stats");
    stats["received"] = state.dataReceived;
    stats["published"] = state.dataPublished;
    stats["mqtt"] = state.mqttMessages;
    
    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
}

void handleApiCommand() {
    if (server.method() != HTTP_POST) {
        server.send(405, "application/json", "{\"error\":\"Method not allowed\"}");
        return;
    }
    
    String body = server.arg("plain");
    StaticJsonDocument<128> doc;
    DeserializationError error = deserializeJson(doc, body);
    
    if (error) {
        server.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
        return;
    }
    
    String command = doc["command"].as<String>();
    
    // Forward to STM32
    StaticJsonDocument<128> cmdDoc;
    cmdDoc["type"] = "command";
    cmdDoc["cmd"] = command;
    cmdDoc["timestamp"] = millis();
    
    char buffer[128];
    serializeJson(cmdDoc, buffer);
    Serial2.println(buffer);
    
    Serial.printf("[CMD] Sent to STM32: %s\n", command.c_str());
    
    server.send(200, "application/json", "{\"success\":true}");
}

void setup() {
    Serial.begin(115200);
    Serial2.begin(STM32_BAUD, SERIAL_8N1, STM32_RX, STM32_TX);
    
    delay(1000);
    
    // Initialize LEDs
    pinMode(LED_WIFI, OUTPUT);
    pinMode(LED_MQTT, OUTPUT);
    pinMode(LED_DATA, OUTPUT);
    
    Serial.println("\n╔══════════════════════════════════════╗");
    Serial.println("║   ESP32 IoT Gateway - STM32 Bridge   ║");
    Serial.println("║   Modul 13: Network Communication    ║");
    Serial.println("╚══════════════════════════════════════╝\n");
    
    // Connect to WiFi
    connectWiFi();
    
    // Configure MQTT
    mqtt.setServer(mqtt_server, mqtt_port);
    mqtt.setCallback(mqttCallback);
    mqtt.setBufferSize(1024);
    
    // Connect to MQTT
    connectMQTT();
    
    // Setup web server
    server.on("/", handleRoot);
    server.on("/api/status", HTTP_GET, handleApiStatus);
    server.on("/api/command", HTTP_POST, handleApiCommand);
    server.begin();
    
    Serial.printf("\n[Dashboard] http://%s/\n", WiFi.localIP().toString().c_str());
    Serial.println("[STM32] Waiting for data on Serial2...\n");
}

void loop() {
    // Handle web server
    server.handleClient();
    
    // Handle MQTT
    if (!mqtt.connected()) {
        digitalWrite(LED_MQTT, LOW);
        state.mqttConnected = false;
        connectMQTT();
    }
    mqtt.loop();
    
    // Handle WiFi reconnection
    if (WiFi.status() != WL_CONNECTED) {
        state.wifiConnected = false;
        digitalWrite(LED_WIFI, LOW);
        connectWiFi();
    }
    
    // Check STM32 connection timeout (10 seconds)
    if (millis() - state.lastHeartbeat > 10000) {
        state.stm32Connected = false;
    }
    
    // Read data from STM32
    while (Serial2.available()) {
        char c = Serial2.read();
        
        if (c == '\n') {
            if (serialBuffer.length() > 0) {
                processSTM32Data(serialBuffer);
                serialBuffer = "";
            }
        } else if (c != '\r') {
            serialBuffer += c;
        }
    }
    
    // Handle serial commands
    if (Serial.available()) {
        String cmd = Serial.readStringUntil('\n');
        cmd.trim();
        
        if (cmd == "status") {
            Serial.println("\n[Gateway Status]");
            Serial.printf("  WiFi: %s | MQTT: %s | STM32: %s\n",
                         state.wifiConnected ? "Connected" : "Disconnected",
                         state.mqttConnected ? "Connected" : "Disconnected",
                         state.stm32Connected ? "Connected" : "No data");
            Serial.printf("  Received: %lu | Published: %lu\n",
                         state.dataReceived, state.dataPublished);
        } else if (cmd.startsWith("send ")) {
            String msg = cmd.substring(5);
            Serial2.println(msg);
            Serial.printf("[STM32] Sent: %s\n", msg.c_str());
        }
    }
    
    delay(10);
}
