/*
 * ==========================================================================
 *  ESP32 DMA Circular Buffer - Configuration
 * ==========================================================================
 *  Modul 08 - Program 07: DMA Circular Buffer Pattern
 *
 *  CATATAN PENTING tentang DMA di ESP32:
 *  - ESP32 TIDAK memiliki DMA controller general-purpose seperti STM32
 *  - DMA terintegrasi di dalam peripheral (SPI, I2S, ADC, dll)
 *  - ADC continuous mode menggunakan DMA internal via I2S/DIG controller
 *  - Ring buffer diimplementasikan secara software di atas DMA buffer
 * ==========================================================================
 */

#ifndef CONFIG_H
#define CONFIG_H

/* ---- Ring Buffer Configuration ---- */
#define RING_BUFFER_SIZE        2048    /* Total ring buffer size (samples) */
#define CHUNK_SIZE              256     /* Size of each read/write chunk */

/* ---- ADC Configuration ---- */
#define ADC_CHANNEL             ADC_CHANNEL_0   /* GPIO36 (VP) */
#define ADC_UNIT                ADC_UNIT_1
#define ADC_ATTEN               ADC_ATTEN_DB_12  /* 0-3.3V range */
#define ADC_BIT_WIDTH           SOC_ADC_DIGI_MAX_BITWIDTH

/* ---- Sampling Configuration ---- */
#define SAMPLE_RATE_HZ          10000   /* 10 kHz sample rate */
#define DMA_CONV_FRAME_SIZE     256     /* ADC DMA conversion frame size */
#define DMA_POOL_SIZE           (DMA_CONV_FRAME_SIZE * 2)

/* ---- Task Configuration ---- */
#define PRODUCER_TASK_STACK     4096
#define CONSUMER_TASK_STACK     4096
#define PRODUCER_TASK_PRIORITY  6       /* Higher priority for producer */
#define CONSUMER_TASK_PRIORITY  5       /* Lower priority for consumer */
#define PRODUCER_PERIOD_MS      10      /* Producer reads every 10ms */
#define CONSUMER_PERIOD_MS      50      /* Consumer processes every 50ms (slower) */

/* ---- Moving Average Filter ---- */
#define MOVING_AVG_WINDOW       16      /* Window size for moving average */

/* ---- Statistics ---- */
#define STATS_PRINT_INTERVAL_MS 2000    /* Print stats every 2 seconds */
#define VISUAL_BAR_WIDTH        40      /* Width of visual bar in chars */

#endif /* CONFIG_H */
