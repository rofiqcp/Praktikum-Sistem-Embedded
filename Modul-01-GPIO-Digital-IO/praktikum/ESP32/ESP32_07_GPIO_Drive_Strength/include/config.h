/**
 * ==========================================================================
 *  ESP32_07 - GPIO Drive Strength Configuration
 * ==========================================================================
 *  Pin & drive strength definitions
 * ==========================================================================
 */

#ifndef CONFIG_H
#define CONFIG_H

/* ---------- LED Output Pin ---------- */
#define LED_PIN             GPIO_NUM_2

/* ---------- Drive Strength Levels ---------- */
#define DRIVE_LEVEL_0       GPIO_DRIVE_CAP_0    /* ~5 mA  - weakest  */
#define DRIVE_LEVEL_1       GPIO_DRIVE_CAP_1    /* ~10 mA - weak     */
#define DRIVE_LEVEL_2       GPIO_DRIVE_CAP_2    /* ~20 mA - default  */
#define DRIVE_LEVEL_3       GPIO_DRIVE_CAP_3    /* ~40 mA - strongest*/

#define NUM_DRIVE_LEVELS    4

/* ---------- Timing ---------- */
#define HOLD_TIME_MS        3000    /* Hold each drive level for 3 s */
#define TRANSITION_PAUSE_MS 500     /* Pause between transitions     */

#endif /* CONFIG_H */
