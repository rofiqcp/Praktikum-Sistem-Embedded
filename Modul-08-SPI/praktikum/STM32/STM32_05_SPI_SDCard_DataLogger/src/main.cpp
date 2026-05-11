#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include "config.h"

// Forward declarations
void saveConfig();
void loadConfig();

File logFile;
File configFile;

struct SensorData {
  uint32_t timestamp;
  float temperature;
  float lightLevel;
  bool buttonState;
  uint16_t batteryLevel;
};

struct LoggerConfig {
  uint16_t logInterval;
  bool enableTemp;
  bool enableLight;
  bool enableButton;
  char deviceName[32];
};

LoggerConfig config = {
  .logInterval = LOG_INTERVAL,
  .enableTemp = true,
  .enableLight = true,
  .enableButton = true,
  .deviceName = "STM32_Logger_01"
};

uint32_t logCounter = 0;
uint32_t fileCounter = 1;
String currentLogFile;

float readTemperature() {
  // Simulate temperature sensor (normally would read from actual sensor)
  int rawValue = analogRead(TEMP_SENSOR_PIN);
  float voltage = (rawValue / 1023.0) * 3.3;
  float temperature = (voltage - 0.5) * TEMP_SCALE + TEMP_OFFSET;
  return temperature;
}

float readLightLevel() {
  // Simulate light sensor
  int rawValue = analogRead(LIGHT_SENSOR_PIN);
  return (rawValue / LIGHT_SCALE) * 100.0; // Convert to percentage
}

bool readButtonState() {
  return digitalRead(BUTTON_PIN) == LOW; // Active low
}

uint16_t readBatteryLevel() {
  // Simulate battery level (normally would read from voltage divider)
  return random(3000, 4200); // mV
}

void loadConfig() {
  if (SD.exists(CONFIG_FILE)) {
    configFile = SD.open(CONFIG_FILE);
    if (configFile) {
      Serial.println("Loading configuration...");
      
      while (configFile.available()) {
        String line = configFile.readStringUntil('\n');
        line.trim();
        
        if (line.startsWith("interval=")) {
          config.logInterval = line.substring(9).toInt();
        } else if (line.startsWith("temp=")) {
          config.enableTemp = line.substring(5) == "true";
        } else if (line.startsWith("light=")) {
          config.enableLight = line.substring(6) == "true";
        } else if (line.startsWith("button=")) {
          config.enableButton = line.substring(7) == "true";
        } else if (line.startsWith("name=")) {
          line.substring(5).toCharArray(config.deviceName, 32);
        }
      }
      
      configFile.close();
      Serial.println("Configuration loaded");
    }
  } else {
    // Create default config file
    saveConfig();
  }
}

void saveConfig() {
  configFile = SD.open(CONFIG_FILE, FILE_WRITE);
  if (configFile) {
    configFile.seek(0); // Overwrite existing content
    configFile.println("# STM32 Data Logger Configuration");
    configFile.print("interval=");
    configFile.println(config.logInterval);
    configFile.print("temp=");
    configFile.println(config.enableTemp ? "true" : "false");
    configFile.print("light=");
    configFile.println(config.enableLight ? "true" : "false");
    configFile.print("button=");
    configFile.println(config.enableButton ? "true" : "false");
    configFile.print("name=");
    configFile.println(config.deviceName);
    configFile.close();
    Serial.println("Configuration saved");
  }
}

void printConfig() {
  Serial.println("\n=== Logger Configuration ===");
  Serial.print("Device Name: ");
  Serial.println(config.deviceName);
  Serial.print("Log Interval: ");
  Serial.print(config.logInterval);
  Serial.println(" ms");
  Serial.print("Temperature: ");
  Serial.println(config.enableTemp ? "Enabled" : "Disabled");
  Serial.print("Light Level: ");
  Serial.println(config.enableLight ? "Enabled" : "Disabled");
  Serial.print("Button State: ");
  Serial.println(config.enableButton ? "Enabled" : "Disabled");
  Serial.println("============================\n");
}

String generateLogFileName() {
  char filename[32];
  sprintf(filename, "/logs/log_%04d.csv", fileCounter);
  return String(filename);
}

