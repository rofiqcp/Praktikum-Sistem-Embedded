#ifndef CONFIG_H
#define CONFIG_H

#include "stm32f1xx_hal.h"

/* SPI1 Master (to ESP32 SPI Slave) */
#define SPI_SCK_PORT        GPIOA
#define SPI_SCK_PIN         GPIO_PIN_5
#define SPI_MISO_PORT       GPIOA
#define SPI_MISO_PIN        GPIO_PIN_6
#define SPI_MOSI_PORT       GPIOA
#define SPI_MOSI_PIN        GPIO_PIN_7
#define SPI_CS_PORT         GPIOA
#define SPI_CS_PIN          GPIO_PIN_4

/* Handshake (ESP32 ready = HIGH) */
#define HS_PORT             GPIOA
#define HS_PIN              GPIO_PIN_3

/* LED (active LOW bluepill) */
#define LED_PORT            GPIOC
#define LED_PIN             GPIO_PIN_13

/* UART debug */
#define UART_BAUDRATE       115200

/* FreeRTOS task priorities */
#define SENSOR_TASK_PRIORITY    2
#define TX_TASK_PRIORITY        3
#define MONITOR_TASK_PRIORITY   1

/* FreeRTOS task stacks (words) */
#define SENSOR_TASK_STACK   256
#define TX_TASK_STACK       256
#define MONITOR_TASK_STACK  256

/* Queue */
#define SPI_QUEUE_LENGTH    8

/* Timing */
#define SENSOR_PERIOD_MS    500
#define MONITOR_PERIOD_MS   3000

/* SPI packet structure sent to ESP32 */
typedef struct __attribute__((packed)) {
    uint8_t  magic;           /* 0xA5 */
    uint8_t  cmd;             /* command code */
    int16_t  temperature_x100;
    uint16_t humidity_x100;
    uint32_t timestamp;
    uint8_t  checksum;
} SpiPacket_t;

/* Commands */
#define CMD_SENSOR_DATA     0x01
#define CMD_STATUS          0x02
#define CMD_ACK             0xAA
#define PACKET_MAGIC        0xA5

void SystemClock_Config(void);
void Error_Handler(void);

#endif /* CONFIG_H */
