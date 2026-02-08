/* ============================================================================
 * STM32_08_ULP_Coprocessor - IWDG as Low-Power Periodic Wakeup
 * ============================================================================
 * STM32F103 tidak punya ULP coprocessor. IWDG + Standby mode digunakan
 * sebagai alternatif periodic wakeup.
 *
 * - Configure IWDG with LSI clock (~40kHz)
 * - IWDG generates reset when timer expires (periodic wakeup from standby)
 * - Set IWDG period to ~10 seconds (max prescaler + max reload)
 * - Before standby: save data to backup register
 * - After IWDG reset: check backup register, increment counter
 * ============================================================================ */
#include "config.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
/* ---- Private variables --------------------------------------------------- */
static UART_HandleTypeDef huart1;
static RTC_HandleTypeDef  hrtc;
static IWDG_HandleTypeDef hiwdg;
/* Backup register map */
#define BKP_CYCLE_COUNT     RTC_BKP_DR1
#define BKP_MAGIC           RTC_BKP_DR2
#define BKP_SENSOR_SIM      RTC_BKP_DR3
#define BKP_TOTAL_CYCLES    RTC_BKP_DR4
#define BKP_LAST_ADC        RTC_BKP_DR5
#define BKP_ERROR_COUNT     RTC_BKP_DR6
#define MAGIC_VALUE         0xDEAD
#define MAX_CYCLES          50       /* Maximum cycles to track */
#define AWAKE_PERIOD_MS     3000     /* Stay awake 3 seconds per cycle */
/* IWDG Configuration:
 * LSI ~ 40kHz
 * Prescaler /256 -> 40000/256 = 156.25 Hz -> period ~6.4ms per tick
 * Reload = 4095 (max for 12-bit) -> ~26.2 seconds timeout
 * We use Reload = 1562 for ~10 seconds
 */
