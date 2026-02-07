/**
 * ============================================================================
 * Program     : STM32_02_ADC_Voltage_Display
 * Modul       : 04 - ADC (Analog to Digital Converter)
 * MCU         : STM32F103C8T6 (Blue Pill)
 * Framework   : STM32Cube HAL
 * Deskripsi   : Membaca nilai ADC dari potensiometer pada PA0 dan mengkonversi
 *               ke tegangan dalam satuan milivolt (mV).
 *               Rumus: V = raw * 3300 / 4095 mV
 * Koneksi     : PA0 -> Potentiometer (wiper)
 *               PA9 -> UART TX
 *               PA10 -> UART RX
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
 * HSE 8MHz -> PLL x9 -> SYSCLK 72MHz
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
 * PA9 = TX, PA10 = RX, Baudrate = 115200
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

    /* Kalibrasi ADC */
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
 * Konversi nilai ADC raw ke tegangan (milivolt)
 * VREF = 3300 mV, Resolusi = 12-bit (4095)
 */
uint32_t ADC_To_Millivolt(uint32_t raw) {
    return (raw * 3300) / 4095;
}

/**
 * Membuat bar grafik sederhana di terminal
 */
void Print_Bar(uint32_t voltage_mv, uint32_t max_mv) {
    uint32_t bar_len = (voltage_mv * 40) / max_mv;
    printf("  |");
    for (uint32_t i = 0; i < 40; i++) {
        if (i < bar_len) printf("#");
        else printf(" ");
    }
    printf("| %lu.%03lu V\r\n", voltage_mv / 1000, voltage_mv % 1000);
}

/**
 * Program Utama
 */
int main(void) {
    HAL_Init();
    SystemClock_Config();
    MX_USART1_UART_Init();
    MX_ADC1_Init();

    printf("=== STM32 ADC Voltage Display ===\r\n");
    printf("Konversi ADC ke Tegangan (mV)\r\n");
    printf("VREF = 3300 mV, Resolusi = 12-bit\r\n\r\n");

    uint32_t adc_raw = 0;
    uint32_t voltage_mv = 0;
    uint32_t counter = 0;

    while (1) {
        /* Baca nilai ADC */
        adc_raw = ADC_Read();

        /* Konversi ke milivolt */
        voltage_mv = ADC_To_Millivolt(adc_raw);

        /* Tampilkan hasil */
        printf("[%lu] ADC_RAW:%lu VOLTAGE_MV:%lu\r\n",
               counter, adc_raw, voltage_mv);

        /* Tampilkan bar grafik */
        Print_Bar(voltage_mv, 3300);

        counter++;
        HAL_Delay(500);
    }
}
