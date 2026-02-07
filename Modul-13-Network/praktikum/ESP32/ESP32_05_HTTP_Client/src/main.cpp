/*
 * ESP32 HTTP Client Demo
 * Modul 13: Network Communication
 * 
 * Demonstrates:
 * - HTTP GET requests
 * - HTTP POST requests with JSON
 * - HTTPS with certificate
 * - REST API interaction
 * - Response parsing
 */

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFiClientSecure.h>

// WiFi credentials
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// Test endpoints
const char* httpbin_get = "http://httpbin.org/get";
const char* httpbin_post = "http://httpbin.org/post";
const char* jsonplaceholder_posts = "http://jsonplaceholder.typicode.com/posts";
const char* jsonplaceholder_users = "http://jsonplaceholder.typicode.com/users/1";
const char* openweather_api = "http://api.openweathermap.org/data/2.5/weather";

// Statistics
unsigned long requestCount = 0;
unsigned long successCount = 0;
unsigned long failCount = 0;
unsigned long totalBytes = 0;

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
    } else {
        Serial.println(" Failed!");
    }
}

// Simple GET request
void httpGetSimple() {
    Serial.println("\n═══════════════════════════════════════");
    Serial.println("        Simple HTTP GET Request         ");
    Serial.println("═══════════════════════════════════════");
    
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[ERROR] WiFi not connected");
        return;
    }
    
    HTTPClient http;
    requestCount++;
    
    Serial.printf("[HTTP] GET: %s\n", httpbin_get);
    
    http.begin(httpbin_get);
    http.setTimeout(10000);
    
    unsigned long startTime = millis();
    int httpCode = http.GET();
    unsigned long duration = millis() - startTime;
    
    if (httpCode > 0) {
        successCount++;
        Serial.printf("[HTTP] Response code: %d\n", httpCode);
        Serial.printf("[HTTP] Response time: %lu ms\n", duration);
        
        if (httpCode == HTTP_CODE_OK) {
            String payload = http.getString();
            totalBytes += payload.length();
            
            Serial.printf("[HTTP] Payload size: %d bytes\n", payload.length());
            Serial.println("\n[Response Preview]");
            
            // Print first 500 characters
            if (payload.length() > 500) {
                Serial.println(payload.substring(0, 500));
                Serial.println("... (truncated)");
            } else {
                Serial.println(payload);
            }
        }
    } else {
        failCount++;
        Serial.printf("[HTTP] Error: %s\n", http.errorToString(httpCode).c_str());
    }
    
    http.end();
}

// GET with JSON parsing
void httpGetJSON() {
    Serial.println("\n═══════════════════════════════════════");
    Serial.println("      HTTP GET with JSON Parsing        ");
    Serial.println("═══════════════════════════════════════");
    
    if (WiFi.status() != WL_CONNECTED) return;
    
    HTTPClient http;
    requestCount++;
    
    Serial.printf("[HTTP] GET: %s\n", jsonplaceholder_users);
    
    http.begin(jsonplaceholder_users);
    int httpCode = http.GET();
    
    if (httpCode == HTTP_CODE_OK) {
        successCount++;
        String payload = http.getString();
        totalBytes += payload.length();
        
        // Parse JSON
        StaticJsonDocument<1024> doc;
        DeserializationError error = deserializeJson(doc, payload);
        
        if (!error) {
            Serial.println("\n[Parsed User Data]");
            Serial.println("┌──────────────────────────────────────┐");
            Serial.printf("│ ID      : %-26d │\n", doc["id"].as<int>());
            Serial.printf("│ Name    : %-26s │\n", doc["name"].as<const char*>());
            Serial.printf("│ Username: %-26s │\n", doc["username"].as<const char*>());
            Serial.printf("│ Email   : %-26s │\n", doc["email"].as<const char*>());
            Serial.printf("│ Phone   : %-26s │\n", doc["phone"].as<const char*>());
            Serial.printf("│ Website : %-26s │\n", doc["website"].as<const char*>());
            Serial.println("├──────────────────────────────────────┤");
            Serial.println("│ Address:                             │");
            Serial.printf("│   Street: %-25s │\n", doc["address"]["street"].as<const char*>());
            Serial.printf("│   City  : %-25s │\n", doc["address"]["city"].as<const char*>());
            Serial.printf("│   Zip   : %-25s │\n", doc["address"]["zipcode"].as<const char*>());
            Serial.println("└──────────────────────────────────────┘");
        } else {
            Serial.printf("[ERROR] JSON parse failed: %s\n", error.c_str());
        }
    } else {
        failCount++;
        Serial.printf("[HTTP] Error: %s\n", http.errorToString(httpCode).c_str());
    }
    
    http.end();
}

