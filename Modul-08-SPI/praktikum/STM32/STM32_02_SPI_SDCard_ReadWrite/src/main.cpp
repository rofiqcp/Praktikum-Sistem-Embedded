#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include "config.h"

File myFile;

void printDirectory(File dir, int numTabs) {
  while (true) {
    File entry = dir.openNextFile();
    if (!entry) {
      break;
    }
    
    for (uint8_t i = 0; i < numTabs; i++) {
      Serial.print('\t');
    }
    
    Serial.print(entry.name());
    if (entry.isDirectory()) {
      Serial.println("/");
      printDirectory(entry, numTabs + 1);
    } else {
      Serial.print("\t\t");
      Serial.println(entry.size(), DEC);
    }
    entry.close();
  }
}

void writeTestFile() {
  Serial.println("\nWriting to test.txt...");
  
  myFile = SD.open(TEST_FILE, FILE_WRITE);
  if (myFile) {
    myFile.println("STM32 SD Card Test");
    myFile.println("==================");
    myFile.println("This is a test file.");
    myFile.print("Timestamp: ");
    myFile.println(millis());
    myFile.close();
    Serial.println("Write completed");
  } else {
    Serial.println("Error opening test.txt for writing");
  }
}

void readTestFile() {
  Serial.println("\nReading from test.txt...");
  
  myFile = SD.open(TEST_FILE);
  if (myFile) {
    Serial.println("File contents:");
    Serial.println("-------------");
    while (myFile.available()) {
      Serial.write(myFile.read());
    }
    Serial.println("\n-------------");
    myFile.close();
  } else {
    Serial.println("Error opening test.txt for reading");
  }
}

void appendDataFile(uint32_t counter, float value) {
  myFile = SD.open(DATA_FILE, FILE_WRITE);
  if (myFile) {
    myFile.print(counter);
    myFile.print(",");
    myFile.print(millis());
    myFile.print(",");
    myFile.println(value, 2);
    myFile.close();
  } else {
    Serial.println("Error appending to data.csv");
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("STM32 SPI SD Card Read/Write Demo");
  Serial.println("==================================");
  
  Serial.print("Initializing SD card...");
  
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("initialization failed!");
    while (1);
  }
  Serial.println("initialization done.");
  
  // Get card info
  Serial.print("Card type: ");
  Serial.println("SD card initialized.");
  
  // Write test file
  writeTestFile();
  
  // Read test file
  readTestFile();
  
  // List files
  Serial.println("\nFiles on SD card:");
  File root = SD.open("/");
  printDirectory(root, 0);
  root.close();
  
  // Initialize data file with header
  if (!SD.exists(DATA_FILE)) {
    myFile = SD.open(DATA_FILE, FILE_WRITE);
    if (myFile) {
      myFile.println("Counter,Timestamp,Value");
      myFile.close();
      Serial.println("\nCreated data.csv with header");
    }
  }
  
  Serial.println("\nStarting data logging...");
}

void loop() {
  static uint32_t counter = 0;
  static uint32_t lastLog = 0;
  
  if (millis() - lastLog >= 2000) {
    lastLog = millis();
    counter++;
    
    // Generate random sensor value
    float sensorValue = 20.0 + (random(0, 100) / 10.0);
    
    // Log to SD card
    appendDataFile(counter, sensorValue);
    
    Serial.print("Logged: ");
    Serial.print(counter);
    Serial.print(", ");
    Serial.print(millis());
    Serial.print(", ");
    Serial.println(sensorValue, 2);
  }
  
  delay(10);
}
