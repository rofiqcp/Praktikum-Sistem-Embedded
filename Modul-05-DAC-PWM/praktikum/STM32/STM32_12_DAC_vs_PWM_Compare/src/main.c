/**
 * ==========================================================
 *  Program     : DAC vs PWM Compare
 *  Modul       : 05 - DAC & PWM
 *  MCU         : STM32F103C8 / STM32F401CC / STM32F411CE
 *  Deskripsi   : Membandingkan output DAC dan PWM+filter.
 *                F4: DAC output (PA4) vs PWM output (PA0),
 *                    keduanya di-readback via ADC (PA6, PA7).
 *                F103: Membandingkan dua konfigurasi PWM berbeda,
 *                      readback via ADC.
 *                Menghitung error antara target dan aktual.
 * ==========================================================
 */

#ifdef STM32F1
#include "stm32f1xx_hal.h"
#define SYSTEM_CLOCK  72000000UL
#elif defined(STM32F4)
#include "stm32f4xx_hal.h"
#define SYSTEM_CLOCK  84000000UL
#endif

#include <stdio.h>
#include <string.h>
#include <math.h>

/* ===================== Handle Peripheral ===================== */
UART_HandleTypeDef huart1;
TIM_HandleTypeDef htim2;      /* PWM channel */
ADC_HandleTypeDef hadc1;

#ifdef STM32F4
DAC_HandleTypeDef hdac;
#endif

/* ===================== Retarget printf ===================== */
int _write(int file, char *ptr, int len) {
    HAL_UART_Transmit(&huart1, (uint8_t *)ptr, len, HAL_MAX_DELAY);
    return len;
}

/* ===================== SysTick Handler ===================== */
void SysTick_Handler(void) {
    HAL_IncTick();
}

/* ===================== Konfigurasi Clock ===================== */
void SystemClock_Config(void) {
#ifdef STM32F1
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
    HAL_RCC_OscConfig(&RCC_OscInitStruct);

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2);
#elif defined(STM32F4)
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = 4;
    RCC_OscInitStruct.PLL.PLLN = 84;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ = 4;
    HAL_RCC_OscConfig(&RCC_OscInitStruct);

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2);
#endif
}

/* ===================== Inisialisasi UART1 ===================== */
void MX_USART1_UART_Init(void) {
    __HAL_RCC_USART1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};

#ifdef STM32F1
    GPIO_InitStruct.Pin = GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    GPIO_InitStruct.Pin = GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
#elif defined(STM32F4)
    GPIO_InitStruct.Pin = GPIO_PIN_9 | GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
#endif

    huart1.Instance = USART1;
    huart1.Init.BaudRate = 115200;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(&huart1);
}

/* ===================== Inisialisasi ADC1 ===================== */
void MX_ADC1_Init(void) {
#ifdef STM32F1
    __HAL_RCC_ADC1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    /* PA6 = ADC1_IN6 (baca output PWM via filter) */
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_6 | GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* Konfigurasi ADC prescaler */
    RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
    PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6; /* 72MHz/6 = 12MHz */
    HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit);

    hadc1.Instance = ADC1;
    hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
    hadc1.Init.ContinuousConvMode = DISABLE;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc1.Init.NbrOfConversion = 1;
    HAL_ADC_Init(&hadc1);

    /* Kalibrasi ADC (khusus F103) */
    HAL_ADCEx_Calibration_Start(&hadc1);
#elif defined(STM32F4)
    __HAL_RCC_ADC1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    /* PA6, PA7 = ADC input */
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_6 | GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    hadc1.Instance = ADC1;
    hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
    hadc1.Init.Resolution = ADC_RESOLUTION_12B;
    hadc1.Init.ScanConvMode = DISABLE;
    hadc1.Init.ContinuousConvMode = DISABLE;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
    hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc1.Init.NbrOfConversion = 1;
    hadc1.Init.DMAContinuousRequests = DISABLE;
    hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
    HAL_ADC_Init(&hadc1);
#endif
}

/* ===================== Baca ADC Channel ===================== */
uint16_t ADC_Read(uint32_t channel) {
    ADC_ChannelConfTypeDef sConfig = {0};
    sConfig.Channel = channel;
    sConfig.Rank = 1;
#ifdef STM32F1
    sConfig.SamplingTime = ADC_SAMPLETIME_71CYCLES_5;
#elif defined(STM32F4)
    sConfig.SamplingTime = ADC_SAMPLETIME_84CYCLES;
#endif
    HAL_ADC_ConfigChannel(&hadc1, &sConfig);

    HAL_ADC_Start(&hadc1);
    HAL_ADC_PollForConversion(&hadc1, HAL_MAX_DELAY);
    uint16_t value = HAL_ADC_GetValue(&hadc1);
    HAL_ADC_Stop(&hadc1);

    return value;
}

/* ===================== Inisialisasi PWM TIM2_CH1 (PA0) ===================== */
void MX_TIM2_PWM_Init(void) {
    __HAL_RCC_TIM2_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};
#ifdef STM32F1
    GPIO_InitStruct.Pin = GPIO_PIN_0;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    htim2.Init.Prescaler = 0;
#elif defined(STM32F4)
    GPIO_InitStruct.Pin = GPIO_PIN_0;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF1_TIM2;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    htim2.Init.Prescaler = 0;
