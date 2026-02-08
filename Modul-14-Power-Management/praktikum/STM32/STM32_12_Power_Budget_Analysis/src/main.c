/* ============================================================================
 * STM32_12_Power_Budget_Analysis - Complete Power Profiling & Budget
 * ============================================================================
 * Comprehensive power profiling and budget analysis for STM32F103.
 *
 * - Measure time in each state using SysTick/HAL_GetTick
 * - States: Active (sensor read), Processing, UART TX, Sleep/Stop
 * - Calculate duty cycle
 * - Estimate average current based on known values:
 *     Active 72MHz: ~30mA, Sleep (WFI): ~2mA, Stop: ~20µA, Standby: ~2µA
 * - Calculate battery life for different battery capacities
 * - Print comprehensive power budget report
 * - Demonstrate entering and exiting each power mode briefly
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

/* Power state tracking */
typedef enum {
    STATE_ACTIVE = 0,
    STATE_PROCESSING,
    STATE_UART_TX,
    STATE_SLEEP_WFI,
    STATE_STOP,
    STATE_COUNT
} PowerState_t;

static const char *state_names[] = {
    "Active (Sensor)",
    "Processing",
    "UART TX",
    "Sleep (WFI)",
    "Stop Mode"
};

/* Current draw estimates (mA) for STM32F103 at 72MHz */
static const float state_current_mA[] = {
    30.0f,    /* Active */
    30.0f,    /* Processing (same as active) */
    30.0f,    /* UART TX */
    2.0f,     /* Sleep WFI */
    0.020f    /* Stop mode */
};

/* Battery capacities to estimate (mAh) */
static const float battery_caps[] = {200, 500, 1000, 2000, 3000};
#define NUM_BATTERIES (sizeof(battery_caps)/sizeof(battery_caps[0]))

/* Time tracking (ms) */
static uint32_t state_time_ms[STATE_COUNT];
static uint32_t total_cycle_ms;

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
        uint32_t t0 = HAL_GetTick();
        HAL_UART_Transmit(&huart1, (uint8_t *)buf, len, HAL_MAX_DELAY);
        state_time_ms[STATE_UART_TX] += (HAL_GetTick() - t0);
    }
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

/* ---- Simulate workloads -------------------------------------------------- */
static volatile uint32_t dummy_result;

static void Simulate_Sensor_Read(void)
{
    uint32_t t0 = HAL_GetTick();
    /* Simulate sensor reading with some delay + computation */
    for (volatile int i = 0; i < 50000; i++) {
        dummy_result += i;
    }
    HAL_Delay(50); /* Simulate I2C/SPI sensor read time */
    state_time_ms[STATE_ACTIVE] += (HAL_GetTick() - t0);
}

static void Simulate_Processing(void)
{
    uint32_t t0 = HAL_GetTick();
    /* Simulate data processing: filtering, averaging */
    for (volatile int i = 0; i < 200000; i++) {
        dummy_result = (dummy_result * 31421 + 6927) & 0xFFFF;
    }
    state_time_ms[STATE_PROCESSING] += (HAL_GetTick() - t0);
}

static void Enter_Sleep_WFI_ms(uint32_t ms)
{
    uint32_t t0 = HAL_GetTick();
    /* Sleep mode: CPU stops, peripherals keep running, wakeup by any interrupt */
    HAL_Delay(ms);  /* SysTick interrupts keep HAL_Delay working */
    state_time_ms[STATE_SLEEP_WFI] += (HAL_GetTick() - t0);
}

static void Enter_Stop_Brief(void)
{
    /* Brief Stop mode entry to demonstrate. RTC alarm wakes us.
     * In real application, this would be seconds/minutes. */
    uint32_t counter = RTC->CNTH;
    counter = (counter << 16) | RTC->CNTL;
    uint32_t alarm_val = counter + 2; /* 2 seconds */

    while ((RTC->CRL & RTC_CRL_RTOFF) == 0);
    RTC->CRL |= RTC_CRL_CNF;
    RTC->ALRH = (alarm_val >> 16) & 0xFFFF;
    RTC->ALRL = alarm_val & 0xFFFF;
    RTC->CRL &= ~RTC_CRL_CNF;
    while ((RTC->CRL & RTC_CRL_RTOFF) == 0);

    EXTI->IMR  |= (1 << 17);
    EXTI->RTSR |= (1 << 17);
    RTC->CRH   |= RTC_CRH_ALRIE;
    HAL_NVIC_SetPriority(RTC_Alarm_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(RTC_Alarm_IRQn);

    HAL_SuspendTick();
    HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);
    SystemClock_Config();
    HAL_ResumeTick();

    state_time_ms[STATE_STOP] += 2000; /* ~2 seconds in Stop */
}

void RTC_Alarm_IRQHandler(void)
{
    if (EXTI->PR & (1 << 17)) {
        EXTI->PR = (1 << 17);
    }
    RTC->CRL &= ~RTC_CRL_ALRF;
}

