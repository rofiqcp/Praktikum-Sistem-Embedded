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
#define BUTTON_PIN     GPIO_PIN_0
#define BUTTON_PORT    GPIOA
#define BUTTON_CLK_EN() __HAL_RCC_GPIOA_CLK_ENABLE()

// Stack sizes (in words)
#define TASK_STACK_SIZE   128

// Task priorities
#define LED_TASK_PRIORITY 1

// UART configuration
#define PRINTF_UART     USART1
#define UART_BAUDRATE   115200

void SystemClock_Config(void);
void Error_Handler(void);

#endif /* CONFIG_H */
