// include/config.h untuk ESP32_08_SPI_RTOS_OLED
// Header guard untuk mencegah include berulang
#ifndef CONFIG_H
#define CONFIG_H

// Mendefinisikan pin MOSI SPI
#define SPI_MOSI_PIN 23

// Mendefinisikan pin MISO SPI
#define SPI_MISO_PIN 19

// Mendefinisikan pin SCLK SPI
#define SPI_SCLK_PIN 18

// Mendefinisikan pin CS (Chip Select) untuk OLED
#define SPI_CS_PIN 5

// Mendefinisikan pin DC (Data/Command) untuk OLED
#define OLED_DC_PIN 2

// Mendefinisikan pin RST (Reset) untuk OLED
#define OLED_RST_PIN 4

// Mendefinisikan nomor bus SPI (SPI2_HOST = HSPI)
#define SPI_HOST SPI2_HOST

// Mendefinisikan baud rate SPI (10 MHz)
#define SPI_CLOCK_SPEED 10000000

// Mendefinisikan baud rate UART untuk komunikasi serial
#define UART_BAUD_RATE 115200

// Mendefinisikan delay untuk task prepare display dalam tick FreeRTOS
#define TASK_PREP_DELAY pdMS_TO_TICKS(1000)

// Mendefinisikan delay untuk task update OLED dalam tick FreeRTOS
#define TASK_UPD_DELAY pdMS_TO_TICKS(500)

// Mendefinisikan ukuran stack untuk task prepare display
#define TASK_PREP_STACK_SIZE 4096

// Mendefinisikan ukuran stack untuk task update OLED
#define TASK_UPD_STACK_SIZE 3072

// Mendefinisikan prioritas untuk task prepare display
#define TASK_PREP_PRIORITY 2

// Mendefinisikan prioritas untuk task update OLED
#define TASK_UPD_PRIORITY 2

// Mengakhiri header guard
#endif
