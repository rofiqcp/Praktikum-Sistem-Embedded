/**
 * ============================================================
 * STM32_04_DMA_ADC_Continuous
 * ADC1 Continuous Mode → DMA1 Channel 1 → Buffer
 * Target: STM32F103C8T6 Blue Pill (72MHz)
 * Framework: STM32Cube HAL
 * ============================================================
 *
 * ADC1 channel 0 (PA0) continuously samples a potentiometer.
 * DMA1 Channel 1 fills a circular buffer automatically.
 * CPU is free while DMA handles data transfer.
 *
 * Hardware:
 *   - PA0: Potentiometer wiper (0–3.3 V)
 *   - PA9/PA10: UART1 debug output (115200 baud)
 *   - PC13: Activity LED
 *
 * Output: prints min/max/avg/latest voltage every second.
 * ============================================================
 */

#include "stm32f1xx_hal.h"
#include "config.h"
#include <stdio.h>
#include <string.h>

/* ---- Peripheral Handles ---- */
UART_HandleTypeDef huart1;
ADC_HandleTypeDef  hadc1;
DMA_HandleTypeDef  hdma_adc1;

/* ---- DMA Circular Buffer ---- */
static volatile uint16_t adc_buffer[ADC_BUFFER_SIZE];
static volatile uint8_t  half_cplt_flag = 0;
static volatile uint8_t  full_cplt_flag = 0;
static uint32_t          sample_count = 0;

/* ---- Forward Declarations ---- */
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART1_Init(void);
static void MX_DMA_Init(void);
static void MX_ADC1_Init(void);
static void print_statistics(const volatile uint16_t *buf, uint32_t len);
void Error_Handler(void);

/* ---- printf retarget ---- */
int _write(int file, char *ptr, int len) {
    HAL_UART_Transmit(&huart1, (uint8_t *)ptr, len, HAL_MAX_DELAY);
    return len;
}

/* ============================================================
 * MAIN
 * ============================================================ */
int main(void) {
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USART1_Init();
    MX_DMA_Init();
    MX_ADC1_Init();

    printf("==============================================================\r\n");
    printf("STM32F103 DMA ADC Continuous Conversion\r\n");
    printf("ADC1 Channel 0 (PA0) → DMA1 Ch1 → Circular Buffer\r\n");
    printf("Buffer size: %d samples, Sample rate: ~continuous\r\n", ADC_BUFFER_SIZE);
    printf("==============================================================\r\n");

    /* Start ADC with DMA in circular mode */
    HAL_ADC_Start_DMA(&hadc1, (uint32_t *)adc_buffer, ADC_BUFFER_SIZE);

    uint32_t last_print = 0;

    while (1) {
        uint32_t now = HAL_GetTick();

        /* Process half-complete: first half of buffer ready */
        if (half_cplt_flag) {
            half_cplt_flag = 0;
            sample_count += ADC_BUFFER_SIZE / 2;
        }

        /* Process full-complete: second half of buffer ready */
        if (full_cplt_flag) {
            full_cplt_flag = 0;
            sample_count += ADC_BUFFER_SIZE / 2;
        }

        /* Print statistics periodically */
        if ((now - last_print) >= STATS_PRINT_INTERVAL_MS) {
            last_print = now;
            LED_TOGGLE();

            printf("\r\n--- ADC DMA Stats (t=%lu ms, samples=%lu) ---\r\n",
                   now, sample_count);
            print_statistics(adc_buffer, ADC_BUFFER_SIZE);
        }
    }
}

/* ============================================================
 * Calculate and print buffer statistics
 * ============================================================ */
static void print_statistics(const volatile uint16_t *buf, uint32_t len) {
    uint32_t sum = 0;
    uint16_t min_val = 0xFFFF;
    uint16_t max_val = 0;

    for (uint32_t i = 0; i < len; i++) {
        uint16_t v = buf[i];
        sum += v;
        if (v < min_val) min_val = v;
        if (v > max_val) max_val = v;
    }

    uint32_t avg = sum / len;
    uint32_t latest = buf[len - 1];

    /* Convert to millivolts */
    uint32_t avg_mv   = (avg * ADC_VREF_MV) / ADC_MAX_VALUE;
    uint32_t min_mv   = (min_val * ADC_VREF_MV) / ADC_MAX_VALUE;
    uint32_t max_mv   = (max_val * ADC_VREF_MV) / ADC_MAX_VALUE;
    uint32_t lat_mv   = (latest * ADC_VREF_MV) / ADC_MAX_VALUE;

    printf("  Latest: %lu (%lu mV)\r\n", latest, lat_mv);
    printf("  Min:    %u (%lu mV)\r\n", min_val, min_mv);
    printf("  Max:    %u (%lu mV)\r\n", max_val, max_mv);
    printf("  Avg:    %lu (%lu mV)\r\n", avg, avg_mv);
    printf("  [DATA] raw=%lu,min=%u,max=%u,avg=%lu,mv=%lu\r\n",
           latest, min_val, max_val, avg, avg_mv);
}

