/**
 * @file config.h
 * @brief Hardware configuration for STM32_01_EXTI_Interrupt
 *
 * External interrupt on PB0 (falling edge), toggle LED on PC13
 * Multi-board support: F103, F401, F411
 */

#ifndef CONFIG_H
#define CONFIG_H

/* ---- Include HAL header based on target MCU ---- */
#ifdef STM32F103xB
  #include "stm32f1xx_hal.h"
#elif defined(STM32F401xC)
  #include "stm32f4xx_hal.h"
#elif defined(STM32F411xE)
  #include "stm32f4xx_hal.h"
#else
  #error "Unsupported STM32 target! Define STM32F103xB, STM32F401xC, or STM32F411xE"
#endif

/* ---- LED Pin Configuration ---- */
#ifdef STM32F103xB
  #define LED_PORT        GPIOC
  #define LED_PIN         GPIO_PIN_13
  #define LED_ACTIVE_LOW  1           /* Blue Pill: LOW = ON, HIGH = OFF */
#elif defined(STM32F401xC) || defined(STM32F411xE)
  #define LED_PORT        GPIOC
  #define LED_PIN         GPIO_PIN_13
  #define LED_ACTIVE_LOW  1
#endif

/* ---- Button Pin Configuration (EXTI source) ---- */
#define BTN_PORT          GPIOB
#define BTN_PIN           GPIO_PIN_0
#define BTN_PRESSED       GPIO_PIN_RESET  /* Active-low with pull-up */

#endif /* CONFIG_H */
