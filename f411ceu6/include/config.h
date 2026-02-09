/**
 * @file config.h
 * @brief Hardware configuration for STM32_01_LED_Blink
 * 
 * LED on PC13 (built-in LED, active-low)
 * Mendukung 3 MCU: STM32F103C8T6, STM32F401CCU6, STM32F411CEU6
 */

#ifndef CONFIG_H
#define CONFIG_H

/* ---- Include HAL header based on target MCU ---- */
#if defined(STM32F103xC)
  #include "stm32f1xx_hal.h"
#elif defined(STM32F401xC) || defined(STM32F411xE)
  #include "stm32f4xx_hal.h"
#else
  #error "Unsupported STM32 target! Define STM32F103xC, STM32F401xC, or STM32F411xE"
#endif

/* ---- LED Pin Configuration ---- */
#define LED_PORT        GPIOC
#define LED_PIN         GPIO_PIN_13
#define LED_ACTIVE_LOW  1           /* PC13: LOW = ON, HIGH = OFF */

/* ---- Timing Configuration ---- */
#define BLINK_DELAY_MS    1000

#endif /* CONFIG_H */
