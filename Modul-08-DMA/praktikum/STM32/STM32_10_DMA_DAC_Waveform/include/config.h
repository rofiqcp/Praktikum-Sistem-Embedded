/**
 * ============================================================================
 * config.h - Konfigurasi DMA DAC Waveform (via Timer PWM)
 * ============================================================================
 * Program   : STM32_10_DMA_DAC_Waveform
 * Deskripsi : Konfigurasi Timer PWM + DMA untuk generasi gelombang
 * MCU       : STM32F103C8 (Blue Pill) - tanpa DAC hardware
 * Catatan   : Menggunakan Timer2 CH1 + DMA untuk variasi duty cycle
 * ============================================================================
 */

#ifndef CONFIG_H
#define CONFIG_H

/* ==================== Konfigurasi LED ==================== */
#define LED_PORT                GPIOC
#define LED_PIN                 GPIO_PIN_13
#define LED_GPIO_CLK_ENABLE()   __HAL_RCC_GPIOC_CLK_ENABLE()

/* ==================== Konfigurasi USART1 ==================== */
#define USART_TX_PORT           GPIOA
#define USART_TX_PIN            GPIO_PIN_9
#define USART_RX_PORT           GPIOA
#define USART_RX_PIN            GPIO_PIN_10
#define USART_BAUDRATE          115200
#define USART_GPIO_CLK_ENABLE() __HAL_RCC_GPIOA_CLK_ENABLE()
#define USART_CLK_ENABLE()      __HAL_RCC_USART1_CLK_ENABLE()

/* ==================== Konfigurasi Timer2 PWM ==================== */
/* TIM2 CH1 → PA0 (output PWM) */
#define PWM_PORT                GPIOA
#define PWM_PIN                 GPIO_PIN_0
#define PWM_GPIO_CLK_ENABLE()   __HAL_RCC_GPIOA_CLK_ENABLE()
#define PWM_TIM_CLK_ENABLE()    __HAL_RCC_TIM2_CLK_ENABLE()

/* Parameter Timer PWM */
#define PWM_TIMER               TIM2
#define PWM_CHANNEL             TIM_CHANNEL_1
#define PWM_TIM_PRESCALER       (72 - 1)    /* 72MHz / 72 = 1MHz timer clock */
#define PWM_TIM_PERIOD          (1000 - 1)  /* 1MHz / 1000 = 1kHz PWM freq */
#define PWM_RESOLUTION          1000        /* Resolusi duty cycle (0-999) */

/* Frekuensi gelombang output */
/* Satu siklus gelombang = WAVEFORM_TABLE_SIZE sampel */
/* Frekuensi gelombang = PWM_freq / WAVEFORM_TABLE_SIZE */
/* 1kHz / 64 = ~15.6 Hz gelombang sinus */
#define PWM_FREQUENCY_HZ        1000

/* ==================== Konfigurasi DMA ==================== */
/* DMA1 Channel 2 = TIM2_UP (update event) -> push data ke CCR1 */
#define DMA_CLK_ENABLE()        __HAL_RCC_DMA1_CLK_ENABLE()
#define PWM_DMA_CHANNEL         DMA1_Channel2
#define PWM_DMA_IRQn            DMA1_Channel2_IRQn

/* ==================== Tabel Gelombang ==================== */
#define WAVEFORM_TABLE_SIZE     64      /* Jumlah sampel per siklus */
#define WAVEFORM_SINE           0       /* Tipe: gelombang sinus */
#define WAVEFORM_TRIANGLE       1       /* Tipe: gelombang segitiga */
#define WAVEFORM_SAWTOOTH       2       /* Tipe: gelombang gergaji */
#define WAVEFORM_SQUARE_SOFT    3       /* Tipe: kotak dengan transisi halus */

#define NUM_WAVEFORM_TYPES      4       /* Total tipe gelombang */
#define WAVEFORM_SWITCH_MS      5000    /* Ganti gelombang setiap 5 detik */

/* ==================== Parameter Demo ==================== */
#define DEMO_DELAY_MS           3000    /* Jeda antar demo */
#define PRINT_INTERVAL_MS       500     /* Interval cetak data */

#endif /* CONFIG_H */
