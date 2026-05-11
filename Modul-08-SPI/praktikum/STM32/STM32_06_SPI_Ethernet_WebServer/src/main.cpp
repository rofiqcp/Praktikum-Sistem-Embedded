/* STM32_06_SPI_Ethernet_WebServer
 * Web server berbasis W5500 Ethernet dengan Arduino framework
 * Mendukung bluepill_f103c8 dan blackpill_f401cc
 *
 * Endpoint:
 *   GET /          -> halaman status HTML
 *   GET /sensor    -> data sensor JSON
 *   GET /led/on    -> nyalakan LED PC13
 *   GET /led/off   -> matikan LED PC13
 *   GET /relay/on  -> aktifkan relay PB1
 *   GET /relay/off -> matikan relay PB1
 *
 * Hardware:
 *   W5500: CS=PA4, RST=PA3, SPI1 (PA5/PA6/PA7)
 *   LED   : PC13 (active LOW)
 *   Relay : PB1  (active HIGH)
 *   ADC   : PA0 (temp sim), PA1 (light sim)
 */

#include <Arduino.h>
#include <SPI.h>
#include <Ethernet.h>
#include "config.h"

/* =========================================================
 * Network configuration
 * ========================================================= */
byte mac[]     = MAC_ADDRESS;
IPAddress ip(IP_ADDRESS);
IPAddress dns_ip(DNS_SERVER);
IPAddress gateway(GATEWAY);
IPAddress subnet(SUBNET);

EthernetServer server(WEB_SERVER_PORT);

/* =========================================================
 * State
 * ========================================================= */
static bool led_state   = false;
static bool relay_state = false;
static uint32_t request_count = 0;

/* =========================================================
 * Sensor reading (simulated ADC)
 * ========================================================= */
static float readTemperature(void) {
    int raw = analogRead(TEMP_SENSOR_PIN);
    return 20.0f + (raw * 30.0f / 1023.0f);
}

static int readLight(void) {
    return analogRead(LIGHT_SENSOR_PIN);
}

/* =========================================================
 * HTTP response helpers
 * ========================================================= */
static void sendResponseLine(EthernetClient &client, const char *line) {
    client.println(line);
}

static void sendHTMLPage(EthernetClient &client) {
    char buf[128];
    float temp = readTemperature();
    int   light = readLight();

    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/html");
    client.println("Connection: close");
    client.println();
    client.println("<!DOCTYPE html><html><head>");
    client.println("<meta charset='UTF-8'>");
    client.println("<meta name='viewport' content='width=device-width,initial-scale=1'>");
    client.println("<title>STM32 Web Server</title>");
    client.println("<style>body{font-family:sans-serif;margin:20px}"
                   "h1{color:#333}.card{background:#f0f0f0;padding:10px;margin:8px 0;"
                   "border-radius:6px}.btn{padding:6px 14px;margin:4px;cursor:pointer}</style>");
    client.println("</head><body>");
    client.println("<h1>STM32 SPI Ethernet Web Server</h1>");

    /* Sensor card */
    snprintf(buf, sizeof(buf),
             "<div class='card'><b>Sensor</b><br>"
             "Suhu: %.1f &deg;C | Cahaya: %d ADC</div>", temp, light);
    client.println(buf);

    /* Control card */
    snprintf(buf, sizeof(buf),
             "<div class='card'><b>LED (PC13):</b> %s &nbsp;"
             "<a href='/led/on'><button class='btn'>ON</button></a>"
             "<a href='/led/off'><button class='btn'>OFF</button></a></div>",
             led_state ? "ON" : "OFF");
    client.println(buf);

    snprintf(buf, sizeof(buf),
             "<div class='card'><b>Relay (PB1):</b> %s &nbsp;"
             "<a href='/relay/on'><button class='btn'>ON</button></a>"
             "<a href='/relay/off'><button class='btn'>OFF</button></a></div>",
             relay_state ? "AKTIF" : "MATI");
    client.println(buf);

    snprintf(buf, sizeof(buf),
             "<div class='card'>Request: %lu | Uptime: %lu ms</div>",
             (unsigned long)request_count, (unsigned long)millis());
    client.println(buf);

    client.println("<p><a href='/sensor'>API JSON /sensor</a></p>");
    client.println("</body></html>");
}

