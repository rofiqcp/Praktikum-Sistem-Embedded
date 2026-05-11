#ifndef CONFIG_H
#define CONFIG_H

// SPI1 Pins (Hardware SPI)
#define SPI_SCK_PIN   PA5
#define SPI_MISO_PIN  PA6
#define SPI_MOSI_PIN  PA7

// W5500 Ethernet Pins
#define ETH_CS_PIN    PA4
#define ETH_RST_PIN   PA3

// Network Configuration
#define MAC_ADDRESS   { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED }
#define IP_ADDRESS    192, 168, 1, 177
#define DNS_SERVER    8, 8, 8, 8
#define GATEWAY       192, 168, 1, 1
#define SUBNET        255, 255, 255, 0

// Test server
#define TEST_SERVER   "www.google.com"
#define TEST_PORT     80

#endif // CONFIG_H
