/**
 * ===========================================================================
 *  ESP32 ADC Continuous (DMA) Mode — Configuration
 * ===========================================================================
 *
 *  CATATAN ARSITEKTUR:
 *  ESP32 ADC DMA terintegrasi langsung di peripheral ADC.
 *  Tidak perlu setup DMA channel manual seperti STM32.
 *  ESP-IDF menyediakan API `adc_continuous` yang mengelola DMA internal.
 *
 *  API compatibility:
 *  - ESP-IDF >= 5.0: adc_continuous_new_handle() (baru)
 *  - ESP-IDF < 5.0 : adc_digi_initialize() (lama)
 *
 *  File ini mendefinisikan parameter ADC dan sampling.
 * ===========================================================================
 */

#ifndef CONFIG_H
#define CONFIG_H

/* ---------- ADC Channel Configuration ---------- */
/*
 * ADC1_CHANNEL_0 = GPIO36 (VP) pada ESP32
 * Pastikan pin terhubung ke sinyal analog atau potentiometer.
 */
#define ADC_CHANNEL         ADC_CHANNEL_0   /* GPIO36 */
#define ADC_UNIT            ADC_UNIT_1
#define ADC_ATTEN           ADC_ATTEN_DB_12 /* 0-3.3V range (ESP-IDF 5.x) */
#define ADC_BITWIDTH        ADC_BITWIDTH_12 /* 12-bit resolution */

/* ---------- Sampling Configuration ---------- */
#define SAMPLE_RATE_HZ      20000           /* 20 kHz sample rate */
#define BUFFER_SIZE_SAMPLES 1024            /* Samples per DMA buffer */
#define READ_LEN_BYTES      (BUFFER_SIZE_SAMPLES * SOC_ADC_DIGI_RESULT_BYTES)

/* ---------- Processing ---------- */
#define STATS_INTERVAL_MS   1000            /* Print stats every 1 second */
#define ZERO_CROSSING_THRESHOLD 2048        /* Mid-range for 12-bit ADC */

/* ---------- Task Configuration ---------- */
#define ADC_TASK_STACK_SIZE 4096
#define ADC_TASK_PRIORITY   10

#endif /* CONFIG_H */
