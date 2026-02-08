/**
 * ============================================================================
 * Program     : STM32_10_ADC_Sampling_Rate
 * Modul       : 04 - ADC (Analog to Digital Converter)
 * MCU         : STM32F103C8T6 (Blue Pill)
 * Framework   : STM32Cube HAL
 * Deskripsi   : Mengukur kecepatan sampling ADC (Samples Per Second / SPS).
 *               Melakukan N konversi ADC, mengukur waktu menggunakan
 *               SysTick/HAL_GetTick(), dan menghitung SPS.
 *               Membandingkan berbagai sampling time setting.
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

/* Jumlah sampel untuk pengukuran */
#define NUM_SAMPLES     1000
#define NUM_TEST_CYCLES 5

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
 * Konfigurasi channel ADC dengan sampling time tertentu
 */
void ADC_Config_SamplingTime(uint32_t sampling_time) {
    ADC_ChannelConfTypeDef sConfig = {0};
    sConfig.Channel = ADC_CHANNEL_0;
    sConfig.Rank = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = sampling_time;
    HAL_ADC_ConfigChannel(&hadc1, &sConfig);
}

/**
 * Mengukur kecepatan sampling ADC
 * Melakukan num_samples konversi dan menghitung waktu total
 * Mengembalikan SPS (Samples Per Second)
 */
uint32_t Measure_SPS(uint32_t num_samples) {
    uint32_t start_tick = HAL_GetTick();

    for (uint32_t i = 0; i < num_samples; i++) {
        HAL_ADC_Start(&hadc1);
        HAL_ADC_PollForConversion(&hadc1, HAL_MAX_DELAY);
        HAL_ADC_GetValue(&hadc1);
        HAL_ADC_Stop(&hadc1);
    }

    uint32_t elapsed_ms = HAL_GetTick() - start_tick;

    /* Hindari pembagian dengan nol */
    if (elapsed_ms == 0) elapsed_ms = 1;

    /* Hitung SPS = samples * 1000 / elapsed_ms */
    uint32_t sps = (num_samples * 1000) / elapsed_ms;
    return sps;
}

/**
 * Mendapatkan nama sampling time untuk tampilan
 */
const char* Get_SamplingTime_Name(uint32_t st) {
    switch (st) {
        case ADC_SAMPLETIME_1CYCLE_5:   return "1.5cy";
        case ADC_SAMPLETIME_7CYCLES_5:  return "7.5cy";
        case ADC_SAMPLETIME_13CYCLES_5: return "13.5cy";
        case ADC_SAMPLETIME_28CYCLES_5: return "28.5cy";
        case ADC_SAMPLETIME_41CYCLES_5: return "41.5cy";
        case ADC_SAMPLETIME_55CYCLES_5: return "55.5cy";
        case ADC_SAMPLETIME_71CYCLES_5: return "71.5cy";
        case ADC_SAMPLETIME_239CYCLES_5: return "239.5cy";
        default: return "unknown";
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

    printf("=== STM32 ADC Sampling Rate Test ===\r\n");
    printf("Mengukur SPS untuk berbagai sampling time\r\n");
    printf("Jumlah sampel per test: %d\r\n\r\n", NUM_SAMPLES);

    /* Daftar sampling time yang akan diuji */
    uint32_t sampling_times[] = {
        ADC_SAMPLETIME_1CYCLE_5,
        ADC_SAMPLETIME_7CYCLES_5,
        ADC_SAMPLETIME_13CYCLES_5,
        ADC_SAMPLETIME_28CYCLES_5,
        ADC_SAMPLETIME_41CYCLES_5,
        ADC_SAMPLETIME_55CYCLES_5,
        ADC_SAMPLETIME_71CYCLES_5,
        ADC_SAMPLETIME_239CYCLES_5
    };
    uint32_t num_tests = sizeof(sampling_times) / sizeof(sampling_times[0]);

    uint32_t cycle = 0;

    while (1) {
        printf("--- Siklus Pengukuran #%lu ---\r\n", cycle);

        /* Uji setiap sampling time */
        for (uint32_t i = 0; i < num_tests; i++) {
            ADC_Config_SamplingTime(sampling_times[i]);

            uint32_t sps = Measure_SPS(NUM_SAMPLES);

            printf("[%lu] STIME:%s SPS:%lu SAMPLES:%d\r\n",
                   cycle * num_tests + i,
                   Get_SamplingTime_Name(sampling_times[i]),
                   sps, NUM_SAMPLES);
        }

        printf("\r\n");
        cycle++;
        HAL_Delay(3000);  /* Jeda 3 detik antar siklus */
    }
}
