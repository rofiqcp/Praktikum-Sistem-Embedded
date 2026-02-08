/**
 * ============================================================================
 * Program     : STM32_08_ADC_Battery_Monitor
 * Modul       : 04 - ADC (Analog to Digital Converter)
 * MCU         : STM32F103C8T6 (Blue Pill)
 * Framework   : STM32Cube HAL
 * Deskripsi   : Monitor tegangan baterai menggunakan voltage divider
 *               2x10kΩ. Tegangan baterai dibagi 2 sebelum masuk ke ADC.
 *               Tegangan sebenarnya = ADC_voltage * 2.
 *               Estimasi persentase baterai berdasarkan kurva discharge.
 * Koneksi     : PA0 -> Titik tengah voltage divider (2x10kΩ)
 *               Baterai -> R1(10k) -> PA0 -> R2(10k) -> GND
 *               PA9 -> UART TX, PA10 -> UART RX
 * ============================================================================
 */

#include "stm32f1xx_hal.h"
#include <stdio.h>
#include <string.h>

/* Handle peripheral */
UART_HandleTypeDef huart1;
ADC_HandleTypeDef hadc1;

/* Konstanta voltage divider */
#define VREF_MV         3300    /* Tegangan referensi ADC (mV) */
#define ADC_RESOLUTION  4095    /* Resolusi 12-bit */
#define DIVIDER_RATIO   2       /* Rasio voltage divider (R1=R2=10k) */

/* Batas tegangan baterai Li-Ion (mV) */
#define BATT_FULL_MV    4200    /* Baterai penuh (4.2V) */
#define BATT_NOMINAL_MV 3700    /* Tegangan nominal (3.7V) */
#define BATT_LOW_MV     3300    /* Baterai rendah (3.3V) */
#define BATT_CRIT_MV    3000    /* Baterai kritis (3.0V) */

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
 * Membaca ADC dengan rata-rata 16 sampel untuk stabilitas
 */
uint32_t ADC_Read_Averaged(void) {
    uint32_t sum = 0;
    ADC_Select_Channel(ADC_CHANNEL_0);
    for (int i = 0; i < 16; i++) {
        HAL_ADC_Start(&hadc1);
        HAL_ADC_PollForConversion(&hadc1, HAL_MAX_DELAY);
        sum += HAL_ADC_GetValue(&hadc1);
        HAL_ADC_Stop(&hadc1);
    }
    return sum / 16;
}

/**
 * Menghitung tegangan baterai sebenarnya (mV)
 * Memperhitungkan rasio voltage divider
 */
uint32_t Calculate_Battery_Voltage(uint32_t adc_raw) {
    uint32_t adc_voltage = (adc_raw * VREF_MV) / ADC_RESOLUTION;
    return adc_voltage * DIVIDER_RATIO;
}

/**
 * Estimasi persentase baterai berdasarkan kurva discharge Li-Ion
 * Menggunakan linear interpolation sederhana
 */
uint32_t Estimate_Battery_Percent(uint32_t battery_mv) {
    if (battery_mv >= BATT_FULL_MV) return 100;
    if (battery_mv <= BATT_CRIT_MV) return 0;

    /* Linear interpolation antara 3.0V (0%) dan 4.2V (100%) */
    uint32_t percent = ((battery_mv - BATT_CRIT_MV) * 100) / (BATT_FULL_MV - BATT_CRIT_MV);
    if (percent > 100) percent = 100;
    return percent;
}

/**
 * Menentukan status baterai berdasarkan tegangan
 */
const char* Get_Battery_Status(uint32_t battery_mv) {
    if (battery_mv >= BATT_FULL_MV) return "PENUH";
    if (battery_mv >= BATT_NOMINAL_MV) return "BAIK";
    if (battery_mv >= BATT_LOW_MV) return "RENDAH";
    if (battery_mv >= BATT_CRIT_MV) return "KRITIS";
    return "HABIS";
}

/**
 * Program Utama
 */
int main(void) {
    HAL_Init();
    SystemClock_Config();
    MX_USART1_UART_Init();
    MX_ADC1_Init();

    printf("=== STM32 Battery Monitor ===\r\n");
    printf("Voltage Divider: 2x10k ohm\r\n");
    printf("Range: %.1fV - %.1fV\r\n\r\n",
           (float)BATT_CRIT_MV / 1000, (float)BATT_FULL_MV / 1000);

    uint32_t adc_raw, battery_mv, battery_pct;
    const char *batt_status;
    uint32_t counter = 0;

    while (1) {
        /* Baca ADC (rata-rata 16 sampel) */
        adc_raw = ADC_Read_Averaged();

        /* Hitung tegangan baterai sebenarnya */
        battery_mv = Calculate_Battery_Voltage(adc_raw);

        /* Estimasi persentase */
        battery_pct = Estimate_Battery_Percent(battery_mv);

        /* Status baterai */
        batt_status = Get_Battery_Status(battery_mv);

        /* Tampilkan data */
        printf("[%lu] ADC:%lu BATT_MV:%lu BATT_PCT:%lu STATUS:%s\r\n",
               counter, adc_raw, battery_mv, battery_pct, batt_status);

        counter++;
        HAL_Delay(2000);  /* Update setiap 2 detik */
    }
}
