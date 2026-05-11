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

// Web server configuration
#define WEB_SERVER_PORT 80
#define MAX_CLIENTS     4

// Sensor pins (simulated)
#define TEMP_SENSOR_PIN   PA0  // ADC
#define LIGHT_SENSOR_PIN  PA1  // ADC
#define LED_PIN           PC13 // Built-in LED
#define RELAY_PIN         PB1  // Relay control

#endif // CONFIG_H
