/* ============================================================================
 * STM32_09_Dynamic_Frequency - CPU Clock Frequency Scaling
 * ============================================================================
 * Dynamically switch CPU clock frequency for power savings.
 *
 * Clock configurations:
 *   - HSE  8MHz  (bypass PLL)  → low power
 *   - HSI  8MHz  (internal RC) → moderate power, no external crystal
 *   - PLL 36MHz  (HSE x4.5)   → medium performance
 *   - PLL 72MHz  (HSE x9)     → max performance
 *
 * At each frequency: run benchmark (loop counting), measure performance,
 * update SysTick and UART baud rate after clock change.
 * ============================================================================ */

#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

/* ---- Private variables --------------------------------------------------- */
static UART_HandleTypeDef huart1;

/* Frequency mode IDs */
typedef enum {
    FREQ_HSE_8MHZ = 0,
    FREQ_HSI_8MHZ,
    FREQ_PLL_36MHZ,
    FREQ_PLL_72MHZ,
    FREQ_MODE_COUNT
} FreqMode_t;

static const char *freq_names[] = {
    "HSE 8 MHz (direct)",
    "HSI 8 MHz (internal)",
    "PLL 36 MHz (HSE x4.5)",
    "PLL 72 MHz (HSE x9)"
};

static const uint32_t freq_values[] = {
    8000000, 8000000, 36000000, 72000000
};

/* Benchmark results */
typedef struct {
    uint32_t freq_hz;
    uint32_t loops_per_sec;
    uint32_t benchmark_ms;
    float    relative_perf;
} BenchResult_t;

static BenchResult_t results[FREQ_MODE_COUNT];

/* ---- UART ---------------------------------------------------------------- */
static void UART_Init_At(uint32_t sysclk)
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

    /* De-init first to reconfigure baud */
    HAL_UART_DeInit(&huart1);

    huart1.Instance          = USART1;
    huart1.Init.BaudRate     = UART_BAUDRATE;
    huart1.Init.WordLength   = UART_WORDLENGTH_8B;
    huart1.Init.StopBits     = UART_STOPBITS_1;
    huart1.Init.Parity       = UART_PARITY_NONE;
    huart1.Init.Mode         = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(&huart1);
    (void)sysclk;
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

/* ---- Clock Switching ----------------------------------------------------- */
static void Switch_To_HSE_8MHz(void)
{
    RCC_OscInitTypeDef osc = {0};
    osc.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    osc.HSEState       = RCC_HSE_ON;
    osc.PLL.PLLState   = RCC_PLL_OFF;
    HAL_RCC_OscConfig(&osc);

    RCC_ClkInitTypeDef clk = {0};
    clk.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                          RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    clk.SYSCLKSource   = RCC_SYSCLKSOURCE_HSE;
    clk.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    clk.APB1CLKDivider = RCC_HCLK_DIV1;
    clk.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_0);
}

static void Switch_To_HSI_8MHz(void)
{
    RCC_OscInitTypeDef osc = {0};
    osc.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    osc.HSIState       = RCC_HSI_ON;
    osc.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    osc.PLL.PLLState   = RCC_PLL_OFF;
    HAL_RCC_OscConfig(&osc);

    RCC_ClkInitTypeDef clk = {0};
    clk.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                          RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    clk.SYSCLKSource   = RCC_SYSCLKSOURCE_HSI;
    clk.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    clk.APB1CLKDivider = RCC_HCLK_DIV1;
    clk.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_0);
}

static void Switch_To_PLL_36MHz(void)
{
    /* First switch to HSI to reconfigure PLL */
    Switch_To_HSI_8MHz();

    RCC_OscInitTypeDef osc = {0};
    osc.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    osc.HSEState       = RCC_HSE_ON;
    osc.HSEPredivValue = RCC_HSE_PREDIV_DIV2;
    osc.PLL.PLLState   = RCC_PLL_ON;
    osc.PLL.PLLSource  = RCC_PLLSOURCE_HSE;
    osc.PLL.PLLMUL     = RCC_PLL_MUL9; /* 8/2 * 9 = 36 MHz */
    HAL_RCC_OscConfig(&osc);

    RCC_ClkInitTypeDef clk = {0};
    clk.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                          RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    clk.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    clk.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    clk.APB1CLKDivider = RCC_HCLK_DIV1;
    clk.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_1);
}

