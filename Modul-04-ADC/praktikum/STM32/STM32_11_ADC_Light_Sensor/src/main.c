/**
 * ============================================================================
 * Program     : STM32_11_ADC_Light_Sensor
 * Modul       : 04 - ADC (Analog to Digital Converter)
 * MCU         : STM32F103C8T6 (Blue Pill)
 * Framework   : STM32Cube HAL
 * Deskripsi   : Membaca sensor cahaya LDR (Light Dependent Resistor)
 *               pada PA0, menghitung perkiraan lux, dan mengklasifikasi
 *               level cahaya (gelap, redup, normal, terang, sangat terang).
 *               Rangkaian: VCC -> LDR -> PA0 -> R(10kΩ) -> GND
 * Koneksi     : PA0 -> Titik tengah LDR + Resistor 10kΩ
 *               PA9 -> UART TX, PA10 -> UART RX
 * ============================================================================
 */

#include "stm32f1xx_hal.h"
#include <stdio.h>
#include <string.h>

/* Handle peripheral */
UART_HandleTypeDef huart1;
ADC_HandleTypeDef hadc1;

/* Konstanta untuk perhitungan lux */
#define VREF_MV         3300
#define ADC_RESOLUTION  4095
#define R_FIXED         10000   /* Resistor tetap 10kΩ */
#define LDR_GAMMA       0.7f   /* Koefisien gamma LDR (tipikal) */
#define LDR_RL10        50.0f  /* Resistansi LDR pada 10 lux (kΩ) */

/* Batas klasifikasi cahaya (ADC raw) */
#define LIGHT_DARK      500     /* Gelap: ADC < 500 */
#define LIGHT_DIM       1500    /* Redup: 500-1500 */
#define LIGHT_NORMAL    2500    /* Normal: 1500-2500 */
#define LIGHT_BRIGHT    3500    /* Terang: 2500-3500 */
                                /* Sangat Terang: > 3500 */

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
 * Konfigurasi dan baca ADC
 */
uint32_t ADC_Read(void) {
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
 * Baca ADC dengan rata-rata 16 sampel
 */
uint32_t ADC_Read_Averaged(void) {
    uint32_t sum = 0;
    for (int i = 0; i < 16; i++) {
        sum += ADC_Read();
    }
    return sum / 16;
}

/**
 * Estimasi lux berdasarkan nilai ADC
 * Menggunakan pendekatan sederhana berdasarkan resistansi LDR
 * Semakin tinggi cahaya, semakin rendah resistansi LDR, semakin tinggi tegangan di PA0
 */
uint32_t Estimate_Lux(uint32_t adc_raw) {
    if (adc_raw == 0) return 0;
    if (adc_raw >= 4095) return 10000;

    /* Hitung tegangan di PA0 (mV) */
    uint32_t voltage_mv = (adc_raw * VREF_MV) / ADC_RESOLUTION;

    /* Hitung resistansi LDR (ohm) */
    /* Voltage divider: V_PA0 = VCC * R_fixed / (R_ldr + R_fixed) */
    /* R_ldr = R_fixed * (VCC - V_PA0) / V_PA0 */
    if (voltage_mv == 0) return 0;
    uint32_t r_ldr = (uint32_t)((uint64_t)R_FIXED * (VREF_MV - voltage_mv) / voltage_mv);

    /* Estimasi lux sederhana: lux ≈ 500000 / R_ldr (untuk LDR tipikal) */
    /* Ini adalah pendekatan linear sederhana */
    if (r_ldr == 0) return 10000;
    uint32_t lux = 500000 / r_ldr;

    if (lux > 10000) lux = 10000;
    return lux;
}

/**
 * Klasifikasi level cahaya berdasarkan nilai ADC
 */
const char* Classify_Light(uint32_t adc_raw) {
    if (adc_raw < LIGHT_DARK)   return "GELAP";
    if (adc_raw < LIGHT_DIM)    return "REDUP";
    if (adc_raw < LIGHT_NORMAL) return "NORMAL";
    if (adc_raw < LIGHT_BRIGHT) return "TERANG";
    return "SANGAT_TERANG";
}

/**
 * Menghitung persentase cahaya (0-100%)
 */
uint32_t Light_Percentage(uint32_t adc_raw) {
    return (adc_raw * 100) / 4095;
}

/**
 * Program Utama
 */
int main(void) {
    HAL_Init();
    SystemClock_Config();
    MX_USART1_UART_Init();
    MX_ADC1_Init();

    printf("=== STM32 Light Sensor (LDR) ===\r\n");
    printf("LDR pada PA0 dengan R=10k ohm\r\n");
    printf("Klasifikasi: Gelap/Redup/Normal/Terang/Sangat Terang\r\n\r\n");

    uint32_t adc_raw, lux, pct;
    const char *level;
    uint32_t counter = 0;

    while (1) {
        /* Baca sensor LDR (rata-rata 16 sampel) */
        adc_raw = ADC_Read_Averaged();

        /* Estimasi lux */
        lux = Estimate_Lux(adc_raw);

        /* Klasifikasi level cahaya */
        level = Classify_Light(adc_raw);

        /* Persentase cahaya */
        pct = Light_Percentage(adc_raw);

        /* Tampilkan data */
        printf("[%lu] ADC:%lu LUX:%lu PCT:%lu LEVEL:%s\r\n",
               counter, adc_raw, lux, pct, level);

        counter++;
        HAL_Delay(500);
    }
}
