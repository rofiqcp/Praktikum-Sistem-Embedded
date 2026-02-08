#ifndef CONFIG_H
#define CONFIG_H

/* ============================================================
 * STM32_01_DMA_Memory_to_Memory - Configuration
 * DMA1 Channel 1 Memory-to-Memory Transfer
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

/* DMA Configuration */
#define DMA_CHANNEL             DMA1_Channel1
#define DMA_IRQn                DMA1_Channel1_IRQn

/* Buffer Configuration */
#define BUFFER_SIZE_WORDS       256
#define BUFFER_SIZE_BYTES       (BUFFER_SIZE_WORDS * 4)

/* Test transfer sizes (in 32-bit words) */
#define TEST_SIZE_1             16
#define TEST_SIZE_2             32
#define TEST_SIZE_3             64
#define TEST_SIZE_4             128
#define TEST_SIZE_5             256
#define NUM_TEST_SIZES          5

/* DMA Transfer timeout (ms) */
#define DMA_TIMEOUT_MS          1000

/* Number of iterations for averaging */
#define NUM_ITERATIONS          100

/* DWT Cycle Counter */
#define DWT_CTRL_REG            (*(volatile uint32_t *)0xE0001000)
#define DWT_CYCCNT_REG          (*(volatile uint32_t *)0xE0001004)
#define DWT_DEMCR_REG           (*(volatile uint32_t *)0xE000EDFC)

/* Print interval */
#define PRINT_INTERVAL_MS       3000

#endif /* CONFIG_H */
