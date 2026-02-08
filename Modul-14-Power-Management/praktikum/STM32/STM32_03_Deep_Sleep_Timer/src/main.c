/* ============================================================================
 * STM32_03_Deep_Sleep_Timer - Stop Mode with RTC Alarm Wakeup
 * ============================================================================
 * Demonstrates Stop mode with RTC alarm wakeup.
 * - Configure RTC alarm to wake in 10 seconds
 * - Enter Stop mode: HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, WFI)
 * - After wakeup: reconfigure system clock (PLL is off in Stop mode!)
 * - Stop mode: ~20µA, SRAM retained, PLL off
 * ============================================================================ */
#include "config.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
/* ---- Private variables --------------------------------------------------- */
static UART_HandleTypeDef huart1;
static RTC_HandleTypeDef  hrtc;
static volatile uint32_t stop_count = 0;
static volatile uint32_t rtc_alarm_flag = 0;
static volatile uint32_t wakeup_source = 0; /* 0=unknown, 1=RTC, 2=other */
#define RTC_WAKEUP_SECONDS  10
/* ---- UART Debug (PA9 TX, PA10 RX) ---- */
static void UART_Init(void)
{
    __HAL_RCC_USART1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    GPIO_InitTypeDef g = {0};
#ifdef STM32F103xB
    g.Pin   = GPIO_PIN_9;
    g.Mode  = GPIO_MODE_AF_PP;
    g.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &g);
    g.Pin   = GPIO_PIN_10;
    g.Mode  = GPIO_MODE_INPUT;
    g.Pull  = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &g);
#elif defined(STM32F401xC) || defined(STM32F411xE)
    g.Pin       = GPIO_PIN_9 | GPIO_PIN_10;
    g.Mode      = GPIO_MODE_AF_PP;
    g.Pull      = GPIO_PULLUP;
    g.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
    g.Alternate = GPIO_AF7_USART1;
    HAL_GPIO_Init(GPIOA, &g);
#endif
    huart1.Instance          = USART1;
    huart1.Init.BaudRate     = 115200;
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
/* ============================================================
 *  System Clock Configuration (Multi-platform)
 * ============================================================ */
