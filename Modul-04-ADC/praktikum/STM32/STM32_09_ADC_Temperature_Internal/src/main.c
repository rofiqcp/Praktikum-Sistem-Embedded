/**
 * ============================================================================
 * Program     : STM32_09_ADC_Temperature_Internal
 * Modul       : 04 - ADC (Analog to Digital Converter)
 * MCU         : STM32F103C8T6 (Blue Pill)
 * Framework   : STM32Cube HAL
 * Deskripsi   : Membaca sensor suhu internal STM32F103 menggunakan
 *               ADC_CHANNEL_TEMPSENSOR. Sensor suhu internal terhubung
 *               ke ADC1 Channel 16.
 *               Rumus: T(°C) = ((V25 - Vsense) / Avg_Slope) + 25
 *               V25 = 1.43V, Avg_Slope = 4.3mV/°C (dari datasheet)
 * Koneksi     : Tidak ada koneksi eksternal (sensor internal)
 *               PA9 -> UART TX, PA10 -> UART RX
 * ============================================================================
 */

#include "stm32f1xx_hal.h"
#include <stdio.h>
#include <string.h>

/* Handle peripheral */
UART_HandleTypeDef huart1;
ADC_HandleTypeDef hadc1;

/* Konstanta sensor suhu internal STM32F103 (dari datasheet) */
#define V25_MV          1430    /* Tegangan pada 25°C (1.43V = 1430mV) */
#define AVG_SLOPE_UV    4300    /* Avg slope = 4.3mV/°C = 4300uV/°C */
#define VREF_MV         3300    /* Tegangan referensi ADC */
#define ADC_RESOLUTION  4095    /* Resolusi ADC 12-bit */

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
 * Inisialisasi ADC1 untuk sensor suhu internal
 * Channel: ADC_CHANNEL_TEMPSENSOR (Channel 16)
 * Sampling time minimal 17.1us untuk akurasi sensor suhu
 */
void MX_ADC1_Init(void) {
    __HAL_RCC_ADC1_CLK_ENABLE();

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
 * Membaca sensor suhu internal
 * Menggunakan sampling time 239.5 cycles (cukup untuk sensor suhu)
 */
uint32_t Read_Temperature_Raw(void) {
    ADC_ChannelConfTypeDef sConfig = {0};
    sConfig.Channel = ADC_CHANNEL_TEMPSENSOR;  /* Channel 16: sensor suhu */
    sConfig.Rank = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLETIME_239CYCLES_5;  /* Sampling time panjang */
    HAL_ADC_ConfigChannel(&hadc1, &sConfig);

    HAL_ADC_Start(&hadc1);
    HAL_ADC_PollForConversion(&hadc1, HAL_MAX_DELAY);
    uint32_t value = HAL_ADC_GetValue(&hadc1);
    HAL_ADC_Stop(&hadc1);
    return value;
}

/**
 * Membaca rata-rata dari N sampel sensor suhu
 */
uint32_t Read_Temperature_Averaged(uint32_t num_samples) {
    uint32_t sum = 0;
    for (uint32_t i = 0; i < num_samples; i++) {
        sum += Read_Temperature_Raw();
    }
    return sum / num_samples;
}

/**
 * Konversi ADC raw ke tegangan sensor (milivolt)
 */
uint32_t ADC_To_Millivolt(uint32_t raw) {
    return (raw * VREF_MV) / ADC_RESOLUTION;
}

/**
 * Menghitung suhu dari tegangan sensor
 * Rumus: T(°C) = ((V25 - Vsense) / Avg_Slope) + 25
 * Menggunakan integer arithmetic: T*10 untuk 1 desimal
 */
int32_t Calculate_Temperature_x10(uint32_t vsense_mv) {
    /* T(°C) x10 = ((V25_mV - Vsense_mV) * 10000 / AVG_SLOPE_uV) + 250 */
    int32_t temp_x10 = (((int32_t)V25_MV - (int32_t)vsense_mv) * 10000) / AVG_SLOPE_UV + 250;
    return temp_x10;
}

/**
 * Program Utama
 */
int main(void) {
    HAL_Init();
    SystemClock_Config();
    MX_USART1_UART_Init();
    MX_ADC1_Init();

    printf("=== STM32 Internal Temperature Sensor ===\r\n");
    printf("Channel: ADC_CHANNEL_TEMPSENSOR\r\n");
    printf("V25=1.43V, Avg_Slope=4.3mV/C\r\n\r\n");

    uint32_t adc_raw, vsense_mv;
    int32_t temp_x10;
    uint32_t counter = 0;

    while (1) {
        /* Baca sensor suhu (rata-rata 32 sampel) */
        adc_raw = Read_Temperature_Averaged(32);

        /* Konversi ke tegangan */
        vsense_mv = ADC_To_Millivolt(adc_raw);

        /* Hitung suhu (x10 untuk 1 desimal) */
        temp_x10 = Calculate_Temperature_x10(vsense_mv);

        /* Tampilkan data */
        printf("[%lu] ADC:%lu VSENSE_MV:%lu TEMP_X10:%ld\r\n",
               counter, adc_raw, vsense_mv, (long)temp_x10);

        counter++;
        HAL_Delay(1000);
    }
}
