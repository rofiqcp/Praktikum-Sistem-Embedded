#ifndef CONFIG_H
#define CONFIG_H

/* --- UART Command Parser Configuration --- */
#define MAX_CMD_LEN       64

/* LED on PC13 (active low on Blue Pill) */
#define LED_PORT          GPIOC
#define LED_PIN           GPIO_PIN_13

/* Buzzer on PB1 */
#define BUZZER_PORT       GPIOB
#define BUZZER_PIN        GPIO_PIN_1

/* USART1 pins */
#define USART1_TX_PIN     GPIO_PIN_9
#define USART1_RX_PIN     GPIO_PIN_10
#define USART1_PORT       GPIOA

/* Baud rate */
#define UART_BAUDRATE     115200

#endif /* CONFIG_H */
