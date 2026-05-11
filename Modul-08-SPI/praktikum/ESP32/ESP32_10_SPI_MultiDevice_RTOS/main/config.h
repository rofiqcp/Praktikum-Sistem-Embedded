#ifndef CONFIG_H
#define CONFIG_H

// SPI Bus Configuration
#define SPI_MOSI_PIN        23
#define SPI_MISO_PIN        19
#define SPI_SCLK_PIN        18

// LCD Configuration (SPI2)
#define LCD_HOST            SPI2_HOST
#define LCD_CS_PIN          5
#define LCD_DC_PIN          2
#define LCD_RST_PIN         4
#define LCD_BL_PIN          15
#define LCD_WIDTH           240
#define LCD_HEIGHT          320

// SD Card Configuration (SPI2)
#define SD_CS_PIN           13

// Ethernet Configuration (SPI3)
#define ETH_HOST            SPI3_HOST
#define ETH_CS_PIN          14
#define ETH_INT_PIN         27
#define ETH_RST_PIN         -1

// FreeRTOS Task Configuration
#define LCD_TASK_PRIORITY           5
#define SD_TASK_PRIORITY            4
#define ETH_TASK_PRIORITY           4
#define COORDINATOR_TASK_PRIORITY   6
#define MONITOR_TASK_PRIORITY       2

#define TASK_STACK_SIZE             4096

// Queue sizes
#define LCD_QUEUE_SIZE              10
#define SD_QUEUE_SIZE               20
#define ETH_QUEUE_SIZE              15

#endif // CONFIG_H
