/* ============================================================================
 * STM32_11_Battery_Powered_Logger - Battery Data Logger with Sleep Cycles
 * ============================================================================
 * Battery-powered data logger that uses Stop mode for low power.
 *
 * - Read battery voltage via ADC1 CH0 (PA0) — voltage divider assumed
 * - Read internal temperature sensor (ADC1 CH16)
 * - Print data as CSV format on UART
 * - Save log count in backup register
 * - Enter Stop mode for 10 seconds (RTC alarm wakeup)
 * - On wakeup: reconfigure clock, read sensors, print, sleep again
 * - Check battery voltage — if below threshold, enter Standby
 * ============================================================================ */

#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

/* ---- Private variables --------------------------------------------------- */
static UART_HandleTypeDef huart1;
static RTC_HandleTypeDef  hrtc;
static ADC_HandleTypeDef  hadc1;

/* Backup registers */
#define BKP_LOG_COUNT       RTC_BKP_DR1
#define BKP_MAGIC           RTC_BKP_DR2
#define BKP_MIN_BATTERY     RTC_BKP_DR3
#define BKP_MAX_TEMP        RTC_BKP_DR4
#define BKP_TOTAL_LOGS      RTC_BKP_DR5

#define MAGIC_VALUE         0xCAFE
#define SLEEP_SECONDS       10
#define BATTERY_LOW_MV      2800    /* 2.8V low battery threshold */
#define VREF_MV             3300    /* 3.3V reference */
#define VDIV_RATIO          2       /* Voltage divider ratio 2:1 */

/* ---- UART Printf -------------------------------------------------------- */
static void UART_Init(void)
{
    __HAL_RCC_USART1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef gpio = {0};
    gpio.Pin   = GPIO_PIN_9;
    gpio.Mode  = GPIO_MODE_AF_PP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &gpio);

    gpio.Pin  = GPIO_PIN_10;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &gpio);

    huart1.Instance          = USART1;
    huart1.Init.BaudRate     = UART_BAUDRATE;
    huart1.Init.WordLength   = UART_WORDLENGTH_8B;
    huart1.Init.StopBits     = UART_STOPBITS_1;
    huart1.Init.Parity       = UART_PARITY_NONE;
    huart1.Init.Mode         = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(&huart1);
}

static void UART_Printf(const char *fmt, ...)
{
    char buf[256];
    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    if (len > 0) HAL_UART_Transmit(&huart1, (uint8_t *)buf, len, HAL_MAX_DELAY);
}

/* ---- Clock Config -------------------------------------------------------- */
static void SystemClock_Config(void)
{
    RCC_OscInitTypeDef osc = {0};
    osc.OscillatorType = RCC_OSCILLATORTYPE_HSE | RCC_OSCILLATORTYPE_LSI;
    osc.HSEState       = RCC_HSE_ON;
    osc.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    osc.LSIState       = RCC_LSI_ON;
    osc.PLL.PLLState   = RCC_PLL_ON;
    osc.PLL.PLLSource  = RCC_PLLSOURCE_HSE;
    osc.PLL.PLLMUL     = RCC_PLL_MUL9;
    HAL_RCC_OscConfig(&osc);

    RCC_ClkInitTypeDef clk = {0};
    clk.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                          RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    clk.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    clk.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    clk.APB1CLKDivider = RCC_HCLK_DIV2;
    clk.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_2);

    /* ADC clock: must be <= 14MHz; PCLK2(72MHz)/6 = 12MHz */
    RCC_PeriphCLKInitTypeDef adc_clk = {0};
    adc_clk.PeriphClockSelection = RCC_PERIPHCLK_ADC;
    adc_clk.AdcClockSelection    = RCC_ADCPCLK2_DIV6;
    HAL_RCCEx_PeriphCLKConfig(&adc_clk);
}

