/**
 * ================================================================================
 * PROGRAM 7: BATTERY MONITOR ADC - STM32F103C8T6
 * ================================================================================
 * Deskripsi:
 *   Monitoring tegangan baterai menggunakan ADC internal STM32.
 *   Menggunakan VREFINT (internal reference 1.2V) untuk kalibrasi.
 *   Pembacaan ADC pada channel PA1 (ADC1_CH1) melalui voltage divider.
 *
 *   Kalibrasi dengan VREFINT:
 *   - VREFINT = 1.2V (tipikal)
 *   - ADC VREFINT = raw_vrefint
 *   - VDDA = (1.2V * 4095) / raw_vrefint
 *   - Voltage = (raw_ch * VDDA) / 4095
 *
 * Hardware:
 *   - STM32F103C8T6 (Blue Pill)
 *   - Voltage divider: VBAT → R1(100k) → PA1 → R2(100k) → GND
 *   - LED PC13 / UART1 (PA9/PA10)
 *
 * ================================================================================
 */

#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include <string.h>
#include <stdio.h>

UART_HandleTypeDef huart1;
ADC_HandleTypeDef hadc1;

/* Baterai Li-Ion thresholds (mV, setelah voltage divider /2) */
#define BAT_FULL_MV     2100    /* 4.2V / 2 */
#define BAT_EMPTY_MV    1500    /* 3.0V / 2 */

static void UART_SendString(const char *str)
{
    HAL_UART_Transmit(&huart1, (uint8_t *)str, strlen(str), HAL_MAX_DELAY);
}

static void UART_Printf(const char *fmt, ...)
{
    char buf[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    UART_SendString(buf);
}

/* ================================================================================
 * KONFIGURASI HARDWARE
 * ================================================================================ */

static void SystemClock_Config(void)
{
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

    /* ADC clock = APB2/6 = 12MHz (max 14MHz) */
    RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
    PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6;
    HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit);
}

static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    /* LED PC13 */
    GPIO_InitStruct.Pin = GPIO_PIN_13;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);

    /* PA1 = Analog input untuk ADC */
    GPIO_InitStruct.Pin = GPIO_PIN_1;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

