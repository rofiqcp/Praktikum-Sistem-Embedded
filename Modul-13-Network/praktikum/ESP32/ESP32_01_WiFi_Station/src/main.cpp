/*
 * ESP32 WiFi Station Mode Demo
 * Modul 13: Network Communication
 * 
 * Demonstrates:
 * - WiFi connection in Station mode
 * - WiFi event handling
 * - Auto-reconnection
 * - Network information display
 */

#include <Arduino.h>
#include <WiFi.h>

// WiFi credentials - CHANGE THESE
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// LED pin for status indication
const int LED_PIN = 2;

// Connection tracking
unsigned long connectionStartTime = 0;
int reconnectCount = 0;

// Function prototypes
void printWiFiStatus();
void WiFiEvent(WiFiEvent_t event);

void printWiFiStatus() {
    Serial.println("\n╔══════════════════════════════════════╗");
    Serial.println("║       WiFi Connection Status         ║");
    Serial.println("╠══════════════════════════════════════╣");
    Serial.printf("║ SSID     : %-25s ║\n", WiFi.SSID().c_str());
    Serial.printf("║ IP       : %-25s ║\n", WiFi.localIP().toString().c_str());
    Serial.printf("║ Gateway  : %-25s ║\n", WiFi.gatewayIP().toString().c_str());
    Serial.printf("║ Subnet   : %-25s ║\n", WiFi.subnetMask().toString().c_str());
    Serial.printf("║ DNS      : %-25s ║\n", WiFi.dnsIP().toString().c_str());
    Serial.printf("║ MAC      : %-25s ║\n", WiFi.macAddress().c_str());
    Serial.printf("║ RSSI     : %-20d dBm ║\n", WiFi.RSSI());
    Serial.printf("║ Channel  : %-25d ║\n", WiFi.channel());
    Serial.println("╚══════════════════════════════════════╝");
}

String getSignalQuality(int rssi) {
    if (rssi >= -50) return "Excellent";
    if (rssi >= -60) return "Good";
    if (rssi >= -70) return "Fair";
    if (rssi >= -80) return "Weak";
    return "Very Weak";
}

void WiFiEvent(WiFiEvent_t event) {
    switch (event) {
        case SYSTEM_EVENT_WIFI_READY:
            Serial.println("[WiFi] WiFi interface ready");
            break;
            
        case SYSTEM_EVENT_STA_START:
            Serial.println("[WiFi] Station mode started");
            break;
            
        case SYSTEM_EVENT_STA_CONNECTED:
            Serial.println("[WiFi] Connected to Access Point");
            break;
            
        case SYSTEM_EVENT_STA_GOT_IP:
            Serial.println("[WiFi] Got IP address");
            digitalWrite(LED_PIN, HIGH);  // LED ON = connected
            break;
            
        case SYSTEM_EVENT_STA_DISCONNECTED:
            Serial.println("[WiFi] Disconnected from Access Point");
            digitalWrite(LED_PIN, LOW);   // LED OFF = disconnected
            reconnectCount++;
            Serial.printf("[WiFi] Attempting reconnection... (attempt #%d)\n", reconnectCount);
            WiFi.reconnect();
            break;
            
        case SYSTEM_EVENT_STA_AUTHMODE_CHANGE:
            Serial.println("[WiFi] Authentication mode changed");
            break;
            
        case SYSTEM_EVENT_STA_LOST_IP:
            Serial.println("[WiFi] Lost IP address");
            break;
            
        default:
            Serial.printf("[WiFi] Event: %d\n", event);
            break;
    }
}