/* ---- LED ----------------------------------------------------------------- */
static void LED_Init(void)
{
    __HAL_RCC_GPIOC_CLK_ENABLE();
    GPIO_InitTypeDef gpio = {0};
    gpio.Pin   = LED_PIN;
    gpio.Mode  = GPIO_MODE_OUTPUT_PP;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_PORT, &gpio);
}

/* ---- Backup Domain ------------------------------------------------------- */
static void Backup_Init(void)
{
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_RCC_BKP_CLK_ENABLE();
    HAL_PWR_EnableBkUpAccess();

    hrtc.Instance = RTC;
    hrtc.Init.AsynchPrediv = RTC_AUTO_1_SECOND;
    hrtc.Init.OutPut = RTC_OUTPUTSOURCE_NONE;
    HAL_RTC_Init(&hrtc);
}

/* ---- ADC ----------------------------------------------------------------- */
static void ADC_Init(void)
{
    __HAL_RCC_ADC1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    /* PA0 as analog input for battery voltage */
    GPIO_InitTypeDef gpio = {0};
    gpio.Pin  = GPIO_PIN_0;
    gpio.Mode = GPIO_MODE_ANALOG;
    HAL_GPIO_Init(GPIOA, &gpio);

    hadc1.Instance                   = ADC1;
    hadc1.Init.DataAlign             = ADC_DATAALIGN_RIGHT;
    hadc1.Init.ScanConvMode          = ADC_SCAN_DISABLE;
    hadc1.Init.ContinuousConvMode    = DISABLE;
    hadc1.Init.NbrOfConversion       = 1;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConv      = ADC_SOFTWARE_START;
    HAL_ADC_Init(&hadc1);

    HAL_ADCEx_Calibration_Start(&hadc1);
}

static uint16_t ADC_Read_Channel(uint32_t channel, uint32_t sample_time)
{
    ADC_ChannelConfTypeDef cfg = {0};
    cfg.Channel      = channel;
    cfg.Rank         = ADC_REGULAR_RANK_1;
    cfg.SamplingTime = sample_time;
    HAL_ADC_ConfigChannel(&hadc1, &cfg);

    HAL_ADC_Start(&hadc1);
    HAL_ADC_PollForConversion(&hadc1, 100);
    uint16_t val = HAL_ADC_GetValue(&hadc1);
    HAL_ADC_Stop(&hadc1);
    return val;
}

static uint32_t Read_Battery_mV(void)
{
    uint16_t raw = ADC_Read_Channel(ADC_CHANNEL_0, ADC_SAMPLETIME_71CYCLES_5);
    /* Convert: mV = raw * VREF / 4095 * divider_ratio */
    return (uint32_t)raw * VREF_MV / 4095 * VDIV_RATIO;
}

static int32_t Read_Temperature_x10(void)
{
    /* Internal temp sensor on CH16 */
    uint16_t raw = ADC_Read_Channel(ADC_CHANNEL_TEMPSENSOR, ADC_SAMPLETIME_239CYCLES_5);
    /* V_sense = raw * 3300 / 4095 mV
     * Temp = (V25 - V_sense) / Avg_slope + 25
     * V25 ~ 1430 mV, Avg_slope ~ 4.3 mV/°C */
    int32_t v_sense = (int32_t)raw * 3300 / 4095;
    int32_t temp_x10 = ((1430 - v_sense) * 10) / 43 + 250; /* x10 for 1 decimal */
    return temp_x10;
}

