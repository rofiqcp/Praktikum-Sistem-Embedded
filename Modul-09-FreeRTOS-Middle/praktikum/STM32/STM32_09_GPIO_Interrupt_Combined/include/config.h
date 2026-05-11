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

// Button configuration
#define BUTTON1_PIN     GPIO_PIN_0
#define BUTTON1_PORT    GPIOA
#define BUTTON2_PIN     GPIO_PIN_1
#define BUTTON2_PORT    GPIOA
#define BUTTON3_PIN     GPIO_PIN_2
#define BUTTON3_PORT    GPIOA
#define BUTTON_CLK_EN() __HAL_RCC_GPIOA_CLK_ENABLE()

// UART configuration
#define UART_BAUDRATE   115200

// Stack sizes (in words)
#define TASK_STACK_SIZE   128

// Task priorities
#define BTN1_TASK_PRIORITY  1
#define BTN2_TASK_PRIORITY  1
#define BTN3_TASK_PRIORITY  1

void SystemClock_Config(void);
void Error_Handler(void);

#endif /* CONFIG_H */
