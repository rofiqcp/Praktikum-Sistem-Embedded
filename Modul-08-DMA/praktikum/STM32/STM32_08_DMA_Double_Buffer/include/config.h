#ifndef CONFIG_H
#define CONFIG_H

/* ============================================================
 * STM32_08_DMA_Double_Buffer - Configuration
 * Software Double Buffer dengan DMA Half/Full Complete Interrupts
 * ADC1 Continuous → DMA Circular, Ping-Pong Processing
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

/* LED (PC13 - active low pada Blue Pill) */
#define LED_PIN                 GPIO_PIN_13
#define LED_PORT                GPIOC
#define LED_ON()                HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_RESET)
#define LED_OFF()               HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET)
#define LED_TOGGLE()            HAL_GPIO_TogglePin(LED_PORT, LED_PIN)

/* ADC Configuration */
#define ADC_CHANNEL_USED        ADC_CHANNEL_0       /* PA0 */
#define ADC_GPIO_PIN            GPIO_PIN_0
#define ADC_GPIO_PORT           GPIOA
#define ADC_RESOLUTION_BITS     12
#define ADC_MAX_VALUE           4095
#define ADC_VREF_MV             3300                /* 3.3V reference */

/* DMA Configuration - ADC1 = DMA1 Channel 1 */
#define ADC_DMA_CHANNEL         DMA1_Channel1
#define ADC_DMA_IRQn            DMA1_Channel1_IRQn

/* Double Buffer Configuration */
#define BUFFER_TOTAL            512         /* Total ukuran buffer DMA */
#define HALF_SIZE               256         /* Setengah buffer = satu "halaman" */

/* Statistik dan interval */
#define STATS_PRINT_INTERVAL_MS 1000
#define NUM_STATS_SAMPLES       10          /* Jumlah iterasi untuk rata-rata */

/* DWT Cycle Counter untuk pengukuran performa */
#define DWT_CTRL_REG            (*(volatile uint32_t *)0xE0001000)
#define DWT_CYCCNT_REG          (*(volatile uint32_t *)0xE0001004)
#define DWT_DEMCR_REG           (*(volatile uint32_t *)0xE000EDFC)

#endif /* CONFIG_H */