/* ---- RTC Alarm Wakeup ---------------------------------------------------- */
static void Setup_RTC_Alarm_Wakeup(uint32_t seconds)
{
    /* Read current RTC counter and set alarm seconds ahead */
    uint32_t counter = RTC->CNTH;
    counter = (counter << 16) | RTC->CNTL;
    uint32_t alarm_val = counter + seconds;

    /* Wait for RTC registers sync */
    while ((RTC->CRL & RTC_CRL_RTOFF) == 0);
    RTC->CRL |= RTC_CRL_CNF;   /* Enter config mode */
    RTC->ALRH = (alarm_val >> 16) & 0xFFFF;
    RTC->ALRL = alarm_val & 0xFFFF;
    RTC->CRL &= ~RTC_CRL_CNF;  /* Exit config mode */
    while ((RTC->CRL & RTC_CRL_RTOFF) == 0);

    /* Enable RTC Alarm interrupt for EXTI line 17 (wakeup from Stop) */
    EXTI->IMR  |= (1 << 17);
    EXTI->RTSR |= (1 << 17);
    RTC->CRH   |= RTC_CRH_ALRIE;
    HAL_NVIC_SetPriority(RTC_Alarm_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(RTC_Alarm_IRQn);
}

void RTC_Alarm_IRQHandler(void)
{
    if (EXTI->PR & (1 << 17)) {
        EXTI->PR = (1 << 17);  /* Clear pending */
    }
    RTC->CRL &= ~RTC_CRL_ALRF; /* Clear alarm flag */
}

/* ---- Logger Task --------------------------------------------------------- */
static void vLoggerTask(void *pvParameters)
{
    (void)pvParameters;

    uint32_t log_count = 0;
    uint32_t total_logs = 0;
    uint32_t min_battery = 0xFFFF;
    int32_t  max_temp = -400;

    /* Load backup data */
    if (HAL_RTCEx_BKUPRead(&hrtc, BKP_MAGIC) == MAGIC_VALUE) {
        log_count   = HAL_RTCEx_BKUPRead(&hrtc, BKP_LOG_COUNT);
        total_logs  = HAL_RTCEx_BKUPRead(&hrtc, BKP_TOTAL_LOGS);
        min_battery = HAL_RTCEx_BKUPRead(&hrtc, BKP_MIN_BATTERY);
        max_temp    = (int32_t)HAL_RTCEx_BKUPRead(&hrtc, BKP_MAX_TEMP);
    }

    UART_Printf("\r\n========================================\r\n");
    UART_Printf("  STM32 Battery-Powered Data Logger\r\n");
    UART_Printf("========================================\r\n\r\n");

    /* Print CSV header on first boot */
    if (log_count == 0) {
        UART_Printf("# Battery Logger - CSV Output\r\n");
        UART_Printf("# Format: log_num, battery_mV, temp_C, status\r\n");
        UART_Printf("log,battery_mV,temp_x10C,status\r\n");
    }

    /* Main logging loop */
    for (int cycle = 0; cycle < 5; cycle++) {
        log_count++;
        total_logs++;

        /* Read sensors */
        ADC_Init();
        uint32_t batt_mv = Read_Battery_mV();
        int32_t  temp_x10 = Read_Temperature_x10();

        /* Update min/max */
        if (batt_mv < min_battery) min_battery = batt_mv;
        if (temp_x10 > max_temp)   max_temp = temp_x10;

        /* Determine status */
        const char *status = "OK";
        if (batt_mv < BATTERY_LOW_MV) status = "LOW_BATT";
        if (temp_x10 > 500)           status = "HIGH_TEMP";

        /* CSV output */
        UART_Printf("%lu,%lu,%ld,%s\r\n", log_count, batt_mv, temp_x10, status);

        /* LED flash */
        HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_RESET);
        vTaskDelay(pdMS_TO_TICKS(100));
        HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET);

        /* Check for low battery */
        if (batt_mv < BATTERY_LOW_MV && batt_mv > 100) {
            UART_Printf("# CRITICAL: Battery low (%lu mV) - entering Standby!\r\n", batt_mv);
            HAL_RTCEx_BKUPWrite(&hrtc, BKP_MAGIC, MAGIC_VALUE);
            HAL_RTCEx_BKUPWrite(&hrtc, BKP_LOG_COUNT, log_count);
            HAL_RTCEx_BKUPWrite(&hrtc, BKP_TOTAL_LOGS, total_logs);
            vTaskDelay(pdMS_TO_TICKS(100));
            HAL_PWR_EnterSTANDBYMode();
        }

        /* Save to backup registers */
        HAL_RTCEx_BKUPWrite(&hrtc, BKP_MAGIC, MAGIC_VALUE);
        HAL_RTCEx_BKUPWrite(&hrtc, BKP_LOG_COUNT, log_count);
        HAL_RTCEx_BKUPWrite(&hrtc, BKP_TOTAL_LOGS, total_logs);
        HAL_RTCEx_BKUPWrite(&hrtc, BKP_MIN_BATTERY, min_battery);
        HAL_RTCEx_BKUPWrite(&hrtc, BKP_MAX_TEMP, (uint32_t)max_temp);

        /* Print summary every 5 logs */
        if (log_count % 5 == 0) {
            UART_Printf("# --- Summary after %lu logs ---\r\n", log_count);
            UART_Printf("# Min battery: %lu mV\r\n", min_battery);
            UART_Printf("# Max temp:    %ld.%ld C\r\n", max_temp / 10, max_temp % 10);
            UART_Printf("# Total logs:  %lu\r\n", total_logs);
        }

        /* Disable ADC before sleep */
        __HAL_RCC_ADC1_CLK_DISABLE();

        /* Setup RTC alarm for wakeup */
        UART_Printf("# Sleeping %d seconds (Stop mode)...\r\n", SLEEP_SECONDS);
        vTaskDelay(pdMS_TO_TICKS(50));

        Setup_RTC_Alarm_Wakeup(SLEEP_SECONDS);

        /* Enter Stop mode */
        HAL_SuspendTick();
        HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);

        /* Woke up from Stop — reconfigure clocks */
        SystemClock_Config();
        HAL_ResumeTick();
        UART_Init();
    }

    UART_Printf("\r\n# Cycle complete. Restarting in 5 seconds...\r\n\r\n");
    vTaskDelay(pdMS_TO_TICKS(5000));

    NVIC_SystemReset();

    for (;;) { vTaskDelay(pdMS_TO_TICKS(1000)); }
}

