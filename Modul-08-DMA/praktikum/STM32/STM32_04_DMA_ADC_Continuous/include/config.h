#ifndef CONFIG_H
#define CONFIG_H

/* ============================================================
 * STM32_04_DMA_ADC_Continuous - Configuration
 * ADC1 Continuous Conversion with DMA1 Channel 1
 * Target: STM32F103C8T6 Blue Pill
 * ============================================================ */

/* System Clock */
#define HSE_VALUE_HZ            8000000U
#define SYSCLK_FREQ_HZ         72000000U

/* UART1 Debug */
#define DEBUG_UART              USART1
#define DEBUG_UART_BAUD         115200
#define DEBUG_UART_TX_PIN       GPIO_PIN_9
#define DEBUG_UART_RX_PIN       GPIO_PIN_10
#define DEBUG_UART_PORT         GPIOA

/* LED (PC13 - active low on Blue Pill) */
#define LED_PIN                 GPIO_PIN_13
#define LED_PORT                GPIOC
#define LED_ON()                HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_RESET)
#define LED_OFF()               HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET)
#define LED_TOGGLE()            HAL_GPIO_TogglePin(LED_PORT, LED_PIN)

/* ADC Configuration */
#define ADC_CHANNEL             ADC_CHANNEL_0       /* PA0 */
#define ADC_GPIO_PIN            GPIO_PIN_0
#define ADC_GPIO_PORT           GPIOA
#define ADC_RESOLUTION_BITS     12
#define ADC_MAX_VALUE           4095
#define ADC_VREF_MV             3300                /* 3.3V reference */

/* DMA Configuration - ADC1 = DMA1 Channel 1 */
#define ADC_DMA_CHANNEL         DMA1_Channel1
#define ADC_DMA_IRQn            DMA1_Channel1_IRQn

/* Buffer Configuration */
#define ADC_BUFFER_SIZE         256                 /* Number of samples */

/* TIM3 as ADC Trigger */
#define ADC_SAMPLE_RATE_HZ      1000                /* 1 kHz sample rate */
#define TIM3_PRESCALER          (72 - 1)            /* 72MHz / 72 = 1MHz timer clock */
#define TIM3_PERIOD             (1000 - 1)          /* 1MHz / 1000 = 1kHz */

/* Statistics print interval */
#define STATS_PRINT_INTERVAL_MS 1000

/* DWT Cycle Counter */
#define DWT_CTRL_REG            (*(volatile uint32_t *)0xE0001000)
#define DWT_CYCCNT_REG          (*(volatile uint32_t *)0xE0001004)
#define DWT_DEMCR_REG           (*(volatile uint32_t *)0xE000EDFC)

#endif /* CONFIG_H */
