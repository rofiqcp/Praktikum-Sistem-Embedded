#ifndef CONFIG_H
#define CONFIG_H

// W5500 Ethernet SPI Configuration
#define ETH_MOSI_PIN        23
#define ETH_MISO_PIN        19
#define ETH_SCLK_PIN        18
#define ETH_CS_PIN          5
#define ETH_INT_PIN         4
#define ETH_RST_PIN         -1

// Network Configuration
#define ETH_PHY_ADDR        1
#define ETH_SPI_CLOCK_MHZ   20

// FreeRTOS Configuration
#define NETWORK_TASK_PRIORITY       5
#define TX_TASK_PRIORITY            4
#define RX_TASK_PRIORITY            4
#define MONITOR_TASK_PRIORITY       3

#define TASK_STACK_SIZE             4096

// Semaphore timeout
#define SEMAPHORE_TIMEOUT_MS        1000

#endif // CONFIG_H
