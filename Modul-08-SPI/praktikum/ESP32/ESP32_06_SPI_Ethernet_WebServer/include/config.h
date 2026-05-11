#ifndef CONFIG_H
#define CONFIG_H

// W5500 Ethernet SPI Configuration
#define ETH_MISO_PIN    19
#define ETH_MOSI_PIN    23
#define ETH_SCLK_PIN    18
#define ETH_CS_PIN      5
#define ETH_INT_PIN     4
#define ETH_RST_PIN     -1

// Network Configuration
#define ETH_SPI_CLOCK_MHZ   20
#define ETH_PHY_ADDR        1

// Web Server Configuration
#define WEB_SERVER_PORT     80
#define MAX_CONNECTIONS     5

// Static IP Configuration (optional)
#define USE_STATIC_IP       0

#endif
