/**
 * ==========================================================================
 * FILE        : config.h
 * PROJECT     : ESP32_05_SPI_MCP3208_ADC
 * MODUL       : 07 - SPI & Storage
 * DESCRIPTION : Configuration for MCP3208 8-channel 12-bit ADC via SPI
 * 
 * HARDWARE CONNECTIONS:
 *   ESP32 GPIO23 (MOSI) --> MCP3208 Pin 11 (DIN)
 *   ESP32 GPIO19 (MISO) <-- MCP3208 Pin 12 (DOUT)
 *   ESP32 GPIO18 (SCLK) --> MCP3208 Pin 13 (CLK)
 *   ESP32 GPIO5  (CS)   --> MCP3208 Pin 10 (CS/SHDN)
 *   MCP3208 Pin 15 (VREF) --> 3.3V
 *   MCP3208 Pin 16 (VDD)  --> 3.3V
 *   MCP3208 Pin 14 (AGND) --> GND
 *   MCP3208 Pin 9  (DGND) --> GND
 * ==========================================================================
 */

#ifndef CONFIG_H
#define CONFIG_H

/* ======================== SPI Pin Definitions ======================== */
#define PIN_MOSI            23      // Master Out Slave In
#define PIN_MISO            19      // Master In Slave Out
#define PIN_SCLK            18      // Serial Clock
#define PIN_CS              5       // Chip Select (active low)

/* ======================== SPI Configuration ========================= */
#define SPI_HOST_ID         HSPI_HOST
#define SPI_CLOCK_SPEED     (1 * 1000 * 1000)   // 1 MHz SPI clock
#define SPI_DMA_CHANNEL     1

/* ======================== MCP3208 Parameters ======================== */
// MCP3208: 8-channel, 12-bit SAR ADC
#define MCP3208_RESOLUTION  4095    // 12-bit ADC (2^12 - 1)
#define MCP3208_NUM_CHANNELS 8      // 8 single-ended channels
#define MCP3208_VREF        3.3f    // Reference voltage in Volts

/* ======================== Channel Definitions ======================= */
#define CH0                 0
#define CH1                 1
#define CH2                 2
#define CH3                 3
#define CH4                 4
#define CH5                 5
#define CH6                 6
#define CH7                 7

/* ======================== Sampling Configuration ==================== */
#define SAMPLE_COUNT        10      // Number of samples for statistics
#define READ_INTERVAL_MS    1000    // Interval between read cycles (ms)
#define CHANNEL_DELAY_MS    10      // Delay between channel reads (ms)

#endif // CONFIG_H