// POST request with JSON body
void httpPostJSON() {
    Serial.println("\n═══════════════════════════════════════");
    Serial.println("        HTTP POST with JSON Body        ");
    Serial.println("═══════════════════════════════════════");
    
    if (WiFi.status() != WL_CONNECTED) return;
    
    HTTPClient http;
    requestCount++;
    
    // Create JSON payload
    StaticJsonDocument<256> doc;
    doc["title"] = "ESP32 Test Post";
    doc["body"] = "This is a test post from ESP32";
    doc["userId"] = 1;
    doc["device"] = "ESP32";
    doc["timestamp"] = millis();
    
    char jsonBuffer[256];
    serializeJson(doc, jsonBuffer);
    
    Serial.printf("[HTTP] POST: %s\n", jsonplaceholder_posts);
    Serial.printf("[HTTP] Body: %s\n", jsonBuffer);
    
    http.begin(jsonplaceholder_posts);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Accept", "application/json");
    
    unsigned long startTime = millis();
    int httpCode = http.POST(jsonBuffer);
    unsigned long duration = millis() - startTime;
    
    if (httpCode == HTTP_CODE_CREATED || httpCode == HTTP_CODE_OK) {
        successCount++;
        String response = http.getString();
        totalBytes += response.length();
        
        Serial.printf("[HTTP] Response code: %d (Created)\n", httpCode);
        Serial.printf("[HTTP] Response time: %lu ms\n", duration);
        
        // Parse response
        StaticJsonDocument<512> respDoc;
        if (!deserializeJson(respDoc, response)) {
            Serial.println("\n[Response]");
            Serial.printf("  ID created: %d\n", respDoc["id"].as<int>());
            Serial.printf("  Title: %s\n", respDoc["title"].as<const char*>());
        }
    } else {
        failCount++;
        Serial.printf("[HTTP] Error: %s\n", http.errorToString(httpCode).c_str());
    }
    
    http.end();
}

// GET with custom headers
void httpGetWithHeaders() {
    Serial.println("\n═══════════════════════════════════════");
    Serial.println("      HTTP GET with Custom Headers      ");
    Serial.println("═══════════════════════════════════════");
    
    if (WiFi.status() != WL_CONNECTED) return;
    
    HTTPClient http;
    requestCount++;
    
    http.begin(httpbin_get);
    
    // Add custom headers
    http.addHeader("User-Agent", "ESP32/1.0");
    http.addHeader("Accept", "application/json");
    http.addHeader("X-Custom-Header", "ESP32-Demo");
    http.addHeader("X-Device-ID", WiFi.macAddress());
    
    Serial.printf("[HTTP] GET with headers: %s\n", httpbin_get);
    
    int httpCode = http.GET();
    
    if (httpCode == HTTP_CODE_OK) {
        successCount++;
        String payload = http.getString();
        totalBytes += payload.length();
        
        // Parse and show headers that were received by server
        StaticJsonDocument<2048> doc;
        if (!deserializeJson(doc, payload)) {
            Serial.println("\n[Server Received Headers]");
            JsonObject headers = doc["headers"];
            for (JsonPair kv : headers) {
                Serial.printf("  %s: %s\n", kv.key().c_str(), kv.value().as<const char*>());
            }
        }
    } else {
        failCount++;
    }
    
    http.end();
}

// GET multiple posts
void httpGetPosts() {
    Serial.println("\n═══════════════════════════════════════");
    Serial.println("         HTTP GET Posts List            ");
    Serial.println("═══════════════════════════════════════");
    
    if (WiFi.status() != WL_CONNECTED) return;
    
    HTTPClient http;
    requestCount++;
    
    // Get first 5 posts
    String url = String(jsonplaceholder_posts) + "?_limit=5";
    
    Serial.printf("[HTTP] GET: %s\n", url.c_str());
    
    http.begin(url);
    int httpCode = http.GET();
    
    if (httpCode == HTTP_CODE_OK) {
        successCount++;
        String payload = http.getString();
        totalBytes += payload.length();
        
        // Parse JSON array
        DynamicJsonDocument doc(4096);
        if (!deserializeJson(doc, payload)) {
            JsonArray posts = doc.as<JsonArray>();
            
            Serial.printf("\n[Posts List] (%d items)\n", posts.size());
            Serial.println("┌────┬────────────────────────────────────────────┐");
            Serial.println("│ ID │ Title                                      │");
            Serial.println("├────┼────────────────────────────────────────────┤");
            
            for (JsonObject post : posts) {
                int id = post["id"];
                String title = post["title"].as<String>();
                
                // Truncate title if too long
                if (title.length() > 40) {
                    title = title.substring(0, 37) + "...";
                }
                
                Serial.printf("│ %2d │ %-42s │\n", id, title.c_str());
            }
            
            Serial.println("└────┴────────────────────────────────────────────┘");
        }
    } else {
        failCount++;
    }
    
    http.end();
}

