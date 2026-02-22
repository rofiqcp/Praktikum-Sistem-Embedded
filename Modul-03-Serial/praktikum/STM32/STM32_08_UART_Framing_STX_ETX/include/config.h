#ifndef CONFIG_H
#define CONFIG_H

/* --- UART Framing STX/ETX Configuration --- */
#define STX               0x02
#define ETX               0x03
#define DLE               0x10

#define MAX_FRAME         256

/* USART1 pins */
#define USART1_TX_PIN     GPIO_PIN_9
#define USART1_RX_PIN     GPIO_PIN_10
#define USART1_PORT       GPIOA

/* LED on PC13 for status indication */
#define LED_PORT          GPIOC
#define LED_PIN           GPIO_PIN_13

/* Baud rate */
#define UART_BAUDRATE     115200

/* Test frame send interval (ms) */
#define FRAME_SEND_INTERVAL  3000

#endif /* CONFIG_H */
