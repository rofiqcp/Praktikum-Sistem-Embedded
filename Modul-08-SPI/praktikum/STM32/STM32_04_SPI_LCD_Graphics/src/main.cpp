#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>
#include "config.h"

// Create ST7735 display object
Adafruit_ST7735 tft = Adafruit_ST7735(LCD_CS_PIN, LCD_DC_PIN, LCD_RST_PIN);

// Ball animation variables
struct Ball {
  int16_t x, y;
  int16_t vx, vy;
  uint16_t color;
};

Ball balls[3];

// Star field variables
struct Star {
  int16_t x, y;
  uint8_t brightness;
};

Star stars[NUM_STARS];

// Animation modes
enum AnimationMode {
  MODE_BOUNCING_BALLS,
  MODE_STARFIELD,
  MODE_SINE_WAVE,
  MODE_SPIRAL,
  MODE_COUNT
};

AnimationMode currentMode = MODE_BOUNCING_BALLS;
uint32_t modeStartTime = 0;
const uint32_t MODE_DURATION = 10000; // 10 seconds per mode

void initBalls() {
  for (int i = 0; i < 3; i++) {
    balls[i].x = random(BALL_RADIUS, LCD_WIDTH - BALL_RADIUS);
    balls[i].y = random(BALL_RADIUS, LCD_HEIGHT - BALL_RADIUS);
    balls[i].vx = random(-BALL_SPEED, BALL_SPEED + 1);
    balls[i].vy = random(-BALL_SPEED, BALL_SPEED + 1);
    if (balls[i].vx == 0) balls[i].vx = 1;
    if (balls[i].vy == 0) balls[i].vy = 1;
    
    switch (i) {
      case 0: balls[i].color = ST77XX_RED; break;
      case 1: balls[i].color = ST77XX_GREEN; break;
      case 2: balls[i].color = ST77XX_BLUE; break;
    }
  }
}

void initStars() {
  for (int i = 0; i < NUM_STARS; i++) {
    stars[i].x = random(0, LCD_WIDTH);
    stars[i].y = random(0, LCD_HEIGHT);
    stars[i].brightness = random(50, 255);
  }
}

void animateBouncingBalls() {
  static uint32_t lastUpdate = 0;
  
  if (millis() - lastUpdate < 50) return;
  lastUpdate = millis();
  
  tft.fillScreen(ST77XX_BLACK);
  
  for (int i = 0; i < 3; i++) {
    // Erase old position
    tft.fillCircle(balls[i].x, balls[i].y, BALL_RADIUS, ST77XX_BLACK);
    
    // Update position
    balls[i].x += balls[i].vx;
    balls[i].y += balls[i].vy;
    
    // Bounce off walls
    if (balls[i].x <= BALL_RADIUS || balls[i].x >= LCD_WIDTH - BALL_RADIUS) {
      balls[i].vx = -balls[i].vx;
      balls[i].x = constrain(balls[i].x, BALL_RADIUS, LCD_WIDTH - BALL_RADIUS);
    }
    
    if (balls[i].y <= BALL_RADIUS || balls[i].y >= LCD_HEIGHT - BALL_RADIUS) {
      balls[i].vy = -balls[i].vy;
      balls[i].y = constrain(balls[i].y, BALL_RADIUS, LCD_HEIGHT - BALL_RADIUS);
    }
    
    // Draw ball
    tft.fillCircle(balls[i].x, balls[i].y, BALL_RADIUS, balls[i].color);
  }
}

void animateStarfield() {
  static uint32_t lastUpdate = 0;
  
  if (millis() - lastUpdate < 100) return;
  lastUpdate = millis();
  
  tft.fillScreen(ST77XX_BLACK);
  
  for (int i = 0; i < NUM_STARS; i++) {
    // Move stars
    stars[i].y += 2;
    if (stars[i].y >= LCD_HEIGHT) {
      stars[i].y = 0;
      stars[i].x = random(0, LCD_WIDTH);
      stars[i].brightness = random(50, 255);
    }
    
    // Draw star with brightness
    uint16_t color = tft.color565(stars[i].brightness, stars[i].brightness, stars[i].brightness);
    tft.drawPixel(stars[i].x, stars[i].y, color);
    if (stars[i].brightness > 150) {
      tft.drawPixel(stars[i].x + 1, stars[i].y, color);
      tft.drawPixel(stars[i].x, stars[i].y + 1, color);
    }
  }
}

