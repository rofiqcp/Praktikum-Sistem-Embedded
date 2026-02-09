/**
 * @file config.h
 * @brief Hardware configuration for STM32_04_Button_Debounce
 *
 * Button on PB0 (input with pull-up), LED on PC13
 * 3-state debounce state machine: IDLE -> PRESS_DETECTED -> CONFIRMED
 * Hardware: 1x button + 10k ohm + 1x LED
 * Multi-board support: F103, F401, F411
 */

#ifndef CONFIG_H
#define CONFIG_H

/* ---- Include HAL header based on target MCU ---- */
#if defined(STM32F103xC)
  #include "stm32f1xx_hal.h"
#elif defined(STM32F401xC)
  #include "stm32f4xx_hal.h"
#elif defined(STM32F411xE)
  #include "stm32f4xx_hal.h"
#else
  #error "Unsupported STM32 target! Define STM32F103xC, STM32F401xC, or STM32F411xE"
#endif

/* ---- LED Pin Configuration ---- */
#if defined(STM32F103xC)
  #define LED_PORT        GPIOC
  #define LED_PIN         GPIO_PIN_13
  #define LED_ACTIVE_LOW  1
#elif defined(STM32F401xC) || defined(STM32F411xE)
  #define LED_PORT        GPIOC
  #define LED_PIN         GPIO_PIN_13
  #define LED_ACTIVE_LOW  1
#endif

/* ---- Button Pin Configuration ---- */
#define BTN_PORT          GPIOB
#define BTN_PIN           GPIO_PIN_0
#define BTN_PRESSED       GPIO_PIN_RESET  /* Active-low with pull-up */

/* ---- Debounce Configuration ---- */
#define DEBOUNCE_DELAY_MS 50

/* ---- Debounce State Machine States ---- */
typedef enum {
    DEBOUNCE_IDLE = 0,
    DEBOUNCE_PRESS_DETECTED,
    DEBOUNCE_CONFIRMED
} DebounceState_t;

#endif /* CONFIG_H */
