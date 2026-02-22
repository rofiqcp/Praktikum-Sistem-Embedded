#ifndef CONFIG_H
#define CONFIG_H

/* --- UART JSON Protocol Configuration --- */
#define MAX_JSON_LEN      256

/* LED on PC13 (active low on Blue Pill) */
#define LED_PORT          GPIOC
#define LED_PIN           GPIO_PIN_13

/* USART1 pins */
#define USART1_TX_PIN     GPIO_PIN_9
#define USART1_RX_PIN     GPIO_PIN_10
#define USART1_PORT       GPIOA

/* Baud rate */
#define UART_BAUDRATE     115200

#endif /* CONFIG_H */
