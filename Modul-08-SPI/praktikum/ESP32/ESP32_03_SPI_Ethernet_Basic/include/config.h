#ifndef CONFIG_H
#define CONFIG_H

// W5500 Ethernet Pin Definitions
#define ETH_MOSI_PIN    13
#define ETH_MISO_PIN    12
#define ETH_SCLK_PIN    14
#define ETH_CS_PIN      15
#define ETH_INT_PIN     4
#define ETH_RST_PIN     5

// SPI Configuration
#define ETH_SPI_HOST    SPI2_HOST
#define ETH_SPI_CLOCK   20000000  // 20 MHz

// Network Configuration
#define ETH_PHY_ADDR    1

// Test Configuration
#define PING_TARGET     "8.8.8.8"  // Google DNS
#define TEST_DOMAIN     "www.google.com"

#endif // CONFIG_H
