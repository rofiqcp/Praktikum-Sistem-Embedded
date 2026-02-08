/**
 * ============================================================================
 * @file    config.h
 * @brief   Configuration for 8-LED Test Pattern Generator
 * @project STM32_12_LED_Test_Pattern
 * ============================================================================
 *
 * Hardware Configuration:
 *   - 8 LEDs on PA0 through PA7 (each with 220 ohm resistor)
 *
 * Wiring Diagram:
 *   PA0 ---[220R]--- LED0 --- GND   (bit 0, LSB)
 *   PA1 ---[220R]--- LED1 --- GND   (bit 1)
 *   PA2 ---[220R]--- LED2 --- GND   (bit 2)
 *   PA3 ---[220R]--- LED3 --- GND   (bit 3)
 *   PA4 ---[220R]--- LED4 --- GND   (bit 4)
 *   PA5 ---[220R]--- LED5 --- GND   (bit 5)
 *   PA6 ---[220R]--- LED6 --- GND   (bit 6)
 *   PA7 ---[220R]--- LED7 --- GND   (bit 7, MSB)
 *
 * Uses direct port write (GPIOA->ODR) for simultaneous 8-bit control.
 *
 * ============================================================================
 */

#ifndef CONFIG_H
#define CONFIG_H

#ifdef STM32F103xB
  #include "stm32f1xx_hal.h"
#elif defined(STM32F401xC)
  #include "stm32f4xx_hal.h"
#elif defined(STM32F411xE)
  #include "stm32f4xx_hal.h"
#else
  #error "Unsupported STM32 target. Define STM32F103xB, STM32F401xC, or STM32F411xE"
#endif

/* ========================= LED Configuration ========================= */
#define LED_PORT        GPIOA
#define LED_ALL_PINS    (GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | \
                         GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7)
#define LED_MASK        0x00FF      /* PA0-PA7 mask for ODR              */
#define LED_COUNT       8

/* ========================= Pattern Definitions ======================= */
#define PATTERN_ALL_ON      0
#define PATTERN_ALL_OFF     1
#define PATTERN_WALK_ONE    2
#define PATTERN_WALK_ZERO   3
#define PATTERN_BINARY_CNT  4
#define PATTERN_ALTERNATE   5
#define NUM_PATTERNS        6

/* ========================= Timing Configuration ====================== */
#define STEP_DELAY_MS       200     /* Delay between pattern steps (ms)  */
#define PATTERN_PAUSE_MS    1000    /* Pause between patterns (ms)       */
#define PATTERN_CYCLES      2       /* Repeat each pattern N times       */

#endif /* CONFIG_H */
