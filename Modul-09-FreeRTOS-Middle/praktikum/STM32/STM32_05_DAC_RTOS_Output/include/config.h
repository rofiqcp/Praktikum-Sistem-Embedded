#ifndef CONFIG_H
#define CONFIG_H_

#include "FreeRTOSConfig.h"

#if defined(STM32F103xB)
#include "stm32f1xx_hal.h"
#endif

// LED configuration
#define LED_PIN         GPIO_PIN_13
#define LED_PORT        GPIOC
#define LED_ACTIVE_LOW  1
#define LED_CLK_EN()    __HAL_RCC_GPIOC_CLK_ENABLE()

// DAC configuration (only for F103)
#ifdef STM32F103xB
#include "stm32f1xx_hal_dac.h"
#define DAC_CHANNEL    DAC_CHANNEL_1
#endif

// Buffer sizes
#define SINE_BUFFER_SIZE 64

// Waveform types
#define WAVEFORM_SINE    0
#define WAVEFORM_SAWTOOTH 1
#define WAVEFORM_TRIANGLE 2

// Queue size
#define WAVEFORM_QUEUE_SIZE 5

// UART configuration
#define UART_BAUDRATE   115200

// Stack sizes (in words)
#define TASK_STACK_SIZE   128

// Task priorities
#define SINE_GEN_TASK_PRIORITY  1
#define DAC_OUTPUT_TASK_PRIORITY 1

void SystemClock_Config(void);
void Error_Handler(void);

#endif /* CONFIG_H */
