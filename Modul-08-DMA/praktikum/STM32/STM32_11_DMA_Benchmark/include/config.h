/**
 * ============================================================================
 * config.h - Konfigurasi DMA Benchmark
 * ============================================================================
 * Program   : STM32_11_DMA_Benchmark
 * Deskripsi : Konfigurasi benchmark perbandingan DMA vs CPU
 * MCU       : STM32F103C8 (Blue Pill) @ 72MHz
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

/* ==================== Konfigurasi DMA ==================== */
/* DMA1 Channel 1 untuk Memory-to-Memory */
#define DMA_CLK_ENABLE()        __HAL_RCC_DMA1_CLK_ENABLE()
#define BENCH_DMA_CHANNEL       DMA1_Channel1
#define BENCH_DMA_IRQn          DMA1_Channel1_IRQn

/* ==================== Ukuran Buffer Benchmark ==================== */
#define BENCH_SIZE_32           32
#define BENCH_SIZE_64           64
#define BENCH_SIZE_128          128
#define BENCH_SIZE_256          256
#define BENCH_SIZE_512          512
#define BENCH_SIZE_1024         1024
#define NUM_BENCH_SIZES         6

/* Buffer maksimum (harus >= ukuran terbesar) */
#define MAX_BUFFER_SIZE         1024

/* ==================== Parameter Pengujian ==================== */
#define BENCH_ITERATIONS        100     /* Iterasi per tes untuk rata-rata */
#define WARMUP_ITERATIONS       5       /* Iterasi pemanasan sebelum ukur */

/* ==================== DWT Cycle Counter ==================== */
/* Register DWT untuk pengukuran siklus presisi tinggi */
#define DWT_CONTROL             (*((volatile uint32_t *)0xE0001000))
#define DWT_CYCCNT              (*((volatile uint32_t *)0xE0001004))
#define DEMCR                   (*((volatile uint32_t *)0xE000EDFC))
#define DEMCR_TRCENA            (1 << 24)
#define DWT_CTRL_CYCCNTENA      (1 << 0)

/* Frekuensi CPU untuk kalkulasi throughput */
#define CPU_FREQ_HZ             72000000
#define CPU_FREQ_MHZ            72

/* ==================== Mode Alignment ==================== */
#define ALIGN_BYTE              0       /* Transfer per byte */
#define ALIGN_HALFWORD          1       /* Transfer per halfword (16-bit) */
#define ALIGN_WORD              2       /* Transfer per word (32-bit) */
#define NUM_ALIGN_MODES         3

/* ==================== Display ==================== */
#define DEMO_DELAY_MS           1000    /* Jeda antar skenario */
#define SECTION_DELAY_MS        2000    /* Jeda antar bagian utama */

#endif /* CONFIG_H */
