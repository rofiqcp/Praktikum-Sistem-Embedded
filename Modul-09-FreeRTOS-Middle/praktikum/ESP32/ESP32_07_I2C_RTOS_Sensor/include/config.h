// include/config.h untuk ESP32_07_I2C_RTOS_Sensor
// Header guard untuk mencegah include berulang
#ifndef CONFIG_H
#define CONFIG_H

// Mendefinisikan alamat I2C BME280 (0x76 atau 0x77)
#define BME280_I2C_ADDR 0x76

// Mendefinisikan pin SDA I2C
#define I2C_SDA_PIN 21

// Mendefinisikan pin SCL I2C
#define I2C_SCL_PIN 22

// Mendefinisikan nomor port I2C (I2C_NUM_0)
#define I2C_PORT I2C_NUM_0

// Mendefinisikan frekuensi clock I2C (100 kHz)
#define I2C_FREQ_HZ 100000

// Mendefinisikan baud rate UART untuk komunikasi serial
#define UART_BAUD_RATE 115200

// Mendefinisikan timeout I2C dalam tick FreeRTOS
#define I2C_TIMEOUT pdMS_TO_TICKS(100)

// Mendefinisikan delay untuk task read sensor dalam tick FreeRTOS
#define TASK_READ_DELAY pdMS_TO_TICKS(1000)

// Mendefinisikan delay untuk task process data dalam tick FreeRTOS
#define TASK_PROC_DELAY pdMS_TO_TICKS(500)

// Mendefinisikan ukuran stack untuk task read sensor
#define TASK_READ_STACK_SIZE 4096

// Mendefinisikan ukuran stack untuk task process data
#define TASK_PROC_STACK_SIZE 3072

// Mendefinisikan prioritas untuk task read sensor
#define TASK_READ_PRIORITY 2

// Mendefinisikan prioritas untuk task process data
#define TASK_PROC_PRIORITY 2

// Mengakhiri header guard
#endif
