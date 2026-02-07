/*
 * ESP32 Web Server Demo
 * Modul 13: Network Communication
 * 
 * Demonstrates:
 * - HTTP server on ESP32
 * - HTML/CSS web pages
 * - REST API endpoints
 * - Real-time sensor display
 * - GPIO control via web
 */

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>

// WiFi credentials
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// Web server on port 80
WebServer server(80);

// GPIO pins
const int LED_PIN = 2;
const int LED2_PIN = 4;
const int RELAY_PIN = 5;

// System state
struct SystemState {
    bool led1;
    bool led2;
    bool relay;
    float temperature;
    float humidity;
    int lightLevel;
    unsigned long uptime;
    int requestCount;
} state = {false, false, false, 25.0, 60.0, 512, 0, 0};

// Simulated sensor readings
void updateSensors() {
    state.temperature = 20.0 + (random(0, 150) / 10.0);
    state.humidity = 40.0 + (random(0, 400) / 10.0);
    state.lightLevel = random(0, 1024);
    state.uptime = millis() / 1000;
}

// Main HTML page
const char* getMainPage() {
    return R"rawliteral(
<!DOCTYPE html>
<html lang="id">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP32 Web Server</title>
    <style>
        * { box-sizing: border-box; margin: 0; padding: 0; }
        body {
            font-family: 'Segoe UI', Arial, sans-serif;
            background: linear-gradient(135deg, #1a1a2e 0%, #16213e 100%);
            min-height: 100vh;
            color: white;
            padding: 20px;
        }
        .container { max-width: 1000px; margin: 0 auto; }
        h1 {
            text-align: center;
            margin-bottom: 30px;
            font-size: 2.5em;
            text-shadow: 0 0 20px rgba(0,255,136,0.5);
        }
        .grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(300px, 1fr));
            gap: 20px;
        }
        .card {
            background: rgba(255,255,255,0.1);
            border-radius: 15px;
            padding: 20px;
            backdrop-filter: blur(10px);
            border: 1px solid rgba(255,255,255,0.2);
        }
        .card h2 {
            border-bottom: 2px solid #00ff88;
            padding-bottom: 10px;
            margin-bottom: 20px;
            color: #00ff88;
        }
        .sensor-row {
            display: flex;
            justify-content: space-between;
            padding: 15px 0;
            border-bottom: 1px solid rgba(255,255,255,0.1);
        }
        .sensor-value {
            font-size: 1.5em;
            font-weight: bold;
            color: #00ff88;
        }
        .btn-group { display: flex; gap: 10px; margin: 15px 0; }
        .btn {
            flex: 1;
            padding: 15px;
            border: none;
            border-radius: 10px;
            font-size: 1em;
            font-weight: bold;
            cursor: pointer;
            transition: all 0.3s;
        }
        .btn-on {
            background: #00ff88;
            color: #1a1a2e;
        }
        .btn-off {
            background: #ff4757;
            color: white;
        }
        .btn:hover { transform: scale(1.05); }
        .status {
            display: inline-block;
            padding: 5px 15px;
            border-radius: 20px;
            font-size: 0.9em;
        }
        .status-on { background: #00ff88; color: #1a1a2e; }
        .status-off { background: #ff4757; color: white; }
        .system-info { font-size: 0.9em; color: #aaa; }
        .system-info span { color: #00ff88; }
        #message {
            position: fixed;
            top: 20px;
            right: 20px;
            padding: 15px 25px;
            border-radius: 10px;
            background: #00ff88;
            color: #1a1a2e;
            display: none;
            animation: slideIn 0.3s;
        }
        @keyframes slideIn {
            from { transform: translateX(100%); }
            to { transform: translateX(0); }
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>🌐 ESP32 Web Server</h1>
        
        <div class="grid">
            <!-- Sensor Card -->
            <div class="card">
                <h2>📊 Sensor Data</h2>
                <div class="sensor-row">
                    <span>🌡️ Temperature</span>
                    <span class="sensor-value" id="temp">--</span>
                </div>
                <div class="sensor-row">
                    <span>💧 Humidity</span>
                    <span class="sensor-value" id="hum">--</span>
                </div>
                <div class="sensor-row">
                    <span>☀️ Light Level</span>
                    <span class="sensor-value" id="light">--</span>
                </div>
            </div>
            
            <!-- Control Card -->
            <div class="card">
                <h2>🎛️ GPIO Control</h2>
                
                <div style="margin-bottom: 20px;">
                    <span>LED 1 (GPIO 2)</span>
                    <span class="status" id="led1-status">OFF</span>
                    <div class="btn-group">
                        <button class="btn btn-on" onclick="controlGPIO('led1', 'on')">ON</button>
                        <button class="btn btn-off" onclick="controlGPIO('led1', 'off')">OFF</button>
                    </div>
                </div>
                
                <div style="margin-bottom: 20px;">
                    <span>LED 2 (GPIO 4)</span>
                    <span class="status" id="led2-status">OFF</span>
                    <div class="btn-group">
                        <button class="btn btn-on" onclick="controlGPIO('led2', 'on')">ON</button>
                        <button class="btn btn-off" onclick="controlGPIO('led2', 'off')">OFF</button>
                    </div>
                </div>
                
                <div>
                    <span>Relay (GPIO 5)</span>
                    <span class="status" id="relay-status">OFF</span>
                    <div class="btn-group">
                        <button class="btn btn-on" onclick="controlGPIO('relay', 'on')">ON</button>
                        <button class="btn btn-off" onclick="controlGPIO('relay', 'off')">OFF</button>
                    </div>
                </div>
            </div>
            
            <!-- System Card -->
            <div class="card">
                <h2>💻 System Info</h2>
                <div class="system-info">
                    <p>IP Address: <span id="ip">--</span></p>
                    <p>Uptime: <span id="uptime">--</span> seconds</p>
                    <p>Free Heap: <span id="heap">--</span> bytes</p>
                    <p>Requests: <span id="requests">--</span></p>
                    <p>WiFi RSSI: <span id="rssi">--</span> dBm</p>
                </div>
            </div>
            
            <!-- API Card -->
            <div class="card">
                <h2>🔌 REST API</h2>
                <div class="system-info">
                    <p><strong>GET</strong> /api/sensors</p>
                    <p><strong>GET</strong> /api/status</p>
                    <p><strong>POST</strong> /api/gpio</p>
                    <p><strong>GET</strong> /api/system</p>
                </div>
            </div>
        </div>
    </div>
    
    <div id="message"></div>
    
    <script>
        function showMessage(text) {
            const msg = document.getElementById('message');
            msg.textContent = text;
            msg.style.display = 'block';
            setTimeout(() => { msg.style.display = 'none'; }, 2000);
        }
        
        function updateStatus(data) {
            document.getElementById('led1-status').textContent = data.led1 ? 'ON' : 'OFF';
            document.getElementById('led1-status').className = 'status ' + (data.led1 ? 'status-on' : 'status-off');
            document.getElementById('led2-status').textContent = data.led2 ? 'ON' : 'OFF';
            document.getElementById('led2-status').className = 'status ' + (data.led2 ? 'status-on' : 'status-off');
            document.getElementById('relay-status').textContent = data.relay ? 'ON' : 'OFF';
            document.getElementById('relay-status').className = 'status ' + (data.relay ? 'status-on' : 'status-off');
        }
        
        async function fetchData() {
            try {
                // Fetch sensor data
                const sensors = await fetch('/api/sensors').then(r => r.json());
                document.getElementById('temp').textContent = sensors.temperature.toFixed(1) + '°C';
                document.getElementById('hum').textContent = sensors.humidity.toFixed(1) + '%';
                document.getElementById('light').textContent = sensors.light;
                
                // Fetch system info
                const system = await fetch('/api/system').then(r => r.json());
                document.getElementById('ip').textContent = system.ip;
                document.getElementById('uptime').textContent = system.uptime;
                document.getElementById('heap').textContent = system.heap;
                document.getElementById('requests').textContent = system.requests;
                document.getElementById('rssi').textContent = system.rssi;
                
                // Fetch GPIO status
                const status = await fetch('/api/status').then(r => r.json());
                updateStatus(status);
            } catch(e) {
                console.error('Fetch error:', e);
            }
        }
        
        async function controlGPIO(pin, action) {
            try {
                const response = await fetch('/api/gpio', {
                    method: 'POST',
                    headers: {'Content-Type': 'application/json'},
                    body: JSON.stringify({pin: pin, action: action})
                });
                const data = await response.json();
                if (data.success) {
                    showMessage(pin.toUpperCase() + ' turned ' + action.toUpperCase());
                    updateStatus(data);
                }
            } catch(e) {
                showMessage('Error: ' + e.message);
            }
        }
        
        // Initial fetch and periodic update
        fetchData();
        setInterval(fetchData, 2000);
    </script>
</body>
</html>
)rawliteral";
}

// Route handlers
void handleRoot() {
    state.requestCount++;
    server.send(200, "text/html", getMainPage());
    Serial.println("[HTTP] GET / - Main page");
}

void handleApiSensors() {
    state.requestCount++;
    updateSensors();
    
    StaticJsonDocument<256> doc;
    doc["temperature"] = state.temperature;
    doc["humidity"] = state.humidity;
    doc["light"] = state.lightLevel;
    doc["timestamp"] = millis();
    
    String response;
    serializeJson(doc, response);
    
    server.send(200, "application/json", response);
    Serial.println("[HTTP] GET /api/sensors");
}

void handleApiStatus() {
    state.requestCount++;
    
    StaticJsonDocument<256> doc;
    doc["led1"] = state.led1;
    doc["led2"] = state.led2;
    doc["relay"] = state.relay;
    
    String response;
    serializeJson(doc, response);
    
    server.send(200, "application/json", response);
    Serial.println("[HTTP] GET /api/status");
}

void handleApiSystem() {
    state.requestCount++;
    
    StaticJsonDocument<256> doc;
    doc["ip"] = WiFi.localIP().toString();
    doc["mac"] = WiFi.macAddress();
    doc["rssi"] = WiFi.RSSI();
    doc["heap"] = ESP.getFreeHeap();
    doc["uptime"] = millis() / 1000;
    doc["requests"] = state.requestCount;
    doc["chip_model"] = ESP.getChipModel();
    doc["sdk_version"] = ESP.getSdkVersion();
    
    String response;
    serializeJson(doc, response);
    
    server.send(200, "application/json", response);
    Serial.println("[HTTP] GET /api/system");
}

void handleApiGpio() {
    state.requestCount++;
    
    if (server.method() != HTTP_POST) {
        server.send(405, "application/json", "{\"error\":\"Method not allowed\"}");
        return;
    }
    
    String body = server.arg("plain");
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, body);
    
    if (error) {
        server.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
        return;
    }
    
    String pin = doc["pin"].as<String>();
    String action = doc["action"].as<String>();
    bool turnOn = (action == "on");
    
    if (pin == "led1") {
        state.led1 = turnOn;
        digitalWrite(LED_PIN, turnOn ? HIGH : LOW);
    } else if (pin == "led2") {
        state.led2 = turnOn;
        digitalWrite(LED2_PIN, turnOn ? HIGH : LOW);
    } else if (pin == "relay") {
        state.relay = turnOn;
        digitalWrite(RELAY_PIN, turnOn ? HIGH : LOW);
    } else {
        server.send(400, "application/json", "{\"error\":\"Invalid pin\"}");
        return;
    }
    
    Serial.printf("[HTTP] POST /api/gpio - %s = %s\n", pin.c_str(), action.c_str());
    
    StaticJsonDocument<256> response;
    response["success"] = true;
    response["pin"] = pin;
    response["action"] = action;
    response["led1"] = state.led1;
    response["led2"] = state.led2;
    response["relay"] = state.relay;
    
    String jsonResponse;
    serializeJson(response, jsonResponse);
    server.send(200, "application/json", jsonResponse);
}

void handleApiToggle() {
    state.requestCount++;
    
    String pin = server.arg("pin");
    bool newState;
    
    if (pin == "led1") {
        state.led1 = !state.led1;
        newState = state.led1;
        digitalWrite(LED_PIN, state.led1 ? HIGH : LOW);
    } else if (pin == "led2") {
        state.led2 = !state.led2;
        newState = state.led2;
        digitalWrite(LED2_PIN, state.led2 ? HIGH : LOW);
    } else if (pin == "relay") {
        state.relay = !state.relay;
        newState = state.relay;
        digitalWrite(RELAY_PIN, state.relay ? HIGH : LOW);
    } else {
        server.send(400, "application/json", "{\"error\":\"Invalid pin\"}");
        return;
    }
    
    StaticJsonDocument<128> response;
    response["pin"] = pin;
    response["state"] = newState;
    
    String jsonResponse;
    serializeJson(response, jsonResponse);
    server.send(200, "application/json", jsonResponse);
    
    Serial.printf("[HTTP] GET /api/toggle?pin=%s -> %s\n", pin.c_str(), newState ? "ON" : "OFF");
}

void handleNotFound() {
    state.requestCount++;
    
    StaticJsonDocument<128> doc;
    doc["error"] = "Not found";
    doc["path"] = server.uri();
    doc["method"] = (server.method() == HTTP_GET) ? "GET" : "POST";
    
    String response;
    serializeJson(doc, response);
    
    server.send(404, "application/json", response);
    Serial.printf("[HTTP] 404 - %s\n", server.uri().c_str());
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

void setupServer() {
    // Web pages
    server.on("/", HTTP_GET, handleRoot);
    
    // REST API endpoints
    server.on("/api/sensors", HTTP_GET, handleApiSensors);
    server.on("/api/status", HTTP_GET, handleApiStatus);
    server.on("/api/system", HTTP_GET, handleApiSystem);
    server.on("/api/gpio", HTTP_POST, handleApiGpio);
    server.on("/api/toggle", HTTP_GET, handleApiToggle);
    
    // 404 handler
    server.onNotFound(handleNotFound);
    
    // Enable CORS
    server.enableCORS(true);
    
    server.begin();
    Serial.println("[HTTP] Server started on port 80");
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    // Initialize GPIOs
    pinMode(LED_PIN, OUTPUT);
    pinMode(LED2_PIN, OUTPUT);
    pinMode(RELAY_PIN, OUTPUT);
    
    digitalWrite(LED_PIN, LOW);
    digitalWrite(LED2_PIN, LOW);
    digitalWrite(RELAY_PIN, LOW);
    
    Serial.println("\n╔══════════════════════════════════════╗");
    Serial.println("║      ESP32 Web Server Demo           ║");
    Serial.println("║   Modul 13: Network Communication    ║");
    Serial.println("╚══════════════════════════════════════╝\n");
    
    // Connect to WiFi
    connectWiFi();
    
    // Setup web server
    setupServer();
    
    Serial.println("\n[Web Interface]");
    Serial.printf("  http://%s/\n", WiFi.localIP().toString().c_str());
    Serial.println("\n[API Endpoints]");
    Serial.printf("  GET  http://%s/api/sensors\n", WiFi.localIP().toString().c_str());
    Serial.printf("  GET  http://%s/api/status\n", WiFi.localIP().toString().c_str());
    Serial.printf("  GET  http://%s/api/system\n", WiFi.localIP().toString().c_str());
    Serial.printf("  POST http://%s/api/gpio\n", WiFi.localIP().toString().c_str());
}

void loop() {
    server.handleClient();
    
    // Serial commands
    if (Serial.available()) {
        String cmd = Serial.readStringUntil('\n');
        cmd.trim();
        cmd.toLowerCase();
        
        if (cmd == "status") {
            Serial.printf("\n[Status] IP: %s | Requests: %d | Heap: %lu bytes\n",
                         WiFi.localIP().toString().c_str(),
                         state.requestCount,
                         ESP.getFreeHeap());
            Serial.printf("  LED1: %s | LED2: %s | Relay: %s\n",
                         state.led1 ? "ON" : "OFF",
                         state.led2 ? "ON" : "OFF",
                         state.relay ? "ON" : "OFF");
        } else if (cmd == "ip") {
            Serial.printf("[IP] %s\n", WiFi.localIP().toString().c_str());
        }
    }
    
    delay(2);
}
