/* ============================================================================
 * STM32_05_Deep_Sleep_RTC - Standby Mode with RTC Wakeup (Deepest Sleep)
 * ============================================================================
 * Demonstrates Standby mode with RTC wakeup.
 * - Enable RTC with LSI clock
 * - Configure RTC alarm for 10-second wakeup
 * - Enter standby: HAL_PWR_EnterSTANDBYMode()
 * - Standby: ~2µA, all off except RTC + backup domain
 * - After wakeup: full reset, check PWR_FLAG_SB for standby flag
 * - Clear standby flag, clear wakeup flag
 * ============================================================================ */

#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

/* ---- Private variables --------------------------------------------------- */
static UART_HandleTypeDef huart1;
static RTC_HandleTypeDef  hrtc;
static volatile uint8_t   from_standby = 0;

#define STANDBY_WAKEUP_SECONDS  10

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
    if (len > 0) {
        HAL_UART_Transmit(&huart1, (uint8_t *)buf, (uint16_t)len, HAL_MAX_DELAY);
    }
}

/* ---- GPIO Init ----------------------------------------------------------- */
static void GPIO_Init(void)
{
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef gpio = {0};
    gpio.Pin   = LED_PIN;
    gpio.Mode  = GPIO_MODE_OUTPUT_PP;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_PORT, &gpio);
    HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET);

    gpio.Pin  = BUTTON_PIN;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_PULLDOWN;
    HAL_GPIO_Init(BUTTON_PORT, &gpio);
}

/* ---- System Clock: HSE 8MHz -> PLL -> 72MHz ------------------------------ */
static void SystemClock_Config(void)
{
    RCC_OscInitTypeDef osc = {0};
    osc.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    osc.HSEState       = RCC_HSE_ON;
    osc.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
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
}

/* ---- RTC Init with LSI --------------------------------------------------- */
static void RTC_Init(void)
{
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_RCC_BKP_CLK_ENABLE();
    HAL_PWR_EnableBkUpAccess();

    RCC_OscInitTypeDef osc = {0};
    osc.OscillatorType = RCC_OSCILLATORTYPE_LSI;
    osc.LSIState       = RCC_LSI_ON;
    osc.PLL.PLLState   = RCC_PLL_NONE;
    HAL_RCC_OscConfig(&osc);

    RCC_PeriphCLKInitTypeDef pclk = {0};
    pclk.PeriphClockSelection = RCC_PERIPHCLK_RTC;
    pclk.RTCClockSelection    = RCC_RTCCLKSOURCE_LSI;
    HAL_RCCEx_PeriphCLKConfig(&pclk);

    __HAL_RCC_RTC_ENABLE();

    hrtc.Instance          = RTC;
    hrtc.Init.AsynchPrediv = (40000 - 1); /* LSI ~40kHz */
    hrtc.Init.OutPut       = RTC_OUTPUTSOURCE_NONE;
    HAL_RTC_Init(&hrtc);
}

/* ---- Set RTC Alarm for standby wakeup ------------------------------------ */
static void RTC_SetAlarm_ForStandby(uint32_t seconds)
{
    /* Read current counter */
    uint32_t current = RTC->CNTH << 16 | RTC->CNTL;
    uint32_t alarm_val = current + seconds;

    /* Deactivate existing alarm */
    HAL_RTC_DeactivateAlarm(&hrtc, RTC_ALARM_A);

    /* Write alarm value */
    while (!(RTC->CRL & RTC_CRL_RTOFF));
    RTC->CRL |= RTC_CRL_CNF;
    RTC->ALRH = (alarm_val >> 16) & 0xFFFF;
    RTC->ALRL = alarm_val & 0xFFFF;
    RTC->CRL &= ~RTC_CRL_CNF;
    while (!(RTC->CRL & RTC_CRL_RTOFF));

    /* Enable alarm interrupt */
    __HAL_RTC_ALARM_ENABLE_IT(&hrtc, RTC_IT_ALRA);

    /* EXTI line 17 for RTC Alarm */
    EXTI->IMR  |= (1 << 17);
    EXTI->RTSR |= (1 << 17);
    EXTI->PR    = (1 << 17);

    UART_Printf("[RTC] Alarm set: current=%lu, alarm=%lu (+%lu sec)\r\n",
                current, alarm_val, seconds);
}

/* ---- Check standby flag -------------------------------------------------- */
static uint8_t Check_Standby_Flag(void)
{
    __HAL_RCC_PWR_CLK_ENABLE();

    if (__HAL_PWR_GET_FLAG(PWR_FLAG_SB) != RESET) {
        /* Clear standby flag */
        __HAL_PWR_CLEAR_FLAG(PWR_FLAG_SB);
        /* Clear wakeup flag */
        __HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);
        return 1;
    }
    return 0;
}

