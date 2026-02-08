#ifndef CONFIG_H
#define CONFIG_H

/* ============================================================
 * STM32_02_DMA_UART_TX - Configuration
 * UART1 DMA Transmit via DMA1 Channel 4
 * Target: STM32F103C8T6 Blue Pill
 * ============================================================ */

/* System Clock */
#define HSE_VALUE_HZ            8000000U
#define SYSCLK_FREQ_HZ         72000000U

/* UART1 Configuration */
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

/* DMA Configuration - USART1_TX = DMA1 Channel 4 */
#define UART_TX_DMA_CHANNEL     DMA1_Channel4
#define UART_TX_DMA_IRQn        DMA1_Channel4_IRQn

/* TX Buffer Configuration */
#define TX_BUFFER_SIZE          256

/* Test data sizes */
#define TEST_SIZE_1             32
#define TEST_SIZE_2             64
#define TEST_SIZE_3             128
#define TEST_SIZE_4             256
#define NUM_TEST_SIZES          4

/* DMA Transfer timeout (ms) */
#define DMA_TIMEOUT_MS          5000

/* Number of iterations for averaging */
#define NUM_ITERATIONS          10

/* LED toggle test duration (ms) */
#define LED_TOGGLE_DURATION_MS  100

/* DWT Cycle Counter */
#define DWT_CTRL_REG            (*(volatile uint32_t *)0xE0001000)
#define DWT_CYCCNT_REG          (*(volatile uint32_t *)0xE0001004)
#define DWT_DEMCR_REG           (*(volatile uint32_t *)0xE000EDFC)

/* Print interval */
#define PRINT_INTERVAL_MS       5000

#endif /* CONFIG_H */