void animateSineWave() {
  static uint32_t lastUpdate = 0;
  static float phase = 0;
  
  if (millis() - lastUpdate < 50) return;
  lastUpdate = millis();
  
  tft.fillScreen(ST77XX_BLACK);
  
  // Draw sine wave
  for (int x = 0; x < LCD_WIDTH - 1; x++) {
    float y1 = LCD_HEIGHT / 2 + 30 * sin((x * 0.1) + phase);
    float y2 = LCD_HEIGHT / 2 + 30 * sin(((x + 1) * 0.1) + phase);
    
    tft.drawLine(x, (int)y1, x + 1, (int)y2, ST77XX_CYAN);
    
    // Second wave with different frequency and color
    y1 = LCD_HEIGHT / 2 + 20 * sin((x * 0.15) + phase * 1.5);
    y2 = LCD_HEIGHT / 2 + 20 * sin(((x + 1) * 0.15) + phase * 1.5);
    
    tft.drawLine(x, (int)y1, x + 1, (int)y2, ST77XX_MAGENTA);
  }
  
  phase += 0.1;
  if (phase > 2 * PI) phase = 0;
}

void animateSpiral() {
  static uint32_t lastUpdate = 0;
  static float angle = 0;
  
  if (millis() - lastUpdate < 30) return;
  lastUpdate = millis();
  
  tft.fillScreen(ST77XX_BLACK);
  
  int centerX = LCD_WIDTH / 2;
  int centerY = LCD_HEIGHT / 2;
  
  // Draw spiral
  for (float a = 0; a < 4 * PI; a += 0.1) {
    float r = a * 8;
    if (r > min(centerX, centerY) - 10) break;
    
    int x = centerX + r * cos(a + angle);
    int y = centerY + r * sin(a + angle);
    
    if (x >= 0 && x < LCD_WIDTH && y >= 0 && y < LCD_HEIGHT) {
      uint16_t color = tft.color565(
        (int)(128 + 127 * sin(a)),
        (int)(128 + 127 * cos(a * 1.5)),
        (int)(128 + 127 * sin(a * 2))
      );
      tft.drawPixel(x, y, color);
      tft.drawPixel(x + 1, y, color);
      tft.drawPixel(x, y + 1, color);
    }
  }
  
  angle += 0.05;
  if (angle > 2 * PI) angle = 0;
}

void drawModeInfo() {
  tft.setCursor(2, 2);
  tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
  tft.setTextSize(1);
  
  switch (currentMode) {
    case MODE_BOUNCING_BALLS:
      tft.print("Bouncing Balls");
      break;
    case MODE_STARFIELD:
      tft.print("Starfield");
      break;
    case MODE_SINE_WAVE:
      tft.print("Sine Waves");
      break;
    case MODE_SPIRAL:
      tft.print("Spiral");
      break;
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("STM32 SPI LCD Graphics Demo");
  
  // Initialize ST7735 display
  tft.initR(INITR_BLACKTAB);
  tft.setRotation(1); // Landscape mode
  
  Serial.println("LCD initialized");
  
  // Clear screen
  tft.fillScreen(ST77XX_BLACK);
  
  // Show startup message
  tft.setCursor(20, 60);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(2);
  tft.println("Graphics");
  tft.setCursor(30, 85);
  tft.println("Demo");
  
  delay(2000);
  
  // Initialize animations
  initBalls();
  initStars();
  
  modeStartTime = millis();
  Serial.println("Starting animations...");
}

void loop() {
  // Check if it's time to switch modes
  if (millis() - modeStartTime >= MODE_DURATION) {
    currentMode = (AnimationMode)((currentMode + 1) % MODE_COUNT);
    modeStartTime = millis();
    
    Serial.print("Switching to mode: ");
    Serial.println(currentMode);
    
    // Reinitialize for new mode
    if (currentMode == MODE_BOUNCING_BALLS) {
      initBalls();
    } else if (currentMode == MODE_STARFIELD) {
      initStars();
    }
  }
  
  // Run current animation
  switch (currentMode) {
    case MODE_BOUNCING_BALLS:
      animateBouncingBalls();
      break;
    case MODE_STARFIELD:
      animateStarfield();
      break;
    case MODE_SINE_WAVE:
      animateSineWave();
      break;
    case MODE_SPIRAL:
      animateSpiral();
      break;
  }
  
  // Draw mode info (less frequently to avoid flicker)
  static uint32_t lastInfoUpdate = 0;
  if (millis() - lastInfoUpdate >= 1000) {
    lastInfoUpdate = millis();
    drawModeInfo();
  }
  
  delay(1);
}