#endif

    htim2.Instance = TIM2;
    htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim2.Init.Period = 4095;  /* 12-bit resolusi */
    htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    HAL_TIM_PWM_Init(&htim2);

    TIM_OC_InitTypeDef sConfigOC = {0};
    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = 0;
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_1);

    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
}

#ifdef STM32F4
/* ===================== Inisialisasi DAC (F4) ===================== */
void MX_DAC_Init(void) {
    __HAL_RCC_DAC_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_4;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    hdac.Instance = DAC;
    HAL_DAC_Init(&hdac);

    DAC_ChannelConfTypeDef sConfig = {0};
    sConfig.DAC_Trigger = DAC_TRIGGER_NONE;
    sConfig.DAC_OutputBuffer = DAC_OUTPUTBUFFER_ENABLE;
    HAL_DAC_ConfigChannel(&hdac, &sConfig, DAC_CHANNEL_1);

    HAL_DAC_Start(&hdac, DAC_CHANNEL_1);
}
#endif

/* ===================== Program Utama ===================== */
int main(void) {
    HAL_Init();
    SystemClock_Config();
    MX_USART1_UART_Init();
    MX_TIM2_PWM_Init();
    MX_ADC1_Init();

#ifdef STM32F4
    MX_DAC_Init();
    printf("\r\n=== DAC vs PWM Compare (STM32F4) ===\r\n");
    printf("DAC output pada PA4 -> readback ADC pada PA6\r\n");
    printf("PWM output pada PA0 -> readback ADC pada PA7\r\n");
    printf("(Hubungkan PA4->PA6 dan PA0->RC_filter->PA7)\r\n\r\n");
#else
    printf("\r\n=== PWM Compare (STM32F103) ===\r\n");
    printf("CATATAN: F103 tidak punya DAC\r\n");
    printf("PWM output pada PA0 -> readback ADC pada PA6\r\n");
    printf("(Hubungkan PA0->RC_filter->PA6)\r\n\r\n");
#endif

    printf("%-6s | %-8s | %-8s | %-8s | %-8s | %-8s\r\n",
           "Step", "Target", "DAC_ADC", "PWM_ADC", "ErrDAC%", "ErrPWM%");
    printf("-------|----------|----------|----------|----------|----------\r\n");

    uint32_t test_cycle = 0;

    while (1) {
        test_cycle++;
        printf("\r\n--- Test Cycle #%lu ---\r\n", test_cycle);

        float total_error_dac = 0.0f;
        float total_error_pwm = 0.0f;
        uint16_t num_steps = 0;

        /* Sweep dari 0 sampai 4095 (12-bit) */
        for (uint16_t target = 0; target <= 4095; target += 256) {
#ifdef STM32F4
            /* Set DAC value */
            HAL_DAC_SetValue(&hdac, DAC_CHANNEL_1, DAC_ALIGN_12B_R, target);
#endif
            /* Set PWM duty */
            __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, target);

            /* Tunggu settling time (penting untuk RC filter) */
            HAL_Delay(100);

            /* Baca ADC */
            uint16_t adc_ch6 = ADC_Read(ADC_CHANNEL_6);  /* Readback channel 1 */
            uint16_t adc_ch7 = ADC_Read(ADC_CHANNEL_7);  /* Readback channel 2 */

            /* Hitung error */
            float error_dac = 0.0f;
            float error_pwm = 0.0f;

            if (target > 0) {
#ifdef STM32F4
                error_dac = ((float)(adc_ch6) - (float)(target)) / (float)(target) * 100.0f;
#else
                error_dac = ((float)(adc_ch6) - (float)(target)) / (float)(target) * 100.0f;
#endif
                error_pwm = ((float)(adc_ch7) - (float)(target)) / (float)(target) * 100.0f;
            }

            total_error_dac += (error_dac < 0) ? -error_dac : error_dac;
            total_error_pwm += (error_pwm < 0) ? -error_pwm : error_pwm;
            num_steps++;

#ifdef STM32F4
            printf("[CMP] Target=%4u | DAC_RB=%4u | PWM_RB=%4u | ErrDAC=%+6.1f%% | ErrPWM=%+6.1f%%\r\n",
                   target, adc_ch6, adc_ch7, error_dac, error_pwm);
#else
            printf("[CMP] Target=%4u | PWM1_RB=%4u | PWM2_RB=%4u | Err1=%+6.1f%% | Err2=%+6.1f%%\r\n",
                   target, adc_ch6, adc_ch7, error_dac, error_pwm);
#endif
        }

        /* Ringkasan error */
        float avg_error_dac = total_error_dac / num_steps;
        float avg_error_pwm = total_error_pwm / num_steps;

        printf("\r\n[RESULT] === Ringkasan Error ===\r\n");
#ifdef STM32F4
        printf("[RESULT] Rata-rata Error DAC   : %.2f%%\r\n", avg_error_dac);
        printf("[RESULT] Rata-rata Error PWM   : %.2f%%\r\n", avg_error_pwm);
        if (avg_error_dac < avg_error_pwm) {
            printf("[RESULT] DAC lebih akurat dari PWM+filter\r\n");
        } else {
            printf("[RESULT] PWM+filter lebih akurat dari DAC\r\n");
        }
#else
        printf("[RESULT] Rata-rata Error PWM1  : %.2f%%\r\n", avg_error_dac);
        printf("[RESULT] Rata-rata Error PWM2  : %.2f%%\r\n", avg_error_pwm);
#endif

        printf("\r\n");
        HAL_Delay(3000);
    }
}
