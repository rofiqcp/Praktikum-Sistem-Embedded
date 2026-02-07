/*
 * ESP32 WiFi Access Point Mode Demo
 * Modul 13: Network Communication
 * 
 * Demonstrates:
 * - WiFi Access Point configuration
 * - Captive portal web server
 * - Client connection tracking
 * - Configuration form handling
 */

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>

// AP Configuration
const char* ap_ssid = "ESP32_ConfigPortal";
const char* ap_password = "12345678";  // Minimum 8 characters

// IP Configuration
IPAddress local_IP(192, 168, 4, 1);
IPAddress gateway(192, 168, 4, 1);
IPAddress subnet(255, 255, 255, 0);

// Servers
WebServer server(80);
DNSServer dnsServer;

// Client tracking
int connectedClients = 0;
unsigned long totalConnections = 0;

// Stored configuration
String storedSSID = "";
String storedPassword = "";
bool configReceived = false;

// HTML Pages
const char* captivePortalHTML = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP32 Configuration Portal</title>
    <style>
        * {
            box-sizing: border-box;
            margin: 0;
            padding: 0;
        }
        body {
            font-family: 'Segoe UI', Arial, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            display: flex;
            justify-content: center;
            align-items: center;
            padding: 20px;
        }
        .container {
            background: white;
            border-radius: 20px;
            box-shadow: 0 20px 60px rgba(0,0,0,0.3);
            padding: 40px;
            width: 100%;
            max-width: 400px;
        }
        h1 {
            color: #333;
            text-align: center;
            margin-bottom: 10px;
            font-size: 24px;
        }
        .subtitle {
            color: #666;
            text-align: center;
            margin-bottom: 30px;
            font-size: 14px;
        }
        .form-group {
            margin-bottom: 20px;
        }
        label {
            display: block;
            color: #555;
            margin-bottom: 8px;
            font-weight: 500;
        }
        input[type="text"],
        input[type="password"],
        select {
            width: 100%;
            padding: 12px 15px;
            border: 2px solid #e1e1e1;
            border-radius: 10px;
            font-size: 16px;
            transition: border-color 0.3s;
        }
        input:focus,
        select:focus {
            border-color: #667eea;
            outline: none;
        }
        button {
            width: 100%;
            padding: 15px;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            border: none;
            border-radius: 10px;
            font-size: 16px;
            font-weight: bold;
            cursor: pointer;
            transition: transform 0.2s, box-shadow 0.2s;
        }
        button:hover {
            transform: translateY(-2px);
            box-shadow: 0 5px 20px rgba(102, 126, 234, 0.4);
        }
        .info-box {
            background: #f8f9fa;
            border-radius: 10px;
            padding: 15px;
            margin-top: 20px;
        }
        .info-box h3 {
            color: #333;
            font-size: 14px;
            margin-bottom: 10px;
        }
        .info-item {
            display: flex;
            justify-content: space-between;
            padding: 5px 0;
            font-size: 13px;
            color: #666;
        }
        .status {
            display: inline-block;
            padding: 3px 10px;
            border-radius: 20px;
            font-size: 12px;
            font-weight: bold;
        }
        .status.online {
            background: #d4edda;
            color: #155724;
        }
        .networks {
            margin-top: 20px;
        }
        .network-item {
            padding: 10px;
            border: 1px solid #e1e1e1;
            border-radius: 8px;
            margin-bottom: 8px;
            cursor: pointer;
            transition: background 0.2s;
        }
        .network-item:hover {
            background: #f0f0f0;
        }
        .signal {
            float: right;
            color: #667eea;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>🔧 ESP32 Config</h1>
        <p class="subtitle">Configure WiFi Connection</p>
        
        <form action="/save" method="POST">
            <div class="form-group">
                <label>WiFi Network (SSID)</label>
                <input type="text" name="ssid" id="ssid" placeholder="Enter network name" required>
            </div>
            
            <div class="form-group">
                <label>Password</label>
                <input type="password" name="password" placeholder="Enter password">
            </div>
            
            <button type="submit">💾 Save & Connect</button>
        </form>
        
        <div class="info-box">
            <h3>📊 Device Information</h3>
            <div class="info-item">
                <span>Status</span>
                <span class="status online">AP Mode</span>
            </div>
            <div class="info-item">
                <span>AP IP</span>
                <span>%AP_IP%</span>
            </div>
            <div class="info-item">
                <span>Connected Clients</span>
                <span>%CLIENTS%</span>
            </div>
            <div class="info-item">
                <span>Free Heap</span>
                <span>%HEAP% bytes</span>
            </div>
            <div class="info-item">
                <span>Uptime</span>
                <span>%UPTIME% sec</span>
            </div>
        </div>
        
        <div class="networks">
            <h3 style="margin-bottom:10px; color:#333;">📶 Available Networks</h3>
            <div id="networkList">
                %NETWORKS%
            </div>
        </div>
    </div>
    
    <script>
        document.querySelectorAll('.network-item').forEach(item => {
            item.addEventListener('click', () => {
                document.getElementById('ssid').value = item.getAttribute('data-ssid');
            });
        });
    </script>
</body>
</html>
)rawliteral";

