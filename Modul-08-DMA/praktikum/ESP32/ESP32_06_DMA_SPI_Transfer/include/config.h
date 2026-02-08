/**
 * ===========================================================================
 *  ESP32 SPI Transfer with DMA — Configuration
 * ===========================================================================
 *
 *  CATATAN ARSITEKTUR:
 *  ESP32 SPI peripheral memiliki DMA terintegrasi. Untuk transfer kecil
 *  (<32 bytes), SPI menggunakan CPU copy langsung. Untuk transfer besar,
 *  DMA digunakan otomatis oleh driver ESP-IDF.
 *
 *  SPI_DMA_CH_AUTO membiarkan driver memilih DMA channel yang tersedia.
 *  Buffer harus DMA-capable: gunakan heap_caps_malloc(size, MALLOC_CAP_DMA).
 *
 *  File ini mendefinisikan parameter SPI dan ukuran transfer untuk benchmark.
 * ===========================================================================
 */

#ifndef CONFIG_H
#define CONFIG_H

/* ---------- SPI Configuration ---------- */
#define SPI_HOST_ID         SPI2_HOST
#define PIN_MOSI            23
#define PIN_MISO            19
#define PIN_SCLK            18
#define PIN_CS              5
#define SPI_CLOCK_HZ        (10 * 1000 * 1000)  /* 10 MHz */
#define SPI_DMA_CHAN        SPI_DMA_CH_AUTO

/* ---------- Transfer Sizes to Benchmark ---------- */
#define NUM_TRANSFER_SIZES  4
#define TRANSFER_SIZE_0     64      /* Small — may not use DMA */
#define TRANSFER_SIZE_1     256     /* Medium — DMA starts helping */
#define TRANSFER_SIZE_2     1024    /* Large — DMA advantage clear */
#define TRANSFER_SIZE_3     4096    /* Very large — DMA essential */

/* Max transfer size for SPI DMA */
#define MAX_TRANSFER_SIZE   4096

/* ---------- Benchmark Parameters ---------- */
#define NUM_ITERATIONS      200

/* ---------- CPU Copy Threshold ---------- */
/*
 * SPI DMA di ESP32: transfer < 32 bytes menggunakan CPU copy.
 * Di atas 32 bytes, DMA digunakan. Benchmark menunjukkan crossover point.
 */
#define CPU_COPY_THRESHOLD  32

/* ---------- Extra test sizes for crossover analysis ---------- */
#define NUM_CROSSOVER_SIZES 8

#endif /* CONFIG_H */
