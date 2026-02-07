/**
 * ============================================================================
 * Program     : STM32_06_ADC_Continuous_DMA
 * Modul       : 04 - ADC (Analog to Digital Converter)
 * MCU         : STM32F103C8T6 (Blue Pill)
 * Framework   : STM32Cube HAL
 * Deskripsi   : ADC dalam mode continuous dengan DMA transfer.
 *               DMA secara otomatis memindahkan data ADC ke buffer memori
 *               tanpa intervensi CPU. Menggunakan mode circular sehingga
 *               buffer terisi secara berulang.
 * Koneksi     : PA0 -> Potentiometer / Sensor Analog
 *               PA9 -> UART TX, PA10 -> UART RX
 * ============================================================================
 */

#include "stm32f1xx_hal.h"
#include <stdio.h>
#include <string.h>

/* Handle peripheral */
UART_HandleTypeDef huart1;
ADC_HandleTypeDef hadc1;
DMA_HandleTypeDef hdma_adc1;

/* Buffer DMA untuk menyimpan hasil konversi ADC */
#define DMA_BUFFER_SIZE 64
volatile uint16_t adc_dma_buffer[DMA_BUFFER_SIZE];

/* Flag untuk menandai DMA selesai */
volatile uint8_t dma_half_complete = 0;
volatile uint8_t dma_full_complete = 0;

/* Redirect printf ke UART */
int _write(int file, char *ptr, int len) {
    HAL_UART_Transmit(&huart1, (uint8_t*)ptr, len, HAL_MAX_DELAY);
    return len;
}

/**
 * Konfigurasi System Clock
 */
void SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
    HAL_RCC_OscConfig(&RCC_OscInitStruct);

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                   RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2);
}

/**
 * Inisialisasi USART1
 */
void MX_USART1_UART_Init(void) {
    __HAL_RCC_USART1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    huart1.Instance = USART1;
    huart1.Init.BaudRate = 115200;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    HAL_UART_Init(&huart1);
}

/**
 * Inisialisasi DMA1 Channel 1 untuk ADC1
 * Mode: Circular, Periph -> Memory, Half-word
 */
void MX_DMA_Init(void) {
    __HAL_RCC_DMA1_CLK_ENABLE();

    hdma_adc1.Instance = DMA1_Channel1;
    hdma_adc1.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma_adc1.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_adc1.Init.MemInc = DMA_MINC_ENABLE;
    hdma_adc1.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    hdma_adc1.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
    hdma_adc1.Init.Mode = DMA_CIRCULAR;
    hdma_adc1.Init.Priority = DMA_PRIORITY_HIGH;
    HAL_DMA_Init(&hdma_adc1);

    /* Hubungkan DMA ke ADC1 */
    __HAL_LINKDMA(&hadc1, DMA_Handle, hdma_adc1);

    /* Aktifkan interrupt DMA */
    HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);
}

/**
 * Inisialisasi ADC1 dalam mode Continuous
 */
void MX_ADC1_Init(void) {
    __HAL_RCC_ADC1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_0;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* ADC dalam mode continuous untuk DMA */
    hadc1.Instance = ADC1;
    hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
    hadc1.Init.ContinuousConvMode = ENABLE;  /* Mode continuous untuk DMA */
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc1.Init.NbrOfConversion = 1;
    HAL_ADC_Init(&hadc1);

    /* Konfigurasi channel */
    ADC_ChannelConfTypeDef sConfig = {0};
    sConfig.Channel = ADC_CHANNEL_0;
    sConfig.Rank = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLETIME_239CYCLES_5;
    HAL_ADC_ConfigChannel(&hadc1, &sConfig);

    /* Kalibrasi ADC */
    HAL_ADCEx_Calibration_Start(&hadc1);
}

/**
 * Callback ketika DMA setengah selesai (half-transfer)
 */
void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef* hadc) {
    if (hadc->Instance == ADC1) {
        dma_half_complete = 1;
    }
}

/**
 * Callback ketika DMA selesai penuh (transfer complete)
 */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc) {
    if (hadc->Instance == ADC1) {
        dma_full_complete = 1;
    }
}

/**
 * Handler interrupt DMA1 Channel 1
 */
void DMA1_Channel1_IRQHandler(void) {
    HAL_DMA_IRQHandler(&hdma_adc1);
}

/**
 * Menghitung rata-rata dari buffer DMA
 */
uint32_t Calculate_Buffer_Average(volatile uint16_t *buffer, uint32_t start, uint32_t length) {
    uint32_t sum = 0;
    for (uint32_t i = start; i < start + length; i++) {
        sum += buffer[i];
    }
    return sum / length;
}

/**
 * Mencari nilai minimum dari buffer
 */
uint16_t Find_Buffer_Min(volatile uint16_t *buffer, uint32_t start, uint32_t length) {
    uint16_t min_val = buffer[start];
    for (uint32_t i = start + 1; i < start + length; i++) {
        if (buffer[i] < min_val) min_val = buffer[i];
    }
    return min_val;
}

/**
 * Mencari nilai maksimum dari buffer
 */
uint16_t Find_Buffer_Max(volatile uint16_t *buffer, uint32_t start, uint32_t length) {
    uint16_t max_val = buffer[start];
    for (uint32_t i = start + 1; i < start + length; i++) {
        if (buffer[i] > max_val) max_val = buffer[i];
    }
    return max_val;
}

/**
 * Program Utama
 */
int main(void) {
    HAL_Init();
    SystemClock_Config();
    MX_USART1_UART_Init();
    MX_DMA_Init();       /* DMA harus diinisialisasi SEBELUM ADC */
    MX_ADC1_Init();

    printf("=== STM32 ADC Continuous DMA ===\r\n");
    printf("Buffer DMA: %d sampel, Mode Circular\r\n\r\n", DMA_BUFFER_SIZE);

    /* Mulai ADC dengan DMA */
    HAL_ADC_Start_DMA(&hadc1, (uint32_t*)adc_dma_buffer, DMA_BUFFER_SIZE);

    uint32_t counter = 0;
    uint32_t avg_val, min_val, max_val, voltage_mv;

    while (1) {
        /* Proses ketika DMA selesai penuh */
        if (dma_full_complete) {
            dma_full_complete = 0;

            /* Hitung statistik dari seluruh buffer */
            avg_val = Calculate_Buffer_Average(adc_dma_buffer, 0, DMA_BUFFER_SIZE);
            min_val = Find_Buffer_Min(adc_dma_buffer, 0, DMA_BUFFER_SIZE);
            max_val = Find_Buffer_Max(adc_dma_buffer, 0, DMA_BUFFER_SIZE);
            voltage_mv = (avg_val * 3300) / 4095;

            printf("[%lu] DMA_AVG:%lu DMA_MIN:%lu DMA_MAX:%lu VOLTAGE_MV:%lu\r\n",
                   counter, avg_val, min_val, max_val, voltage_mv);

            counter++;
        }

        /* Proses ketika DMA setengah selesai */
        if (dma_half_complete) {
            dma_half_complete = 0;
            /* Bisa digunakan untuk double-buffering */
        }

        HAL_Delay(500);
    }
}
