/**
 * @file config.h
 * @brief Hardware configuration untuk P04: Push Button Pull-Down EKSTERNAL
 *
 * RANGKAIAN:
 *   PB1 ──┬──[10kΩ]── GND
 *          │
 *     [BUTTON]
 *          │
 *        3.3V
 *
 *   LED: PA1 ──[220Ω]──[LED+]──[LED-]── GND
 *
 * LOGIKA:
 *   PB1 idle  = LOW   (ditarik ke GND oleh 10kΩ)
 *   PB1 press = HIGH  (tersambung 3.3V via button)
 *   → Active-HIGH button
 *
 * Platform: STM32F103C8T6 (Blue Pill)
 */

#ifndef CONFIG_H
#define CONFIG_H

#include "stm32f1xx_hal.h"

/* ===== Button (Active HIGH - pull-down external) ===== */
#define BTN_PORT          GPIOB
#define BTN_PIN           GPIO_PIN_1    /* PB1 */
#define BTN_PRESSED       GPIO_PIN_SET      /* HIGH = ditekan */
#define BTN_RELEASED      GPIO_PIN_RESET    /* LOW  = tidak ditekan */

/* ===== LED Output (Active HIGH) ===== */
#define LED_PORT          GPIOA
#define LED_PIN           GPIO_PIN_1    /* PA1 */

#endif /* CONFIG_H */
