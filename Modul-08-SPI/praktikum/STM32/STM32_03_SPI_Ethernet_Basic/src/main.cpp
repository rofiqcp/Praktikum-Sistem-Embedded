#include <Arduino.h>
#include <SPI.h>
#include <Ethernet.h>
#include "config.h"

// Network configuration
byte mac[] = MAC_ADDRESS;
IPAddress ip(IP_ADDRESS);
IPAddress dnsServer(DNS_SERVER);
IPAddress gateway(GATEWAY);
IPAddress subnet(SUBNET);

EthernetClient client;

void printIPAddress() {
  Serial.print("IP Address: ");
  Serial.println(Ethernet.localIP());
  Serial.print("Subnet Mask: ");
  Serial.println(Ethernet.subnetMask());
  Serial.print("Gateway: ");
  Serial.println(Ethernet.gatewayIP());
  Serial.print("DNS Server: ");
  Serial.println(Ethernet.dnsServerIP());
}

void testConnection() {
  Serial.println("\n--- Testing HTTP Connection ---");
  Serial.print("Connecting to ");
  Serial.print(TEST_SERVER);
  Serial.println("...");
  
  if (client.connect(TEST_SERVER, TEST_PORT)) {
    Serial.println("Connected!");
    
    // Send HTTP GET request
    client.println("GET / HTTP/1.1");
    client.print("Host: ");
    client.println(TEST_SERVER);
    client.println("Connection: close");
    client.println();
    
    Serial.println("Request sent, waiting for response...");
    
    // Wait for response
    uint32_t timeout = millis();
    while (client.connected() && !client.available()) {
      if (millis() - timeout > 5000) {
        Serial.println("Timeout!");
        client.stop();
        return;
      }
      delay(10);
    }
    
    // Read response
    Serial.println("\nResponse:");
    Serial.println("----------");
    int lineCount = 0;
    while (client.available() && lineCount < 20) {
      String line = client.readStringUntil('\n');
      Serial.println(line);
      lineCount++;
    }
    Serial.println("----------");
    
    client.stop();
    Serial.println("\nConnection closed");
  } else {
    Serial.println("Connection failed!");
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("STM32 SPI Ethernet Basic Demo");
  Serial.println("=============================");
  
  // Reset W5500
  pinMode(ETH_RST_PIN, OUTPUT);
  digitalWrite(ETH_RST_PIN, LOW);
  delay(100);
  digitalWrite(ETH_RST_PIN, HIGH);
  delay(500);
  
  Serial.println("Initializing Ethernet...");
  
  // Initialize Ethernet with static IP
  Ethernet.begin(mac, ip, dnsServer, gateway, subnet);
  
  // Give the Ethernet shield time to initialize
  delay(1000);
  
  // Check for Ethernet hardware
  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    Serial.println("Ethernet shield not found!");
    while (1) {
      delay(1000);
    }
  }
  
  if (Ethernet.linkStatus() == LinkOFF) {
    Serial.println("Ethernet cable is not connected.");
  } else {
    Serial.println("Ethernet cable connected.");
  }
  
  Serial.println("Ethernet initialized!");
  printIPAddress();
  
  // Test connection
  testConnection();
  
  Serial.println("\nSetup complete. Monitoring link status...");
}

void loop() {
  static uint32_t lastCheck = 0;
  static bool lastLinkStatus = false;
  
  if (millis() - lastCheck >= 5000) {
    lastCheck = millis();
    
    bool currentLinkStatus = (Ethernet.linkStatus() == LinkON);
    
    if (currentLinkStatus != lastLinkStatus) {
      lastLinkStatus = currentLinkStatus;
      
      if (currentLinkStatus) {
        Serial.println("\n[+] Ethernet cable connected");
        printIPAddress();
      } else {
        Serial.println("\n[-] Ethernet cable disconnected");
      }
    }
    
    if (currentLinkStatus) {
      Serial.print("Link: UP | IP: ");
      Serial.println(Ethernet.localIP());
    } else {
      Serial.println("Link: DOWN");
    }
  }
  
  delay(10);
}
