#ifndef CONFIG_H
#define CONFIG_H

#ifdef STM32F103xB
  #include "stm32f1xx_hal.h"
#elif defined(STM32F401xC) || defined(STM32F411xE)
  #include "stm32f4xx_hal.h"
#endif

/* -------- Buttons -------- */
#define BTN_HIGH_PORT   GPIOB
#define BTN_HIGH_PIN    GPIO_PIN_0      /* EXTI0 — high priority */

#define BTN_LOW_PORT    GPIOB
#define BTN_LOW_PIN     GPIO_PIN_1      /* EXTI1 — low priority  */

/* -------- LEDs -------- */
#define LED1_PORT       GPIOA
#define LED1_PIN        GPIO_PIN_0      /* fast blink (high prio) */

#define LED2_PORT       GPIOA
#define LED2_PIN        GPIO_PIN_1      /* slow blink (low prio)  */

#endif /* CONFIG_H */
