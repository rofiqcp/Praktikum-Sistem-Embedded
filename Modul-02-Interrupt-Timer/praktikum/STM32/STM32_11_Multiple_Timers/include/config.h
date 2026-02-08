#ifndef CONFIG_H
#define CONFIG_H

#ifdef STM32F103xB
  #include "stm32f1xx_hal.h"
#elif defined(STM32F401xC) || defined(STM32F411xE)
  #include "stm32f4xx_hal.h"
#endif

/* -------- LED Pins -------- */
#define LED1_PORT       GPIOA
#define LED1_PIN        GPIO_PIN_0      /* TIM2 — 200 ms (5 Hz) */

#define LED2_PORT       GPIOA
#define LED2_PIN        GPIO_PIN_1      /* TIM3 — 500 ms (2 Hz) */

#define LED3_PORT       GPIOA
#define LED3_PIN        GPIO_PIN_2      /* TIM4 — 1000 ms (1 Hz) */

#endif /* CONFIG_H */
