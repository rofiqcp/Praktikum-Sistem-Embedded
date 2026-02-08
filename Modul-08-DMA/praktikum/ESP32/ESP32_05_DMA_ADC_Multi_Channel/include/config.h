/**
 * ===========================================================================
 *  ESP32 Multi-Channel ADC Scan with DMA — Configuration
 * ===========================================================================
 *
 *  CATATAN ARSITEKTUR:
 *  ESP32 ADC continuous mode mendukung multi-channel scanning via DMA.
 *  Semua channel dikonfigurasi dalam pattern table, dan DMA secara
 *  otomatis mengumpulkan sample dari setiap channel secara bergantian
 *  (interleaved).
 *
 *  Channel mapping (ESP32):
 *    ADC1_CHANNEL_0 = GPIO36 (VP)
 *    ADC1_CHANNEL_3 = GPIO39 (VN)
 *    ADC1_CHANNEL_6 = GPIO34
 *
 *  File ini mendefinisikan parameter multi-channel ADC.
 * ===========================================================================
 */

#ifndef CONFIG_H
#define CONFIG_H

/* ---------- ADC Channel Configuration ---------- */
#define NUM_ADC_CHANNELS    3

/* Channel 0: GPIO36 (VP) */
#define ADC_CH0             ADC_CHANNEL_0
#define ADC_CH0_GPIO        36

/* Channel 1: GPIO39 (VN) */
#define ADC_CH1             ADC_CHANNEL_3
#define ADC_CH1_GPIO        39

/* Channel 2: GPIO34 */
#define ADC_CH2             ADC_CHANNEL_6
#define ADC_CH2_GPIO        34

/* ---------- ADC Parameters ---------- */
#define ADC_UNIT            ADC_UNIT_1
#define ADC_ATTEN           ADC_ATTEN_DB_12
#define ADC_BITWIDTH        ADC_BITWIDTH_12

/* ---------- Sampling Configuration ---------- */
#define SAMPLE_RATE_HZ      (10000 * NUM_ADC_CHANNELS) /* 10kHz per channel */
#define BUFFER_SIZE_SAMPLES 1024
#define READ_LEN_BYTES      (BUFFER_SIZE_SAMPLES * SOC_ADC_DIGI_RESULT_BYTES)

/* ---------- Processing ---------- */
#define STATS_INTERVAL_MS   2000
#define MAX_SAMPLES_PER_CH  4096    /* Max samples to store per channel */

/* ---------- Task Configuration ---------- */
#define ADC_TASK_STACK_SIZE 8192
#define ADC_TASK_PRIORITY   10

#endif /* CONFIG_H */
