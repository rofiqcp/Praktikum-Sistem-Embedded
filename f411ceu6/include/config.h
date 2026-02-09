#ifndef CONFIG_H
#define CONFIG_H

#include "stm32f4xx_hal.h"

/* ============================================================
 *  LED Configuration (PC13 - Built-in LED)
 * ============================================================ */
#define LED_PORT    GPIOC
#define LED_PIN     GPIO_PIN_13
#define BLINK_DELAY_MS  500

#endif /* CONFIG_H */
