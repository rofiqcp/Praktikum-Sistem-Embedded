/**
 * ==========================================================================
 *  ESP32_12 - LED Manufacturing Test Pattern
 * ==========================================================================
 *  Pin definitions for 8-LED test array
 * ==========================================================================
 */

#ifndef CONFIG_H
#define CONFIG_H

/* ---------- 8 LED Output Pins ---------- */
#define LED0_PIN        GPIO_NUM_2
#define LED1_PIN        GPIO_NUM_4
#define LED2_PIN        GPIO_NUM_16
#define LED3_PIN        GPIO_NUM_17
#define LED4_PIN        GPIO_NUM_18
#define LED5_PIN        GPIO_NUM_19
#define LED6_PIN        GPIO_NUM_21
#define LED7_PIN        GPIO_NUM_22

#define NUM_LEDS        8

/* ---------- Timing ---------- */
#define ALL_ON_HOLD_MS      2000    /* Hold "all on" pattern           */
#define ALL_OFF_HOLD_MS     1000    /* Hold "all off" pattern          */
#define WALK_STEP_MS        200     /* Time per step in walking tests  */
#define BINARY_STEP_MS      100     /* Time per step in binary count   */
#define ALT_HOLD_MS         500     /* Hold alternating patterns       */
#define PATTERN_PAUSE_MS    1000    /* Pause between test patterns     */

#endif /* CONFIG_H */
