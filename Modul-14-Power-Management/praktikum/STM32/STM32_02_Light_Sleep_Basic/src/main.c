/* ============================================================================
 * STM32_02_Light_Sleep_Basic - Sleep Mode with WFI
 * ============================================================================
 * Demonstrates Sleep mode using WFI (Wait For Interrupt).
 * - Run task for 5 seconds with LED blinking
 * - Enter sleep with HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, WFI)
 * - Button press (EXTI PA0) wakes MCU
 * - LED pattern changes after wakeup (fast blink)
 * - Sleep: CPU stopped, peripherals active, ~2mA
 * ============================================================================ */
#include "config.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
/* ---- Private variables --------------------------------------------------- */
static UART_HandleTypeDef huart1;
static volatile uint8_t wakeup_flag = 0;
static volatile uint32_t sleep_count = 0;
static volatile uint32_t wake_count = 0;
static volatile uint32_t sleep_entry_tick = 0;
static volatile uint32_t total_sleep_ms = 0;
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
/* ---- GPIO Init with EXTI on PA0 ------------------------------------------ */
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
    HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET); /* LED off */
    /* Button PA0 - EXTI rising edge for wakeup */
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
/* ---- EXTI IRQ Handler ---------------------------------------------------- */
void EXTI0_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(BUTTON_PIN);
}
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == BUTTON_PIN) {
        wakeup_flag = 1;
        wake_count++;
    }
}
/* ---- LED blink patterns -------------------------------------------------- */
static void LED_SlowBlink(uint32_t duration_ms)
{
    uint32_t start = HAL_GetTick();
    while ((HAL_GetTick() - start) < duration_ms) {
        HAL_GPIO_TogglePin(LED_PORT, LED_PIN);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
static void LED_FastBlink(uint32_t duration_ms)
{
    uint32_t start = HAL_GetTick();
    while ((HAL_GetTick() - start) < duration_ms) {
        HAL_GPIO_TogglePin(LED_PORT, LED_PIN);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
/* ---- Enter Sleep Mode ---------------------------------------------------- */
static void Enter_Sleep_Mode(void)
{
    UART_Printf("[SLEEP] Entering SLEEP mode (WFI)...\r\n");
    UART_Printf("[SLEEP] CPU halted, peripherals remain active\r\n");
    UART_Printf("[SLEEP] Expected current: ~2-5 mA\r\n");
    UART_Printf("[SLEEP] Press PA0 button to wake up\r\n\r\n");
    /* Turn off LED before sleep */
    HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET);
    sleep_count++;
    wakeup_flag = 0;
    sleep_entry_tick = HAL_GetTick();
    /* Enable PWR clock */
    __HAL_RCC_PWR_CLK_ENABLE();
    /* Suspend FreeRTOS tick to avoid spurious wakeups from SysTick */
    HAL_SuspendTick();
    /* Enter Sleep Mode - WFI: any interrupt will wake */
    HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);
    /* --- MCU resumes here after wakeup --- */
    HAL_ResumeTick();
    uint32_t sleep_duration = HAL_GetTick() - sleep_entry_tick;
    total_sleep_ms += sleep_duration;
    UART_Printf("\r\n[WAKE] Woken up from SLEEP mode!\r\n");
    UART_Printf("[WAKE] Sleep duration : ~%lu ms\r\n", sleep_duration);
    UART_Printf("[WAKE] Total sleeps   : %lu\r\n", sleep_count);
    UART_Printf("[WAKE] Total wake IRQs: %lu\r\n", wake_count);
    UART_Printf("[WAKE] Total sleep ms : %lu\r\n", total_sleep_ms);
    UART_Printf("[WAKE] Clock still at 72MHz (no reconfiguration needed)\r\n\r\n");
}
/* ---- Main sleep cycle task ----------------------------------------------- */
static void SleepCycleTask(void *pvParameters)
{
    (void)pvParameters;
    UART_Printf("\r\n");
    UART_Printf("============================================\r\n");
    UART_Printf("  STM32F103 Sleep Mode (WFI) Demo\r\n");
    UART_Printf("============================================\r\n");
    UART_Printf("  Mode    : Sleep (WFI)\r\n");
    UART_Printf("  Wakeup  : EXTI PA0 Button Press\r\n");
    UART_Printf("  Current : Active ~30mA -> Sleep ~2mA\r\n");
    UART_Printf("============================================\r\n\r\n");
    while (1) {
        /* Phase 1: Active period with slow LED blink (5 seconds) */
        UART_Printf(">>> ACTIVE phase: LED slow blink for 5 sec...\r\n");
        LED_SlowBlink(5000);
        /* Phase 2: Enter sleep mode */
        Enter_Sleep_Mode();
        /* Phase 3: After wakeup - fast blink to indicate wake state */
        UART_Printf(">>> WAKEUP phase: LED fast blink for 3 sec...\r\n");
        LED_FastBlink(3000);
        UART_Printf(">>> Cycle complete. Restarting...\r\n\r\n");
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
    xTaskCreate(SleepCycleTask, "SleepCyc", MAIN_TASK_STACK_SIZE,
                NULL, MAIN_TASK_PRIORITY, NULL);
    vTaskStartScheduler();
    while (1);
    return 0;
}