static void sendJSONSensor(EthernetClient &client) {
    char buf[256];
    float temp = readTemperature();
    int   light = readLight();

    snprintf(buf, sizeof(buf),
             "{\"temperature\":%.2f,\"light\":%d,"
             "\"led\":%s,\"relay\":%s,"
             "\"uptime\":%lu,\"requests\":%lu}",
             temp, light,
             led_state ? "true" : "false",
             relay_state ? "true" : "false",
             (unsigned long)millis(),
             (unsigned long)request_count);

    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: application/json");
    client.println("Access-Control-Allow-Origin: *");
    client.println("Connection: close");
    client.println();
    client.println(buf);
}

static void send302(EthernetClient &client, const char *location) {
    client.print("HTTP/1.1 302 Found\r\nLocation: ");
    client.println(location);
    client.println("Connection: close\r\n");
}

static void send404(EthernetClient &client) {
    client.println("HTTP/1.1 404 Not Found");
    client.println("Content-Type: text/plain");
    client.println("Connection: close");
    client.println();
    client.println("404 Not Found");
}

/* =========================================================
 * Process one HTTP request
 * ========================================================= */
static void handleClient(EthernetClient &client) {
    char req[128] = {0};
    int  pos = 0;
    bool firstLine = true;

    unsigned long timeout = millis() + 1500;
    while (client.connected() && millis() < timeout) {
        if (client.available()) {
            char c = client.read();
            if (firstLine) {
                if (c == '\n') {
                    firstLine = false;
                } else if (pos < (int)sizeof(req) - 1) {
                    req[pos++] = c;
                }
            } else {
                /* Discard rest of headers */
                if (c == '\n') break;
            }
        }
    }

    request_count++;
    Serial.print("REQ: ");
    Serial.println(req);

    /* Parse path from "GET /path HTTP/1.1" */
    char *path = NULL;
    char *tok = strtok(req, " ");
    if (tok) tok = strtok(NULL, " ");
    if (tok) path = tok;

    if (!path) {
        send404(client);
    } else if (strcmp(path, "/") == 0 || strcmp(path, "/index.html") == 0) {
        sendHTMLPage(client);
    } else if (strcmp(path, "/sensor") == 0) {
        sendJSONSensor(client);
    } else if (strcmp(path, "/led/on") == 0) {
        led_state = true;
        digitalWrite(LED_PIN, LOW); /* Active LOW */
        send302(client, "/");
    } else if (strcmp(path, "/led/off") == 0) {
        led_state = false;
        digitalWrite(LED_PIN, HIGH);
        send302(client, "/");
    } else if (strcmp(path, "/relay/on") == 0) {
        relay_state = true;
        digitalWrite(RELAY_PIN, HIGH);
        send302(client, "/");
    } else if (strcmp(path, "/relay/off") == 0) {
        relay_state = false;
        digitalWrite(RELAY_PIN, LOW);
        send302(client, "/");
    } else {
        send404(client);
    }

    delay(1);
    client.stop();
}

/* =========================================================
 * Setup & Loop
 * ========================================================= */
void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("\r\n=== STM32 SPI Ethernet Web Server ===");

    /* GPIO */
    pinMode(LED_PIN,   OUTPUT);
    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(LED_PIN,   HIGH); /* LED off */
    digitalWrite(RELAY_PIN, LOW);  /* Relay off */

    pinMode(ETH_RST_PIN, OUTPUT);
    digitalWrite(ETH_RST_PIN, LOW);
    delay(50);
    digitalWrite(ETH_RST_PIN, HIGH);
    delay(200);

    /* SPI */
    SPI.begin();
    Ethernet.init(ETH_CS_PIN);

    /* Start Ethernet */
    Serial.println("Starting Ethernet...");
    if (Ethernet.begin(mac) == 0) {
        /* DHCP failed, use static IP */
        Serial.println("DHCP failed, using static IP");
        Ethernet.begin(mac, ip, dns_ip, gateway, subnet);
    }

    delay(1000);
    Serial.print("IP Address: ");
    Serial.println(Ethernet.localIP());

    if (Ethernet.hardwareStatus() == EthernetNoHardware) {
        Serial.println("ERROR: W5500 not found!");
        while (true) { delay(1000); }
    }
    if (Ethernet.linkStatus() == LinkOFF) {
        Serial.println("WARNING: Ethernet cable not connected.");
    }

    server.begin();
    Serial.print("Web server running on port ");
    Serial.println(WEB_SERVER_PORT);
    Serial.println("Endpoints: / | /sensor | /led/on | /led/off | /relay/on | /relay/off");
}

void loop() {
    EthernetClient client = server.available();
    if (client) {
        Serial.println("Client connected");
        handleClient(client);
        Serial.println("Client disconnected");
    }

    /* Maintain DHCP lease */
    Ethernet.maintain();
}
