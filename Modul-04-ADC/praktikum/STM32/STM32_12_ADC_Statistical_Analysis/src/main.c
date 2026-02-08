/**
 * ============================================================================
 * Program     : STM32_12_ADC_Statistical_Analysis
 * Modul       : 04 - ADC (Analog to Digital Converter)
 * MCU         : STM32F103C8T6 (Blue Pill)
 * Framework   : STM32Cube HAL
 * Deskripsi   : Mengambil 100 sampel ADC, kemudian menghitung statistik:
 *               - Nilai minimum dan maksimum
 *               - Rata-rata (mean)
 *               - Standar deviasi (stddev)
 *               - Histogram distribusi (8 bin)
 *               Berguna untuk analisis kualitas sinyal dan noise ADC.
 * Koneksi     : PA0 -> Potentiometer / Sensor Analog
 *               PA9 -> UART TX, PA10 -> UART RX
 * ============================================================================
 */

#include "stm32f1xx_hal.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

/* Handle peripheral */
UART_HandleTypeDef huart1;
ADC_HandleTypeDef hadc1;

/* Konfigurasi analisis statistik */
#define NUM_SAMPLES     100     /* Jumlah sampel per analisis */
#define NUM_BINS        8       /* Jumlah bin histogram */

/* Buffer penyimpanan sampel */
uint32_t samples[NUM_SAMPLES];

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
 * Membaca satu sampel ADC
 */
uint32_t ADC_Read_Single(void) {
    ADC_ChannelConfTypeDef sConfig = {0};
    sConfig.Channel = ADC_CHANNEL_0;
    sConfig.Rank = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLETIME_239CYCLES_5;
    HAL_ADC_ConfigChannel(&hadc1, &sConfig);

    HAL_ADC_Start(&hadc1);
    HAL_ADC_PollForConversion(&hadc1, HAL_MAX_DELAY);
    uint32_t value = HAL_ADC_GetValue(&hadc1);
    HAL_ADC_Stop(&hadc1);
    return value;
}

/**
 * Mengambil N sampel ADC ke dalam buffer
 */
void Collect_Samples(uint32_t *buffer, uint32_t num) {
    for (uint32_t i = 0; i < num; i++) {
        buffer[i] = ADC_Read_Single();
    }
}

/**
 * Menghitung nilai minimum dari array
 */
uint32_t Calc_Min(uint32_t *data, uint32_t len) {
    uint32_t min_val = data[0];
    for (uint32_t i = 1; i < len; i++) {
        if (data[i] < min_val) min_val = data[i];
    }
    return min_val;
}

/**
 * Menghitung nilai maksimum dari array
 */
uint32_t Calc_Max(uint32_t *data, uint32_t len) {
    uint32_t max_val = data[0];
    for (uint32_t i = 1; i < len; i++) {
        if (data[i] > max_val) max_val = data[i];
    }
    return max_val;
}

/**
 * Menghitung rata-rata (mean)
 */
uint32_t Calc_Mean(uint32_t *data, uint32_t len) {
    uint32_t sum = 0;
    for (uint32_t i = 0; i < len; i++) {
        sum += data[i];
    }
    return sum / len;
}

/**
 * Menghitung standar deviasi (x100 untuk 2 desimal tanpa float)
 * stddev = sqrt(sum((xi - mean)^2) / N)
 * Mengembalikan stddev * 100
 */
uint32_t Calc_StdDev_x100(uint32_t *data, uint32_t len, uint32_t mean) {
    uint64_t variance_sum = 0;
    for (uint32_t i = 0; i < len; i++) {
        int32_t diff = (int32_t)data[i] - (int32_t)mean;
        variance_sum += (uint64_t)(diff * diff);
    }
    uint32_t variance = (uint32_t)(variance_sum / len);

    /* Hitung akar kuadrat menggunakan integer sqrt (Newton's method) */
    /* Kita hitung sqrt(variance) * 100 = sqrt(variance * 10000) */
    uint64_t val = (uint64_t)variance * 10000;
    if (val == 0) return 0;

    uint64_t x = val;
    uint64_t y = (x + 1) / 2;
    while (y < x) {
        x = y;
        y = (x + val / x) / 2;
    }
    return (uint32_t)x;
}

/**
 * Menghitung histogram (distribusi ke dalam NUM_BINS bin)
 */
void Calc_Histogram(uint32_t *data, uint32_t len, uint32_t min_val, uint32_t max_val,
                    uint32_t *bins, uint32_t num_bins) {
    /* Reset bins */
    for (uint32_t i = 0; i < num_bins; i++) bins[i] = 0;

    uint32_t range = max_val - min_val;
    if (range == 0) {
        bins[0] = len;
        return;
    }

    uint32_t bin_width = range / num_bins;
    if (bin_width == 0) bin_width = 1;

    for (uint32_t i = 0; i < len; i++) {
        uint32_t bin_idx = (data[i] - min_val) / bin_width;
        if (bin_idx >= num_bins) bin_idx = num_bins - 1;
        bins[bin_idx]++;
    }
}

/**
 * Program Utama
 */
int main(void) {
    HAL_Init();
    SystemClock_Config();
    MX_USART1_UART_Init();
    MX_ADC1_Init();

    printf("=== STM32 ADC Statistical Analysis ===\r\n");
    printf("Sampel: %d, Histogram: %d bin\r\n\r\n", NUM_SAMPLES, NUM_BINS);

    uint32_t counter = 0;
    uint32_t min_val, max_val, mean_val, stddev_x100, range_val;
    uint32_t histogram[NUM_BINS];

    while (1) {
        /* Ambil 100 sampel */
        printf("Mengambil %d sampel...\r\n", NUM_SAMPLES);
        Collect_Samples(samples, NUM_SAMPLES);

        /* Hitung statistik */
        min_val = Calc_Min(samples, NUM_SAMPLES);
        max_val = Calc_Max(samples, NUM_SAMPLES);
        mean_val = Calc_Mean(samples, NUM_SAMPLES);
        stddev_x100 = Calc_StdDev_x100(samples, NUM_SAMPLES, mean_val);
        range_val = max_val - min_val;

        /* Hitung histogram */
        Calc_Histogram(samples, NUM_SAMPLES, min_val, max_val, histogram, NUM_BINS);

        /* Tampilkan ringkasan statistik (format yang bisa di-parse) */
        printf("[%lu] MIN:%lu MAX:%lu MEAN:%lu STDDEV_X100:%lu RANGE:%lu\r\n",
               counter, min_val, max_val, mean_val, stddev_x100, range_val);

        /* Tampilkan histogram */
        printf("HIST:");
        for (int i = 0; i < NUM_BINS; i++) {
            printf("%lu", histogram[i]);
            if (i < NUM_BINS - 1) printf(",");
        }
        printf("\r\n");

        /* Tampilkan histogram visual */
        printf("--- Histogram ---\r\n");
        uint32_t bin_width = (range_val > 0) ? range_val / NUM_BINS : 1;
        for (int i = 0; i < NUM_BINS; i++) {
            uint32_t bin_start = min_val + i * bin_width;
            uint32_t bin_end = bin_start + bin_width;
            printf("  %4lu-%4lu [%2lu] ", bin_start, bin_end, histogram[i]);
            for (uint32_t j = 0; j < histogram[i] && j < 40; j++) {
                printf("#");
            }
            printf("\r\n");
        }
        printf("--- Selesai ---\r\n\r\n");

        counter++;
        HAL_Delay(3000);  /* Analisis setiap 3 detik */
    }
}
