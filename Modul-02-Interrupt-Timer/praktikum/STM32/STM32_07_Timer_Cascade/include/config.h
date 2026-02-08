/**
 * @file config.h
 * @brief Hardware configuration for STM32_07_Timer_Cascade
 *
 * TIM2 periodic interrupt at 100ms (fast tick)
 * Software counter cascades to toggle LED_SLOW every 10 ticks (1s)
 * LED_FAST on PA0 toggles every 100ms, LED_SLOW on PA1 toggles every 1s
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
#define LED_FAST_PORT     GPIOA
#define LED_FAST_PIN      GPIO_PIN_0
#define LED_SLOW_PORT     GPIOA
#define LED_SLOW_PIN      GPIO_PIN_1

/* ---- Cascade Parameters ---- */
#define FAST_MS           100         /* Fast tick period in ms */
#define CASCADE_COUNT     10          /* Toggle LED_SLOW every 10 fast ticks */

/* ---- Timer Prescaler & Period for 100ms ---- */
#ifdef STM32F103xB
  /* F103: timer clock = 72MHz
     PSC=7199 -> 72MHz/7200 = 10kHz, ARR=999 -> 1000/10kHz = 100ms */
  #define TIM_PRESCALER   7199
  #define TIM_PERIOD      999
#elif defined(STM32F401xC) || defined(STM32F411xE)
  /* F4xx: timer clock = 84MHz
     PSC=8399 -> 84MHz/8400 = 10kHz, ARR=999 -> 1000/10kHz = 100ms */
  #define TIM_PRESCALER   8399
  #define TIM_PERIOD      999
#endif

#endif /* CONFIG_H */
