/**
 * ==========================================================================
 *  ESP32_09 - GPIO Port Register (Direct Register-Level Access)
 * ==========================================================================
 *  Pin definitions for register-level GPIO control
 *  This is the ESP32 equivalent of STM32's BSRR register access!
 * ==========================================================================
 */

#ifndef CONFIG_H
#define CONFIG_H

/* ---------- LED Output Pins (GPIO16-19) ---------- */
#define LED0_PIN        GPIO_NUM_16
#define LED1_PIN        GPIO_NUM_17
#define LED2_PIN        GPIO_NUM_18
#define LED3_PIN        GPIO_NUM_19

#define NUM_LEDS        4

/* ---------- Bit Masks for Register Access ---------- */
/* These masks are used with GPIO_OUT_W1TS_REG / GPIO_OUT_W1TC_REG */
#define LED0_MASK       (1UL << LED0_PIN)   /* Bit 16 */
#define LED1_MASK       (1UL << LED1_PIN)   /* Bit 17 */
#define LED2_MASK       (1UL << LED2_PIN)   /* Bit 18 */
#define LED3_MASK       (1UL << LED3_PIN)   /* Bit 19 */

/* Combined mask for all 4 LEDs */
#define ALL_LEDS_MASK   (LED0_MASK | LED1_MASK | LED2_MASK | LED3_MASK)

/* ---------- Timing ---------- */
#define PATTERN_DELAY_MS    500     /* Delay between pattern steps     */
#define SPEED_TEST_CYCLES   100000  /* Number of toggles for benchmark */

#endif /* CONFIG_H */