static void Switch_To_PLL_72MHz(void)
{
    /* First switch to HSI to reconfigure PLL */
    Switch_To_HSI_8MHz();

    RCC_OscInitTypeDef osc = {0};
    osc.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    osc.HSEState       = RCC_HSE_ON;
    osc.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    osc.PLL.PLLState   = RCC_PLL_ON;
    osc.PLL.PLLSource  = RCC_PLLSOURCE_HSE;
    osc.PLL.PLLMUL     = RCC_PLL_MUL9; /* 8 * 9 = 72 MHz */
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

/* ---- Benchmark ----------------------------------------------------------- */
static volatile uint32_t benchmark_counter;

static uint32_t Run_Benchmark(void)
{
    /* Count loop iterations in a fixed time window using SysTick */
    benchmark_counter = 0;
    uint32_t start = HAL_GetTick();
    while ((HAL_GetTick() - start) < 1000) {
        benchmark_counter++;
    }
    return benchmark_counter;
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

/* ---- Frequency Scaling Task ---------------------------------------------- */
static void vFreqScalingTask(void *pvParameters)
{
    (void)pvParameters;

    UART_Printf("\r\n========================================\r\n");
    UART_Printf("  STM32 Dynamic Frequency Scaling Demo\r\n");
    UART_Printf("========================================\r\n\r\n");

    for (;;) {
        /* Test each frequency mode */
        for (int mode = 0; mode < FREQ_MODE_COUNT; mode++) {
            UART_Printf("--- Switching to: %s ---\r\n", freq_names[mode]);

            /* Switch clock */
            switch (mode) {
                case FREQ_HSE_8MHZ:  Switch_To_HSE_8MHz();  break;
                case FREQ_HSI_8MHZ:  Switch_To_HSI_8MHz();  break;
                case FREQ_PLL_36MHZ: Switch_To_PLL_36MHz(); break;
                case FREQ_PLL_72MHZ: Switch_To_PLL_72MHz(); break;
            }

            /* Update SysTick for new frequency */
            HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq() / 1000);
            SystemCoreClock = HAL_RCC_GetSysClockFreq();

            /* Reconfigure UART with new clock */
            UART_Init_At(SystemCoreClock);

            uint32_t actual_freq = HAL_RCC_GetSysClockFreq();
            UART_Printf("[CLK] SYSCLK: %lu Hz\r\n", actual_freq);
            UART_Printf("[CLK] HCLK:   %lu Hz\r\n", HAL_RCC_GetHCLKFreq());
            UART_Printf("[CLK] PCLK1:  %lu Hz\r\n", HAL_RCC_GetPCLK1Freq());
            UART_Printf("[CLK] PCLK2:  %lu Hz\r\n", HAL_RCC_GetPCLK2Freq());

            /* Run benchmark */
            UART_Printf("[BENCH] Running 1-second loop benchmark...\r\n");
            uint32_t loops = Run_Benchmark();

            results[mode].freq_hz      = actual_freq;
            results[mode].loops_per_sec = loops;
            results[mode].benchmark_ms = 1000;

            UART_Printf("[BENCH] Loops/sec: %lu\r\n", loops);
            UART_Printf("[BENCH] Freq/loop: %.2f Hz/loop\r\n",
                        (float)actual_freq / loops);

            /* LED toggle to show activity */
            HAL_GPIO_TogglePin(LED_PORT, LED_PIN);
            HAL_Delay(500);
            UART_Printf("\r\n");
        }

        /* Restore to 72MHz and print summary */
        Switch_To_PLL_72MHz();
        HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq() / 1000);
        SystemCoreClock = HAL_RCC_GetSysClockFreq();
        UART_Init_At(SystemCoreClock);

        /* Calculate relative performance (72MHz = 100%) */
        uint32_t max_loops = results[FREQ_PLL_72MHZ].loops_per_sec;
        if (max_loops == 0) max_loops = 1;

        UART_Printf("========================================\r\n");
        UART_Printf("  Performance Summary\r\n");
        UART_Printf("========================================\r\n");
        UART_Printf("%-22s %10s %8s\r\n", "Mode", "Loops/s", "Rel%%");
        UART_Printf("----------------------------------------------\r\n");
        for (int i = 0; i < FREQ_MODE_COUNT; i++) {
            results[i].relative_perf = (float)results[i].loops_per_sec / max_loops * 100.0f;
            UART_Printf("%-22s %10lu %7.1f%%\r\n",
                        freq_names[i],
                        results[i].loops_per_sec,
                        results[i].relative_perf);
        }
        UART_Printf("----------------------------------------------\r\n\r\n");

        /* Power estimate */
        UART_Printf("Estimated Power Consumption:\r\n");
        UART_Printf("  HSE  8MHz:  ~8 mA\r\n");
        UART_Printf("  HSI  8MHz:  ~9 mA  (RC oscillator active)\r\n");
        UART_Printf("  PLL 36MHz:  ~18 mA\r\n");
        UART_Printf("  PLL 72MHz:  ~30 mA\r\n\r\n");

        UART_Printf("Next test cycle in 10 seconds...\r\n\r\n");
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}

/* ---- FreeRTOS Hooks ------------------------------------------------------ */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    UART_Printf("[FATAL] Stack overflow: %s\r\n", pcTaskName);
    for (;;);
}

void vApplicationMallocFailedHook(void)
{
    UART_Printf("[FATAL] Malloc failed!\r\n");
    for (;;);
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
    Switch_To_PLL_72MHz();
    HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq() / 1000);
    LED_Init();
    UART_Init_At(72000000);

    xTaskCreate(vFreqScalingTask, "FREQ", MAIN_TASK_STACK_SIZE * 2, NULL,
                MAIN_TASK_PRIORITY, NULL);
    vTaskStartScheduler();
    for (;;);
}
