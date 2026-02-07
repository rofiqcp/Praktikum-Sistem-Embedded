/**
 * ================================================================================
 * PROGRAM 9: ADAPTIVE DUTY CYCLE STATE MACHINE - STM32F103C8T6
 * ================================================================================
 * Deskripsi:
 *   Program lengkap yang menggabungkan semua teknik power management
 *   ke dalam state machine untuk adaptive duty cycling:
 *
 *   State Machine:
 *   ┌─────────┐    ┌──────────┐    ┌───────────┐    ┌──────────┐
 *   │  INIT   │───>│  SENSE   │───>│  PROCESS  │───>│  SLEEP   │
 *   └─────────┘    └──────────┘    └───────────┘    └──────────┘
 *                       ▲                                  │
 *                       └──────────────────────────────────┘
 *                              (wake-up timer/button)
 *
 *   - INIT    : Konfigurasi, baca backup register
 *   - SENSE   : Baca sensor (ADC), baca baterai
 *   - PROCESS : Tentukan sleep duration, simpan data
 *   - SLEEP   : Masuk low-power mode yang sesuai
 *
 *   Mode sleep dipilih berdasarkan baterai:
 *   - >60% : Sleep Mode (cepat bangun, ~2mA)
 *   - 30-60%: Stop Mode (hemat, ~20µA)
 *   - <30% : Standby Mode (ultra hemat, ~2µA)
 *
 * Hardware:
 *   - STM32F103C8T6 (Blue Pill)
 *   - LED PC13, Button PA0
 *   - UART1 (PA9/PA10)
 *   - ADC PA1 (battery monitor)
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
RTC_HandleTypeDef hrtc;

/* State machine states */
typedef enum {
    STATE_INIT,
    STATE_SENSE,
    STATE_PROCESS,
    STATE_SLEEP
} system_state_t;

/* Sleep mode selection */
typedef enum {
    SLEEP_NORMAL,       /* HAL_PWR_EnterSLEEPMode */
    SLEEP_STOP,         /* HAL_PWR_EnterSTOPMode */
    SLEEP_STANDBY       /* HAL_PWR_EnterSTANDBYMode */
} sleep_mode_t;

/* Backup register addresses */
#define BKP_BOOT_COUNT  RTC_BKP_DR1
#define BKP_CYCLE_COUNT RTC_BKP_DR2
#define BKP_LAST_BAT    RTC_BKP_DR3
#define BKP_MAGIC       RTC_BKP_DR4
#define MAGIC_VAL       0xCAFE

/* System data */
static volatile system_state_t current_state = STATE_INIT;
static volatile uint8_t wakeup_from_button = 0;
static uint32_t boot_count = 0;
static uint32_t cycle_count = 0;
static uint32_t battery_mv = 0;
static uint8_t battery_pct = 0;
static uint32_t sensor_value = 0;
static sleep_mode_t selected_sleep = SLEEP_NORMAL;
static uint32_t sleep_duration_ms = 5000;

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
    RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE | RCC_OSCILLATORTYPE_LSI;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    RCC_OscInitStruct.LSIState = RCC_LSI_ON;
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

    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_RTC | RCC_PERIPHCLK_ADC;
    PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_LSI;
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

    /* Button PA0 EXTI */
    GPIO_InitStruct.Pin = GPIO_PIN_0;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* PA1 = ADC analog */
    GPIO_InitStruct.Pin = GPIO_PIN_1;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    HAL_NVIC_SetPriority(EXTI0_IRQn, 6, 0);
    HAL_NVIC_EnableIRQ(EXTI0_IRQn);
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
    HAL_ADCEx_Calibration_Start(&hadc1);
}

static void MX_RTC_Init(void)
{
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_RCC_BKP_CLK_ENABLE();
    HAL_PWR_EnableBkUpAccess();
    hrtc.Instance = RTC;
    hrtc.Init.AsynchPrediv = RTC_AUTO_1_SECOND;
    hrtc.Init.OutPut = RTC_OUTPUTSOURCE_NONE;
    HAL_RTC_Init(&hrtc);
}

