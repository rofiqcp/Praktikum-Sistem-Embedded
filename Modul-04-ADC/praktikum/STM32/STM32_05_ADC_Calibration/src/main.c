/**
 * ============================================================================
 * Program     : STM32_05_ADC_Calibration
 * Modul       : 04 - ADC (Analog to Digital Converter)
 * MCU         : STM32F103C8T6 (Blue Pill)
 * Framework   : STM32Cube HAL
 * Deskripsi   : Demonstrasi kalibrasi ADC pada STM32F1. Membandingkan
 *               pembacaan sebelum dan sesudah HAL_ADCEx_Calibration_Start().
 *               Kalibrasi penting untuk mendapatkan akurasi pembacaan
 *               yang lebih baik pada STM32F1.
 * Koneksi     : PA0 -> Potentiometer (wiper)
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
 * Inisialisasi ADC1 TANPA kalibrasi
 * Digunakan untuk membandingkan pembacaan sebelum kalibrasi
 */
void MX_ADC1_Init_NoCalibration(void) {
    __HAL_RCC_ADC1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_0;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    hadc1.Instance = ADC1;
    hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
    hadc1.Init.ContinuousConvMode = DISABLE;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc1.Init.NbrOfConversion = 1;
    HAL_ADC_Init(&hadc1);

    /* TIDAK ada kalibrasi di sini */
}

/**
 * Konfigurasi channel ADC
 */
void ADC_Select_Channel(uint32_t channel) {
    ADC_ChannelConfTypeDef sConfig = {0};
    sConfig.Channel = channel;
    sConfig.Rank = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLETIME_239CYCLES_5;
    HAL_ADC_ConfigChannel(&hadc1, &sConfig);
}

/**
 * Membaca nilai ADC
 */
uint32_t ADC_Read(void) {
    ADC_Select_Channel(ADC_CHANNEL_0);
    HAL_ADC_Start(&hadc1);
    HAL_ADC_PollForConversion(&hadc1, HAL_MAX_DELAY);
    uint32_t value = HAL_ADC_GetValue(&hadc1);
    HAL_ADC_Stop(&hadc1);
    return value;
}

/**
 * Membaca rata-rata dari N sampel
 */
uint32_t ADC_Read_Average(uint32_t num_samples) {
    uint32_t sum = 0;
    for (uint32_t i = 0; i < num_samples; i++) {
        sum += ADC_Read();
    }
    return sum / num_samples;
}

/**
 * Program Utama
 * Demonstrasi perbandingan ADC sebelum dan sesudah kalibrasi
 */
int main(void) {
    HAL_Init();
    SystemClock_Config();
    MX_USART1_UART_Init();

    printf("=== STM32 ADC Calibration Demo ===\r\n");
    printf("Perbandingan sebelum & sesudah kalibrasi\r\n\r\n");

    uint32_t pre_cal_raw, post_cal_raw;
    uint32_t pre_cal_avg, post_cal_avg;
    uint32_t pre_cal_mv, post_cal_mv;
    int32_t difference;
    uint32_t counter = 0;
    uint32_t num_samples = 32;

    while (1) {
        /* ===== FASE 1: Pembacaan TANPA kalibrasi ===== */
        /* De-init ADC lalu init ulang tanpa kalibrasi */
        HAL_ADC_DeInit(&hadc1);
        MX_ADC1_Init_NoCalibration();

        /* Baca rata-rata tanpa kalibrasi */
        pre_cal_avg = ADC_Read_Average(num_samples);
        pre_cal_mv = (pre_cal_avg * 3300) / 4095;

        /* ===== FASE 2: Pembacaan DENGAN kalibrasi ===== */
        /* Jalankan kalibrasi */
        HAL_ADCEx_Calibration_Start(&hadc1);

        /* Baca rata-rata setelah kalibrasi */
        post_cal_avg = ADC_Read_Average(num_samples);
        post_cal_mv = (post_cal_avg * 3300) / 4095;

        /* Hitung selisih */
        difference = (int32_t)post_cal_avg - (int32_t)pre_cal_avg;

        /* Tampilkan perbandingan */
        printf("[%lu] PRE_CAL:%lu PRE_MV:%lu POST_CAL:%lu POST_MV:%lu DIFF:%ld\r\n",
               counter, pre_cal_avg, pre_cal_mv, post_cal_avg, post_cal_mv, difference);

        counter++;
        HAL_Delay(1000);
    }
}