/* ---- FreeRTOS Hooks ------------------------------------------------------ */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask; UART_Printf("[FATAL] Stack overflow: %s\r\n", pcTaskName); for (;;);
}
void vApplicationMallocFailedHook(void)
{
    UART_Printf("[FATAL] Malloc failed!\r\n"); for (;;);
}
static StaticTask_t xIdleTaskTCB;
static StackType_t  uxIdleTaskStack[configMINIMAL_STACK_SIZE];
void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer,
                                   StackType_t **ppxIdleTaskStackBuffer,
                                   uint32_t *pulIdleTaskStackSize)
{
    *ppxIdleTaskTCBBuffer   = &xIdleTaskTCB;
    *ppxIdleTaskStackBuffer = uxIdleTaskStack;
    *pulIdleTaskStackSize   = configMINIMAL_STACK_SIZE;
}
static StaticTask_t xTimerTaskTCB;
static StackType_t  uxTimerTaskStack[configTIMER_TASK_STACK_DEPTH];
void vApplicationGetTimerTaskMemory(StaticTask_t **ppxTimerTaskTCBBuffer,
                                    StackType_t **ppxTimerTaskStackBuffer,
                                    uint32_t *pulTimerTaskStackSize)
{
    *ppxTimerTaskTCBBuffer   = &xTimerTaskTCB;
    *ppxTimerTaskStackBuffer = uxTimerTaskStack;
    *pulTimerTaskStackSize   = configTIMER_TASK_STACK_DEPTH;
}

/* ---- Main ---------------------------------------------------------------- */
int main(void)
{
    HAL_Init();
    SystemClock_Config();
    LED_Init();
    Backup_Init();
    UART_Init();

    xTaskCreate(vLoggerTask, "LOGGER", MAIN_TASK_STACK_SIZE * 2, NULL,
                MAIN_TASK_PRIORITY, NULL);
    vTaskStartScheduler();
    for (;;);
}