/* ================================================================================
 * ADC FUNCTIONS
 * ================================================================================ */

static uint32_t ADC_Read(uint32_t channel)
{
    ADC_ChannelConfTypeDef sConfig = {0};
    sConfig.Channel = channel;
    sConfig.Rank = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLETIME_239CYCLES_5;
    HAL_ADC_ConfigChannel(&hadc1, &sConfig);

    HAL_ADC_Start(&hadc1);
    HAL_ADC_PollForConversion(&hadc1, 100);
    uint32_t val = HAL_ADC_GetValue(&hadc1);
    HAL_ADC_Stop(&hadc1);
    return val;
}

/* ================================================================================
 * INTERRUPT HANDLER
 * ================================================================================ */

void EXTI0_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_0);
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == GPIO_PIN_0) {
        wakeup_from_button = 1;
    }
}

/* ================================================================================
 * STATE MACHINE FUNCTIONS
 * ================================================================================ */

static void state_init(void)
{
    UART_SendString("\r\n");
    UART_SendString("==========================================\r\n");
    UART_SendString("  STM32 Adaptive Duty Cycle\r\n");
    UART_SendString("  Modul 14: Power Management\r\n");
    UART_SendString("==========================================\r\n");
    UART_SendString("\r\n");

    /* Cek Standby wake */
    if (__HAL_PWR_GET_FLAG(PWR_FLAG_SB)) {
        UART_SendString("[INIT] Woke from STANDBY\r\n");
        __HAL_PWR_CLEAR_FLAG(PWR_FLAG_SB);
        __HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);
    }

    /* Backup register init */
    if (HAL_RTCEx_BKUPRead(&hrtc, BKP_MAGIC) != MAGIC_VAL) {
        HAL_RTCEx_BKUPWrite(&hrtc, BKP_BOOT_COUNT, 0);
        HAL_RTCEx_BKUPWrite(&hrtc, BKP_CYCLE_COUNT, 0);
        HAL_RTCEx_BKUPWrite(&hrtc, BKP_MAGIC, MAGIC_VAL);
    }

    boot_count = HAL_RTCEx_BKUPRead(&hrtc, BKP_BOOT_COUNT) + 1;
    cycle_count = HAL_RTCEx_BKUPRead(&hrtc, BKP_CYCLE_COUNT);
    HAL_RTCEx_BKUPWrite(&hrtc, BKP_BOOT_COUNT, boot_count);

    UART_Printf("[INIT] Boot #%lu, Cycles: %lu\r\n", boot_count, cycle_count);

    current_state = STATE_SENSE;
}

static void state_sense(void)
{
    cycle_count++;
    HAL_RTCEx_BKUPWrite(&hrtc, BKP_CYCLE_COUNT, cycle_count);

    UART_Printf("\r\n[SENSE] Cycle #%lu\r\n", cycle_count);

    /* Baca VREFINT untuk kalibrasi */
    uint32_t vrefint = ADC_Read(ADC_CHANNEL_VREFINT);
    uint32_t vdda = (vrefint > 0) ? (1200UL * 4095UL / vrefint) : 3300;

    /* Baca baterai (PA1) */
    uint32_t bat_raw = ADC_Read(ADC_CHANNEL_1);
    battery_mv = (bat_raw * vdda / 4095) * 2;  /* x2 for divider */
    battery_pct = (battery_mv >= 4200) ? 100 :
                  (battery_mv <= 3000) ? 0 :
                  (uint8_t)((battery_mv - 3000) * 100 / 1200);

    /* Sensor simulasi */
    sensor_value = ADC_Read(ADC_CHANNEL_TEMPSENSOR);

    UART_Printf("  Battery : %lu mV (%d%%)\r\n", battery_mv, battery_pct);
    UART_Printf("  Sensor  : %lu (raw)\r\n", sensor_value);

    /* LED pulse */
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
    vTaskDelay(pdMS_TO_TICKS(50));
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);

    current_state = STATE_PROCESS;
}