// PUT request
void httpPutRequest() {
    Serial.println("\n═══════════════════════════════════════");
    Serial.println("            HTTP PUT Request            ");
    Serial.println("═══════════════════════════════════════");
    
    if (WiFi.status() != WL_CONNECTED) return;
    
    HTTPClient http;
    requestCount++;
    
    String url = String(jsonplaceholder_posts) + "/1";
    
    // Create update payload
    StaticJsonDocument<256> doc;
    doc["id"] = 1;
    doc["title"] = "Updated from ESP32";
    doc["body"] = "This post was updated using ESP32 HTTP PUT request";
    doc["userId"] = 1;
    
    char jsonBuffer[256];
    serializeJson(doc, jsonBuffer);
    
    Serial.printf("[HTTP] PUT: %s\n", url.c_str());
    
    http.begin(url);
    http.addHeader("Content-Type", "application/json");
    
    int httpCode = http.PUT(jsonBuffer);
    
    if (httpCode == HTTP_CODE_OK) {
        successCount++;
        Serial.println("[HTTP] Update successful!");
        
        String response = http.getString();
        totalBytes += response.length();
        
        StaticJsonDocument<256> respDoc;
        if (!deserializeJson(respDoc, response)) {
            Serial.printf("  Updated title: %s\n", respDoc["title"].as<const char*>());
        }
    } else {
        failCount++;
        Serial.printf("[HTTP] Error: %s\n", http.errorToString(httpCode).c_str());
    }
    
    http.end();
}

// DELETE request
void httpDeleteRequest() {
    Serial.println("\n═══════════════════════════════════════");
    Serial.println("          HTTP DELETE Request           ");
    Serial.println("═══════════════════════════════════════");
    
    if (WiFi.status() != WL_CONNECTED) return;
    
    HTTPClient http;
    requestCount++;
    
    String url = String(jsonplaceholder_posts) + "/1";
    
    Serial.printf("[HTTP] DELETE: %s\n", url.c_str());
    
    http.begin(url);
    
    // Use sendRequest for DELETE
    int httpCode = http.sendRequest("DELETE");
    
    if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_NO_CONTENT) {
        successCount++;
        Serial.println("[HTTP] Delete successful!");
        Serial.printf("[HTTP] Response code: %d\n", httpCode);
    } else {
        failCount++;
        Serial.printf("[HTTP] Error: %s\n", http.errorToString(httpCode).c_str());
    }
    
    http.end();
}

void printStats() {
    Serial.println("\n╔══════════════════════════════════════╗");
    Serial.println("║       HTTP Client Statistics         ║");
    Serial.println("╠══════════════════════════════════════╣");
    Serial.printf("║ Total Requests   : %-18lu ║\n", requestCount);
    Serial.printf("║ Successful       : %-18lu ║\n", successCount);
    Serial.printf("║ Failed           : %-18lu ║\n", failCount);
    Serial.printf("║ Success Rate     : %-17.1f%% ║\n", 
                 requestCount > 0 ? (successCount * 100.0 / requestCount) : 0);
    Serial.printf("║ Total Data       : %-14lu bytes ║\n", totalBytes);
    Serial.printf("║ Free Heap        : %-14lu bytes ║\n", ESP.getFreeHeap());
    Serial.println("╚══════════════════════════════════════╝");
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\n╔══════════════════════════════════════╗");
    Serial.println("║      ESP32 HTTP Client Demo          ║");
    Serial.println("║   Modul 13: Network Communication    ║");
    Serial.println("╚══════════════════════════════════════╝\n");
    
    // Connect to WiFi
    connectWiFi();
    
    Serial.println("\n[Commands]");
    Serial.println("  get     - Simple GET request");
    Serial.println("  json    - GET with JSON parsing");
    Serial.println("  post    - POST with JSON body");
    Serial.println("  headers - GET with custom headers");
    Serial.println("  posts   - GET posts list");
    Serial.println("  put     - PUT request");
    Serial.println("  delete  - DELETE request");
    Serial.println("  all     - Run all tests");
    Serial.println("  stats   - Show statistics");
    Serial.println("  help    - Show this help");
    Serial.println();
}

void loop() {
    // Handle serial commands
    if (Serial.available()) {
        String cmd = Serial.readStringUntil('\n');
        cmd.trim();
        cmd.toLowerCase();
        
        if (cmd == "get") {
            httpGetSimple();
        } else if (cmd == "json") {
            httpGetJSON();
        } else if (cmd == "post") {
            httpPostJSON();
        } else if (cmd == "headers") {
            httpGetWithHeaders();
        } else if (cmd == "posts") {
            httpGetPosts();
        } else if (cmd == "put") {
            httpPutRequest();
        } else if (cmd == "delete") {
            httpDeleteRequest();
        } else if (cmd == "all") {
            httpGetSimple();
            delay(1000);
            httpGetJSON();
            delay(1000);
            httpPostJSON();
            delay(1000);
            httpGetWithHeaders();
            delay(1000);
            httpGetPosts();
            delay(1000);
            httpPutRequest();
            delay(1000);
            httpDeleteRequest();
            delay(1000);
            printStats();
        } else if (cmd == "stats") {
            printStats();
        } else if (cmd == "help") {
            Serial.println("\nCommands:");
            Serial.println("  get     - Simple GET request");
            Serial.println("  json    - GET with JSON parsing");
            Serial.println("  post    - POST with JSON body");
            Serial.println("  headers - GET with custom headers");
            Serial.println("  posts   - GET posts list");
            Serial.println("  put     - PUT request");
            Serial.println("  delete  - DELETE request");
            Serial.println("  all     - Run all tests");
            Serial.println("  stats   - Show statistics");
        }
    }
    
    delay(10);
}
