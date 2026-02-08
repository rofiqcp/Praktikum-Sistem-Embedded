/*
 * ==========================================================================
 *  ESP32 DMA Double Buffer (Ping-Pong) - Configuration
 * ==========================================================================
 *  Modul 08 - Program 08: Double Buffer Pattern
 *
 *  CATATAN tentang Double Buffer:
 *  - STM32 F4 DMA memiliki hardware double-buffer mode
 *  - ESP32 TIDAK memiliki hardware double-buffer DMA
 *  - Kita implementasikan software double-buffer (ping-pong) pattern
 *  - Ini adalah teknik umum: satu buffer diisi sementara yang lain diproses
 * ==========================================================================
 */

#ifndef CONFIG_H
#define CONFIG_H

/* ---- Buffer Configuration ---- */
#define BUFFER_A_SIZE           1024    /* Buffer A size (samples) */
#define BUFFER_B_SIZE           1024    /* Buffer B size (samples) */
#define NUM_SAMPLES             512     /* Samples per collection cycle */

/* ---- Processing Configuration ---- */
#define PROCESSING_DELAY_MS     20      /* Simulated processing delay */

/* ---- Task Configuration ---- */
#define COLLECTOR_TASK_STACK    4096
#define PROCESSOR_TASK_STACK    8192    /* Larger stack for FFT-like calc */
#define COLLECTOR_TASK_PRIO     6
#define PROCESSOR_TASK_PRIO     5
#define STATS_TASK_PRIO         3

/* ---- Timing Analysis ---- */
#define TIMING_HISTORY_SIZE     50      /* Keep last N timing measurements */
#define STATS_INTERVAL_MS       3000    /* Print stats every 3 seconds */

/* ---- ADC Configuration ---- */
#define ADC_CHANNEL             ADC_CHANNEL_0   /* GPIO36 */
#define ADC_ATTEN               ADC_ATTEN_DB_12
#define ADC_WIDTH               ADC_BITWIDTH_12

/* ---- Single Buffer Mode (for comparison) ---- */
#define SINGLE_BUFFER_TEST_CYCLES   20
#define DOUBLE_BUFFER_TEST_CYCLES   20

#endif /* CONFIG_H */
