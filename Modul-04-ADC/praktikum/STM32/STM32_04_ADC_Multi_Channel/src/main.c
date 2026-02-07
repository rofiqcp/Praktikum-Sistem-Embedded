/**
 * ============================================================================
 * Program     : STM32_04_ADC_Multi_Channel
 * Modul       : 04 - ADC (Analog to Digital Converter)
 * MCU         : STM32F103C8T6 (Blue Pill)
 * Framework   : STM32Cube HAL
 * Deskripsi   : Membaca dua channel ADC secara bergantian (PA0 dan PA1)
 *               dengan teknik channel switching. PA0 = Channel 0,
 *               PA1 = Channel 1. Masing-masing bisa dihubungkan ke
 *               potensiometer atau sensor analog berbeda.
 * Koneksi     : PA0 -> Potentiometer 1 / Sensor 1
 *               PA1 -> Potentiometer 2 / Sensor 2
 *               PA9 -> UART TX, PA10 -> UART RX
 * ============================================================================
 */

#include "stm32f1xx_hal.h"
#include <stdio.h>
#include <string.h>

/* Handle peripheral */
UART_HandleTypeDef huart1;
ADC_HandleTypeDef hadc1;

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
 * Inisialisasi ADC1 dengan dua channel (PA0 dan PA1)
 */
void MX_ADC1_Init(void) {
    __HAL_RCC_ADC1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    /* Konfigurasi PA0 dan PA1 sebagai input analog */
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* Konfigurasi ADC1 - mode single, software trigger */
    hadc1.Instance = ADC1;
    hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
    hadc1.Init.ContinuousConvMode = DISABLE;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc1.Init.NbrOfConversion = 1;
    HAL_ADC_Init(&hadc1);

    /* Kalibrasi ADC */
    HAL_ADCEx_Calibration_Start(&hadc1);
}

/**
 * Pilih channel ADC yang akan dibaca
 * Teknik channel switching: ubah konfigurasi channel sebelum konversi
 */
void ADC_Select_Channel(uint32_t channel) {
    ADC_ChannelConfTypeDef sConfig = {0};
    sConfig.Channel = channel;
    sConfig.Rank = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLETIME_239CYCLES_5;
    HAL_ADC_ConfigChannel(&hadc1, &sConfig);
}

/**
 * Membaca ADC dari channel tertentu
 */
uint32_t ADC_Read_Channel(uint32_t channel) {
    ADC_Select_Channel(channel);
    HAL_ADC_Start(&hadc1);
    HAL_ADC_PollForConversion(&hadc1, HAL_MAX_DELAY);
    uint32_t value = HAL_ADC_GetValue(&hadc1);
    HAL_ADC_Stop(&hadc1);
    return value;
}

/**
 * Konversi ADC ke milivolt
 */
uint32_t ADC_To_Millivolt(uint32_t raw) {
    return (raw * 3300) / 4095;
}

/**
 * Program Utama
 */
int main(void) {
    HAL_Init();
    SystemClock_Config();
    MX_USART1_UART_Init();
    MX_ADC1_Init();

    printf("=== STM32 ADC Multi Channel ===\r\n");
    printf("Channel 0: PA0, Channel 1: PA1\r\n\r\n");

    uint32_t ch0_raw, ch1_raw;
    uint32_t ch0_mv, ch1_mv;
    uint32_t counter = 0;

    while (1) {
        /* Baca Channel 0 (PA0) */
        ch0_raw = ADC_Read_Channel(ADC_CHANNEL_0);
        ch0_mv = ADC_To_Millivolt(ch0_raw);

        /* Baca Channel 1 (PA1) */
        ch1_raw = ADC_Read_Channel(ADC_CHANNEL_1);
        ch1_mv = ADC_To_Millivolt(ch1_raw);

        /* Tampilkan hasil kedua channel */
        printf("[%lu] CH0_RAW:%lu CH0_MV:%lu CH1_RAW:%lu CH1_MV:%lu\r\n",
               counter, ch0_raw, ch0_mv, ch1_raw, ch1_mv);

        counter++;
        HAL_Delay(500);
    }
}
