#ifndef CONFIG_H
#define CONFIG_H

#include "stm32f1xx_hal.h"

/* Node identification */
#define NODE_ID             0x01

/* SPI1: Master (to ESP32 SPI Slave) - PA4-PA7 */
#define SPI1_CS_PORT        GPIOA
#define SPI1_CS_PIN         GPIO_PIN_4
#define SPI1_SCK_PIN        GPIO_PIN_5
#define SPI1_MISO_PIN       GPIO_PIN_6
#define SPI1_MOSI_PIN       GPIO_PIN_7

/* Handshake (ESP32 ready = HIGH) */
#define HS_PORT             GPIOA
#define HS_PIN              GPIO_PIN_3

/* SPI2: SD Card - PB12-PB15 */
#define SD_CS_PORT          GPIOB
#define SD_CS_PIN           GPIO_PIN_12
#define SD_SCK_PIN          GPIO_PIN_13
#define SD_MISO_PIN         GPIO_PIN_14
#define SD_MOSI_PIN         GPIO_PIN_15

/* LED (active LOW bluepill) */
#define LED_PORT            GPIOC
#define LED_PIN             GPIO_PIN_13

/* UART1 debug */
#define UART_BAUDRATE       115200

/* FreeRTOS priorities */
#define SENSOR_TASK_PRIORITY    2
#define SD_LOG_TASK_PRIORITY    2
#define TX_TASK_PRIORITY        3
#define MONITOR_TASK_PRIORITY   1

/* FreeRTOS stacks (words) */
#define SENSOR_TASK_STACK   256
#define SD_LOG_TASK_STACK   256
#define TX_TASK_STACK       256
#define MONITOR_TASK_STACK  256

/* Queue sizes */
#define TX_QUEUE_LEN    8
#define SD_QUEUE_LEN    8

/* Timing */
#define SENSOR_PERIOD_MS    1000
#define MONITOR_PERIOD_MS   5000

/* Distributed packet */
typedef struct __attribute__((packed)) {
    uint8_t  magic;         /* 0xB6 */
    uint8_t  node_id;
    uint8_t  packet_type;
    uint16_t seq_num;
    uint32_t timestamp;
    int16_t  temperature_x100;
    uint16_t humidity_x100;
    uint16_t sd_write_count;
    uint8_t  checksum;
} DistPacket_t;

#define DIST_MAGIC          0xB6
#define PKT_TYPE_SENSOR     0x01
#define PKT_TYPE_STATUS     0x02

void SystemClock_Config(void);
void Error_Handler(void);

#endif /* CONFIG_H */
