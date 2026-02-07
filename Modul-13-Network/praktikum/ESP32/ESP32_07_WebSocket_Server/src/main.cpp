/*
 * ESP32 WebSocket Server Demo
 * Modul 13: Network Communication
 * 
 * Demonstrates:
 * - WebSocket server implementation
 * - Real-time bidirectional communication
 * - Multiple client handling
 * - Broadcasting messages
 * - JSON data streaming
 */

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>

// WiFi credentials
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// Servers
WebServer http(80);
WebSocketsServer webSocket(81);

// GPIO pins
const int LED_PIN = 2;
const int BUTTON_PIN = 0;  // Boot button on ESP32

// Client tracking
const int MAX_CLIENTS = 10;
struct ClientInfo {
    uint8_t num;
    bool connected;
    String name;
    unsigned long connectedAt;
    unsigned long messageCount;
} clients[MAX_CLIENTS];

// Statistics
unsigned long totalMessages = 0;
unsigned long totalBroadcasts = 0;
int connectedClients = 0;
unsigned long lastSensorUpdate = 0;
const int sensorUpdateInterval = 1000;  // Send sensor data every second

// Sensor data
struct SensorData {
    float temperature;
    float humidity;
    int light;
    bool buttonState;
    bool ledState;
} sensors;

void updateSensors() {
    sensors.temperature = 20.0 + (random(0, 150) / 10.0);
    sensors.humidity = 40.0 + (random(0, 400) / 10.0);
    sensors.light = random(0, 1024);
    sensors.buttonState = !digitalRead(BUTTON_PIN);
}

// Broadcast sensor data to all clients
void broadcastSensorData() {
    if (connectedClients == 0) return;
    
    StaticJsonDocument<256> doc;
    doc["type"] = "sensors";
    doc["temperature"] = sensors.temperature;
    doc["humidity"] = sensors.humidity;
    doc["light"] = sensors.light;
    doc["button"] = sensors.buttonState;
    doc["led"] = sensors.ledState;
    doc["timestamp"] = millis();
    
    char buffer[256];
    serializeJson(doc, buffer);
    
    webSocket.broadcastTXT(buffer);
    totalBroadcasts++;
}

// Broadcast message to all clients
void broadcastMessage(const char* message) {
    StaticJsonDocument<256> doc;
    doc["type"] = "broadcast";
    doc["message"] = message;
    doc["timestamp"] = millis();
    
    char buffer[256];
    serializeJson(doc, buffer);
    
    webSocket.broadcastTXT(buffer);
    totalBroadcasts++;
}

// Send client list to all clients
void broadcastClientList() {
    StaticJsonDocument<512> doc;
    doc["type"] = "clients";
    
    JsonArray clientList = doc.createNestedArray("list");
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].connected) {
            JsonObject client = clientList.createNestedObject();
            client["id"] = clients[i].num;
            client["name"] = clients[i].name;
            client["messages"] = clients[i].messageCount;
        }
    }
    
    doc["count"] = connectedClients;
    
    char buffer[512];
    serializeJson(doc, buffer);
    
    webSocket.broadcastTXT(buffer);
}

