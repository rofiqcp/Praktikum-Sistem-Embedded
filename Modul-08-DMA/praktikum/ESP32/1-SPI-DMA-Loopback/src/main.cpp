/*
 * Project: ESP32 SPI DMA Loopback
 * Board: ESP32 DevKit V1
 * Wiring: Connect MOSI (GPIO 23) to MISO (GPIO 19)
 */

#include <Arduino.h>
#include <SPI.h>

#define BUFFER_SIZE 256

SPIClass *vspi = NULL;

// Buffers must be in DMA-capable memory (usually safe in DRAM in simple cases)
uint8_t *tx_buf;
uint8_t *rx_buf;

void setup() {
    Serial.begin(115200);
    
    // Allocate memory
    tx_buf = (uint8_t*)malloc(BUFFER_SIZE);
    rx_buf = (uint8_t*)malloc(BUFFER_SIZE);
    
    if(!tx_buf || !rx_buf) {
        Serial.println("Malloc Failed");
        while(1);
    }
    
    // Initialize Pattern
    for(int i=0; i<BUFFER_SIZE; i++) {
        tx_buf[i] = (uint8_t)(i & 0xFF);
        rx_buf[i] = 0;
    }

    // Init SPI
    vspi = new SPIClass(VSPI);
    // SCLK=18, MISO=19, MOSI=23, SS=5
    vspi->begin(); 
    
    Serial.println("Starting SPI DMA Transfer...");
}

void loop() {
    // 1. Transaction Settings
    vspi->beginTransaction(SPISettings(5000000, MSBFIRST, SPI_MODE0)); // 5 MHz
    
    // 2. Transfer Data
    // Arduino ESP32 core automatically uses DMA for transfers > 32/64 bytes
    // depending on the version and specific API call.
    // transferBytes is the most efficient block transfer method.
    vspi->transferBytes(tx_buf, rx_buf, BUFFER_SIZE);
    
    vspi->endTransaction();
    
    // 3. Verify
    int errors = 0;
    for(int i=0; i<BUFFER_SIZE; i++) {
        if(rx_buf[i] != tx_buf[i]) {
            errors++;
        }
    }
    
    if(errors == 0) {
        Serial.printf("Transfer Success. Buffer Size: %d\n", BUFFER_SIZE);
        // Clear RX for next test
        memset(rx_buf, 0, BUFFER_SIZE);
    } else {
        Serial.printf("Error: %d Mismatches\n", errors);
    }
    
    delay(1000);
}
