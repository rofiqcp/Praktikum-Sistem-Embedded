/**
 * ==========================================================================
 *  ESP32_11 - Emergency Stop (Fail-Safe Logic)
 * ==========================================================================
 *  Pin and timing definitions for E-Stop system
 * ==========================================================================
 */

#ifndef CONFIG_H
#define CONFIG_H

/* ---------- Input Pin ---------- */
/*
 * E-Stop button is Normally Closed (NC) and wired with a pull-up resistor.
 *
 * NORMAL state : NC button closed → GPIO reads HIGH (1)
 * EMERGENCY    : Button pressed OR wire broken → GPIO reads LOW (0)
 *
 * This is FAIL-SAFE: any fault (wire break, disconnection) triggers emergency.
 */
#define ESTOP_PIN           GPIO_NUM_4

/* ---------- Output Pins ---------- */
#define LED_STATUS_PIN      GPIO_NUM_2      /* Status LED               */
#define BUZZER_PIN          GPIO_NUM_16     /* Buzzer / alarm output    */

/* ---------- Timing ---------- */
#define POLL_INTERVAL_MS    50              /* Check E-Stop every 50 ms */
#define FAST_BLINK_MS       100             /* LED blink in emergency   */
#define RESET_HOLD_MS       2000            /* Must hold 2 s to reset   */
#define NORMAL_BLINK_MS     1000            /* Slow heartbeat in normal */

/* ---------- Logic ---------- */
#define ESTOP_ACTIVE_LEVEL  0               /* LOW = emergency          */
#define ESTOP_NORMAL_LEVEL  1               /* HIGH = normal            */

#endif /* CONFIG_H */
