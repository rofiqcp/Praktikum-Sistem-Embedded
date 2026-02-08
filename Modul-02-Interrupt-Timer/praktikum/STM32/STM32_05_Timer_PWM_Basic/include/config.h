/**
 * @file config.h
 * @brief Hardware configuration for STM32_05_Timer_PWM_Basic
 *
 * TIM2 CH1 PWM output on PA0 at 50Hz, 50% duty cycle
 * F103: APB1=36MHz, timer=72MHz. PSC=71, ARR=19999 -> 50Hz
 * F4xx: APB1=42MHz, timer=84MHz. PSC=83, ARR=19999 -> 50Hz
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

/* ---- PWM Output Pin Configuration (PA0 = TIM2_CH1) ---- */
#define PWM_PORT          GPIOA
#define PWM_PIN           GPIO_PIN_0

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

/* ---- PWM Parameters ---- */
#define PWM_FREQ          50          /* 50 Hz */
#define DUTY_PERCENT      50          /* 50% duty cycle */

/* ---- Timer Prescaler & Period for 50Hz ---- */
#ifdef STM32F103xB
  /* F103: timer clock = 72MHz
     PSC=71 -> 72MHz/72 = 1MHz, ARR=19999 -> 1MHz/20000 = 50Hz */
  #define TIM_PRESCALER   71
  #define TIM_PERIOD      19999
#elif defined(STM32F401xC) || defined(STM32F411xE)
  /* F4xx: timer clock = 84MHz
     PSC=83 -> 84MHz/84 = 1MHz, ARR=19999 -> 1MHz/20000 = 50Hz */
  #define TIM_PRESCALER   83
  #define TIM_PERIOD      19999
#endif

/* ---- Duty cycle value: (DUTY_PERCENT / 100) * (ARR+1) ---- */
#define PWM_DUTY_VALUE    ((DUTY_PERCENT * (TIM_PERIOD + 1)) / 100)

#endif /* CONFIG_H */