const char* successHTML = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Configuration Saved</title>
    <style>
        body {
            font-family: 'Segoe UI', Arial, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            display: flex;
            justify-content: center;
            align-items: center;
            padding: 20px;
        }
        .container {
            background: white;
            border-radius: 20px;
            padding: 40px;
            text-align: center;
            max-width: 400px;
        }
        .checkmark {
            font-size: 60px;
            margin-bottom: 20px;
        }
        h1 { color: #28a745; }
        p { color: #666; margin: 10px 0; }
        .ssid { 
            font-weight: bold; 
            color: #333;
            padding: 10px;
            background: #f0f0f0;
            border-radius: 5px;
            margin: 10px 0;
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="checkmark">✅</div>
        <h1>Configuration Saved!</h1>
        <p>WiFi credentials stored:</p>
        <div class="ssid">%SSID%</div>
        <p>The device will attempt to connect to this network.</p>
        <p style="font-size:12px; color:#999;">You can close this page.</p>
    </div>
</body>
</html>
)rawliteral";

String scanNetworks() {
    Serial.println("[AP] Scanning networks...");
    int numNetworks = WiFi.scanNetworks(false, true);
    String html = "";
    
    if (numNetworks == 0) {
        html = "<p style='color:#999;'>No networks found</p>";
    } else {
        for (int i = 0; i < min(numNetworks, 10); i++) {
            int rssi = WiFi.RSSI(i);
            String signal = "📶";
            if (rssi < -70) signal = "📶";
            if (rssi < -80) signal = "📵";
            
            html += "<div class='network-item' data-ssid='" + WiFi.SSID(i) + "'>";
            html += WiFi.SSID(i);
            html += "<span class='signal'>" + signal + " " + String(rssi) + "dBm</span>";
            html += "</div>";
        }
    }
    
    WiFi.scanDelete();
    return html;
}

String processTemplate(String html) {
    html.replace("%AP_IP%", WiFi.softAPIP().toString());
    html.replace("%CLIENTS%", String(WiFi.softAPgetStationNum()));
    html.replace("%HEAP%", String(ESP.getFreeHeap()));
    html.replace("%UPTIME%", String(millis() / 1000));
    html.replace("%NETWORKS%", scanNetworks());
    return html;
}

void handleRoot() {
    Serial.println("[Web] Serving root page");
    String html = processTemplate(captivePortalHTML);
    server.send(200, "text/html", html);
}

void handleSave() {
    if (server.hasArg("ssid")) {
        storedSSID = server.arg("ssid");
        storedPassword = server.arg("password");
        configReceived = true;
        
        Serial.println("\n[Config] Received new configuration:");
        Serial.printf("  SSID: %s\n", storedSSID.c_str());
        Serial.printf("  Password: %s\n", storedPassword.length() > 0 ? "****" : "(none)");
        
        String html = successHTML;
        html.replace("%SSID%", storedSSID);
        server.send(200, "text/html", html);
    } else {
        server.send(400, "text/plain", "Missing SSID parameter");
    }
}

void handleNotFound() {
    // Redirect all requests to root (captive portal behavior)
    server.sendHeader("Location", "http://" + WiFi.softAPIP().toString(), true);
    server.send(302, "text/plain", "");
}

void handleAPI() {
    String json = "{";
    json += "\"ap_ip\":\"" + WiFi.softAPIP().toString() + "\",";
    json += "\"clients\":" + String(WiFi.softAPgetStationNum()) + ",";
    json += "\"heap\":" + String(ESP.getFreeHeap()) + ",";
    json += "\"uptime\":" + String(millis() / 1000) + ",";
    json += "\"config_received\":" + String(configReceived ? "true" : "false");
    json += "}";
    
    server.send(200, "application/json", json);
}

void WiFiAPEvent(WiFiEvent_t event) {
    switch (event) {
        case SYSTEM_EVENT_AP_START:
            Serial.println("[AP] Access Point started");
            break;
        case SYSTEM_EVENT_AP_STOP:
            Serial.println("[AP] Access Point stopped");
            break;
        case SYSTEM_EVENT_AP_STACONNECTED:
            totalConnections++;
            Serial.printf("[AP] Client connected (Total: %lu)\n", totalConnections);
            break;
        case SYSTEM_EVENT_AP_STADISCONNECTED:
            Serial.println("[AP] Client disconnected");
            break;
        case SYSTEM_EVENT_AP_STAIPASSIGNED:
            Serial.println("[AP] IP assigned to client");
            break;
        default:
            break;
    }
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\n╔══════════════════════════════════════╗");
    Serial.println("║   ESP32 WiFi Access Point Demo       ║");
    Serial.println("║   Modul 13: Network Communication    ║");
    Serial.println("╚══════════════════════════════════════╝\n");
    
    // Register event handler
    WiFi.onEvent(WiFiAPEvent);
    
    // Configure AP
    Serial.println("[AP] Configuring Access Point...");
    WiFi.mode(WIFI_AP);
    
    if (!WiFi.softAPConfig(local_IP, gateway, subnet)) {
        Serial.println("[AP] Failed to configure IP!");
    }
    
    // Start AP with configuration
    // Parameters: SSID, password, channel, hidden, max_connections
    if (WiFi.softAP(ap_ssid, ap_password, 1, 0, 8)) {
        Serial.println("[AP] Access Point started successfully");
        Serial.printf("[AP] SSID: %s\n", ap_ssid);
        Serial.printf("[AP] Password: %s\n", ap_password);
        Serial.printf("[AP] IP Address: %s\n", WiFi.softAPIP().toString().c_str());
        Serial.printf("[AP] MAC Address: %s\n", WiFi.softAPmacAddress().c_str());
    } else {
        Serial.println("[AP] Failed to start Access Point!");
    }
    
    // Start DNS server for captive portal
    dnsServer.start(53, "*", local_IP);
    Serial.println("[DNS] Captive portal DNS started");
    
    // Configure web server routes
    server.on("/", handleRoot);
    server.on("/save", HTTP_POST, handleSave);
    server.on("/api", handleAPI);
    server.on("/generate_204", handleRoot);  // Android captive portal
    server.on("/fwlink", handleRoot);        // Microsoft captive portal
    server.onNotFound(handleNotFound);
    
    server.begin();
    Serial.println("[Web] HTTP server started on port 80");
    
    Serial.println("\n[Ready] Connect to WiFi network: " + String(ap_ssid));
    Serial.println("[Ready] Then open browser to http://192.168.4.1");
}

void loop() {
    // Handle DNS requests (for captive portal)
    dnsServer.processNextRequest();
    
    // Handle HTTP requests
    server.handleClient();
    
    // Track client count changes
    static int lastClientCount = 0;
    int currentClients = WiFi.softAPgetStationNum();
    if (currentClients != lastClientCount) {
        lastClientCount = currentClients;
        Serial.printf("[AP] Connected clients: %d\n", currentClients);
    }
    
    // Print status periodically
    static unsigned long lastStatus = 0;
    if (millis() - lastStatus > 30000) {
        Serial.printf("\n[Status] AP: %s | Clients: %d | Heap: %d | Config: %s\n",
                     ap_ssid,
                     WiFi.softAPgetStationNum(),
                     ESP.getFreeHeap(),
                     configReceived ? "Received" : "Waiting");
        lastStatus = millis();
    }
    
    // Handle serial commands
    if (Serial.available()) {
        String cmd = Serial.readStringUntil('\n');
        cmd.trim();
        
        if (cmd == "status") {
            Serial.println("\n=== AP Status ===");
            Serial.printf("SSID: %s\n", ap_ssid);
            Serial.printf("IP: %s\n", WiFi.softAPIP().toString().c_str());
            Serial.printf("Clients: %d\n", WiFi.softAPgetStationNum());
            Serial.printf("Total Connections: %lu\n", totalConnections);
            Serial.printf("Config Received: %s\n", configReceived ? "Yes" : "No");
            if (configReceived) {
                Serial.printf("Stored SSID: %s\n", storedSSID.c_str());
            }
        } else if (cmd == "help") {
            Serial.println("\nCommands: status, help");
        }
    }
}
