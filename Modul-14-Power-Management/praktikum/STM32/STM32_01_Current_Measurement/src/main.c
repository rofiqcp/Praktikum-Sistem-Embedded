/* ============================================================================
 * STM32_01_Current_Measurement - Power Baseline at 72MHz
 * ============================================================================
 * Measure active current consumption, establish baseline at 72MHz.
 * Toggle LED, print SysTick count and HCLK frequency periodically.
 * Instructions for inline multimeter measurement.
 * Expected: ~30mA at 72MHz active mode.
 * ============================================================================ */

#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

/* ---- Private variables --------------------------------------------------- */
static UART_HandleTypeDef huart1;
static volatile uint32_t tick_count = 0;
static volatile uint32_t led_toggle_count = 0;
static volatile uint32_t report_cycle = 0;

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

/* ---- LED & Button Init --------------------------------------------------- */
static void GPIO_Init(void)
{
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef gpio = {0};
    /* LED PC13 - active low */
    gpio.Pin   = LED_PIN;
    gpio.Mode  = GPIO_MODE_OUTPUT_PP;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_PORT, &gpio);
    HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET); /* LED off */

    /* Button PA0 - input pull-down */
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

/* ---- Print power measurement instructions -------------------------------- */
static void Print_Measurement_Guide(void)
{
    UART_Printf("\r\n");
    UART_Printf("============================================\r\n");
    UART_Printf("  STM32F103 Current Measurement Baseline\r\n");
    UART_Printf("============================================\r\n");
    UART_Printf("\r\n");
    UART_Printf("--- HOW TO MEASURE CURRENT ---\r\n");
    UART_Printf("1. Set multimeter to mA DC range\r\n");
    UART_Printf("2. Cut VCC trace or use jumper on 3.3V rail\r\n");
    UART_Printf("3. Insert multimeter in series with 3.3V supply\r\n");
    UART_Printf("4. Read current while MCU is running\r\n");
    UART_Printf("\r\n");
    UART_Printf("--- EXPECTED VALUES ---\r\n");
    UART_Printf("  Active 72MHz : ~25-35 mA\r\n");
    UART_Printf("  Active 36MHz : ~15-20 mA\r\n");
    UART_Printf("  Sleep  (WFI) : ~2-5 mA\r\n");
    UART_Printf("  Stop         : ~20 uA\r\n");
    UART_Printf("  Standby      : ~2 uA\r\n");
    UART_Printf("============================================\r\n\r\n");
}

/* ---- Print system info --------------------------------------------------- */
static void Print_System_Info(void)
{
    uint32_t hclk  = HAL_RCC_GetHCLKFreq();
    uint32_t pclk1 = HAL_RCC_GetPCLK1Freq();
    uint32_t pclk2 = HAL_RCC_GetPCLK2Freq();
    uint32_t sysclk = HAL_RCC_GetSysClockFreq();

    UART_Printf("--- System Clock Configuration ---\r\n");
    UART_Printf("  SYSCLK : %lu Hz\r\n", sysclk);
    UART_Printf("  HCLK   : %lu Hz\r\n", hclk);
    UART_Printf("  PCLK1  : %lu Hz\r\n", pclk1);
    UART_Printf("  PCLK2  : %lu Hz\r\n", pclk2);
    UART_Printf("  SysTick: %lu ticks/sec\r\n", (uint32_t)configTICK_RATE_HZ);
    UART_Printf("\r\n");
}

/* ---- Main power monitoring task ------------------------------------------ */
static void PowerMonitorTask(void *pvParameters)
{
    (void)pvParameters;
    uint32_t start_tick = HAL_GetTick();

    Print_Measurement_Guide();
    Print_System_Info();

    UART_Printf(">>> Starting active power measurement loop...\r\n\r\n");

    while (1) {
        /* Toggle LED to show active operation */
        HAL_GPIO_TogglePin(LED_PORT, LED_PIN);
        led_toggle_count++;

        /* Every 3 seconds: print periodic stats */
        uint32_t elapsed = HAL_GetTick() - start_tick;
        if (elapsed >= 3000) {
            report_cycle++;
            uint32_t hclk = HAL_RCC_GetHCLKFreq();
            uint32_t systick_val = SysTick->VAL;
            uint32_t systick_load = SysTick->LOAD;

            UART_Printf("--- Report #%lu (Uptime: %lu ms) ---\r\n",
                         report_cycle, HAL_GetTick());
            UART_Printf("  HCLK Frequency  : %lu Hz\r\n", hclk);
            UART_Printf("  SysTick VAL     : %lu\r\n", systick_val);
            UART_Printf("  SysTick LOAD    : %lu\r\n", systick_load);
            UART_Printf("  LED Toggles     : %lu\r\n", led_toggle_count);
            UART_Printf("  FreeRTOS Ticks  : %lu\r\n", xTaskGetTickCount());
            UART_Printf("  Stack HWM       : %u words\r\n",
                         (unsigned)uxTaskGetStackHighWaterMark(NULL));

            /* Read button state */
            GPIO_PinState btn = HAL_GPIO_ReadPin(BUTTON_PORT, BUTTON_PIN);
            UART_Printf("  Button (PA0)    : %s\r\n",
                         btn == GPIO_PIN_SET ? "PRESSED" : "RELEASED");

            /* Dummy CPU load indicator */
            volatile uint32_t dummy = 0;
            for (volatile uint32_t i = 0; i < 100000; i++) {
                dummy += i;
            }
            UART_Printf("  CPU Load Test   : %lu (dummy sum)\r\n", dummy);
            UART_Printf("  Mode            : ACTIVE @ 72MHz\r\n");
            UART_Printf("  Measure current : Place ammeter inline on 3.3V\r\n");
            UART_Printf("\r\n");

            start_tick = HAL_GetTick();
        }

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

/* ---- FreeRTOS hooks ------------------------------------------------------ */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    UART_Printf("!!! Stack overflow in task: %s\r\n", pcTaskName);
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

    xTaskCreate(PowerMonitorTask, "PwrMon", MAIN_TASK_STACK_SIZE,
                NULL, MAIN_TASK_PRIORITY, NULL);

    vTaskStartScheduler();

    while (1);
    return 0;
}