#ifdef STM32F103xB
/* F103: 8 MHz HSE -> PLL x9 -> 72 MHz SYSCLK */
static void SystemClock_Config(void)
{
    RCC_OscInitTypeDef osc = {0};
    RCC_ClkInitTypeDef clk = {0};
    osc.OscillatorType      = RCC_OSCILLATORTYPE_HSE;
    osc.HSEState            = RCC_HSE_ON;
    osc.HSEPredivValue      = RCC_HSE_PREDIV_DIV1;
    osc.PLL.PLLState        = RCC_PLL_ON;
    osc.PLL.PLLSource       = RCC_PLLSOURCE_HSE;
    osc.PLL.PLLMUL          = RCC_PLL_MUL9;
    HAL_RCC_OscConfig(&osc);
    clk.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                       | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    clk.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    clk.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    clk.APB1CLKDivider = RCC_HCLK_DIV2;
    clk.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_2);
}
#elif defined(STM32F401xC) || defined(STM32F411xE)
/* F4xx: 25 MHz HSE -> PLL -> 84 MHz SYSCLK */
static void SystemClock_Config(void)
{
    RCC_OscInitTypeDef osc = {0};
    RCC_ClkInitTypeDef clk = {0};
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);
    osc.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    osc.HSEState       = RCC_HSE_ON;
    osc.PLL.PLLState   = RCC_PLL_ON;
    osc.PLL.PLLSource  = RCC_PLLSOURCE_HSE;
    osc.PLL.PLLM       = 25;
    osc.PLL.PLLN       = 336;
    osc.PLL.PLLP       = RCC_PLLP_DIV4;   /* 336/4 = 84 MHz */
    osc.PLL.PLLQ       = 7;
    HAL_RCC_OscConfig(&osc);
    clk.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                       | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    clk.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    clk.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    clk.APB1CLKDivider = RCC_HCLK_DIV2;
    clk.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_2);
}
#endif
/* ---- RTC Init with LSI --------------------------------------------------- */
static void RTC_Init(void)
{
    __HAL_RCC_PWR_CLK_ENABLE();
    #ifdef STM32F103xB
    __HAL_RCC_BKP_CLK_ENABLE();
    #endif
    HAL_PWR_EnableBkUpAccess();
    /* Enable LSI oscillator */
    RCC_OscInitTypeDef osc = {0};
    osc.OscillatorType = RCC_OSCILLATORTYPE_LSI;
    osc.LSIState       = RCC_LSI_ON;
    osc.PLL.PLLState   = RCC_PLL_NONE;
    HAL_RCC_OscConfig(&osc);
    /* Select LSI as RTC clock */
    RCC_PeriphCLKInitTypeDef pclk = {0};
    pclk.PeriphClockSelection = RCC_PERIPHCLK_RTC;
    pclk.RTCClockSelection    = RCC_RTCCLKSOURCE_LSI;
    HAL_RCCEx_PeriphCLKConfig(&pclk);
    __HAL_RCC_RTC_ENABLE();
    hrtc.Instance            = RTC;
    hrtc.Init.AsynchPrediv   = (40000 - 1); /* LSI ~40kHz -> 1Hz */
    hrtc.Init.OutPut         = RTC_OUTPUTSOURCE_NONE;
    HAL_RTC_Init(&hrtc);
    /* Enable RTC Alarm interrupt via EXTI line 17 */
    HAL_NVIC_SetPriority(RTC_Alarm_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(RTC_Alarm_IRQn);
}
/* ---- Set RTC Alarm in N seconds ------------------------------------------ */
static void RTC_SetAlarm_Seconds(uint32_t seconds)
{
    uint32_t current_counter = RTC->CNTH << 16 | RTC->CNTL;
    uint32_t alarm_value = current_counter + seconds;
    RTC_AlarmTypeDef alarm = {0};
    alarm.AlarmTime.Hours   = 0;
    alarm.AlarmTime.Minutes = 0;
    alarm.AlarmTime.Seconds = 0;
    HAL_RTC_DeactivateAlarm(&hrtc, RTC_ALARM_A);
    /* Write alarm directly to registers */
    while (!(RTC->CRL & RTC_CRL_RTOFF));
    RTC->CRL |= RTC_CRL_CNF;
    RTC->ALRH = (alarm_value >> 16) & 0xFFFF;
    RTC->ALRL = alarm_value & 0xFFFF;
    RTC->CRL &= ~RTC_CRL_CNF;
    while (!(RTC->CRL & RTC_CRL_RTOFF));
    /* Enable alarm interrupt */
    __HAL_RTC_ALARM_ENABLE_IT(&hrtc, RTC_IT_ALRA);
    /* Configure EXTI Line 17 for RTC Alarm */
    EXTI->IMR  |= (1 << 17);
    EXTI->RTSR |= (1 << 17);
    EXTI->PR    = (1 << 17); /* Clear pending */
    UART_Printf("[RTC] Alarm set for %lu seconds from now\r\n", seconds);
    UART_Printf("[RTC] Current counter: %lu, Alarm at: %lu\r\n",
                current_counter, alarm_value);
}
/* ---- RTC Alarm IRQ Handler ----------------------------------------------- */
void RTC_Alarm_IRQHandler(void)
{
    if (__HAL_RTC_ALARM_GET_IT_SOURCE(&hrtc, RTC_IT_ALRA)) {
        __HAL_RTC_ALARM_CLEAR_FLAG(&hrtc, RTC_FLAG_ALRAF);
        EXTI->PR = (1 << 17);
        rtc_alarm_flag = 1;
        wakeup_source = 1;
    }
}
/* ---- LED Blink ----------------------------------------------------------- */
static void LED_Blink(uint32_t count, uint32_t delay_ms)
{
    for (uint32_t i = 0; i < count; i++) {
        HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_RESET); /* ON */
        vTaskDelay(pdMS_TO_TICKS(delay_ms));
        HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET);   /* OFF */
        vTaskDelay(pdMS_TO_TICKS(delay_ms));
    }
}
/* ---- Stop Mode Task ------------------------------------------------------ */
static void StopModeTask(void *pvParameters)
{
    (void)pvParameters;
    UART_Printf("\r\n");
    UART_Printf("============================================\r\n");
    UART_Printf("  STM32F103 Stop Mode + RTC Alarm Wakeup\r\n");
    UART_Printf("============================================\r\n");
    UART_Printf("  Mode   : STOP (Low Power Regulator)\r\n");
    UART_Printf("  Wakeup : RTC Alarm (%d seconds)\r\n", RTC_WAKEUP_SECONDS);
    UART_Printf("  Current: Active ~30mA -> Stop ~20uA\r\n");
    UART_Printf("  Note   : PLL off in Stop, must reconfigure!\r\n");
    UART_Printf("============================================\r\n\r\n");
    RTC_Init();
    while (1) {
        /* Active phase: slow blink */
        UART_Printf(">>> ACTIVE phase: slow blink for 3 sec...\r\n");
        LED_Blink(3, 500);
        /* Prepare for Stop mode */
        stop_count++;
        rtc_alarm_flag = 0;
        wakeup_source = 0;
        UART_Printf("\r\n[STOP] Entering STOP mode (cycle #%lu)\r\n", stop_count);
        UART_Printf("[STOP] SRAM retained, PLL will be OFF\r\n");
        UART_Printf("[STOP] Expected current: ~20 uA\r\n");
        /* Set RTC alarm for wakeup */
        RTC_SetAlarm_Seconds(RTC_WAKEUP_SECONDS);
        /* Turn off LED */
        HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET);
        /* Suspend tick */
        HAL_SuspendTick();
        /* Enter STOP mode with low power regulator */
        HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);
        /* ---- MCU resumes here after wakeup ---- */
        /* CRITICAL: Reconfigure system clock! PLL is off after Stop mode */
        SystemClock_Config();
        HAL_ResumeTick();
        /* Re-init UART (clock changed) */
        UART_Init();
        UART_Printf("\r\n[WAKE] Woken up from STOP mode!\r\n");
        UART_Printf("[WAKE] Clock reconfigured to 72MHz\r\n");
        UART_Printf("[WAKE] Source: %s\r\n",
                     rtc_alarm_flag ? "RTC Alarm" : "Unknown/Other IRQ");
        UART_Printf("[WAKE] Stop cycles: %lu\r\n", stop_count);
        UART_Printf("[WAKE] HCLK: %lu Hz\r\n\r\n", HAL_RCC_GetHCLKFreq());
        /* Wakeup indication: fast blink */
        UART_Printf(">>> WAKEUP phase: fast blink for 2 sec...\r\n\r\n");
        LED_Blink(10, 100);
        UART_Printf("--------------------------------------------\r\n\r\n");
    }
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
    GPIO_Init();
    UART_Init();
    __HAL_RCC_PWR_CLK_ENABLE();
    xTaskCreate(StopModeTask, "StopMode", MAIN_TASK_STACK_SIZE,
                NULL, MAIN_TASK_PRIORITY, NULL);
    vTaskStartScheduler();
    while (1);
    return 0;
}
