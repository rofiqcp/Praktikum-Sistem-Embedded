/**
 * ==========================================================================
 * FILE        : config.h
 * PROJECT     : ESP32_07_SPI_Multi_Slave
 * MODUL       : 07 - SPI & Storage
 * DESCRIPTION : Configuration for multi-slave SPI communication
 * 
 * HARDWARE CONNECTIONS:
 *   ESP32 GPIO23 (MOSI) --> Shared MOSI bus to both slaves
 *   ESP32 GPIO19 (MISO) <-- Shared MISO bus from both slaves
 *   ESP32 GPIO18 (SCLK) --> Shared SCLK bus to both slaves
 *   ESP32 GPIO5  (CS1)  --> Slave 1 CS (e.g., ADC / Sensor)
 *   ESP32 GPIO17 (CS2)  --> Slave 2 CS (e.g., Flash / Memory)
 * 
 * NOTE: MOSI, MISO, SCLK are shared; only CS pins differ per slave.
 * ==========================================================================
 */

#ifndef CONFIG_H
#define CONFIG_H

/* ======================== Shared SPI Bus Pins ======================== */
#define PIN_MOSI            23      // Master Out Slave In (shared)
#define PIN_MISO            19      // Master In Slave Out (shared)
#define PIN_SCLK            18      // Serial Clock (shared)

/* ======================== Slave Chip Select Pins ==================== */
#define PIN_CS_SLAVE1       5       // Chip Select for Slave 1
#define PIN_CS_SLAVE2       17      // Chip Select for Slave 2

/* ======================== SPI Configuration ========================= */
#define SPI_HOST_ID         HSPI_HOST
#define SPI_CLOCK_SPEED     (1 * 1000 * 1000)   // 1 MHz for both devices
#define SPI_DMA_CHANNEL     1

/* ======================== Device Identifiers ======================== */
#define DEVICE_ID_SLAVE1    0x01    // Simulated ADC / Sensor device
#define DEVICE_ID_SLAVE2    0x02    // Simulated Flash / Memory device

/* ======================== Device Names ============================== */
#define DEVICE_NAME_SLAVE1  "ADC-Sensor"
#define DEVICE_NAME_SLAVE2  "Flash-Memory"

/* ======================== Test Configuration ======================== */
#define NUM_TRANSFERS       5       // Number of transfers per slave per cycle
#define CYCLE_DELAY_MS      2000    // Delay between communication cycles
#define INTER_SLAVE_DELAY_MS 100    // Delay between switching slaves
#define TRANSFER_SIZE       4       // Bytes per transfer

#endif // CONFIG_H
