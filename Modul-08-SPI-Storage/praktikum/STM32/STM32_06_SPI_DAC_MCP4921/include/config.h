/**
 * ============================================================================
 * Program     : STM32_06_SPI_DAC_MCP4921
 * Description : Configuration for MCP4921 12-bit DAC via SPI
 * Board       : STM32F103C8 (Blue Pill)
 * Framework   : STM32Cube HAL
 * ============================================================================
 */

#ifndef CONFIG_H
#define CONFIG_H

/* ==================== SPI1 Pin Configuration ==================== */
#define SPI1_SCK_PIN        GPIO_PIN_5      /* PA5 - SPI1 SCK  */
#define SPI1_SCK_PORT       GPIOA
#define SPI1_MOSI_PIN       GPIO_PIN_7      /* PA7 - SPI1 MOSI */
#define SPI1_MOSI_PORT      GPIOA
#define SPI1_MISO_PIN       GPIO_PIN_6      /* PA6 - SPI1 MISO */
#define SPI1_MISO_PORT      GPIOA

/* ==================== Chip Select & LDAC Pins ==================== */
#define MCP4921_CS_PIN      GPIO_PIN_4      /* PA4 - CS (active low) */
#define MCP4921_CS_PORT     GPIOA
#define MCP4921_LDAC_PIN    GPIO_PIN_0      /* PB0 - LDAC (active low) */
#define MCP4921_LDAC_PORT   GPIOB

/* ==================== MCP4921 Configuration ==================== */
#define MCP4921_VREF        3.3f            /* Reference voltage (V) */
#define MCP4921_RESOLUTION  4095            /* 12-bit max value */
#define MCP4921_CMD_MASK    0x3000          /* DAC A | Unbuffered | 1x gain | Active */

/* ==================== Waveform Configuration ==================== */
typedef enum {
    WAVE_SINE = 0,
    WAVE_SAWTOOTH,
    WAVE_TRIANGLE,
    WAVE_SQUARE,
    WAVE_COUNT          /* Total number of waveform types */
} Waveform_TypeDef;

#define LUT_SIZE            256             /* Look-up table entries */
#define WAVEFORM_SWITCH_SEC 5               /* Switch waveform every N seconds */

/* ==================== SPI Configuration ==================== */
#define SPI_PRESCALER       SPI_BAUDRATEPRESCALER_4   /* 72MHz/4 = 18MHz */

/* ==================== UART Configuration ==================== */
#define UART_BAUDRATE       115200
#define UART_TX_PIN         GPIO_PIN_9      /* PA9 - USART1 TX */
#define UART_RX_PIN         GPIO_PIN_10     /* PA10 - USART1 RX */
#define UART_PORT           GPIOA

/* ==================== LED Configuration ==================== */
#define LED_PIN             GPIO_PIN_13     /* PC13 - Onboard LED */
#define LED_PORT            GPIOC

#endif /* CONFIG_H */