static void state_process(void)
{
    UART_SendString("\r\n[PROCESS] Determining sleep strategy...\r\n");

    HAL_RTCEx_BKUPWrite(&hrtc, BKP_LAST_BAT, (uint16_t)(battery_mv & 0xFFFF));

    /* Adaptive strategy */
    if (battery_pct > 60) {
        selected_sleep = SLEEP_NORMAL;
        sleep_duration_ms = 3000;
        UART_SendString("  Strategy: SLEEP Mode (3s)\r\n");
        UART_SendString("  Reason  : Battery high, fast response\r\n");
    } else if (battery_pct > 30) {
        selected_sleep = SLEEP_STOP;
        sleep_duration_ms = 10000;
        UART_SendString("  Strategy: STOP Mode (10s)\r\n");
        UART_SendString("  Reason  : Battery medium, save power\r\n");
    } else {
        selected_sleep = SLEEP_STANDBY;
        sleep_duration_ms = 30000;
        UART_SendString("  Strategy: STANDBY Mode (30s)\r\n");
        UART_SendString("  Reason  : Battery low, ultra save\r\n");
    }

    UART_SendString("\r\n");
    UART_SendString("  Power Mode Comparison:\r\n");
    UART_Printf("  >60%%  : Sleep  (~2mA)  %s\r\n",
                (selected_sleep == SLEEP_NORMAL) ? "<< SELECTED" : "");
    UART_Printf("  30-60%%: Stop  (~20uA) %s\r\n",
                (selected_sleep == SLEEP_STOP) ? "<< SELECTED" : "");
    UART_Printf("  <30%%  : Standby(~2uA) %s\r\n",
                (selected_sleep == SLEEP_STANDBY) ? "<< SELECTED" : "");

    current_state = STATE_SLEEP;
}

static void state_sleep(void)
{
    UART_Printf("\r\n[SLEEP] Entering %s mode...\r\n",
                (selected_sleep == SLEEP_NORMAL) ? "SLEEP" :
                (selected_sleep == SLEEP_STOP) ? "STOP" : "STANDBY");
    vTaskDelay(pdMS_TO_TICKS(50));

    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
    wakeup_from_button = 0;

    switch (selected_sleep) {
        case SLEEP_NORMAL:
            HAL_SuspendTick();
            HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);
            HAL_ResumeTick();
            UART_SendString("\r\n[WAKEUP] From Sleep mode\r\n");
            break;

        case SLEEP_STOP:
            HAL_SuspendTick();
            HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);
            SystemClock_Config();
            HAL_ResumeTick();
            MX_USART1_UART_Init();
            UART_SendString("\r\n[WAKEUP] From Stop mode\r\n");
            break;

        case SLEEP_STANDBY:
            HAL_PWR_EnableWakeUpPin(PWR_WAKEUP_PIN1);
            __HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);
            HAL_PWR_EnterSTANDBYMode();
            /* Never reaches here - MCU resets */
            break;
    }

    if (wakeup_from_button) {
        UART_SendString("[WAKEUP] Source: Button (PA0)\r\n");
    } else {
        UART_SendString("[WAKEUP] Source: Timer/Other\r\n");
    }

    current_state = STATE_SENSE;
}

/* ================================================================================
 * FREERTOS TASK
 * ================================================================================ */

static void AdaptiveTask(void *pvParameters)
{
    current_state = STATE_INIT;

    while (1) {
        switch (current_state) {
            case STATE_INIT:    state_init();    break;
            case STATE_SENSE:   state_sense();   break;
            case STATE_PROCESS: state_process(); break;
            case STATE_SLEEP:   state_sleep();   break;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
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
    MX_RTC_Init();

    __HAL_RCC_PWR_CLK_ENABLE();

    xTaskCreate(AdaptiveTask, "Adaptive", 512, NULL,
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
