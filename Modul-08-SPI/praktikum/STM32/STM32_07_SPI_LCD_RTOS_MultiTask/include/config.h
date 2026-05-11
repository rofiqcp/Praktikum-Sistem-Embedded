#ifndef CONFIG_H
#define CONFIG_H

#if defined(STM32F103xB)
#include "stm32f1xx_hal.h"
#elif defined(STM32F401xC) || defined(STM32F411xE)
#include "stm32f4xx_hal.h"
#endif

// LCD ST7735 on SPI1: CS=PA4, DC=PA3, RST=PA2, BL=PB0
#define LCD_CS_PORT         GPIOA
#define LCD_CS_PIN          GPIO_PIN_4
#define LCD_DC_PORT         GPIOA
#define LCD_DC_PIN          GPIO_PIN_3
#define LCD_RST_PORT        GPIOA
#define LCD_RST_PIN         GPIO_PIN_2
#define LCD_BL_PORT         GPIOB
#define LCD_BL_PIN          GPIO_PIN_0

// SPI1 pins
#define LCD_SCK_PORT        GPIOA
#define LCD_SCK_PIN         GPIO_PIN_5
#define LCD_MOSI_PORT       GPIOA
#define LCD_MOSI_PIN        GPIO_PIN_7

// LCD Dimensions
#define LCD_WIDTH           128
#define LCD_HEIGHT          160

// LED (active LOW on bluepill)
#define LED_PORT            GPIOC
#define LED_PIN             GPIO_PIN_13

// UART1: TX=PA9, RX=PA10
#define UART_BAUDRATE       115200

// FreeRTOS Task Priorities
#define ANIMATION_TASK_PRIORITY  2
#define DISPLAY_TASK_PRIORITY    2
#define STATUS_TASK_PRIORITY     1

// FreeRTOS Task Stack Sizes (words)
#define ANIMATION_TASK_STACK     256
#define DISPLAY_TASK_STACK       512
#define STATUS_TASK_STACK        256

// Timing
#define ANIMATION_DELAY_MS  50
#define DISPLAY_REFRESH_MS  100
#define STATUS_UPDATE_MS    2000

// Ball animation
#define BALL_RADIUS         8
#define BALL_SPEED          3

// Colors RGB565
#define COLOR_BLACK         0x0000
#define COLOR_WHITE         0xFFFF
#define COLOR_RED           0xF800
#define COLOR_GREEN         0x07E0
#define COLOR_BLUE          0x001F
#define COLOR_YELLOW        0xFFE0
#define COLOR_CYAN          0x07FF
#define COLOR_MAGENTA       0xF81F

void SystemClock_Config(void);
void Error_Handler(void);

#endif /* CONFIG_H */

