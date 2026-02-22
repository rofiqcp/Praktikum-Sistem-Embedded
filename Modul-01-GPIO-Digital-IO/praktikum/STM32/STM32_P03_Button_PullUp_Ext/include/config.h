/**
 * @file config.h
 * @brief Hardware configuration untuk P03: Push Button Pull-Up EKSTERNAL
 *
 * RANGKAIAN:
 *   3.3V ──[10kΩ]──┬──── PB0
 *                   │
 *              [BUTTON]
 *                   │
 *                  GND
 *
 *   LED: PA0 ──[220Ω]──[LED+]──[LED-]── GND
 *
 * LOGIKA:
 *   PB0 idle  = HIGH  (ditarik ke 3.3V oleh 10kΩ)
 *   PB0 press = LOW   (tersambung GND via button)
 *   → Active-LOW button
 *
 * Platform: STM32F103C8T6 (Blue Pill)
 */

#ifndef CONFIG_H
#define CONFIG_H

#include "stm32f1xx_hal.h"

/* ===== Button (Active LOW - pull-up external) ===== */
#define BTN_PORT          GPIOB
#define BTN_PIN           GPIO_PIN_0    /* PB0 */
#define BTN_PRESSED       GPIO_PIN_RESET    /* LOW = ditekan */
#define BTN_RELEASED      GPIO_PIN_SET      /* HIGH = tidak ditekan */

/* ===== LED Output (Active HIGH) ===== */
#define LED_PORT          GPIOA
#define LED_PIN           GPIO_PIN_0    /* PA0 */

#endif /* CONFIG_H */
