#ifndef CONFIG_H
#define CONFIG_H

/* ============================================================
 * STM32_05_DMA_ADC_Multi_Channel - Configuration
 * ADC1 Scan Mode: Channel 0 (PA0) + Channel 1 (PA1) with DMA
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
#define NUM_ADC_CHANNELS        2
#define ADC_CH0                 ADC_CHANNEL_0       /* PA0 */
#define ADC_CH1                 ADC_CHANNEL_1       /* PA1 */
#define ADC_CH0_PIN             GPIO_PIN_0
#define ADC_CH1_PIN             GPIO_PIN_1
#define ADC_GPIO_PORT           GPIOA
#define ADC_RESOLUTION_BITS     12
#define ADC_MAX_VALUE           4095
#define ADC_VREF_MV             3300                /* 3.3V reference */

/* DMA Configuration - ADC1 = DMA1 Channel 1 */
#define ADC_DMA_CHANNEL         DMA1_Channel1
#define ADC_DMA_IRQn            DMA1_Channel1_IRQn

/* Buffer Configuration */
#define NUM_SAMPLES             128                 /* Samples per channel */
#define ADC_BUFFER_SIZE         (NUM_ADC_CHANNELS * NUM_SAMPLES)  /* Interleaved */

/* Statistics print interval */
#define STATS_PRINT_INTERVAL_MS 1000

/* DWT Cycle Counter */
#define DWT_CTRL_REG            (*(volatile uint32_t *)0xE0001000)
#define DWT_CYCCNT_REG          (*(volatile uint32_t *)0xE0001004)
#define DWT_DEMCR_REG           (*(volatile uint32_t *)0xE000EDFC)

#endif /* CONFIG_H */