void createNewLogFile() {
  currentLogFile = generateLogFileName();
  
  logFile = SD.open(currentLogFile, FILE_WRITE);
  if (logFile) {
    // Write CSV header
    logFile.print("Timestamp,Counter");
    if (config.enableTemp) logFile.print(",Temperature");
    if (config.enableLight) logFile.print(",Light_Level");
    if (config.enableButton) logFile.print(",Button_State");
    logFile.println(",Battery_mV");
    logFile.close();
    
    Serial.print("Created new log file: ");
    Serial.println(currentLogFile);
  }
}

void logSensorData(const SensorData& data) {
  logFile = SD.open(currentLogFile, FILE_WRITE);
  if (logFile) {
    logFile.print(data.timestamp);
    logFile.print(",");
    logFile.print(logCounter);
    
    if (config.enableTemp) {
      logFile.print(",");
      logFile.print(data.temperature, 2);
    }
    
    if (config.enableLight) {
      logFile.print(",");
      logFile.print(data.lightLevel, 1);
    }
    
    if (config.enableButton) {
      logFile.print(",");
      logFile.print(data.buttonState ? "1" : "0");
    }
    
    logFile.print(",");
    logFile.println(data.batteryLevel);
    
    logFile.close();
    
    Serial.print("[LOG] ");
    Serial.print(logCounter);
    Serial.print(": T=");
    Serial.print(data.temperature, 1);
    Serial.print("°C, L=");
    Serial.print(data.lightLevel, 1);
    Serial.print("%, B=");
    Serial.print(data.buttonState ? "ON" : "OFF");
    Serial.print(", Bat=");
    Serial.print(data.batteryLevel);
    Serial.println("mV");
  } else {
    Serial.println("Error: Could not open log file for writing");
  }
}

void printLogSummary() {
  Serial.println("\n=== Log Summary ===");
  Serial.print("Current file: ");
  Serial.println(currentLogFile);
  Serial.print("Total entries: ");
  Serial.println(logCounter);
  Serial.print("File number: ");
  Serial.println(fileCounter);
  
  if (SD.exists(currentLogFile)) {
    File file = SD.open(currentLogFile);
    if (file) {
      Serial.print("File size: ");
      Serial.print(file.size());
      Serial.println(" bytes");
      file.close();
    }
  }
  Serial.println("==================\n");
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("STM32 SPI SD Card Data Logger");
  Serial.println("=============================");
  
  // Initialize pins
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  
  Serial.print("Initializing SD card...");
  
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("initialization failed!");
    while (1) {
      delay(1000);
    }
  }
  Serial.println("initialization done.");
  
  // Create logs directory if it doesn't exist
  if (!SD.exists(DATA_DIR)) {
    SD.mkdir(DATA_DIR);
    Serial.println("Created logs directory");
  }
  
  // Load configuration
  loadConfig();
  printConfig();
  
  // Find next available log file number
  while (SD.exists(generateLogFileName())) {
    fileCounter++;
  }
  
  // Create new log file
  createNewLogFile();
  
  Serial.println("Data logger ready!");
  Serial.println("Commands: 's' = summary, 'c' = config, 'r' = reset counter\n");
}

void loop() {
  static uint32_t lastLog = 0;
  static uint32_t lastSummary = 0;
  
  // Handle serial commands
  if (Serial.available()) {
    char cmd = Serial.read();
    switch (cmd) {
      case 's':
      case 'S':
        printLogSummary();
        break;
      case 'c':
      case 'C':
        printConfig();
        break;
      case 'r':
      case 'R':
        logCounter = 0;
        Serial.println("Counter reset");
        break;
    }
  }
  
  // Log data at specified interval
  if (millis() - lastLog >= config.logInterval) {
    lastLog = millis();
    
    SensorData data;
    data.timestamp = millis();
    data.temperature = config.enableTemp ? readTemperature() : 0.0;
    data.lightLevel = config.enableLight ? readLightLevel() : 0.0;
    data.buttonState = config.enableButton ? readButtonState() : false;
    data.batteryLevel = readBatteryLevel();
    
    logSensorData(data);
    logCounter++;
    
    // Create new file if current one is getting too large
    if (logCounter % MAX_LOG_SIZE == 0) {
      fileCounter++;
      createNewLogFile();
    }
  }
  
  // Print summary periodically
  if (millis() - lastSummary >= 30000) { // Every 30 seconds
    lastSummary = millis();
    printLogSummary();
  }
  
  delay(10);
}
