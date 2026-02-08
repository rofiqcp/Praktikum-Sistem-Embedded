#ifndef CONFIG_H
#define CONFIG_H

/* ============================================================
 * STM32_07_DMA_Circular_Buffer - Configuration
 * UART1 RX dengan DMA Circular Buffer untuk penerimaan kontinyu
 * Target: STM32F103C8T6 Blue Pill
 * ============================================================ */

/* System Clock */
#define HSE_VALUE_HZ            8000000U
#define SYSCLK_FREQ_HZ         72000000U

/* UART1 Debug / Data */
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

/* DMA Configuration - USART1_RX = DMA1 Channel 5 */
#define UART_RX_DMA_CHANNEL     DMA1_Channel5
#define UART_RX_DMA_IRQn        DMA1_Channel5_IRQn

/* DMA Configuration - USART1_TX = DMA1 Channel 4 */
#define UART_TX_DMA_CHANNEL     DMA1_Channel4
#define UART_TX_DMA_IRQn        DMA1_Channel4_IRQn

/* Buffer Configuration */
#define RX_BUFFER_SIZE          256         /* Ukuran circular buffer DMA RX */
#define TX_BUFFER_SIZE          256         /* Ukuran buffer TX */
#define PROCESS_BUFFER_SIZE     256         /* Buffer untuk memproses data */

/* Timeout dan interval */
#define IDLE_TIMEOUT_MS         50          /* Timeout idle deteksi data baru */
#define STATS_PRINT_INTERVAL_MS 2000        /* Interval cetak statistik */

/* DWT Cycle Counter */
#define DWT_CTRL_REG            (*(volatile uint32_t *)0xE0001000)
#define DWT_CYCCNT_REG          (*(volatile uint32_t *)0xE0001004)
#define DWT_DEMCR_REG           (*(volatile uint32_t *)0xE000EDFC)

#endif /* CONFIG_H */
