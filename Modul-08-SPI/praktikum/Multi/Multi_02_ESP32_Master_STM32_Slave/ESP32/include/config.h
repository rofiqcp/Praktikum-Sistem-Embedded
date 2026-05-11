#ifndef CONFIG_H
#define CONFIG_H

// SPI Master Configuration
#define SPI_MASTER_HOST     SPI2_HOST
#define PIN_NUM_MISO        12
#define PIN_NUM_MOSI        13
#define PIN_NUM_CLK         14
#define PIN_NUM_CS          15

// SPI Clock speed
#define SPI_CLOCK_SPEED     1000000  // 1 MHz

// Buffer size
#define BUFFER_SIZE         64

// LED Pin
#define LED_PIN             2

#endif // CONFIG_H
