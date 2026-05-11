#ifndef CONFIG_H
#define CONFIG_H

// Enable ADC HAL module
#define HAL_ADC_MODULE_ENABLED

#include "FreeRTOSConfig.h"

#if defined(STM32F103xB)
#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_adc.h"
#elif defined(STM32F401xC) || defined(STM32F411xE)
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_adc.h"
#endif

// LED configuration
#define LED_PIN         GPIO_PIN_13
#define LED_PORT        GPIOC
#define LED_ACTIVE_LOW  1
#define LED_CLK_EN()    __HAL_RCC_GPIOC_CLK_ENABLE()

// ADC configuration
#define ADC_CHANNEL    ADC_CHANNEL_0

// Filter and queue sizes
#define FILTER_SIZE    10
#define ADC_QUEUE_SIZE 10

// UART configuration
#define UART_BAUDRATE   115200

// Stack sizes (in words)
#define TASK_STACK_SIZE   128

// Task priorities
#define ADC_READ_TASK_PRIORITY    1
#define ADC_PROCESS_TASK_PRIORITY 1

void SystemClock_Config(void);
void Error_Handler(void);

#endif /* CONFIG_H */
