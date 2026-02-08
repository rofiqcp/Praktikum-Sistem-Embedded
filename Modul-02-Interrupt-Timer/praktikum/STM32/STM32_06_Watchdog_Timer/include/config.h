/**
 * @file config.h
 * @brief Hardware configuration for STM32_06_Watchdog_Timer
 *
 * Independent Watchdog (IWDG) demo with ~4s timeout
 * Button press simulates hang (stops feeding IWDG)
 * F103: LSI=40kHz, IWDG_PRESCALER_64, Reload=2499 -> ~4s
 * F4xx: LSI=32kHz, IWDG_PRESCALER_64, Reload=1999 -> ~4s
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

/* ---- Button Pin Configuration ---- */
#define BTN_PORT          GPIOB
#define BTN_PIN           GPIO_PIN_0
#define BTN_PRESSED       GPIO_PIN_RESET  /* Active-low with pull-up */

/* ---- Watchdog Timeout ---- */
#define WDT_TIMEOUT_MS    4000        /* ~4 second watchdog timeout */

/* ---- IWDG Prescaler & Reload ---- */
#ifdef STM32F103xB
  /* F103: LSI = 40kHz
     IWDG_PRESCALER_64 -> 40kHz/64 = 625Hz
     Reload=2499 -> (2499+1)/625 = 4.0s */
  #define IWDG_PRESCALER_VAL  IWDG_PRESCALER_64
  #define IWDG_RELOAD_VAL     2499
#elif defined(STM32F401xC) || defined(STM32F411xE)
  /* F4xx: LSI = 32kHz
     IWDG_PRESCALER_64 -> 32kHz/64 = 500Hz
     Reload=1999 -> (1999+1)/500 = 4.0s */
  #define IWDG_PRESCALER_VAL  IWDG_PRESCALER_64
  #define IWDG_RELOAD_VAL     1999
#endif

#endif /* CONFIG_H */
