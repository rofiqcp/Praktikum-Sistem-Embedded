#ifndef CONFIG_H
#define CONFIG_H

/* --- UART Line Editor Configuration --- */
#define MAX_LINE          128
#define HISTORY_SIZE      5

/* USART1 pins */
#define USART1_TX_PIN     GPIO_PIN_9
#define USART1_RX_PIN     GPIO_PIN_10
#define USART1_PORT       GPIOA

/* Baud rate */
#define UART_BAUDRATE     115200

#endif /* CONFIG_H */
