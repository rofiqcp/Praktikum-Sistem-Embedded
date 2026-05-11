#ifndef CONFIG_H
#define CONFIG_H

#include "FreeRTOSConfig.h"

#if defined(STM32F103xB)
#include "stm32f1xx_hal.h"
#elif defined(STM32F401xC) || defined(STM32F411xE)
#include "stm32f4xx_hal.h"
#endif

// LED configuration
#define LED_PIN         GPIO_PIN_13
#define LED_PORT        GPIOC
#define LED_ACTIVE_LOW  1
#define LED_CLK_EN()    __HAL_RCC_GPIOC_CLK_ENABLE()

// OLED configuration
#define OLED_RST_PIN   GPIO_PIN_2
#define OLED_RST_PORT  GPIOA
#define OLED_DC_PIN    GPIO_PIN_3
#define OLED_DC_PORT   GPIOA
#define OLED_CS_PIN    GPIO_PIN_4
#define OLED_CS_PORT   GPIOA

// Display buffer size
#define DISPLAY_BUFFER_SIZE 128

// UART configuration
#define UART_BAUDRATE   115200

// Stack sizes (in words)
#define TASK_STACK_SIZE   128

// Task priorities
#define PREPARE_TASK_PRIORITY 1
#define UPDATE_TASK_PRIORITY  1

void SystemClock_Config(void);
void Error_Handler(void);

#endif /* CONFIG_H */
