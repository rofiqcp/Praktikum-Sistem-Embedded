#ifndef CONFIG_H
#define CONFIG_H

#ifdef STM32F103xB
  #include "stm32f1xx_hal.h"
#elif defined(STM32F401xC) || defined(STM32F411xE)
  #include "stm32f4xx_hal.h"
#endif

/* -------- Encoder Pins (TIM3 CH1/CH2) -------- */
#define ENC_A_PORT      GPIOA
#define ENC_A_PIN       GPIO_PIN_6

#define ENC_B_PORT      GPIOA
#define ENC_B_PIN       GPIO_PIN_7

/* -------- Reset Button -------- */
#define BTN_PORT        GPIOB
#define BTN_PIN         GPIO_PIN_0

/* -------- Status LED -------- */
#define LED_PORT        GPIOC
#define LED_PIN         GPIO_PIN_13

#endif /* CONFIG_H */
