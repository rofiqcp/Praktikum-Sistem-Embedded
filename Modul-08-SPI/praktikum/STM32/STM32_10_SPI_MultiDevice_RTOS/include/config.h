#ifndef CONFIG_H
#define CONFIG_H

#if defined(STM32F103xB)
#include "stm32f1xx_hal.h"
#elif defined(STM32F401xC) || defined(STM32F411xE)
#include "stm32f4xx_hal.h"
#endif

/* =========================================================
 * SPI1 shared bus (PA5=SCK, PA6=MISO, PA7=MOSI)
 * LCD ST7735: CS=PA4, DC=PB0, RST=PB1
 * SD Card:    CS=PA3  (use SPI1)
 * W5500 ETH:  CS=PA2, RST=PA1, INT=PA0
 * ========================================================= */

/* SPI1 bus pins */
#define SPI1_SCK_PIN        GPIO_PIN_5
#define SPI1_MISO_PIN       GPIO_PIN_6
#define SPI1_MOSI_PIN       GPIO_PIN_7

/* LCD ST7735 */
#define LCD_CS_PORT         GPIOA
#define LCD_CS_PIN          GPIO_PIN_4
#define LCD_DC_PORT         GPIOB
#define LCD_DC_PIN          GPIO_PIN_0
#define LCD_RST_PORT        GPIOB
#define LCD_RST_PIN         GPIO_PIN_1

/* SD Card */
#define SD_CS_PORT          GPIOA
#define SD_CS_PIN           GPIO_PIN_3

/* W5500 Ethernet */
#define ETH_CS_PORT         GPIOA
#define ETH_CS_PIN          GPIO_PIN_2
#define ETH_RST_PORT        GPIOA
#define ETH_RST_PIN         GPIO_PIN_1
#define ETH_INT_PORT        GPIOA
#define ETH_INT_PIN         GPIO_PIN_0

/* LED (active LOW bluepill) */
#define LED_PORT            GPIOC
#define LED_PIN             GPIO_PIN_13

/* UART */
#define UART_BAUDRATE       115200

/* Network */
#define ETH_MAC_0   0xDE
#define ETH_MAC_1   0xAD
#define ETH_MAC_2   0xBE
#define ETH_MAC_3   0xEF
#define ETH_MAC_4   0xFE
#define ETH_MAC_5   0xED

/* W5500 Common Registers */
#define W5500_MR            0x0000
#define W5500_GAR0          0x0001
#define W5500_SUBR0         0x0005
#define W5500_SHAR0         0x0009
#define W5500_SIPR0         0x000F
#define W5500_BSB_COMMON    0x00
#define W5500_RW_WRITE      0x04
#define W5500_RW_READ       0x00

/* FreeRTOS Task priorities */
#define TASK_DATA_COLLECTOR_PRIORITY    2
#define TASK_LCD_UPDATE_PRIORITY        2
#define TASK_SD_LOGGER_PRIORITY         2
#define TASK_ETH_COMM_PRIORITY          3
#define TASK_MONITOR_PRIORITY           1
#define TASK_INIT_PRIORITY              4

/* FreeRTOS Task stacks (words) */
#define TASK_DATA_COLLECTOR_STACK       256
#define TASK_LCD_UPDATE_STACK           512
#define TASK_SD_LOGGER_STACK            512
#define TASK_ETH_COMM_STACK             512
#define TASK_MONITOR_STACK              256
#define TASK_INIT_STACK                 512

/* Queue sizes */
#define QUEUE_DATA_LENGTH   10
#define QUEUE_LOG_LENGTH    5

/* Timing (ms) */
#define SENSOR_PERIOD_MS    1000
#define LCD_PERIOD_MS       500
#define SD_PERIOD_MS        5000
#define ETH_PERIOD_MS       2000
#define MONITOR_PERIOD_MS   3000

/* Shared data structure */
typedef struct {
    uint32_t timestamp;
    int32_t  temperature_x100;  /* e.g. 2512 = 25.12 degC */
    int32_t  humidity_x100;
    uint32_t counter;
    uint16_t freeHeap;
} SensorData_t;

void SystemClock_Config(void);
void Error_Handler(void);

#endif /* CONFIG_H */

