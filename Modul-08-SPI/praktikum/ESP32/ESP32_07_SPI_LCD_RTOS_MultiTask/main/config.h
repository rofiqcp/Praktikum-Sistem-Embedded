#ifndef CONFIG_H
#define CONFIG_H

// SPI LCD Configuration
#define LCD_HOST        SPI2_HOST
#define LCD_MOSI_PIN    23
#define LCD_SCLK_PIN    18
#define LCD_CS_PIN      5
#define LCD_DC_PIN      2
#define LCD_RST_PIN     4
#define LCD_BL_PIN      15

// LCD Dimensions
#define LCD_WIDTH       240
#define LCD_HEIGHT      320

// FreeRTOS Configuration
#define LCD_UPDATE_TASK_PRIORITY    5
#define SENSOR_READ_TASK_PRIORITY   4
#define UI_TASK_PRIORITY            3
#define BACKGROUND_TASK_PRIORITY    2

#define LCD_UPDATE_STACK_SIZE       4096
#define SENSOR_READ_STACK_SIZE      2048
#define UI_STACK_SIZE               3072
#define BACKGROUND_STACK_SIZE       2048

// Update intervals (ms)
#define LCD_UPDATE_INTERVAL         50
#define SENSOR_READ_INTERVAL        100
#define UI_UPDATE_INTERVAL          200
#define BACKGROUND_INTERVAL         1000

#endif // CONFIG_H
