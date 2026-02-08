/*
 * ==========================================================================
 *  ESP32 DAC Waveform Output with DMA (I2S-DAC Mode) - Configuration
 * ==========================================================================
 *  Modul 08 - Program 10: DAC Waveform Generation
 *
 *  CATATAN:
 *  - ESP32 memiliki 2 channel DAC (8-bit): GPIO25 (CH1) dan GPIO26 (CH2)
 *  - DAC bisa diakses langsung (dac_output_voltage) atau via I2S DMA
 *  - I2S-DAC mode menggunakan DMA untuk output waveform secara kontinu
 *  - ESP-IDF 5.x menyediakan dac_continuous API
 * ==========================================================================
 */

#ifndef CONFIG_H
#define CONFIG_H

/* ---- DAC Configuration ---- */
#define DAC_CHAN                 DAC_CHAN_0      /* GPIO25 (DAC Channel 1) */
#define DAC_GPIO                25              /* GPIO25 */

/* ---- Waveform Configuration ---- */
#define WAVEFORM_FREQ_HZ        1000            /* Target waveform frequency */
#define SAMPLE_RATE_HZ          100000          /* 100 kHz sample rate */
#define WAVEFORM_BUF_SIZE       256             /* Points per waveform cycle */
#define DAC_RESOLUTION          256             /* 8-bit DAC: 0-255 */
#define DAC_VDD                 3.3f            /* DAC reference voltage */

/* ---- Waveform Types ---- */
#define WAVEFORM_SINE           0
#define WAVEFORM_SQUARE         1
#define WAVEFORM_TRIANGLE       2
#define WAVEFORM_SAWTOOTH       3
#define NUM_WAVEFORMS           4
#define WAVEFORM_SWITCH_SEC     5               /* Switch every 5 seconds */

/* ---- DMA / Continuous Output ---- */
#define DMA_DESC_NUM            4               /* Number of DMA descriptors */
#define DMA_BUF_SIZE            1024            /* DMA buffer size */

/* ---- Task Configuration ---- */
#define DAC_TASK_STACK          4096
#define DAC_TASK_PRIO           7
#define DISPLAY_TASK_STACK      4096
#define DISPLAY_TASK_PRIO       3

#endif /* CONFIG_H */
