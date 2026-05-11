#ifndef CONFIG_H
#define CONFIG_H

#if defined(STM32F103xB)
#include "stm32f1xx_hal.h"
#elif defined(STM32F401xC) || defined(STM32F411xE)
#include "stm32f4xx_hal.h"
#endif

// SD Card on SPI2: CS=PB12, SCK=PB13, MISO=PB14, MOSI=PB15
#define SD_CS_PORT          GPIOB
#define SD_CS_PIN           GPIO_PIN_12
#define SD_SCK_PORT         GPIOB
#define SD_SCK_PIN          GPIO_PIN_13
#define SD_MISO_PORT        GPIOB
#define SD_MISO_PIN         GPIO_PIN_14
#define SD_MOSI_PORT        GPIOB
#define SD_MOSI_PIN         GPIO_PIN_15

// LED (active LOW)
#define LED_PORT            GPIOC
#define LED_PIN             GPIO_PIN_13

// UART baudrate
#define UART_BAUDRATE       115200

// FreeRTOS task priorities
#define SENSOR_TASK_PRIORITY    2
#define LOGGER_TASK_PRIORITY    2
#define STATUS_TASK_PRIORITY    1

// FreeRTOS task stack sizes (words)
#define SENSOR_TASK_STACK       256
#define LOGGER_TASK_STACK       512
#define STATUS_TASK_STACK       256

// Queue depth
#define DATA_QUEUE_LENGTH       10

// Timing (ms)
#define SENSOR_READ_MS          2000
#define STATUS_UPDATE_MS        5000

// Simulated sensor ranges
#define TEMP_MIN                20.0f
#define TEMP_MAX                35.0f
#define HUMIDITY_MIN            40.0f
#define HUMIDITY_MAX            80.0f

void SystemClock_Config(void);
void Error_Handler(void);

#endif /* CONFIG_H */