#define IWDG_PRESCALER      IWDG_PRESCALER_256
#define IWDG_RELOAD_10S     1562
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
    if (len > 0) HAL_UART_Transmit(&huart1, (uint8_t *)buf, len, HAL_MAX_DELAY);
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
    #ifdef STM32F103xB
    __HAL_RCC_BKP_CLK_ENABLE();
    #endif
    HAL_PWR_EnableBkUpAccess();
    hrtc.Instance = RTC;
    #ifdef STM32F103xB
    hrtc.Init.AsynchPrediv = RTC_AUTO_1_SECOND;
    #elif defined(STM32F401xC) || defined(STM32F411xE)
    hrtc.Init.AsynchPrediv = 127;
    hrtc.Init.SynchPrediv    = 255;
    #endif
    #ifdef STM32F103xB
    hrtc.Init.OutPut = RTC_OUTPUTSOURCE_NONE;
    #elif defined(STM32F401xC) || defined(STM32F411xE)
    hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
    #endif
    HAL_RTC_Init(&hrtc);
}
/* ---- Simulated sensor reading -------------------------------------------- */
static uint16_t Simulate_Sensor(uint32_t cycle)
{
    /* Simulate a sensor value that varies with cycle number */
    return (uint16_t)((cycle * 37 + 128) % 4096);
}
/* ---- IWDG Wakeup Task --------------------------------------------------- */
static void vIWDGTask(void *pvParameters)
{
    (void)pvParameters;
    uint32_t cycle_count = 0;
    uint32_t total_cycles = 0;
    uint32_t error_count = 0;
    uint8_t  from_iwdg_reset = 0;
    /* Check reset source */
    if (__HAL_RCC_GET_FLAG(RCC_FLAG_IWDGRST)) {
        from_iwdg_reset = 1;
        __HAL_RCC_CLEAR_RESET_FLAGS();
    }
    /* Read backup registers */
    if (HAL_RTCEx_BKUPRead(&hrtc, BKP_MAGIC) == MAGIC_VALUE) {
        cycle_count  = HAL_RTCEx_BKUPRead(&hrtc, BKP_CYCLE_COUNT);
        total_cycles = HAL_RTCEx_BKUPRead(&hrtc, BKP_TOTAL_CYCLES);
        error_count  = HAL_RTCEx_BKUPRead(&hrtc, BKP_ERROR_COUNT);
    }
    if (from_iwdg_reset) {
        cycle_count++;
        total_cycles++;
    } else {
        cycle_count = 0;
    }
    UART_Printf("\r\n========================================\r\n");
    UART_Printf("  STM32 IWDG Periodic Wakeup Demo\r\n");
    UART_Printf("========================================\r\n");
    UART_Printf("NOTE: STM32F103 tidak punya ULP coprocessor.\r\n");
    UART_Printf("      IWDG + Standby mode digunakan sebagai\r\n");
    UART_Printf("      alternatif periodic wakeup.\r\n\r\n");
    if (from_iwdg_reset) {
        UART_Printf("[WAKEUP] IWDG timeout reset (cycle #%lu)\r\n", cycle_count);
    } else {
        UART_Printf("[BOOT] Fresh start (power-on/external reset)\r\n");
    }
    UART_Printf("[INFO] Current cycle:  %lu\r\n", cycle_count);
    UART_Printf("[INFO] Total cycles:   %lu\r\n", total_cycles);
    UART_Printf("[INFO] Error count:    %lu\r\n", error_count);
    UART_Printf("[INFO] IWDG period:    ~10 seconds\r\n");
    UART_Printf("[INFO] LSI clock:      ~40 kHz\r\n");
    /* Simulate sensor reading */
    uint16_t sensor_val = Simulate_Sensor(cycle_count);
    UART_Printf("\r\n[SENSOR] Simulated reading: %u\r\n", sensor_val);
    UART_Printf("[SENSOR] Previous reading: %lu\r\n",
                HAL_RTCEx_BKUPRead(&hrtc, BKP_LAST_ADC));
    /* Duty cycle calculation */
    float duty = (float)AWAKE_PERIOD_MS / (10000.0f + AWAKE_PERIOD_MS) * 100.0f;
    UART_Printf("\r\n[POWER] Awake time: %d ms per cycle\r\n", AWAKE_PERIOD_MS);
    UART_Printf("[POWER] Sleep time: ~10000 ms (IWDG timeout)\r\n");
    UART_Printf("[POWER] Duty cycle: %.1f%%\r\n", duty);
    UART_Printf("[POWER] Estimated avg current: ~%.1f mA\r\n",
                30.0f * duty / 100.0f + 0.002f * (100.0f - duty) / 100.0f);
    /* LED blink to show cycle */
    HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_RESET);
    vTaskDelay(pdMS_TO_TICKS(500));
    HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET);
    /* Countdown before standby */
    UART_Printf("\r\n[CYCLE] Processing for %d seconds...\r\n", AWAKE_PERIOD_MS / 1000);
    for (int i = AWAKE_PERIOD_MS / 1000; i > 0; i--) {
        UART_Printf("  Standby in %d...\r\n", i);
        HAL_GPIO_TogglePin(LED_PORT, LED_PIN);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    /* Save to backup registers */
    HAL_RTCEx_BKUPWrite(&hrtc, BKP_MAGIC, MAGIC_VALUE);
    HAL_RTCEx_BKUPWrite(&hrtc, BKP_CYCLE_COUNT, cycle_count);
    HAL_RTCEx_BKUPWrite(&hrtc, BKP_TOTAL_CYCLES, total_cycles);
    HAL_RTCEx_BKUPWrite(&hrtc, BKP_SENSOR_SIM, sensor_val);
    HAL_RTCEx_BKUPWrite(&hrtc, BKP_LAST_ADC, sensor_val);
    HAL_RTCEx_BKUPWrite(&hrtc, BKP_ERROR_COUNT, error_count);
    UART_Printf("\r\n[IWDG] Starting IWDG (~10s timeout)...\r\n");
    UART_Printf("[STANDBY] Entering Standby mode.\r\n");
    UART_Printf("[STANDBY] IWDG will reset MCU in ~10 seconds.\r\n\r\n");
    vTaskDelay(pdMS_TO_TICKS(100));
    /* Configure and start IWDG */
    hiwdg.Instance       = IWDG;
    hiwdg.Init.Prescaler = IWDG_PRESCALER;
    hiwdg.Init.Reload    = IWDG_RELOAD_10S;
    HAL_IWDG_Init(&hiwdg);
    /* Enter standby — IWDG continues running on LSI */
    HAL_PWR_EnterSTANDBYMode();
    for (;;) { vTaskDelay(pdMS_TO_TICKS(1000)); }
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
    SystemClock_Config();
    LED_Init();
    Backup_Init();
    UART_Init();
    xTaskCreate(vIWDGTask, "IWDG", MAIN_TASK_STACK_SIZE, NULL,
                MAIN_TASK_PRIORITY, NULL);
    vTaskStartScheduler();
    for (;;);
}
