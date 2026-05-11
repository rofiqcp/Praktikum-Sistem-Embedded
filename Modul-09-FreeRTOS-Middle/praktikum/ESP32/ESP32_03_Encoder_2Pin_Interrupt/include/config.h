// include/config.h untuk ESP32_03_Encoder_2Pin_Interrupt
// Header guard untuk mencegah include berulang
#ifndef CONFIG_H
#define CONFIG_H

// Mendefinisikan GPIO untuk Encoder Pin A
#define ENCODER_PIN_A 0

// Mendefinisikan GPIO untuk Encoder Pin B
#define ENCODER_PIN_B 1

// Mendefinisikan GPIO untuk LED indikator CW (Clockwise)
#define LED_CW_GPIO 2

// Mendefinisikan GPIO untuk LED indikator CCW (Counter-Clockwise)
#define LED_CCW_GPIO 4

// Mendefinisikan baud rate UART untuk komunikasi serial
#define UART_BAUD_RATE 115200

// Mendefinisikan ukuran queue untuk event encoder
#define QUEUE_SIZE 10

// Mendefinisikan delay untuk task pemroses encoder dalam tick FreeRTOS
#define TASK_DELAY pdMS_TO_TICKS(10)

// Mendefinisikan ukuran stack untuk task
#define TASK_STACK_SIZE 2048

// Mendefinisikan prioritas untuk task pemroses encoder
#define TASK_PRIORITY 2

// Mendefinisikan nilai HIGH untuk GPIO
#define GPIO_HIGH 1

// Mendefinisikan nilai LOW untuk GPIO
#define GPIO_LOW 0

// Mengakhiri header guard
#endif
