/**
 * ============================================================================
 * @file    config.h
 * @brief   Configuration for Direct GPIO Port Register Access Demo
 * @project STM32_09_GPIO_Port_Register
 * ============================================================================
 *
 * Hardware Configuration:
 *   - LED0 on PA0 (with 220 ohm resistor)
 *   - LED1 on PA1 (with 220 ohm resistor)
 *   - LED2 on PA2 (with 220 ohm resistor)
 *   - LED3 on PA3 (with 220 ohm resistor)
 *
 * Wiring Diagram:
 *   PA0 ---[220R]--- LED0 --- GND
 *   PA1 ---[220R]--- LED1 --- GND
 *   PA2 ---[220R]--- LED2 --- GND
 *   PA3 ---[220R]--- LED3 --- GND
 *
 * Register Reference:
 *   BSRR (Bit Set/Reset Register):
 *     - Bits [15:0]  = BS (Bit Set)   : Write 1 to SET pin HIGH
 *     - Bits [31:16] = BR (Bit Reset) : Write 1 to SET pin LOW
 *   ODR (Output Data Register):
 *     - Read/write current output state
 *   IDR (Input Data Register):
 *     - Read-only current pin input state
 *
 * ============================================================================
 */

#ifndef CONFIG_H
#define CONFIG_H

#if defined(STM32F103xC)
  #include "stm32f1xx_hal.h"
#elif defined(STM32F401xC)
  #include "stm32f4xx_hal.h"
#elif defined(STM32F411xE)
  #include "stm32f4xx_hal.h"
#else
  #error "Unsupported STM32 target! Define STM32F103xC, STM32F401xC, or STM32F411xE"
#endif

/* ========================= LED Configuration ========================= */
#define LED_PORT        GPIOA

#define LED0_PIN        GPIO_PIN_0
#define LED1_PIN        GPIO_PIN_1
#define LED2_PIN        GPIO_PIN_2
#define LED3_PIN        GPIO_PIN_3

#define LED_ALL_PINS    (LED0_PIN | LED1_PIN | LED2_PIN | LED3_PIN)
#define LED_MASK        0x000F      /* PA0-PA3 mask for ODR operations   */
#define LED_COUNT       4

/* ========================= Timing Configuration ====================== */
#define PATTERN_DELAY_MS    300     /* Delay between pattern steps (ms)  */
#define DEMO_PAUSE_MS       2000   /* Pause between demo sections (ms)  */
#define BENCHMARK_ITERS     100000 /* Iterations for speed comparison    */

#endif /* CONFIG_H */