// Handle WebSocket events
void webSocketEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
    switch (type) {
        case WStype_DISCONNECTED:
            Serial.printf("[WS] Client #%u disconnected\n", num);
            clients[num].connected = false;
            connectedClients--;
            
            // Notify others
            {
                StaticJsonDocument<128> doc;
                doc["type"] = "system";
                doc["event"] = "user_left";
                doc["name"] = clients[num].name;
                
                char buffer[128];
                serializeJson(doc, buffer);
                webSocket.broadcastTXT(buffer);
            }
            
            broadcastClientList();
            break;
            
        case WStype_CONNECTED:
            {
                IPAddress ip = webSocket.remoteIP(num);
                Serial.printf("[WS] Client #%u connected from %s\n", num, ip.toString().c_str());
                
                clients[num].num = num;
                clients[num].connected = true;
                clients[num].name = "User_" + String(num);
                clients[num].connectedAt = millis();
                clients[num].messageCount = 0;
                connectedClients++;
                
                // Send welcome message
                StaticJsonDocument<256> doc;
                doc["type"] = "welcome";
                doc["id"] = num;
                doc["message"] = "Connected to ESP32 WebSocket Server";
                doc["timestamp"] = millis();
                doc["totalClients"] = connectedClients;
                
                char buffer[256];
                serializeJson(doc, buffer);
                webSocket.sendTXT(num, buffer);
                
                // Notify others
                StaticJsonDocument<128> notifyDoc;
                notifyDoc["type"] = "system";
                notifyDoc["event"] = "user_joined";
                notifyDoc["name"] = clients[num].name;
                
                char notifyBuffer[128];
                serializeJson(notifyDoc, notifyBuffer);
                webSocket.broadcastTXT(notifyBuffer);
                
                broadcastClientList();
            }
            break;
            
        case WStype_TEXT:
            {
                Serial.printf("[WS] #%u: %s\n", num, payload);
                totalMessages++;
                clients[num].messageCount++;
                
                // Parse incoming JSON
                StaticJsonDocument<512> doc;
                DeserializationError error = deserializeJson(doc, payload);
                
                if (error) {
                    Serial.println("[WS] JSON parse error");
                    return;
                }
                
                String msgType = doc["type"].as<String>();
                
                if (msgType == "chat") {
                    // Chat message - broadcast to all
                    String message = doc["message"].as<String>();
                    
                    StaticJsonDocument<256> response;
                    response["type"] = "chat";
                    response["from"] = clients[num].name;
                    response["fromId"] = num;
                    response["message"] = message;
                    response["timestamp"] = millis();
                    
                    char buffer[256];
                    serializeJson(response, buffer);
                    webSocket.broadcastTXT(buffer);
                    
                } else if (msgType == "led") {
                    // LED control
                    bool state = doc["state"];
                    sensors.ledState = state;
                    digitalWrite(LED_PIN, state ? HIGH : LOW);
                    
                    StaticJsonDocument<128> response;
                    response["type"] = "led_changed";
                    response["state"] = state;
                    response["changedBy"] = clients[num].name;
                    
                    char buffer[128];
                    serializeJson(response, buffer);
                    webSocket.broadcastTXT(buffer);
                    
                    Serial.printf("[LED] Set to %s by %s\n", state ? "ON" : "OFF", clients[num].name.c_str());
                    
                } else if (msgType == "setname") {
                    // Set username
                    String oldName = clients[num].name;
                    String newName = doc["name"].as<String>();
                    clients[num].name = newName;
                    
                    StaticJsonDocument<128> response;
                    response["type"] = "name_changed";
                    response["oldName"] = oldName;
                    response["newName"] = newName;
                    
                    char buffer[128];
                    serializeJson(response, buffer);
                    webSocket.broadcastTXT(buffer);
                    
                    broadcastClientList();
                    
                } else if (msgType == "ping") {
                    // Respond to ping
                    StaticJsonDocument<128> response;
                    response["type"] = "pong";
                    response["clientTime"] = doc["time"];
                    response["serverTime"] = millis();
                    
                    char buffer[128];
                    serializeJson(response, buffer);
                    webSocket.sendTXT(num, buffer);
                    
                } else if (msgType == "command") {
                    // Process command
                    String cmd = doc["cmd"].as<String>();
                    
                    StaticJsonDocument<256> response;
                    response["type"] = "command_response";
                    response["cmd"] = cmd;
                    
                    if (cmd == "status") {
                        response["heap"] = ESP.getFreeHeap();
                        response["uptime"] = millis() / 1000;
                        response["clients"] = connectedClients;
                        response["messages"] = totalMessages;
                    } else if (cmd == "restart") {
                        response["message"] = "Restarting...";
                        char buffer[256];
                        serializeJson(response, buffer);
                        webSocket.sendTXT(num, buffer);
                        delay(1000);
                        ESP.restart();
                    } else {
                        response["error"] = "Unknown command";
                    }
                    
                    char buffer[256];
                    serializeJson(response, buffer);
                    webSocket.sendTXT(num, buffer);
                }
            }
            break;
            
        case WStype_BIN:
            Serial.printf("[WS] Binary message from #%u (%u bytes)\n", num, length);
            break;
            
        case WStype_PING:
            Serial.printf("[WS] Ping from #%u\n", num);
            break;
            
        case WStype_PONG:
            Serial.printf("[WS] Pong from #%u\n", num);
            break;
            
        case WStype_ERROR:
            Serial.printf("[WS] Error from #%u\n", num);
            break;
    }
}

