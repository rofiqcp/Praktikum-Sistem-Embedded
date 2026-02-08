/**
 * @file config.h
 * @brief Hardware configuration for STM32_08_Output_Compare_Toggle
 *
 * TIM2 CH1 Output Compare toggle mode on PA0
 * Hardware toggles pin on match - 1Hz toggle (0.5Hz square wave)
 * F103: PSC=71, ARR=499999 -> 72MHz/72/500000 = 2Hz toggle = 1Hz wave
 * F4xx: PSC=83, ARR=499999 -> 84MHz/84/500000 = 2Hz toggle = 1Hz wave
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

/* ---- Output Compare Pin Configuration (PA0 = TIM2_CH1) ---- */
#define OC_PORT           GPIOA
#define OC_PIN            GPIO_PIN_0

/* ---- LED Pin Configuration (status) ---- */
#ifdef STM32F103xB
  #define LED_PORT        GPIOC
  #define LED_PIN         GPIO_PIN_13
  #define LED_ACTIVE_LOW  1           /* Blue Pill: LOW = ON, HIGH = OFF */
#elif defined(STM32F401xC) || defined(STM32F411xE)
  #define LED_PORT        GPIOC
  #define LED_PIN         GPIO_PIN_13
  #define LED_ACTIVE_LOW  1
#endif

/* ---- Timer Prescaler & Period for 1Hz toggle ---- */
#ifdef STM32F103xB
  /* F103: timer clock = 72MHz
     PSC=71 -> 72MHz/72 = 1MHz, ARR=499999 -> 1MHz/500000 = 2Hz
     Toggle mode: output toggles every period -> 1Hz square wave */
  #define TIM_PRESCALER   71
  #define TIM_PERIOD      499999
#elif defined(STM32F401xC) || defined(STM32F411xE)
  /* F4xx: timer clock = 84MHz
     PSC=83 -> 84MHz/84 = 1MHz, ARR=499999 -> 1MHz/500000 = 2Hz
     Toggle mode: output toggles every period -> 1Hz square wave */
  #define TIM_PRESCALER   83
  #define TIM_PERIOD      499999
#endif

#endif /* CONFIG_H */
