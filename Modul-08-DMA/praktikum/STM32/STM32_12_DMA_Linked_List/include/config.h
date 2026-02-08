/**
 * ============================================================================
 * config.h - Konfigurasi DMA Linked List (Software Scatter-Gather)
 * ============================================================================
 * Program   : STM32_12_DMA_Linked_List
 * Deskripsi : Konfigurasi linked-list DMA transfer secara software
 * MCU       : STM32F103C8 (Blue Pill)
 * Catatan   : STM32F103 tidak punya hardware scatter-gather DMA,
 *             teknik ini menggunakan interrupt Transfer Complete
 *             untuk memuat deskriptor berikutnya secara manual.
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
#define LL_DMA_CHANNEL          DMA1_Channel1
#define LL_DMA_IRQn             DMA1_Channel1_IRQn

/* ==================== Deskriptor Linked List ==================== */
#define MAX_DESCRIPTORS         5       /* Jumlah maksimum deskriptor */
#define NUM_DEMO_DESCRIPTORS    4       /* Deskriptor untuk demo utama */

/* Ukuran buffer per deskriptor */
#define DESC_BUF_SIZE_0         32      /* Buffer deskriptor 0 */
#define DESC_BUF_SIZE_1         48      /* Buffer deskriptor 1 */
#define DESC_BUF_SIZE_2         24      /* Buffer deskriptor 2 */
#define DESC_BUF_SIZE_3         64      /* Buffer deskriptor 3 */
#define DESC_BUF_SIZE_4         16      /* Buffer deskriptor 4 (cadangan) */

/* Total ukuran destination buffer (harus >= sum semua source) */
#define DEST_BUFFER_SIZE        256

/* ==================== Parameter Pengujian ==================== */
#define TEST_ITERATIONS         5       /* Iterasi pengujian */
#define DEMO_DELAY_MS           2000    /* Jeda antar demo */
#define TRANSFER_TIMEOUT_MS     1000    /* Timeout per transfer */

/* ==================== DWT Cycle Counter ==================== */
#define DWT_CONTROL             (*((volatile uint32_t *)0xE0001000))
#define DWT_CYCCNT              (*((volatile uint32_t *)0xE0001004))
#define DEMCR                   (*((volatile uint32_t *)0xE000EDFC))
#define DEMCR_TRCENA            (1 << 24)
#define DWT_CTRL_CYCCNTENA      (1 << 0)

/* Frekuensi CPU */
#define CPU_FREQ_HZ             72000000

/* ==================== Mode Verifikasi ==================== */
#define VERIFY_TRANSFERS        1       /* 1=verifikasi data, 0=skip */
#define FILL_PATTERN_BASE       0xA0    /* Pola pengisi awal */

#endif /* CONFIG_H */
