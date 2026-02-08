/**
 * @file config.h
 * @brief Hardware configuration for STM32_02_Multi_LED_Running
 *
 * 4 LEDs on PA0, PA1, PA2, PA3 - Running light pattern
 * Hardware: 4x LED + 4x 220 ohm resistors
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

/* ---- LED Pin Configuration (same for all boards) ---- */
#define LED_PORT     GPIOA

#define LED1_PIN     GPIO_PIN_0
#define LED2_PIN     GPIO_PIN_1
#define LED3_PIN     GPIO_PIN_2
#define LED4_PIN     GPIO_PIN_3

#define LED_ALL_PINS (LED1_PIN | LED2_PIN | LED3_PIN | LED4_PIN)
#define NUM_LEDS     4

/* ---- Timing Configuration ---- */
#define RUNNING_DELAY_MS  200

#endif /* CONFIG_H */
