/**
 * ============================================================================
 * @file    config.h
 * @brief   Configuration for Emergency Stop System with NC Button
 * @project STM32_11_Emergency_Stop
 * ============================================================================
 *
 * Hardware Configuration:
 *   - Emergency Stop (NC push button) on PB0 (input, pull-up, active-low)
 *   - Status LED on PC13 (active-low on Blue Pill onboard LED)
 *   - Buzzer on PA1 (output push-pull, active-high)
 *
 * Wiring Diagram:
 *   PB0 ---[NC_BUTTON]--- GND     (Normally Closed emergency button)
 *         |
 *         +---[10K]--- VCC         (External pull-up recommended)
 *
 *   PC13 ---[220R]--- LED --- GND  (or onboard LED, active-low)
 *   PA1  ---[NPN/MOSFET]--- BUZZER --- VCC
 *
 * Safety Logic (Fail-Safe):
 *   - NC button normally connects PB0 to GND through closed contact
 *   - With external pull-up to VCC and NC button to GND:
 *     Normal: NC closed -> PB0 = LOW (grounded through button)
 *     BUT we invert logic: reading LOW = normal operation
 *   - ACTUAL implementation per spec:
 *     Normal:    PB0 = HIGH (safe state)
 *     Emergency: PB0 = LOW  (wire break / button opened / fault)
 *   - Wire break or open contact -> pin goes LOW -> EMERGENCY
 *   - Reset: PB0 must read HIGH continuously for RESET_HOLD_MS
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

/* ========================= Emergency Stop Button ===================== */
#define ESTOP_PORT          GPIOB
#define ESTOP_PIN           GPIO_PIN_0
#define ESTOP_ACTIVE_STATE  GPIO_PIN_RESET  /* LOW = emergency */

/* ========================= Status LED ================================ */
#define LED_STATUS_PORT     GPIOC
#define LED_STATUS_PIN      GPIO_PIN_13
/* PC13 on Blue Pill is active-LOW (LED on when pin LOW) */
#define LED_ON_STATE        GPIO_PIN_RESET
#define LED_OFF_STATE       GPIO_PIN_SET

/* ========================= Buzzer ==================================== */
#define BUZZER_PORT         GPIOA
#define BUZZER_PIN          GPIO_PIN_1

/* ========================= Timing Configuration ====================== */
#define RESET_HOLD_MS       2000    /* Hold duration to reset (ms)       */
#define EMERGENCY_BLINK_MS  100     /* Fast blink interval in emergency  */
#define NORMAL_BLINK_MS     1000    /* Slow blink in normal mode (ms)    */
#define DEBOUNCE_MS         50      /* Button debounce time (ms)         */

#endif /* CONFIG_H */
