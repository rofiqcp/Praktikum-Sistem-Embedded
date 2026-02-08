/**
 * ============================================================================
 * Program     : STM32_07_ADC_Threshold_Alert
 * Modul       : 04 - ADC (Analog to Digital Converter)
 * MCU         : STM32F103C8T6 (Blue Pill)
 * Framework   : STM32Cube HAL
 * Deskripsi   : Menggunakan fitur Analog Watchdog pada ADC untuk
 *               mendeteksi ketika nilai ADC melewati batas threshold.
 *               LED pada PC13 menyala jika nilai ADC di luar range.
 *               Menggunakan HAL_ADC_AnalogWDGConfig().
 * Koneksi     : PA0 -> Potentiometer / Sensor Analog
 *               PC13 -> LED (active low pada Blue Pill)
 *               PA9 -> UART TX, PA10 -> UART RX
 * ============================================================================
 */

#include "stm32f1xx_hal.h"
#include <stdio.h>
#include <string.h>

/* Handle peripheral */
UART_HandleTypeDef huart1;
ADC_HandleTypeDef hadc1;

/* Threshold batas atas dan bawah (12-bit: 0-4095) */
#define THRESHOLD_HIGH  3000    /* Batas atas (~2.4V) */
#define THRESHOLD_LOW   1000    /* Batas bawah (~0.8V) */

/* Flag alert dari analog watchdog */
volatile uint8_t awdg_alert = 0;
volatile uint32_t alert_count = 0;

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
 * Inisialisasi LED pada PC13 (active low pada Blue Pill)
 */
void MX_GPIO_LED_Init(void) {
    __HAL_RCC_GPIOC_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_13;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    /* LED mati (PC13 active low, HIGH = mati) */
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
}

/**
 * Inisialisasi ADC1 dengan Analog Watchdog
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
    hadc1.Init.ContinuousConvMode = ENABLE;  /* Continuous untuk analog watchdog */
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

    /* Konfigurasi Analog Watchdog */
    ADC_AnalogWDGConfTypeDef AnalogWDGConfig = {0};
    AnalogWDGConfig.WatchdogMode = ADC_ANALOGWATCHDOG_SINGLE_REG;
    AnalogWDGConfig.HighThreshold = THRESHOLD_HIGH;
    AnalogWDGConfig.LowThreshold = THRESHOLD_LOW;
    AnalogWDGConfig.Channel = ADC_CHANNEL_0;
    AnalogWDGConfig.ITMode = ENABLE;  /* Aktifkan interrupt */
    HAL_ADC_AnalogWDGConfig(&hadc1, &AnalogWDGConfig);

    /* Kalibrasi ADC */
    HAL_ADCEx_Calibration_Start(&hadc1);

    /* Aktifkan interrupt ADC */
    HAL_NVIC_SetPriority(ADC1_2_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(ADC1_2_IRQn);
}

/**
 * Callback Analog Watchdog - dipanggil saat nilai di luar threshold
 */
void HAL_ADC_LevelOutOfWindowCallback(ADC_HandleTypeDef* hadc) {
    if (hadc->Instance == ADC1) {
        awdg_alert = 1;
        alert_count++;
    }
}

/**
 * Handler interrupt ADC1/ADC2
 */
void ADC1_2_IRQHandler(void) {
    HAL_ADC_IRQHandler(&hadc1);
}

/**
 * Program Utama
 */
int main(void) {
    HAL_Init();
    SystemClock_Config();
    MX_USART1_UART_Init();
    MX_GPIO_LED_Init();
    MX_ADC1_Init();

    printf("=== STM32 ADC Threshold Alert ===\r\n");
    printf("Analog Watchdog: LOW=%d HIGH=%d\r\n", THRESHOLD_LOW, THRESHOLD_HIGH);
    printf("LED PC13 menyala jika di luar range\r\n\r\n");

    /* Mulai ADC dengan interrupt */
    HAL_ADC_Start_IT(&hadc1);

    uint32_t adc_raw = 0;
    uint32_t voltage_mv = 0;
    uint32_t counter = 0;
    const char *status;

    while (1) {
        /* Baca nilai ADC saat ini */
        adc_raw = HAL_ADC_GetValue(&hadc1);
        voltage_mv = (adc_raw * 3300) / 4095;

        /* Tentukan status berdasarkan threshold */
        if (adc_raw > THRESHOLD_HIGH) {
            status = "TINGGI";
            HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);  /* LED nyala */
        } else if (adc_raw < THRESHOLD_LOW) {
            status = "RENDAH";
            HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);  /* LED nyala */
        } else {
            status = "NORMAL";
            HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);    /* LED mati */
        }

        /* Tampilkan data */
        printf("[%lu] ADC:%lu MV:%lu STATUS:%s ALERT_CNT:%lu\r\n",
               counter, adc_raw, voltage_mv, status, alert_count);

        /* Reset flag alert */
        if (awdg_alert) {
            printf("  >> PERINGATAN: Nilai di luar range!\r\n");
            awdg_alert = 0;
        }

        counter++;
        HAL_Delay(500);
    }
}
