#ifndef CONFIG_H
#define CONFIG_H

/* ============================================================================
 * KONFIGURASI MODUL 14: POWER MANAGEMENT & LOW-POWER DESIGN
 * ============================================================================ */

/* Hardware - ESP32 DevKit */
#define LED_GPIO_PIN        GPIO_NUM_2
#define BUTTON_GPIO_PIN     GPIO_NUM_0

/* Wake-up GPIO (harus RTC GPIO) */
#define WAKEUP_GPIO_PIN     GPIO_NUM_33
#define TOUCH_PAD_GPIO      GPIO_NUM_4   /* TOUCH_PAD_NUM0 */

/* ADC Battery Monitor */
#define BATT_ADC_CHANNEL    ADC1_CHANNEL_6   /* GPIO34 */
#define BATT_ADC_ATTEN      ADC_ATTEN_DB_11
#define BATT_V_DIVIDER      2.0f

/* Sleep Durations (microseconds) */
#define SLEEP_5_SEC         5000000ULL
#define SLEEP_10_SEC        10000000ULL
#define SLEEP_30_SEC        30000000ULL
#define SLEEP_60_SEC        60000000ULL
#define SLEEP_300_SEC       300000000ULL

/* FreeRTOS */
#define TASK_STACK_SIZE     4096
#define QUEUE_LENGTH        10

/* Chip Info */
#define CHIP_NAME           "ESP32"

/* Touch Pad */
#define TOUCH_THRESHOLD     400

/* UART */
#define UART_BAUD           115200

#endif /* CONFIG_H */
