#ifndef CONFIG_H
#define CONFIG_H

#ifdef STM32F103xB
  #include "stm32f1xx_hal.h"
#elif defined(STM32F401xC) || defined(STM32F411xE)
  #include "stm32f4xx_hal.h"
#endif

/* -------- Pin Definitions -------- */
#define BTN_PORT        GPIOB
#define BTN_PIN         GPIO_PIN_0

#define LED_PORT        GPIOC
#define LED_PIN         GPIO_PIN_13

#endif /* CONFIG_H */
