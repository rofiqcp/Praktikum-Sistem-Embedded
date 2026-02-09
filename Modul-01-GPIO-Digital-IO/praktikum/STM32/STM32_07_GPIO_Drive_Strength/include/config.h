/**
 * ============================================================================
 * @file    config.h
 * @brief   Configuration for GPIO Drive Strength (Output Speed) Demo
 * @project STM32_07_GPIO_Drive_Strength
 * ============================================================================
 *
 * Hardware Configuration:
 *   - LED on PA0 (with 220 ohm resistor to GND)
 *   - Optional: oscilloscope probe on PA0 to observe slew rate
 *
 * Wiring Diagram:
 *   PA0 ---[220R]--- LED_ANODE --- LED_CATHODE --- GND
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
#define LED_PIN         GPIO_PIN_0
#define LED_PIN_NUM     0

/* ========================= Timing Configuration ====================== */
#define LED_TOGGLE_MS       200     /* LED toggle interval (ms)          */
#define SPEED_SWITCH_MS     3000    /* Speed change interval (ms)        */

#endif /* CONFIG_H */
