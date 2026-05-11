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

// I2C configuration
#define BME280_I2C_ADDR  0x76

// Sensor queue size
#define SENSOR_QUEUE_SIZE 5

// I2C timeout
#define I2C_TIMEOUT_MS   100

// UART configuration
#define UART_BAUDRATE   115200

// Stack sizes (in words)
#define TASK_STACK_SIZE   128

// Task priorities
#define I2C_READ_TASK_PRIORITY    1
#define I2C_PROCESS_TASK_PRIORITY 1

void SystemClock_Config(void);
void Error_Handler(void);

#endif /* CONFIG_H */
