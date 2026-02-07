# Jobsheet Modul 13: Embedded Networking & IoT Protocols

## 🎯 Tujuan Praktikum
1.  Mahasiswa mampu menghubungkan mikrokontroler (ESP32/STM32) ke jaringan TCP/IP.
2.  Mahasiswa memahami penggunaan **Sockets** (TCP/UDP) pada sistem embedded.
3.  Mahasiswa mampu mengimplementasikan protokol aplikasi standar: **HTTP** dan **MQTT**.
4.  Mahasiswa dapat membuat sistem Telemetri data sensor ke Cloud Dashboard.

## ⚠️ Peringatan
- Pastikan kredensial WiFi (SSID & Password) sesuai dengan Access Point di Lab.
- Untuk STM32, pastikan modul Ethernet W5500 terhubung dengan pin SPI yang benar.
- Gunakan Serial Monitor dengan baudrate **115200**.

---

## 🛠️ Percobaan 1: Network Connectivity (WiFi / Ethernet)
**Tujuan:** Menghubungkan device ke jaringan lokal dan mendapatkan IP Address via DHCP.

### A. ESP32 (WiFi Station)
```cpp
#include <WiFi.h>

const char* ssid = "Lab_Embedded";
const char* password = "password123";

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);

  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nConnected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

void loop() {}
```

### B. STM32 (W5500 Ethernet)
*Note: Pastikan library `Ethernet` terinstall di platformio.ini.*
```cpp
#include <SPI.h>
#include <Ethernet.h>

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
// Pin CS untuk W5500 (STM32 Bluepill biasanya PA4)
#define W5500_CS_PIN PA4

void setup() {
  Serial.begin(115200);
  Ethernet.init(W5500_CS_PIN);

  Serial.println("Initialize Ethernet with DHCP...");
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    // Cek hardware
    if (Ethernet.hardwareStatus() == EthernetNoHardware) {
      Serial.println("W5500 not found!");
    } else if (Ethernet.linkStatus() == LinkOFF) {
      Serial.println("Ethernet cable is not connected.");
    }
    while (true) delay(1);
  }

  Serial.print("IP Address: ");
  Serial.println(Ethernet.localIP());
}

void loop() {
  Ethernet.maintain(); // Perbaharui lease DHCP jika perlu
}
```

---

## 🛠️ Percobaan 2: HTTP Client (GET Request)
**Tujuan:** Mengambil data dari internet (API public) menggunakan HTTP GET method.
**Target URL:** `http://jsonplaceholder.typicode.com/todos/1` (Contoh JSON API).

### Kode Program (ESP32 Example)
```cpp
#include <WiFi.h>
#include <HTTPClient.h>

const char* ssid = "Lab_Embedded";
const char* password = "password123";

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  while(WiFi.status() != WL_CONNECTED) delay(500);
}

void loop() {
  if(WiFi.status() == WL_CONNECTED){
    HTTPClient http;
    // Domain & Path
    http.begin("http://jsonplaceholder.typicode.com/todos/1"); 
    
    int httpResponseCode = http.GET();
    
    if (httpResponseCode > 0) {
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
      String payload = http.getString();
      Serial.println(payload);
    }
    else {
      Serial.print("Error code: ");
      Serial.println(httpResponseCode);
    }
    http.end();
  }
  delay(10000); // Request setiap 10 detik
}
```

---

## 🛠️ Percobaan 3: MQTT Publisher & Subscriber
**Tujuan:** Mengirim dan menerima data realtime menggunakan protokol MQTT.
**Broker:** Gunakan Public Broker (misal `broker.hivemq.com` atau `test.mosquitto.org`) atau Broker lokal Lab.

### Konfigurasi
- **Server**: `broker.hivemq.com`
- **Port**: 1883
- **Topic Pub**: `lab/embedded/nim_anda/sensor`
- **Topic Sub**: `lab/embedded/nim_anda/control`

### Kode Program (Menggunakan Library `PubSubClient`)
```cpp
#include <WiFi.h>
#include <PubSubClient.h>

const char* ssid = "Lab_Embedded";
const char* pass = "password123";
const char* mqtt_server = "broker.hivemq.com";

WiFiClient espClient;
PubSubClient client(espClient);

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);
    
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      // Subscribe topic control
      client.subscribe("lab/embedded/+/control");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, pass);
  while(WiFi.status() != WL_CONNECTED) delay(500);
  
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Publish data dummy setiap 2 detik
  static unsigned long lastMsg = 0;
  if (millis() - lastMsg > 2000) {
    lastMsg = millis();
    char msg[50];
    int value = random(0, 100);
    snprintf(msg, 50, "Temperature: %d", value);
    Serial.print("Publish message: ");
    Serial.println(msg);
    client.publish("lab/embedded/test/sensor", msg);
  }
}
```

---

## 🛠️ Percobaan 4: Simple Web Server (Control LED)
**Tujuan:** Mengontrol GPIO dari browser melalaui WiFi lokal.

### Kode Program (ESP32)
```cpp
#include <WiFi.h>
#include <WebServer.h>

WebServer server(80);
const int ledPin = 2;

void handleRoot() {
  String html = "<h1>ESP32 LED Control</h1>";
  html += "<p><a href=\"/on\"><button>ON</button></a></p>";
  html += "<p><a href=\"/off\"><button>OFF</button></a></p>";
  server.send(200, "text/html", html);
}

void handleOn() {
  digitalWrite(ledPin, HIGH);
  server.send(200, "text/plain", "LED is ON");
}

void handleOff() {
  digitalWrite(ledPin, LOW);
  server.send(200, "text/plain", "LED is OFF");
}

void setup() {
  pinMode(ledPin, OUTPUT);
  WiFi.begin("SSID", "PASS");
  while(WiFi.status() != WL_CONNECTED) delay(100);
  
  server.on("/", handleRoot);
  server.on("/on", handleOn);
  server.on("/off", handleOff);
  server.begin();
}

void loop() {
  server.handleClient();
}
```

