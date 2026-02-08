#ifndef CONFIG_H
#define CONFIG_H

/* ============================================================
 * STM32_06_DMA_SPI_Transfer - Configuration
 * SPI1 with DMA1 Channel 2 (RX) and Channel 3 (TX)
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

/* SPI1 Configuration */
#define SPI_INSTANCE            SPI1
#define SPI_SCK_PIN             GPIO_PIN_5          /* PA5 */
#define SPI_MISO_PIN            GPIO_PIN_6          /* PA6 */
#define SPI_MOSI_PIN            GPIO_PIN_7          /* PA7 */
#define SPI_GPIO_PORT           GPIOA
#define SPI_CS_PIN              GPIO_PIN_4          /* PA4 - software CS */
#define SPI_CS_PORT             GPIOA
#define SPI_PRESCALER           SPI_BAUDRATEPRESCALER_8  /* 72MHz/8 = 9MHz */

/* DMA Configuration */
/* SPI1_RX = DMA1 Channel 2 */
#define SPI_RX_DMA_CHANNEL      DMA1_Channel2
#define SPI_RX_DMA_IRQn         DMA1_Channel2_IRQn
/* SPI1_TX = DMA1 Channel 3 */
#define SPI_TX_DMA_CHANNEL      DMA1_Channel3
#define SPI_TX_DMA_IRQn         DMA1_Channel3_IRQn

/* Buffer Configuration */
#define SPI_BUFFER_SIZE         256

/* Test data sizes (bytes) */
#define TEST_SIZE_1             16
#define TEST_SIZE_2             32
#define TEST_SIZE_3             64
#define TEST_SIZE_4             128
#define TEST_SIZE_5             256
#define NUM_TEST_SIZES          5

/* DMA Transfer timeout (ms) */
#define DMA_TIMEOUT_MS          5000

/* Number of iterations for averaging */
#define NUM_ITERATIONS          50

/* DWT Cycle Counter */
#define DWT_CTRL_REG            (*(volatile uint32_t *)0xE0001000)
#define DWT_CYCCNT_REG          (*(volatile uint32_t *)0xE0001004)
#define DWT_DEMCR_REG           (*(volatile uint32_t *)0xE000EDFC)

/* Print interval */
#define PRINT_INTERVAL_MS       5000

#endif /* CONFIG_H */
