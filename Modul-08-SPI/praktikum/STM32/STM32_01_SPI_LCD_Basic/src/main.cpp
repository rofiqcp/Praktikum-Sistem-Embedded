#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>
#include "config.h"

// Create ST7735 display object
Adafruit_ST7735 tft = Adafruit_ST7735(LCD_CS_PIN, LCD_DC_PIN, LCD_RST_PIN);

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("STM32 SPI LCD Basic Demo");
  
  // Initialize ST7735 display (1.8" TFT)
  tft.initR(INITR_BLACKTAB);
  tft.setRotation(1); // Landscape mode
  
  Serial.println("LCD initialized");
  
  // Clear screen with black
  tft.fillScreen(ST77XX_BLACK);
  
  // Display welcome message
  tft.setCursor(10, 20);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(2);
  tft.println("STM32 SPI");
  
  tft.setCursor(10, 45);
  tft.setTextColor(ST77XX_CYAN);
  tft.println("LCD Demo");
  
  tft.setCursor(10, 70);
  tft.setTextColor(ST77XX_GREEN);
  tft.setTextSize(1);
  tft.println("ST7735 Display");
  
  // Draw some shapes
  tft.drawRect(5, 95, 150, 50, ST77XX_YELLOW);
  tft.fillRect(10, 100, 140, 40, ST77XX_BLUE);
  
  tft.setCursor(30, 115);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(1);
  tft.println("Ready!");
  
  Serial.println("Display ready");
}

void loop() {
  static uint32_t counter = 0;
  static uint32_t lastUpdate = 0;
  
  if (millis() - lastUpdate >= 1000) {
    lastUpdate = millis();
    counter++;
    
    // Update counter on display
    tft.fillRect(60, 100, 80, 20, ST77XX_BLUE);
    tft.setCursor(65, 105);
    tft.setTextColor(ST77XX_YELLOW);
    tft.setTextSize(2);
    tft.print(counter);
    
    Serial.print("Counter: ");
    Serial.println(counter);
  }
  
  delay(10);
}
