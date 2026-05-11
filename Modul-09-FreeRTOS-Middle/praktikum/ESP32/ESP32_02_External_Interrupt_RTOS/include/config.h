// include/config.h untuk ESP32_02_External_Interrupt_RTOS
// Header guard untuk mencegah include berulang
#ifndef CONFIG_H
#define CONFIG_H

// Mendefinisikan GPIO untuk interrupt input (tombol/sinyal eksternal)
#define INT_GPIO 0

// Mendefinisikan GPIO untuk LED indikator
#define LED_GPIO 2

// Mendefinisikan baud rate UART untuk komunikasi serial
#define UART_BAUD_RATE 115200

// Mendefinisikan delay untuk task pemroses interrupt dalam tick FreeRTOS
#define TASK_DELAY pdMS_TO_TICKS(10)

// Mendefinisikan ukuran stack untuk task
#define TASK_STACK_SIZE 2048

// Mendefinisikan prioritas untuk task pemroses interrupt
#define TASK_PRIORITY 2

// Mendefinisikan nilai HIGH untuk GPIO
#define GPIO_HIGH 1

// Mendefinisikan nilai LOW untuk GPIO
#define GPIO_LOW 0

// Mengakhiri header guard
#endif
