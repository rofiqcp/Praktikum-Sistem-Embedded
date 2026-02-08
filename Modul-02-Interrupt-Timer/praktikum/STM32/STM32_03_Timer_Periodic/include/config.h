/**
 * @file config.h
 * @brief Hardware configuration for STM32_03_Timer_Periodic
 *
 * TIM2 periodic interrupt (1 second), toggle LED on PC13
 * F103: APB1=36MHz, prescaler=7199, period=9999 -> 1s
 * F4xx: APB1 timer=84MHz, prescaler=8399, period=9999 -> 1s
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

/* ---- Timer Prescaler & Period ---- */
#ifdef STM32F103xB
  /* F103: TIM2 on APB1 = 36MHz, timer clock = 36MHz (APB1 prescaler=2, but
     since APB1 prescaler != 1, timer clock = APB1 x 2 = 72MHz... 
     Actually: SYSCLK=72MHz, AHB=72MHz, APB1=36MHz, timer x2=72MHz
     So prescaler=7199 -> 72MHz/7200 = 10kHz, period=9999 -> 10000/10kHz = 1s */
  #define TIM_PRESCALER   7199
  #define TIM_PERIOD      9999
#elif defined(STM32F401xC) || defined(STM32F411xE)
  /* F4xx: TIM2 on APB1 = 42MHz, timer clock x2 = 84MHz
     prescaler=8399 -> 84MHz/8400 = 10kHz, period=9999 -> 10000/10kHz = 1s */
  #define TIM_PRESCALER   8399
  #define TIM_PERIOD      9999
#endif

#endif /* CONFIG_H */