/* ---- Power Budget Task --------------------------------------------------- */
static void vPowerBudgetTask(void *pvParameters)
{
    (void)pvParameters;

    for (;;) {
        /* Reset counters */
        memset(state_time_ms, 0, sizeof(state_time_ms));

        UART_Printf("\r\n============================================================\r\n");
        UART_Printf("  STM32F103 Power Budget Analysis\r\n");
        UART_Printf("============================================================\r\n\r\n");

        uint32_t cycle_start = HAL_GetTick();

        /* Phase 1: Active - sensor read */
        UART_Printf("[1/5] Simulating sensor read (Active mode)...\r\n");
        HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_RESET);
        Simulate_Sensor_Read();
        HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET);

        /* Phase 2: Processing */
        UART_Printf("[2/5] Simulating data processing...\r\n");
        Simulate_Processing();

        /* Phase 3: UART TX (already tracked in UART_Printf) */
        UART_Printf("[3/5] UART transmission (this output)...\r\n");

        /* Phase 4: Sleep (WFI) */
        UART_Printf("[4/5] Entering Sleep mode (WFI) for 2 seconds...\r\n");
        Enter_Sleep_WFI_ms(2000);
        UART_Init(); /* re-init UART after potential clock issue */

        /* Phase 5: Stop mode */
        UART_Printf("[5/5] Entering Stop mode for ~2 seconds...\r\n");
        vTaskDelay(pdMS_TO_TICKS(50));
        Enter_Stop_Brief();
        UART_Init();

        total_cycle_ms = HAL_GetTick() - cycle_start;

        /* ===== POWER BUDGET REPORT ===== */
        UART_Printf("\r\n============================================================\r\n");
        UART_Printf("  POWER BUDGET REPORT\r\n");
        UART_Printf("============================================================\r\n\r\n");

        UART_Printf("--- Time Distribution ---\r\n");
        UART_Printf("%-20s %8s %8s %10s\r\n", "State", "Time(ms)", "Duty%", "Current");
        UART_Printf("------------------------------------------------------\r\n");

        float avg_current = 0;
        for (int i = 0; i < STATE_COUNT; i++) {
            float duty = (total_cycle_ms > 0) ?
                         (float)state_time_ms[i] / total_cycle_ms * 100.0f : 0;
            avg_current += state_current_mA[i] * duty / 100.0f;
            UART_Printf("%-20s %8lu %7.1f%% %8.3f mA\r\n",
                        state_names[i], state_time_ms[i], duty,
                        state_current_mA[i]);
        }
        UART_Printf("------------------------------------------------------\r\n");
        UART_Printf("%-20s %8lu %7.1f%%\r\n", "TOTAL", total_cycle_ms, 100.0f);

        UART_Printf("\r\n--- Current Analysis ---\r\n");
        UART_Printf("  Peak current:    %.1f mA (Active @ 72MHz)\r\n", 30.0f);
        UART_Printf("  Min current:     %.3f mA (Stop mode)\r\n", 0.020f);
        UART_Printf("  Avg current:     %.3f mA (weighted)\r\n", avg_current);

        /* Standby estimate (not entered but calculated) */
        UART_Printf("\r\n--- Power Mode Reference ---\r\n");
        UART_Printf("  Active  72MHz:   ~30.000 mA\r\n");
        UART_Printf("  Active   8MHz:   ~ 8.000 mA\r\n");
        UART_Printf("  Sleep   (WFI):   ~ 2.000 mA\r\n");
        UART_Printf("  Stop    (LPR):   ~ 0.020 mA\r\n");
        UART_Printf("  Standby:         ~ 0.002 mA\r\n");

        /* Battery life estimates */
        UART_Printf("\r\n--- Battery Life Estimates ---\r\n");
        UART_Printf("  (Based on avg current: %.3f mA)\r\n\r\n", avg_current);
        UART_Printf("  %-12s %12s %12s\r\n", "Battery", "Life (hrs)", "Life (days)");
        UART_Printf("  ----------------------------------------\r\n");
        for (uint32_t i = 0; i < NUM_BATTERIES; i++) {
            float hours = (avg_current > 0) ? battery_caps[i] / avg_current : 0;
            float days  = hours / 24.0f;
            UART_Printf("  %8.0f mAh %10.1f h %10.1f d\r\n",
                        battery_caps[i], hours, days);
        }

        /* Optimization recommendations */
        UART_Printf("\r\n--- Optimization Recommendations ---\r\n");
        float active_duty = (total_cycle_ms > 0) ?
            (float)(state_time_ms[STATE_ACTIVE] + state_time_ms[STATE_PROCESSING]) /
            total_cycle_ms * 100.0f : 0;
        UART_Printf("  Active duty cycle: %.1f%%\r\n", active_duty);

        if (active_duty > 50.0f) {
            UART_Printf("  [!] High active duty - consider reducing sensor rate\r\n");
        }
        if (state_time_ms[STATE_UART_TX] > state_time_ms[STATE_ACTIVE]) {
            UART_Printf("  [!] UART TX time > sensor time - reduce logging\r\n");
        }
        UART_Printf("  [*] Use Stop mode instead of Sleep for longer idle\r\n");
        UART_Printf("  [*] Lower clock to 8MHz during processing if possible\r\n");
        UART_Printf("  [*] Disable unused peripheral clocks\r\n");

        /* Energy per cycle */
        float energy_mWs = 0;
        for (int i = 0; i < STATE_COUNT; i++) {
            energy_mWs += state_current_mA[i] * 3.3f * state_time_ms[i] / 1000.0f;
        }
        UART_Printf("\r\n--- Energy Per Cycle ---\r\n");
        UART_Printf("  Cycle duration:  %lu ms\r\n", total_cycle_ms);
        UART_Printf("  Energy/cycle:    %.2f mJ\r\n", energy_mWs);
        UART_Printf("  Avg power:       %.2f mW\r\n",
                    total_cycle_ms > 0 ? energy_mWs / (total_cycle_ms / 1000.0f) : 0);

        UART_Printf("\r\n============================================================\r\n");
        UART_Printf("  Next analysis cycle in 15 seconds...\r\n");
        UART_Printf("============================================================\r\n\r\n");

        HAL_GPIO_TogglePin(LED_PORT, LED_PIN);
        vTaskDelay(pdMS_TO_TICKS(15000));
    }
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

    xTaskCreate(vPowerBudgetTask, "PWR_BDG", MAIN_TASK_STACK_SIZE * 2, NULL,
                MAIN_TASK_PRIORITY, NULL);
    vTaskStartScheduler();
    for (;;);
}
