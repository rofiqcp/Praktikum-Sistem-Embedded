/**
 * @file config.h
 * @brief Hardware configuration for STM32_03_LED_Binary_Counter
 *
 * 4 LEDs on PA0-PA3 display binary count 0-15
 * Uses bitwise operations: (count >> bit) & 0x01
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

#define LED0_PIN     GPIO_PIN_0   /* Bit 0 (LSB) */
#define LED1_PIN     GPIO_PIN_1   /* Bit 1 */
#define LED2_PIN     GPIO_PIN_2   /* Bit 2 */
#define LED3_PIN     GPIO_PIN_3   /* Bit 3 (MSB) */

#define LED_ALL_PINS (LED0_PIN | LED1_PIN | LED2_PIN | LED3_PIN)
#define NUM_BITS     4
#define MAX_COUNT    16           /* 2^4 = 16 (counts 0-15) */

/* ---- Timing Configuration ---- */
#define COUNT_DELAY_MS  500

#endif /* CONFIG_H */
