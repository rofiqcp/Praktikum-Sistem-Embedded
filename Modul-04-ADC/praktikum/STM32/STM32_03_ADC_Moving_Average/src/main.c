/**
 * ============================================================================
 * Program     : STM32_03_ADC_Moving_Average
 * Modul       : 04 - ADC (Analog to Digital Converter)
 * MCU         : STM32F103C8T6 (Blue Pill)
 * Framework   : STM32Cube HAL
 * Deskripsi   : Membaca ADC dari PA0 dengan filter Moving Average.
 *               Membandingkan hasil raw vs filtered dengan N=16 dan N=32.
 *               Filter ini mengurangi noise pembacaan sensor analog.
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

/* Buffer untuk Moving Average */
#define MA_SIZE_16  16
#define MA_SIZE_32  32

uint32_t ma_buffer_16[MA_SIZE_16];   /* Buffer moving average N=16 */
uint32_t ma_buffer_32[MA_SIZE_32];   /* Buffer moving average N=32 */
uint32_t ma_index_16 = 0;            /* Indeks buffer N=16 */
uint32_t ma_index_32 = 0;            /* Indeks buffer N=32 */
uint32_t ma_sum_16 = 0;              /* Jumlah akumulasi N=16 */
uint32_t ma_sum_32 = 0;              /* Jumlah akumulasi N=32 */
uint8_t ma_filled_16 = 0;            /* Flag buffer penuh N=16 */
uint8_t ma_filled_32 = 0;            /* Flag buffer penuh N=32 */

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
 * Inisialisasi ADC1 Channel 0 (PA0)
 */
void MX_ADC1_Init(void) {
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

    HAL_ADCEx_Calibration_Start(&hadc1);
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
 * Filter Moving Average dengan ukuran N=16
 * Menghitung rata-rata dari 16 sampel terakhir
 */
uint32_t Moving_Average_16(uint32_t new_value) {
    /* Kurangi nilai lama, tambah nilai baru */
    ma_sum_16 -= ma_buffer_16[ma_index_16];
    ma_buffer_16[ma_index_16] = new_value;
    ma_sum_16 += new_value;

    ma_index_16++;
    if (ma_index_16 >= MA_SIZE_16) {
        ma_index_16 = 0;
        ma_filled_16 = 1;
    }

    /* Hitung rata-rata */
    if (ma_filled_16) {
        return ma_sum_16 / MA_SIZE_16;
    } else {
        return ma_sum_16 / ma_index_16;
    }
}

/**
 * Filter Moving Average dengan ukuran N=32
 * Menghitung rata-rata dari 32 sampel terakhir
 */
uint32_t Moving_Average_32(uint32_t new_value) {
    ma_sum_32 -= ma_buffer_32[ma_index_32];
    ma_buffer_32[ma_index_32] = new_value;
    ma_sum_32 += new_value;

    ma_index_32++;
    if (ma_index_32 >= MA_SIZE_32) {
        ma_index_32 = 0;
        ma_filled_32 = 1;
    }

    if (ma_filled_32) {
        return ma_sum_32 / MA_SIZE_32;
    } else {
        return ma_sum_32 / ma_index_32;
    }
}

/**
 * Inisialisasi buffer Moving Average ke nol
 */
void Moving_Average_Init(void) {
    for (int i = 0; i < MA_SIZE_16; i++) ma_buffer_16[i] = 0;
    for (int i = 0; i < MA_SIZE_32; i++) ma_buffer_32[i] = 0;
    ma_index_16 = 0; ma_index_32 = 0;
    ma_sum_16 = 0; ma_sum_32 = 0;
    ma_filled_16 = 0; ma_filled_32 = 0;
}

/**
 * Program Utama
 */
int main(void) {
    HAL_Init();
    SystemClock_Config();
    MX_USART1_UART_Init();
    MX_ADC1_Init();
    Moving_Average_Init();

    printf("=== STM32 ADC Moving Average Filter ===\r\n");
    printf("Perbandingan Raw vs MA-16 vs MA-32\r\n\r\n");

    uint32_t adc_raw = 0;
    uint32_t avg_16 = 0;
    uint32_t avg_32 = 0;
    uint32_t counter = 0;

    while (1) {
        /* Baca nilai ADC raw */
        adc_raw = ADC_Read();

        /* Hitung moving average */
        avg_16 = Moving_Average_16(adc_raw);
        avg_32 = Moving_Average_32(adc_raw);

        /* Tampilkan perbandingan */
        printf("[%lu] RAW:%lu MA16:%lu MA32:%lu\r\n",
               counter, adc_raw, avg_16, avg_32);

        counter++;
        HAL_Delay(100);  /* Sampling setiap 100ms */
    }
}
