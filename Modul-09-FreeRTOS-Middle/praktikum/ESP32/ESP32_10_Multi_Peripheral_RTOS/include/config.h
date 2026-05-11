// include/config.h untuk ESP32_10_Multi_Peripheral_RTOS
// Header guard untuk mencegah include berulang
#ifndef CONFIG_H
#define CONFIG_H

// === GPIO DEFINITIONS ===
// Mendefinisikan GPIO untuk Button
#define BUTTON_GPIO 0
// Mendefinisikan GPIO untuk LED 1
#define LED1_GPIO 2
// Mendefinisikan GPIO untuk LED 2
#define LED2_GPIO 4

// === UART DEFINITIONS ===
// Mendefinisikan UART port
#define UART_PORT UART_NUM_0
// Mendefinisikan baud rate UART
#define UART_BAUD_RATE 115200
// Mendefinisikan ukuran buffer UART RX
#define UART_BUF_SIZE 1024

// === ADC DEFINITIONS ===
// Mendefinisikan channel ADC
#define ADC_CHANNEL ADC_CHANNEL_0

// === PWM/DAC DEFINITIONS ===
// Mendefinisikan GPIO untuk PWM
#define PWM_GPIO 5
// Mendefinisikan channel LEDC
#define LEDC_CHANNEL LEDC_CHANNEL_0
// Mendefinisikan timer LEDC
#define LEDC_TIMER LEDC_TIMER_0
// Mendefinisikan frekuensi PWM
#define PWM_FREQUENCY 1000

// === I2C DEFINITIONS ===
// Mendefinisikan pin SDA I2C
#define I2C_SDA_PIN 21
// Mendefinisikan pin SCL I2C
#define I2C_SCL_PIN 22
// Mendefinisikan port I2C
#define I2C_PORT I2C_NUM_0

// === SPI DEFINITIONS ===
// Mendefinisikan pin MOSI SPI
#define SPI_MOSI_PIN 23
// Mendefinisikan pin SCLK SPI
#define SPI_SCLK_PIN 18
// Mendefinisikan pin CS SPI
#define SPI_CS_PIN 19

// === QUEUE & SEMAPHORE SIZES ===
// Mendefinisikan ukuran queue button events
#define BUTTON_QUEUE_SIZE 5
// Mendefinisikan ukuran queue UART commands
#define UART_QUEUE_SIZE 5
// Mendefinisikan ukuran queue ADC data
#define ADC_QUEUE_SIZE 5

// === TASK DELAYS (in ticks) ===
// Mendefinisikan delay untuk task LED
#define TASK_LED_DELAY pdMS_TO_TICKS(500)
// Mendefinisikan delay untuk task UART
#define TASK_UART_DELAY pdMS_TO_TICKS(100)
// Mendefinisikan delay untuk task ADC
#define TASK_ADC_DELAY pdMS_TO_TICKS(200)
// Mendefinisikan delay untuk task system monitor
#define TASK_MON_DELAY pdMS_TO_TICKS(1000)

// === STACK SIZES ===
// Mendefinisikan ukuran stack untuk task LED
#define TASK_LED_STACK_SIZE 2048
// Mendefinisikan ukuran stack untuk task UART
#define TASK_UART_STACK_SIZE 4096
// Mendefinisikan ukuran stack untuk task ADC
#define TASK_ADC_STACK_SIZE 2048
// Mendefinisikan ukuran stack untuk task system monitor
#define TASK_MON_STACK_SIZE 3072

// === PRIORITIES ===
// Mendefinisikan prioritas untuk task LED
#define TASK_LED_PRIORITY 1
// Mendefinisikan prioritas untuk task UART
#define TASK_UART_PRIORITY 2
// Mendefinisikan prioritas untuk task ADC
#define TASK_ADC_PRIORITY 2
// Mendefinisikan prioritas untuk task system monitor
#define TASK_MON_PRIORITY 1

// === EVENT GROUP BITS ===
// Mendefinisikan bit untuk button event
#define EVENT_BIT_BUTTON (1 << 0)
// Mendefinisikan bit untuk UART command
#define EVENT_BIT_UART (1 << 1)
// Mendefinisikan bit untuk ADC data ready
#define EVENT_BIT_ADC (1 << 2)
// Mendefinisikan bit untuk system error
#define EVENT_BIT_ERROR (1 << 3)

// Mengakhiri header guard
#endif