/* ---- LED patterns -------------------------------------------------------- */
static void LED_Blink(uint32_t count, uint32_t on_ms, uint32_t off_ms)
{
    for (uint32_t i = 0; i < count; i++) {
        HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_RESET);
        vTaskDelay(pdMS_TO_TICKS(on_ms));
        HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET);
        vTaskDelay(pdMS_TO_TICKS(off_ms));
    }
}

static void LED_TriplePulse(void)
{
    for (int i = 0; i < 3; i++) {
        HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_RESET);
        HAL_Delay(30);
        HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET);
        HAL_Delay(70);
    }
}

/* ---- Standby Mode Task --------------------------------------------------- */
static void StandbyTask(void *pvParameters)
{
    (void)pvParameters;

    UART_Printf("\r\n");
    UART_Printf("============================================\r\n");
    UART_Printf("  STM32F103 Standby Mode + RTC Wakeup\r\n");
    UART_Printf("============================================\r\n");
    UART_Printf("  Mode    : STANDBY (Deepest Sleep)\r\n");
    UART_Printf("  Wakeup  : RTC Alarm (%d seconds)\r\n", STANDBY_WAKEUP_SECONDS);
    UART_Printf("  Current : ~2 uA in Standby\r\n");
    UART_Printf("  SRAM    : NOT retained (full reset on wake)\r\n");
    UART_Printf("  Backup  : Only backup domain survives\r\n");
    UART_Printf("============================================\r\n\r\n");

    /* Check if we came from standby */
    if (from_standby) {
        UART_Printf("!!! WAKEUP FROM STANDBY DETECTED !!!\r\n");
        UART_Printf("[WAKE] This is a full MCU reset after Standby\r\n");
        UART_Printf("[WAKE] All SRAM data was lost\r\n");
        UART_Printf("[WAKE] Only backup registers survived\r\n\r\n");

        /* Read RTC counter to show it persisted */
        __HAL_RCC_PWR_CLK_ENABLE();
        __HAL_RCC_BKP_CLK_ENABLE();
        HAL_PWR_EnableBkUpAccess();
        uint32_t rtc_cnt = RTC->CNTH << 16 | RTC->CNTL;
        UART_Printf("[RTC] Counter value: %lu (survived standby)\r\n\r\n", rtc_cnt);

        /* Wakeup indication: rapid flash */
        LED_TriplePulse();
        UART_Printf(">>> Wakeup LED pattern: triple pulse\r\n\r\n");
    } else {
        UART_Printf("[BOOT] Normal boot (not from Standby)\r\n\r\n");
    }

    /* Active phase: show we're alive */
    UART_Printf(">>> ACTIVE phase: LED blink for 5 seconds...\r\n");
    LED_Blink(5, 400, 400);

    UART_Printf("[INFO] HCLK: %lu Hz\r\n", HAL_RCC_GetHCLKFreq());
    UART_Printf("[INFO] FreeRTOS ticks: %lu\r\n\r\n", xTaskGetTickCount());

    /* Prepare for Standby */
    UART_Printf("[STANDBY] Preparing to enter STANDBY mode...\r\n");

    /* Init RTC and set alarm */
    RTC_Init();
    RTC_SetAlarm_ForStandby(STANDBY_WAKEUP_SECONDS);

    UART_Printf("[STANDBY] RTC alarm configured\r\n");
    UART_Printf("[STANDBY] ALL peripherals will be OFF\r\n");
    UART_Printf("[STANDBY] SRAM will be LOST\r\n");
    UART_Printf("[STANDBY] Expected current: ~2 uA\r\n");
    UART_Printf("[STANDBY] MCU will perform full reset on wakeup\r\n");
    UART_Printf("[STANDBY] Entering STANDBY now...\r\n\r\n");

    /* Brief pause for UART to flush */
    vTaskDelay(pdMS_TO_TICKS(100));

    /* Turn off LED */
    HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET);

    __HAL_RCC_PWR_CLK_ENABLE();

    /* Clear wakeup flag before entering standby */
    __HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);

    /* Enter STANDBY mode - this is a one-way trip until wakeup/reset */
    HAL_PWR_EnterSTANDBYMode();

    /* Should never reach here - MCU resets after Standby wakeup */
    UART_Printf("!!! ERROR: Should not reach here after Standby!\r\n");
    while (1);
}

/* ---- FreeRTOS hooks ------------------------------------------------------ */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    UART_Printf("!!! Stack overflow: %s\r\n", pcTaskName);
    while (1);
}

void vApplicationMallocFailedHook(void)
{
    UART_Printf("!!! Malloc failed!\r\n");
    while (1);
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

    /* Check standby flag BEFORE GPIO init (early detection) */
    from_standby = Check_Standby_Flag();

    GPIO_Init();
    UART_Init();

    xTaskCreate(StandbyTask, "Standby", MAIN_TASK_STACK_SIZE,
                NULL, MAIN_TASK_PRIORITY, NULL);

    vTaskStartScheduler();

    while (1);
    return 0;
}