void scanNetworks() {
    Serial.println("\n[WiFi] Scanning available networks...");
    
    int numNetworks = WiFi.scanNetworks();
    
    if (numNetworks == 0) {
        Serial.println("[WiFi] No networks found");
    } else {
        Serial.printf("[WiFi] Found %d networks:\n", numNetworks);
        Serial.println("┌────┬──────────────────────────┬──────┬────────────┐");
        Serial.println("│ No │          SSID            │ RSSI │  Security  │");
        Serial.println("├────┼──────────────────────────┼──────┼────────────┤");
        
        for (int i = 0; i < numNetworks; i++) {
            String security = "OPEN";
            switch (WiFi.encryptionType(i)) {
                case WIFI_AUTH_WEP:
                    security = "WEP";
                    break;
                case WIFI_AUTH_WPA_PSK:
                    security = "WPA";
                    break;
                case WIFI_AUTH_WPA2_PSK:
                    security = "WPA2";
                    break;
                case WIFI_AUTH_WPA_WPA2_PSK:
                    security = "WPA/WPA2";
                    break;
                case WIFI_AUTH_WPA2_ENTERPRISE:
                    security = "WPA2-EAP";
                    break;
                case WIFI_AUTH_WPA3_PSK:
                    security = "WPA3";
                    break;
                default:
                    security = "UNKNOWN";
            }
            
            Serial.printf("│ %2d │ %-24s │ %4d │ %-10s │\n",
                         i + 1,
                         WiFi.SSID(i).substring(0, 24).c_str(),
                         WiFi.RSSI(i),
                         security.c_str());
        }
        Serial.println("└────┴──────────────────────────┴──────┴────────────┘");
    }
    
    WiFi.scanDelete();
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\n╔══════════════════════════════════════╗");
    Serial.println("║   ESP32 WiFi Station Mode Demo       ║");
    Serial.println("║   Modul 13: Network Communication    ║");
    Serial.println("╚══════════════════════════════════════╝\n");
    
    // Setup LED
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);
    
    // Register WiFi event handler
    WiFi.onEvent(WiFiEvent);
    
    // Scan for available networks first
    scanNetworks();
    
    // Configure WiFi
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    WiFi.persistent(true);
    
    // Set hostname
    WiFi.setHostname("ESP32-Station");
    
    Serial.printf("\n[WiFi] Connecting to %s", ssid);
    connectionStartTime = millis();
    WiFi.begin(ssid, password);
    
    // Wait for connection with timeout
    int timeout = 30;  // 30 seconds timeout
    while (WiFi.status() != WL_CONNECTED && timeout > 0) {
        delay(1000);
        Serial.print(".");
        timeout--;
        
        // Blink LED while connecting
        digitalWrite(LED_PIN, !digitalRead(LED_PIN));
    }
    
    Serial.println();
    
    if (WiFi.status() == WL_CONNECTED) {
        unsigned long connectionTime = millis() - connectionStartTime;
        Serial.printf("[WiFi] Connected in %lu ms\n", connectionTime);
        printWiFiStatus();
    } else {
        Serial.println("[WiFi] Connection failed!");
        Serial.println("[WiFi] Please check SSID and password");
    }
}

void loop() {
    static unsigned long lastStatusPrint = 0;
    static unsigned long lastRSSICheck = 0;
    
    // Print status every 30 seconds
    if (millis() - lastStatusPrint > 30000) {
        if (WiFi.status() == WL_CONNECTED) {
            Serial.printf("\n[Status] Connected to %s | IP: %s | RSSI: %d dBm (%s) | Uptime: %lu s\n",
                         WiFi.SSID().c_str(),
                         WiFi.localIP().toString().c_str(),
                         WiFi.RSSI(),
                         getSignalQuality(WiFi.RSSI()).c_str(),
                         millis() / 1000);
        } else {
            Serial.printf("\n[Status] Disconnected | Reconnect attempts: %d\n", reconnectCount);
        }
        lastStatusPrint = millis();
    }
    
    // Check RSSI and warn if signal is weak
    if (millis() - lastRSSICheck > 5000) {
        if (WiFi.status() == WL_CONNECTED) {
            int rssi = WiFi.RSSI();
            if (rssi < -80) {
                Serial.printf("[Warning] Weak signal: %d dBm\n", rssi);
                // Blink LED rapidly for weak signal
                for (int i = 0; i < 3; i++) {
                    digitalWrite(LED_PIN, LOW);
                    delay(100);
                    digitalWrite(LED_PIN, HIGH);
                    delay(100);
                }
            }
        }
        lastRSSICheck = millis();
    }
    
    // Handle serial commands
    if (Serial.available()) {
        String cmd = Serial.readStringUntil('\n');
        cmd.trim();
        cmd.toLowerCase();
        
        if (cmd == "status") {
            printWiFiStatus();
        } else if (cmd == "scan") {
            scanNetworks();
        } else if (cmd == "reconnect") {
            Serial.println("[WiFi] Manual reconnect requested");
            WiFi.disconnect();
            delay(1000);
            WiFi.begin(ssid, password);
        } else if (cmd == "help") {
            Serial.println("\nAvailable commands:");
            Serial.println("  status    - Show WiFi status");
            Serial.println("  scan      - Scan available networks");
            Serial.println("  reconnect - Force reconnection");
            Serial.println("  help      - Show this help");
        }
    }
}
