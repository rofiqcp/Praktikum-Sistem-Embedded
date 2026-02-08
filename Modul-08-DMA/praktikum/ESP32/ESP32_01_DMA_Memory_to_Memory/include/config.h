/**
 * ===========================================================================
 *  ESP32 Memory Copy Benchmark — Configuration
 * ===========================================================================
 *
 *  CATATAN ARSITEKTUR:
 *  ESP32 TIDAK memiliki DMA controller general-purpose (Memory-to-Memory)
 *  seperti STM32. Pada ESP32, DMA terintegrasi di dalam peripheral
 *  (SPI, I2S, UART, ADC). Untuk transfer memory-to-memory, kita
 *  menggunakan CPU copy (memcpy) atau memanfaatkan SPI DMA loopback.
 *
 *  File ini mendefinisikan ukuran buffer dan parameter benchmark.
 * ===========================================================================
 */

#ifndef CONFIG_H
#define CONFIG_H

/* ---------- Buffer Sizes (bytes) ---------- */
#define BUFFER_SIZE_0       256
#define BUFFER_SIZE_1       1024
#define BUFFER_SIZE_2       4096
#define BUFFER_SIZE_3       16384
#define BUFFER_SIZE_4       65536

#define NUM_BUFFER_SIZES    5

/* ---------- Benchmark Parameters ---------- */
#define NUM_ITERATIONS      100

/* ---------- SPI Loopback DMA Config ---------- */
#define SPI_DMA_HOST        SPI2_HOST
#define PIN_MOSI            23
#define PIN_MISO            19
#define PIN_SCLK            18
#define PIN_CS              5
#define SPI_CLOCK_HZ        (10 * 1000 * 1000)  /* 10 MHz */
#define SPI_MAX_TRANSFER    4096                 /* Max bytes per SPI DMA transfer */

#endif /* CONFIG_H */
