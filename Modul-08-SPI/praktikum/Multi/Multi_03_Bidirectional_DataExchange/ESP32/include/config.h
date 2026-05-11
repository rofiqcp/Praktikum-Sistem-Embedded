#ifndef CONFIG_H
#define CONFIG_H

// SPI Slave Configuration
#define SPI_SLAVE_HOST      SPI2_HOST
#define PIN_NUM_MISO        12
#define PIN_NUM_MOSI        13
#define PIN_NUM_CLK         14
#define PIN_NUM_CS          15

// Handshake pin (ESP32 signals ready)
#define PIN_NUM_HANDSHAKE   27

// Buffer size
#define BUFFER_SIZE         128

// LED Pin
#define LED_PIN             2

// Data exchange protocol
#define CMD_READ_SENSOR     0x01
#define CMD_WRITE_LED       0x02
#define CMD_GET_STATUS      0x03
#define CMD_ECHO            0x04

#endif // CONFIG_H
