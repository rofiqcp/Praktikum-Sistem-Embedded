// include/config.h untuk ESP32_09_GPIO_Interrupt_Combined
// Header guard untuk mencegah include berulang
#ifndef CONFIG_H
#define CONFIG_H

// Mendefinisikan GPIO untuk Button 1
#define BUTTON1_GPIO 0

// Mendefinisikan GPIO untuk Button 2
#define BUTTON2_GPIO 35

// Mendefinisikan GPIO untuk Button 3
#define BUTTON3_GPIO 34

// Mendefinisikan GPIO untuk LED 1
#define LED1_GPIO 2

// Mendefinisikan GPIO untuk LED 2
#define LED2_GPIO 4

// Mendefinisikan GPIO untuk LED 3
#define LED3_GPIO 16

// Mendefinisikan baud rate UART untuk komunikasi serial
#define UART_BAUD_RATE 115200

// Mendefinisikan ukuran queue untuk button events
#define BUTTON_QUEUE_SIZE 10

// Mendefinisikan delay untuk task LED control dalam tick FreeRTOS
#define TASK_LED_DELAY pdMS_TO_TICKS(50)

// Mendefinisikan delay untuk task button monitor dalam tick FreeRTOS
#define TASK_BTN_DELAY pdMS_TO_TICKS(20)

// Mendefinisikan ukuran stack untuk task LED control
#define TASK_LED_STACK_SIZE 2048

// Mendefinisikan ukuran stack untuk task button monitor
#define TASK_BTN_STACK_SIZE 3072

// Mendefinisikan prioritas untuk task LED control
#define TASK_LED_PRIORITY 2

// Mendefinisikan prioritas untuk task button monitor
#define TASK_BTN_PRIORITY 2

// Mendefinisikan tipe event button
typedef enum {
    BUTTON_PRESSED = 0,
    BUTTON_RELEASED = 1
} button_event_type_t;

// Mengakhiri header guard
#endif
