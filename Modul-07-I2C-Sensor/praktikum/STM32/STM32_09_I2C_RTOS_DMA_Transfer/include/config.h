#ifndef CONFIG_H
#define CONFIG_H

#if defined(STM32F103xB)
    #include "stm32f1xx_hal.h"
#elif defined(STM32F401xC) || defined(STM32F411xE)
    #include "stm32f4xx_hal.h"
#else
    #error "Unsupported STM32 target"
#endif

#define LED_PORT        GPIOC
#define LED_PIN         GPIO_PIN_13
#define LED_ACTIVE_LOW  1
#define LED_CLK_EN()    __HAL_RCC_GPIOC_CLK_ENABLE()

#define PRINTF_UART     USART1

void SystemClock_Config(void);
void Error_Handler(void);

#endif
