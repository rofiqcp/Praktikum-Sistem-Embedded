/**
 * ==========================================================================
 * FILE        : config.h
 * PROJECT     : ESP32_06_SPI_DAC_MCP4921
 * MODUL       : 07 - SPI & Storage
 * DESCRIPTION : Configuration for MCP4921 12-bit single DAC via SPI
 * 
 * HARDWARE CONNECTIONS:
 *   ESP32 GPIO23 (MOSI) --> MCP4921 Pin 4 (SDI)
 *   ESP32 GPIO18 (SCLK) --> MCP4921 Pin 3 (SCK)
 *   ESP32 GPIO5  (CS)   --> MCP4921 Pin 2 (CS)
 *   ESP32 GPIO21 (LDAC) --> MCP4921 Pin 5 (LDAC)
 *   MCP4921 Pin 6 (VREF) --> 3.3V
 *   MCP4921 Pin 1 (VDD)  --> 3.3V
 *   MCP4921 Pin 7 (VSS)  --> GND
 *   MCP4921 Pin 8 (VOUT) --> Output (connect to oscilloscope/load)
 * ==========================================================================
 */

#ifndef CONFIG_H
#define CONFIG_H

/* ======================== SPI Pin Definitions ======================== */
#define PIN_MOSI            23      // Master Out Slave In (SDI)
#define PIN_SCLK            18      // Serial Clock (SCK)
#define PIN_CS              5       // Chip Select (active low)
#define PIN_LDAC            21      // Latch DAC (active low)

/* ======================== SPI Configuration ========================= */
#define SPI_HOST_ID         HSPI_HOST
#define SPI_CLOCK_SPEED     (8 * 1000 * 1000)   // 8 MHz SPI clock
#define SPI_DMA_CHANNEL     1

/* ======================== MCP4921 Parameters ======================== */
// MCP4921: Single channel, 12-bit voltage output DAC
#define MCP4921_RESOLUTION  4095    // 12-bit DAC (2^12 - 1)
#define MCP4921_VREF        3.3f   // Reference voltage in Volts

/* ======================== MCP4921 Command Bits ====================== */
// 16-bit write frame: [A/B | BUF | GA | SHDN | D11:D0]
#define MCP4921_DAC_A       (0 << 15)   // Bit 15: 0=DAC A
#define MCP4921_UNBUFFERED  (0 << 14)   // Bit 14: 0=Unbuffered VREF
#define MCP4921_GAIN_1X     (1 << 13)   // Bit 13: 1=1x gain (VOUT = VREF * D/4096)
#define MCP4921_ACTIVE      (1 << 12)   // Bit 12: 1=Active mode (not shutdown)

#define MCP4921_CONFIG_BITS (MCP4921_DAC_A | MCP4921_UNBUFFERED | MCP4921_GAIN_1X | MCP4921_ACTIVE)

/* ======================== Waveform Definitions ====================== */
typedef enum {
    WAVEFORM_SINE = 0,
    WAVEFORM_SAWTOOTH,
    WAVEFORM_TRIANGLE,
    WAVEFORM_SQUARE,
    WAVEFORM_COUNT          // Total number of waveform types
} waveform_type_t;

/* ======================== Waveform Configuration ==================== */
#define WAVEFORM_POINTS     256         // Number of points per waveform cycle
#define WAVEFORM_SWITCH_SEC 5           // Switch waveform every N seconds
#define WAVEFORM_STEP_US    100         // Microseconds between DAC updates
#define WAVEFORM_AMPLITUDE  4095        // Full scale amplitude

#endif // CONFIG_H
