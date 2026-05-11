// include/config.h untuk ESP32_01_GPIO_RTOS_Task
// Header guard untuk mencegah include berulang
#ifndef CONFIG_H
#define CONFIG_H

// Mendefinisikan GPIO untuk LED yang berkedip setiap 500ms
#define LED_1_GPIO 2

// Mendefinisikan GPIO untuk LED yang berkedip setiap 1000ms
#define LED_2_GPIO 4

// Mendefinisikan GPIO untuk tombol yang dimonitor
#define BUTTON_GPIO 0

// Mendefinisikan baud rate UART untuk komunikasi serial
#define UART_BAUD_RATE 115200

// Mendefinisikan delay untuk Task 1 (LED 500ms) dalam tick FreeRTOS
#define TASK_1_DELAY pdMS_TO_TICKS(500)

// Mendefinisikan delay untuk Task 2 (LED 1000ms) dalam tick FreeRTOS
#define TASK_2_DELAY pdMS_TO_TICKS(1000)

// Mendefinisikan delay untuk Task 3 (Button monitor) dalam tick FreeRTOS
#define TASK_3_DELAY pdMS_TO_TICKS(50)

// Mendefinisikan ukuran stack untuk semua task
#define TASK_STACK_SIZE 2048

// Mendefinisikan prioritas untuk Task 1 (LED 500ms)
#define TASK_1_PRIORITY 1

// Mendefinisikan prioritas untuk Task 2 (LED 1000ms)
#define TASK_2_PRIORITY 1

// Mendefinisikan prioritas untuk Task 3 (Button monitor)
#define TASK_3_PRIORITY 2

// Mendefinisikan nilai HIGH untuk GPIO
#define GPIO_HIGH 1

// Mendefinisikan nilai LOW untuk GPIO
#define GPIO_LOW 0

// Mengakhiri header guard
#endif
