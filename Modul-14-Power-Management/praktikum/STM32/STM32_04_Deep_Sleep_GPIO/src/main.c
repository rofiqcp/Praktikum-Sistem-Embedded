/* ============================================================================
 * STM32_04_Deep_Sleep_GPIO - Stop Mode with EXTI GPIO Wakeup
 * ============================================================================
 * Demonstrates Stop mode with GPIO EXTI wakeup.
 * - Configure PA0 as EXTI with rising edge interrupt
 * - Enter Stop mode
 * - Button press generates EXTI -> wakes MCU
 * - Reconfigure clock after wakeup
 * - LED indicator: slow blink before sleep, fast blink after wake
 * ============================================================================ */
#include "config.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
/* ---- Private variables --------------------------------------------------- */
static UART_HandleTypeDef huart1;
static volatile uint8_t  exti_wakeup_flag = 0;
static volatile uint32_t stop_entry_count = 0;
static volatile uint32_t gpio_wakeup_count = 0;
static volatile uint32_t last_active_duration_ms = 0;
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
/* ---- GPIO Init with EXTI ------------------------------------------------- */
static void GPIO_Init(void)
{
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_AFIO_CLK_ENABLE();
    GPIO_InitTypeDef gpio = {0};
    /* LED PC13 - active low */
    gpio.Pin   = LED_PIN;
    gpio.Mode  = GPIO_MODE_OUTPUT_PP;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_PORT, &gpio);
    HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET);
    /* Button PA0 - EXTI rising edge */
    gpio.Pin  = BUTTON_PIN;
    gpio.Mode = GPIO_MODE_IT_RISING;
    gpio.Pull = GPIO_PULLDOWN;
    HAL_GPIO_Init(BUTTON_PORT, &gpio);
    HAL_NVIC_SetPriority(EXTI0_IRQn, 6, 0);
    HAL_NVIC_EnableIRQ(EXTI0_IRQn);
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
/* ---- EXTI0 IRQ Handler --------------------------------------------------- */
void EXTI0_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(BUTTON_PIN);
}
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == BUTTON_PIN) {
        exti_wakeup_flag = 1;
        gpio_wakeup_count++;
    }
}
/* ---- LED patterns -------------------------------------------------------- */
static void LED_SlowBlink(uint32_t count)
{
    for (uint32_t i = 0; i < count; i++) {
        HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_RESET);
        vTaskDelay(pdMS_TO_TICKS(400));
        HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET);
        vTaskDelay(pdMS_TO_TICKS(400));
    }
}
static void LED_FastBlink(uint32_t count)
{
    for (uint32_t i = 0; i < count; i++) {
        HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_RESET);
        vTaskDelay(pdMS_TO_TICKS(80));
        HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET);
        vTaskDelay(pdMS_TO_TICKS(80));
    }
}
static void LED_DoubleFlash(void)
{
    for (int i = 0; i < 2; i++) {
        HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_RESET);
        HAL_Delay(50);
        HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET);
        HAL_Delay(100);
    }
}
/* ---- Print EXTI wakeup event log ----------------------------------------- */
static void Print_Wakeup_Log(void)
{
    UART_Printf("--- GPIO Wakeup Event Log ---\r\n");
    UART_Printf("  Total STOP entries  : %lu\r\n", stop_entry_count);
    UART_Printf("  Total GPIO wakeups  : %lu\r\n", gpio_wakeup_count);
    UART_Printf("  EXTI flag           : %s\r\n",
                 exti_wakeup_flag ? "SET" : "CLEAR");
    UART_Printf("  Last active duration: %lu ms\r\n", last_active_duration_ms);
    UART_Printf("  HCLK after wake     : %lu Hz\r\n", HAL_RCC_GetHCLKFreq());
    UART_Printf("  Stack HWM           : %u words\r\n",
                 (unsigned)uxTaskGetStackHighWaterMark(NULL));
    UART_Printf("-----------------------------\r\n\r\n");
}
/* ---- GPIO Stop Mode Task ------------------------------------------------- */
static void GPIOStopTask(void *pvParameters)
{
    (void)pvParameters;
    UART_Printf("\r\n");
    UART_Printf("============================================\r\n");
    UART_Printf("  STM32F103 Stop Mode + GPIO EXTI Wakeup\r\n");
    UART_Printf("============================================\r\n");
    UART_Printf("  Mode   : STOP (Low Power Regulator)\r\n");
    UART_Printf("  Wakeup : PA0 Button (EXTI Rising Edge)\r\n");
    UART_Printf("  Current: Active ~30mA -> Stop ~20uA\r\n");
    UART_Printf("  SRAM   : Retained during Stop\r\n");
    UART_Printf("  PLL    : OFF in Stop, reconfigured on wake\r\n");
    UART_Printf("============================================\r\n\r\n");
    while (1) {
        uint32_t active_start = HAL_GetTick();
        /* Pre-sleep: slow LED blink */
        UART_Printf(">>> PRE-SLEEP: Slow blink (preparing to sleep)...\r\n");
        LED_SlowBlink(5);
        last_active_duration_ms = HAL_GetTick() - active_start;
        /* Enter Stop mode */
        stop_entry_count++;
        exti_wakeup_flag = 0;
        UART_Printf("\r\n[STOP] Entering STOP mode (cycle #%lu)\r\n",
                     stop_entry_count);
        UART_Printf("[STOP] Waiting for PA0 button press...\r\n");
        UART_Printf("[STOP] Expected current: ~20 uA\r\n\r\n");
        HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET); /* LED off */
        __HAL_RCC_PWR_CLK_ENABLE();
        HAL_SuspendTick();
        /* Enter STOP mode */
        HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);
        /* ---- MCU resumes here after EXTI wakeup ---- */
        /* CRITICAL: Reconfigure clock (PLL off in Stop mode) */
        SystemClock_Config();
        HAL_ResumeTick();
        /* Re-init UART */
        UART_Init();
        /* Double flash to confirm wakeup */
        LED_DoubleFlash();
        UART_Printf("\r\n[WAKE] Woken up from STOP mode!\r\n");
        UART_Printf("[WAKE] Source: %s\r\n",
                     exti_wakeup_flag ? "GPIO PA0 (EXTI)" : "Unknown IRQ");
        UART_Printf("[WAKE] Clock restored to 72MHz\r\n");
        Print_Wakeup_Log();
        /* Post-wakeup: fast blink */
        UART_Printf(">>> POST-WAKE: Fast blink (awake state)...\r\n\r\n");
        LED_FastBlink(15);
        UART_Printf("============================================\r\n\r\n");
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
    xTaskCreate(GPIOStopTask, "GPIOStop", MAIN_TASK_STACK_SIZE,
                NULL, MAIN_TASK_PRIORITY, NULL);
    vTaskStartScheduler();
    while (1);
    return 0;
}