/* ============================================================
 * DMA Callbacks
 * ============================================================ */
void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef *hadc) {
    if (hadc->Instance == ADC1) {
        half_cplt_flag = 1;
    }
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc) {
    if (hadc->Instance == ADC1) {
        full_cplt_flag = 1;
    }
}

/* ============================================================
 * ADC1 Init: Channel 0, Continuous mode, DMA enabled
 * ============================================================ */
static void MX_ADC1_Init(void) {
    __HAL_RCC_ADC1_CLK_ENABLE();

    /* Configure PA0 as analog input */
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin  = ADC_GPIO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    HAL_GPIO_Init(ADC_GPIO_PORT, &GPIO_InitStruct);

    /* ADC configuration */
    hadc1.Instance                   = ADC1;
    hadc1.Init.DataAlign             = ADC_DATAALIGN_RIGHT;
    hadc1.Init.ScanConvMode          = ADC_SCAN_DISABLE;
    hadc1.Init.ContinuousConvMode    = ENABLE;
    hadc1.Init.NbrOfConversion       = 1;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConv      = ADC_SOFTWARE_START;
    if (HAL_ADC_Init(&hadc1) != HAL_OK) {
        Error_Handler();
    }

    /* Link DMA handle */
    __HAL_LINKDMA(&hadc1, DMA_Handle, hdma_adc1);

    /* Configure channel */
    ADC_ChannelConfTypeDef sConfig = {0};
    sConfig.Channel      = ADC_CHANNEL;
    sConfig.Rank          = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime  = ADC_SAMPLETIME_239CYCLES_5;
    if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) {
        Error_Handler();
    }

    /* Calibrate ADC */
    HAL_ADCEx_Calibration_Start(&hadc1);
}

/* ============================================================
 * DMA1 Init: Channel 1 for ADC1 (circular mode)
 * ============================================================ */
static void MX_DMA_Init(void) {
    __HAL_RCC_DMA1_CLK_ENABLE();

    hdma_adc1.Instance                 = ADC_DMA_CHANNEL;
    hdma_adc1.Init.Direction           = DMA_PERIPH_TO_MEMORY;
    hdma_adc1.Init.PeriphInc           = DMA_PINC_DISABLE;
    hdma_adc1.Init.MemInc              = DMA_MINC_ENABLE;
    hdma_adc1.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    hdma_adc1.Init.MemDataAlignment    = DMA_MDATAALIGN_HALFWORD;
    hdma_adc1.Init.Mode                = DMA_CIRCULAR;
    hdma_adc1.Init.Priority            = DMA_PRIORITY_HIGH;
    if (HAL_DMA_Init(&hdma_adc1) != HAL_OK) {
        Error_Handler();
    }

    HAL_NVIC_SetPriority(ADC_DMA_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(ADC_DMA_IRQn);
}

/* ============================================================
 * System Clock: HSE 8MHz → PLL x9 → 72MHz
 * ============================================================ */
void SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState       = RCC_HSE_ON;
    RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    RCC_OscInitStruct.PLL.PLLState   = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource  = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL     = RCC_PLL_MUL9;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) Error_Handler();

    RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                       RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) Error_Handler();

    /* ADC clock prescaler: PCLK2/6 = 12 MHz */
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
    PeriphClkInit.AdcClockSelection    = RCC_ADCPCLK2_DIV6;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK) Error_Handler();
}

/* ============================================================
 * GPIO Init
 * ============================================================ */
static void MX_GPIO_Init(void) {
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin   = LED_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_PORT, &GPIO_InitStruct);
    LED_OFF();
}

/* ============================================================
 * USART1 Init
 * ============================================================ */
static void MX_USART1_Init(void) {
    __HAL_RCC_USART1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin   = DEBUG_UART_TX_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(DEBUG_UART_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin  = DEBUG_UART_RX_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(DEBUG_UART_PORT, &GPIO_InitStruct);

    huart1.Instance          = DEBUG_UART;
    huart1.Init.BaudRate     = DEBUG_UART_BAUD;
    huart1.Init.WordLength   = UART_WORDLENGTH_8B;
    huart1.Init.StopBits     = UART_STOPBITS_1;
    huart1.Init.Parity       = UART_PARITY_NONE;
    huart1.Init.Mode         = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart1) != HAL_OK) Error_Handler();
}

/* ============================================================
 * IRQ Handlers
 * ============================================================ */
void DMA1_Channel1_IRQHandler(void) {
    HAL_DMA_IRQHandler(&hdma_adc1);
}

void SysTick_Handler(void) {
    HAL_IncTick();
}

/* ============================================================
 * Error Handler
 * ============================================================ */
void Error_Handler(void) {
    __disable_irq();
    while (1) {
        LED_TOGGLE();
        for (volatile int i = 0; i < 200000; i++);
    }
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line) {
    printf("ASSERT: %s:%lu\r\n", file, line);
    Error_Handler();
}
#endif
