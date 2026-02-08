/**
 * ============================================================================
 * config.h - Konfigurasi DMA I2C Transfer
 * ============================================================================
 * Program   : STM32_09_DMA_I2C_Transfer
 * Deskripsi : Konfigurasi pin dan parameter I2C DMA
 * MCU       : STM32F103C8 (Blue Pill)
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

/* ==================== Konfigurasi I2C1 ==================== */
#define I2C_SCL_PORT            GPIOB
#define I2C_SCL_PIN             GPIO_PIN_6
#define I2C_SDA_PORT            GPIOB
#define I2C_SDA_PIN             GPIO_PIN_7
#define I2C_GPIO_CLK_ENABLE()   __HAL_RCC_GPIOB_CLK_ENABLE()
#define I2C_CLK_ENABLE()        __HAL_RCC_I2C1_CLK_ENABLE()

/* Alamat slave I2C (mode demo - simulasi) */
#define I2C_SLAVE_ADDRESS       0x68    /* Alamat umum (misal MPU6050) */
#define I2C_OWN_ADDRESS         0x30    /* Alamat sendiri untuk loopback */
#define I2C_CLOCK_SPEED         100000  /* 100 kHz standard mode */

/* ==================== Konfigurasi DMA ==================== */
/* DMA1 Channel 6 = I2C1_TX */
/* DMA1 Channel 7 = I2C1_RX */
#define DMA_CLK_ENABLE()        __HAL_RCC_DMA1_CLK_ENABLE()
#define I2C_TX_DMA_CHANNEL      DMA1_Channel6
#define I2C_RX_DMA_CHANNEL      DMA1_Channel7
#define I2C_TX_DMA_IRQn         DMA1_Channel6_IRQn
#define I2C_RX_DMA_IRQn         DMA1_Channel7_IRQn

/* ==================== Konfigurasi Buffer ==================== */
#define I2C_BUFFER_SIZE         64      /* Ukuran buffer transfer */
#define I2C_SMALL_BUFFER        16      /* Buffer kecil untuk tes */
#define I2C_MEDIUM_BUFFER       32      /* Buffer sedang */
#define I2C_LARGE_BUFFER        64      /* Buffer besar */

/* ==================== Parameter Pengujian ==================== */
#define TEST_ITERATIONS         10      /* Jumlah iterasi per tes */
#define DEMO_DELAY_MS           2000    /* Jeda antar demo (ms) */
#define POLLING_TIMEOUT_MS      1000    /* Timeout I2C polling */

/* ==================== Mode Operasi ==================== */
#define MODE_DEMO               1       /* 1=demo tanpa perangkat, 0=perangkat nyata */

#endif /* CONFIG_H */
