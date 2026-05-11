// include/config.h untuk ESP32_06_ADC_RTOS_Input
// Header guard untuk mencegah include berulang
#ifndef CONFIG_H
#define CONFIG_H

// Mendefinisikan channel ADC yang digunakan (ADC_CHANNEL_0 = GPIO36)
#define ADC_CHANNEL ADC_CHANNEL_0

// Mendefinisikan GPIO untuk LED indikator
#define LED_GPIO 2

// Mendefinisikan baud rate UART untuk komunikasi serial
#define UART_BAUD_RATE 115200

// Mendefinisikan ukuran queue untuk ADC values
#define ADC_QUEUE_SIZE 10

// Mendefinisikan delay untuk task read ADC dalam tick FreeRTOS
#define TASK_READ_DELAY pdMS_TO_TICKS(100)

// Mendefinisikan delay untuk task filter dan process dalam tick FreeRTOS
#define TASK_PROC_DELAY pdMS_TO_TICKS(50)

// Mendefinisikan ukuran stack untuk task read ADC
#define TASK_READ_STACK_SIZE 2048

// Mendefinisikan ukuran stack untuk task filter dan process
#define TASK_PROC_STACK_SIZE 3072

// Mendefinisikan prioritas untuk task read ADC
#define TASK_READ_PRIORITY 2

// Mendefinisikan prioritas untuk task filter dan process
#define TASK_PROC_PRIORITY 2

// Mendefinisikan konstanta untuk filter (faktor smoothing)
#define FILTER_ALPHA 0.1f

// Mengakhiri header guard
#endif
