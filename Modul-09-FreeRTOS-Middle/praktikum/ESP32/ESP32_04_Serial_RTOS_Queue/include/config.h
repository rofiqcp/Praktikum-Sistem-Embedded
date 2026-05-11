// include/config.h untuk ESP32_04_Serial_RTOS_Queue
// Header guard untuk mencegah include berulang
#ifndef CONFIG_H
#define CONFIG_H

// Mendefinisikan UART port yang digunakan (UART0)
#define UART_PORT UART_NUM_0

// Mendefinisikan baud rate UART untuk komunikasi serial
#define UART_BAUD_RATE 115200

// Mendefinisikan pin TX UART (TX0 = GPIO1)
#define UART_TX_PIN 1

// Mendefinisikan pin RX UART (RX0 = GPIO3)
#define UART_RX_PIN 3

// Mendefinisikan ukuran buffer UART RX
#define UART_BUF_SIZE 1024

// Mendefinisikan ukuran queue untuk command
#define CMD_QUEUE_SIZE 10

// Mendefinisikan delay untuk task read UART dalam tick FreeRTOS
#define TASK_READ_DELAY pdMS_TO_TICKS(100)

// Mendefinisikan delay untuk task process command dalam tick FreeRTOS
#define TASK_PROC_DELAY pdMS_TO_TICKS(50)

// Mendefinisikan ukuran stack untuk task read UART
#define TASK_READ_STACK_SIZE 4096

// Mendefinisikan ukuran stack untuk task process command
#define TASK_PROC_STACK_SIZE 3072

// Mendefinisikan prioritas untuk task read UART
#define TASK_READ_PRIORITY 2

// Mendefinisikan prioritas untuk task process command
#define TASK_PROC_PRIORITY 2

// Mendefinisikan panjang maksimum command string
#define MAX_CMD_LEN 50

// Mengakhiri header guard
#endif