// HTML page
const char* getHTML() {
    return R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP32 WebSocket Demo</title>
    <style>
        * { box-sizing: border-box; margin: 0; padding: 0; }
        body {
            font-family: 'Segoe UI', Arial, sans-serif;
            background: #1a1a2e;
            color: white;
            padding: 20px;
            min-height: 100vh;
        }
        .container { max-width: 1200px; margin: 0 auto; }
        h1 { text-align: center; margin-bottom: 20px; color: #00ff88; }
        .grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(350px, 1fr));
            gap: 20px;
        }
        .card {
            background: rgba(255,255,255,0.1);
            border-radius: 15px;
            padding: 20px;
        }
        .card h2 { color: #00ff88; margin-bottom: 15px; }
        .sensor { display: flex; justify-content: space-between; padding: 10px 0; }
        .sensor-value { color: #00ff88; font-weight: bold; font-size: 1.2em; }
        .status { padding: 5px 15px; border-radius: 20px; font-size: 0.9em; }
        .status-on { background: #00ff88; color: #1a1a2e; }
        .status-off { background: #ff4757; }
        #chat { height: 250px; overflow-y: auto; background: rgba(0,0,0,0.3); padding: 10px; border-radius: 10px; margin-bottom: 10px; }
        .msg { padding: 5px 0; border-bottom: 1px solid rgba(255,255,255,0.1); }
        .msg-system { color: #ffa502; font-style: italic; }
        .msg-name { color: #00ff88; font-weight: bold; }
        input, button {
            padding: 12px;
            border: none;
            border-radius: 8px;
            font-size: 1em;
        }
        input {
            background: rgba(255,255,255,0.1);
            color: white;
            width: 100%;
            margin-bottom: 10px;
        }
        button {
            background: #00ff88;
            color: #1a1a2e;
            cursor: pointer;
            width: 100%;
        }
        button:hover { background: #00cc6a; }
        .btn-row { display: flex; gap: 10px; }
        .btn-row button { flex: 1; }
        #connectionStatus {
            text-align: center;
            padding: 10px;
            border-radius: 10px;
            margin-bottom: 20px;
        }
        .connected { background: rgba(0,255,136,0.2); color: #00ff88; }
        .disconnected { background: rgba(255,71,87,0.2); color: #ff4757; }
        .client-list { max-height: 200px; overflow-y: auto; }
        .client-item { padding: 5px 10px; background: rgba(0,0,0,0.2); margin: 5px 0; border-radius: 5px; }
    </style>
</head>
<body>
    <div class="container">
        <h1>🔌 ESP32 WebSocket Demo</h1>
        
        <div id="connectionStatus" class="disconnected">Disconnected</div>
        
        <div class="grid">
            <div class="card">
                <h2>📊 Real-time Sensors</h2>
                <div class="sensor"><span>🌡️ Temperature</span><span class="sensor-value" id="temp">--</span></div>
                <div class="sensor"><span>💧 Humidity</span><span class="sensor-value" id="hum">--</span></div>
                <div class="sensor"><span>☀️ Light</span><span class="sensor-value" id="light">--</span></div>
                <div class="sensor"><span>🔘 Button</span><span id="button" class="status status-off">Released</span></div>
                <div class="sensor"><span>💡 LED</span><span id="led" class="status status-off">OFF</span></div>
                <div class="btn-row" style="margin-top: 15px;">
                    <button onclick="sendLED(true)">LED ON</button>
                    <button onclick="sendLED(false)">LED OFF</button>
                </div>
            </div>
            
            <div class="card">
                <h2>💬 Chat</h2>
                <div id="chat"></div>
                <input type="text" id="chatInput" placeholder="Type a message..." onkeypress="if(event.keyCode==13)sendChat()">
                <button onclick="sendChat()">Send</button>
                <input type="text" id="nameInput" placeholder="Your name..." style="margin-top: 10px;">
                <button onclick="setName()" style="background: #5352ed;">Change Name</button>
            </div>
            
            <div class="card">
                <h2>👥 Connected Users</h2>
                <div id="clients" class="client-list"></div>
            </div>
            
            <div class="card">
                <h2>📈 Statistics</h2>
                <div class="sensor"><span>Your ID</span><span class="sensor-value" id="myId">--</span></div>
                <div class="sensor"><span>Connected Users</span><span class="sensor-value" id="userCount">--</span></div>
                <div class="sensor"><span>Latency</span><span class="sensor-value" id="latency">--</span></div>
                <button onclick="pingServer()" style="margin-top: 15px;">Test Latency</button>
            </div>
        </div>
    </div>
    
    <script>
        let ws;
        let myId = -1;
        let pingTime = 0;
        
        function connect() {
            ws = new WebSocket('ws://' + window.location.hostname + ':81/');
            
            ws.onopen = () => {
                document.getElementById('connectionStatus').textContent = '✓ Connected';
                document.getElementById('connectionStatus').className = 'connected';
                addChatMsg('System', 'Connected to server', true);
            };
            
            ws.onclose = () => {
                document.getElementById('connectionStatus').textContent = '✗ Disconnected';
                document.getElementById('connectionStatus').className = 'disconnected';
                addChatMsg('System', 'Disconnected - reconnecting...', true);
                setTimeout(connect, 3000);
            };
            
            ws.onmessage = (event) => {
                const data = JSON.parse(event.data);
                
                switch(data.type) {
                    case 'welcome':
                        myId = data.id;
                        document.getElementById('myId').textContent = data.id;
                        break;
                    case 'sensors':
                        document.getElementById('temp').textContent = data.temperature.toFixed(1) + '°C';
                        document.getElementById('hum').textContent = data.humidity.toFixed(1) + '%';
                        document.getElementById('light').textContent = data.light;
                        document.getElementById('button').textContent = data.button ? 'Pressed' : 'Released';
                        document.getElementById('button').className = 'status ' + (data.button ? 'status-on' : 'status-off');
                        document.getElementById('led').textContent = data.led ? 'ON' : 'OFF';
                        document.getElementById('led').className = 'status ' + (data.led ? 'status-on' : 'status-off');
                        break;
                    case 'chat':
                        addChatMsg(data.from, data.message, false);
                        break;
                    case 'system':
                        addChatMsg('System', data.event.replace(/_/g, ' ') + ': ' + data.name, true);
                        break;
                    case 'clients':
                        document.getElementById('userCount').textContent = data.count;
                        let html = '';
                        data.list.forEach(c => {
                            html += '<div class="client-item">' + c.name + ' (msgs: ' + c.messages + ')</div>';
                        });
                        document.getElementById('clients').innerHTML = html;
                        break;
                    case 'led_changed':
                        addChatMsg('System', 'LED ' + (data.state ? 'ON' : 'OFF') + ' by ' + data.changedBy, true);
                        break;
                    case 'pong':
                        const latency = Date.now() - pingTime;
                        document.getElementById('latency').textContent = latency + ' ms';
                        break;
                }
            };
        }
        
        function addChatMsg(from, msg, isSystem) {
            const chat = document.getElementById('chat');
            const div = document.createElement('div');
            div.className = 'msg' + (isSystem ? ' msg-system' : '');
            div.innerHTML = isSystem ? msg : '<span class="msg-name">' + from + ':</span> ' + msg;
            chat.appendChild(div);
            chat.scrollTop = chat.scrollHeight;
        }
        
        function sendChat() {
            const input = document.getElementById('chatInput');
            if (input.value.trim() && ws.readyState === 1) {
                ws.send(JSON.stringify({type: 'chat', message: input.value}));
                input.value = '';
            }
        }
        
        function setName() {
            const name = document.getElementById('nameInput').value.trim();
            if (name && ws.readyState === 1) {
                ws.send(JSON.stringify({type: 'setname', name: name}));
                document.getElementById('nameInput').value = '';
            }
        }
        
        function sendLED(state) {
            if (ws.readyState === 1) {
                ws.send(JSON.stringify({type: 'led', state: state}));
            }
        }
        
        function pingServer() {
            if (ws.readyState === 1) {
                pingTime = Date.now();
                ws.send(JSON.stringify({type: 'ping', time: pingTime}));
            }
        }
        
        connect();
    </script>
</body>
</html>
)rawliteral";
}

void handleRoot() {
    http.send(200, "text/html", getHTML());
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

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    // Initialize GPIOs
    pinMode(LED_PIN, OUTPUT);
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    digitalWrite(LED_PIN, LOW);
    
    // Initialize client tracking
    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].connected = false;
    }
    
    Serial.println("\n╔══════════════════════════════════════╗");
    Serial.println("║    ESP32 WebSocket Server Demo       ║");
    Serial.println("║   Modul 13: Network Communication    ║");
    Serial.println("╚══════════════════════════════════════╝\n");
    
    // Connect to WiFi
    connectWiFi();
    
    // Start HTTP server
    http.on("/", handleRoot);
    http.begin();
    Serial.println("[HTTP] Server started on port 80");
    
    // Start WebSocket server
    webSocket.begin();
    webSocket.onEvent(webSocketEvent);
    Serial.println("[WS] Server started on port 81");
    
    Serial.printf("\n[URL] http://%s/\n", WiFi.localIP().toString().c_str());
}

void loop() {
    http.handleClient();
    webSocket.loop();
    
    // Update and broadcast sensor data periodically
    if (millis() - lastSensorUpdate >= sensorUpdateInterval) {
        updateSensors();
        broadcastSensorData();
        lastSensorUpdate = millis();
    }
    
    // Handle serial commands
    if (Serial.available()) {
        String cmd = Serial.readStringUntil('\n');
        cmd.trim();
        
        if (cmd == "status") {
            Serial.printf("[Status] Clients: %d | Messages: %lu | Broadcasts: %lu\n",
                         connectedClients, totalMessages, totalBroadcasts);
        } else if (cmd.startsWith("broadcast ")) {
            String msg = cmd.substring(10);
            broadcastMessage(msg.c_str());
            Serial.printf("[Broadcast] %s\n", msg.c_str());
        } else if (cmd == "clients") {
            Serial.println("[Clients]");
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (clients[i].connected) {
                    Serial.printf("  #%d: %s (msgs: %lu)\n", 
                                 clients[i].num, 
                                 clients[i].name.c_str(),
                                 clients[i].messageCount);
                }
            }
        }
    }
}