static void MX_USART1_UART_Init(void)
{
    __HAL_RCC_USART1_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
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

static void MX_ADC1_Init(void)
{
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

/* ================================================================================
 * ADC READING FUNCTIONS
 * ================================================================================ */

static uint32_t ADC_ReadChannel(uint32_t channel)
{
    ADC_ChannelConfTypeDef sConfig = {0};
    sConfig.Channel = channel;
    sConfig.Rank = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLETIME_239CYCLES_5;
    HAL_ADC_ConfigChannel(&hadc1, &sConfig);

    HAL_ADC_Start(&hadc1);
    HAL_ADC_PollForConversion(&hadc1, 100);
    uint32_t raw = HAL_ADC_GetValue(&hadc1);
    HAL_ADC_Stop(&hadc1);

    return raw;
}

static uint32_t ADC_ReadAverage(uint32_t channel, uint8_t samples)
{
    uint32_t sum = 0;
    for (uint8_t i = 0; i < samples; i++) {
        sum += ADC_ReadChannel(channel);
    }
    return sum / samples;
}

static uint32_t get_vdda_mv(void)
{
    /* Baca VREFINT (channel 17) */
    uint32_t vrefint_raw = ADC_ReadAverage(ADC_CHANNEL_VREFINT, 16);
    /* VDDA = 1200mV * 4095 / vrefint_raw */
    if (vrefint_raw == 0) return 3300;
    return (1200UL * 4095UL) / vrefint_raw;
}

static uint32_t get_channel_mv(uint32_t channel, uint32_t vdda_mv)
{
    uint32_t raw = ADC_ReadAverage(channel, 16);
    return (raw * vdda_mv) / 4095;
}

static uint8_t mv_to_percent(uint32_t mv)
{
    if (mv >= BAT_FULL_MV) return 100;
    if (mv <= BAT_EMPTY_MV) return 0;
    return (uint8_t)((mv - BAT_EMPTY_MV) * 100 / (BAT_FULL_MV - BAT_EMPTY_MV));
}

static void print_bar(uint8_t percent)
{
    char bar[22];
    uint8_t filled = percent / 5;  /* 0-20 */
    for (int i = 0; i < 20; i++) {
        bar[i] = (i < filled) ? '#' : '-';
    }
    bar[20] = '\0';
    UART_Printf("  [%s] %d%%\r\n", bar, percent);
}

/* ================================================================================
 * FREERTOS TASK
 * ================================================================================ */

static void BatteryMonitorTask(void *pvParameters)
{
    UART_SendString("\r\n");
    UART_SendString("==========================================\r\n");
    UART_SendString("  STM32 Battery Monitor ADC Demo\r\n");
    UART_SendString("  Modul 14: Power Management\r\n");
    UART_SendString("==========================================\r\n");
    UART_SendString("\r\n");
    UART_SendString("ADC Channel : PA1 (ADC1_CH1)\r\n");
    UART_SendString("Calibration : VREFINT (1.2V internal)\r\n");
    UART_SendString("Divider     : R1=100k, R2=100k (ratio /2)\r\n");
    UART_SendString("\r\n");

    uint32_t reading_num = 0;

    while (1) {
        reading_num++;

        /* Kalibrasi VDDA */
        uint32_t vdda = get_vdda_mv();

        /* Baca tegangan baterai */
        uint32_t bat_adc_mv = get_channel_mv(ADC_CHANNEL_1, vdda);
        uint32_t bat_actual_mv = bat_adc_mv * 2;  /* Kompensasi divider */
        uint8_t percent = mv_to_percent(bat_adc_mv);

        /* Tampilkan hasil */
        UART_Printf("[#%lu] Battery Monitor\r\n", reading_num);
        UART_Printf("  VDDA      : %lu mV\r\n", vdda);
        UART_Printf("  ADC (PA1) : %lu mV\r\n", bat_adc_mv);
        UART_Printf("  Battery   : %lu mV\r\n", bat_actual_mv);
        UART_Printf("  Level     : %d%%\r\n", percent);
        print_bar(percent);

        /* Status warning */
        if (percent < 20) {
            UART_SendString("  *** WARNING: BATTERY LOW! ***\r\n");
            /* LED fast blink */
            for (int i = 0; i < 6; i++) {
                HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
                vTaskDelay(pdMS_TO_TICKS(100));
            }
        } else if (percent < 40) {
            UART_SendString("  * Battery getting low\r\n");
            HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
            vTaskDelay(pdMS_TO_TICKS(300));
            HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
        } else {
            /* Single LED pulse = OK */
            HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
            vTaskDelay(pdMS_TO_TICKS(50));
            HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
        }

        UART_SendString("\r\n");
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

/* ================================================================================
 * MAIN
 * ================================================================================ */

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USART1_UART_Init();
    MX_ADC1_Init();

    xTaskCreate(BatteryMonitorTask, "BatMon", 512, NULL,
                tskIDLE_PRIORITY + 2, NULL);

    vTaskStartScheduler();
    while (1) {}
}

/* ================================================================================
 * EXCEPTION HANDLERS & FREERTOS HOOKS
 * ================================================================================ */

void NMI_Handler(void) {}
void HardFault_Handler(void) { while (1) {} }
void MemManage_Handler(void) { while (1) {} }
void BusFault_Handler(void) { while (1) {} }
void UsageFault_Handler(void) { while (1) {} }
void DebugMon_Handler(void) {}

void SysTick_Handler(void)
{
    HAL_IncTick();
    if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED) {
        xPortSysTickHandler();
    }
}

void vApplicationMallocFailedHook(void) { while (1) {} }
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask; (void)pcTaskName; while (1) {}
}

static StaticTask_t xIdleTaskTCB;
static StackType_t uxIdleTaskStack[configMINIMAL_STACK_SIZE];
void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer,
                                   StackType_t **ppxIdleTaskStackBuffer,
                                   uint32_t *pulIdleTaskStackSize)
{
    *ppxIdleTaskTCBBuffer = &xIdleTaskTCB;
    *ppxIdleTaskStackBuffer = uxIdleTaskStack;
    *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}

static StaticTask_t xTimerTaskTCB;
static StackType_t uxTimerTaskStack[configTIMER_TASK_STACK_DEPTH];
void vApplicationGetTimerTaskMemory(StaticTask_t **ppxTimerTaskTCBBuffer,
                                    StackType_t **ppxTimerTaskStackBuffer,
                                    uint32_t *pulTimerTaskStackSize)
{
    *ppxTimerTaskTCBBuffer = &xTimerTaskTCB;
    *ppxTimerTaskStackBuffer = uxTimerTaskStack;
    *pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
}
