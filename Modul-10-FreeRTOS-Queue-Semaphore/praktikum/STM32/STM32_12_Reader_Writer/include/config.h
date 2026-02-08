#ifndef CONFIG_H
#define CONFIG_H

/* ============================================================================
 * STM32_12_Reader_Writer
 * Modul-10-FreeRTOS-Queue-Semaphore
 * Platform: STM32 (Blue Pill F103 / Black Pill F401/F411) + FreeRTOS
 * ============================================================================ */

/* --- Board Detection & HAL Include --- */
#if defined(STM32F103xB)
    #include "stm32f1xx_hal.h"
#elif defined(STM32F401xC) || defined(STM32F411xE)
    #include "stm32f4xx_hal.h"
#else
    #error "Unsupported STM32 target"
#endif

/* --- LED Pins --- */
#if defined(STM32F103xB)
    #define LED_PORT        GPIOC
    #define LED_PIN         GPIO_PIN_13
    #define LED_ACTIVE_LOW  1
    #define LED_CLK_EN()    __HAL_RCC_GPIOC_CLK_ENABLE()
#else
    #define LED_PORT        GPIOC
    #define LED_PIN         GPIO_PIN_13
    #define LED_ACTIVE_LOW  1
    #define LED_CLK_EN()    __HAL_RCC_GPIOC_CLK_ENABLE()
#endif


/* --- Task Parameters --- */
#define TASK_DEFAULT_STACK  256
#define TASK_DEFAULT_PRIO   2

/* --- Timing --- */
#define BLINK_DELAY_MS  500

/* --- UART for printf --- */
#define PRINTF_UART     USART1

/* --- Function Prototypes --- */
void SystemClock_Config(void);
void Error_Handler(void);

#endif /* CONFIG_H */
