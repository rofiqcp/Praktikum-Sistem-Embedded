/* ============================================================================
 * STM32_06_Sleep_Data_Retention - Backup Registers for Standby Persistence
 * ============================================================================
 * Demonstrates backup domain registers to persist data across Standby mode.
 * - Enable backup domain access: HAL_PWR_EnableBkUpAccess()
 * - Write data to backup registers: HAL_RTCEx_BKUPWrite()
 * - Read data after standby wakeup: HAL_RTCEx_BKUPRead()
 * - Store boot count, simulated sensor reading in backup registers
 * - Compare: backup regs survive standby, regular SRAM variables don't
 * - 10 backup registers available on STM32F103
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
static volatile uint8_t   from_standby = 0;
/* SRAM variables - will be lost in Standby */
static volatile uint32_t sram_boot_count = 0;
static volatile uint32_t sram_sensor_val = 0;
static volatile uint32_t sram_magic      = 0;
/* Backup register map (STM32F103 has BKP_DR1 to BKP_DR10) */
#define BKP_BOOT_COUNT      RTC_BKP_DR1   /* Boot counter */
#define BKP_SENSOR_VALUE    RTC_BKP_DR2   /* Last sensor reading */
#define BKP_MAGIC_KEY       RTC_BKP_DR3   /* Magic key for validation */
#define BKP_TIMESTAMP       RTC_BKP_DR4   /* RTC counter at sleep */
#define BKP_TOTAL_RUNTIME   RTC_BKP_DR5   /* Accumulated runtime (sec) */
#define BKP_STATUS_FLAGS    RTC_BKP_DR6   /* Status bit flags */
#define MAGIC_VALUE         0xCAFE
#define STANDBY_SECONDS     10
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
/* ---- Backup Domain Init -------------------------------------------------- */
static void BKP_Init(void)
{
    __HAL_RCC_PWR_CLK_ENABLE();
    #ifdef STM32F103xB
    __HAL_RCC_BKP_CLK_ENABLE();
    #endif
    HAL_PWR_EnableBkUpAccess();
}
/* ---- RTC Init with LSI --------------------------------------------------- */
static void RTC_Init(void)
{
    BKP_Init();
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
    hrtc.Init.AsynchPrediv = (40000 - 1);
    hrtc.Init.OutPut       = RTC_OUTPUTSOURCE_NONE;
    HAL_RTC_Init(&hrtc);
}
/* ---- Simulated sensor reading -------------------------------------------- */
static uint16_t Read_Simulated_Sensor(void)
{
    /* Simulate ADC reading with varying value based on tick */
    uint32_t tick = HAL_GetTick();
    return (uint16_t)((tick * 37 + 1234) % 4096);
}
/* ---- Read all backup registers and display ------------------------------- */
static void Print_Backup_Registers(void)
{
    BKP_Init();
    UART_Printf("--- Backup Register Contents ---\r\n");
    UART_Printf("  BKP_DR1 (Boot Count)  : %lu\r\n",
                 HAL_RTCEx_BKUPRead(&hrtc, BKP_BOOT_COUNT));
    UART_Printf("  BKP_DR2 (Sensor Val)  : %lu\r\n",
                 HAL_RTCEx_BKUPRead(&hrtc, BKP_SENSOR_VALUE));
    UART_Printf("  BKP_DR3 (Magic Key)   : 0x%04lX %s\r\n",
                 HAL_RTCEx_BKUPRead(&hrtc, BKP_MAGIC_KEY),
                 HAL_RTCEx_BKUPRead(&hrtc, BKP_MAGIC_KEY) == MAGIC_VALUE ?
                 "(VALID)" : "(INVALID)");
    UART_Printf("  BKP_DR4 (Timestamp)   : %lu\r\n",
                 HAL_RTCEx_BKUPRead(&hrtc, BKP_TIMESTAMP));
    UART_Printf("  BKP_DR5 (Total RT)    : %lu sec\r\n",
                 HAL_RTCEx_BKUPRead(&hrtc, BKP_TOTAL_RUNTIME));
    UART_Printf("  BKP_DR6 (Status Flags): 0x%04lX\r\n",
                 HAL_RTCEx_BKUPRead(&hrtc, BKP_STATUS_FLAGS));
    UART_Printf("--------------------------------\r\n\r\n");
}
/* ---- Compare SRAM vs Backup persistence ---------------------------------- */
static void Print_Persistence_Comparison(uint32_t bkp_boot, uint32_t bkp_sensor)
{
    UART_Printf("=== Data Persistence Comparison ===\r\n");
    UART_Printf("  Source          | Boot Count | Sensor Val\r\n");
    UART_Printf("  ----------------+------------+-----------\r\n");
    UART_Printf("  SRAM (volatile) | %-10lu | %-10lu\r\n",
                 sram_boot_count, sram_sensor_val);
    UART_Printf("  Backup Regs     | %-10lu | %-10lu\r\n",
                 bkp_boot, bkp_sensor);
    UART_Printf("  ----------------+------------+-----------\r\n");
    if (from_standby) {
        UART_Printf("  >> SRAM was RESET to 0 (lost in Standby)\r\n");
        UART_Printf("  >> Backup registers SURVIVED Standby!\r\n");
    } else {
        UART_Printf("  >> First boot: both start fresh\r\n");
    }
    UART_Printf("===================================\r\n\r\n");
}
/* ---- Set RTC alarm for standby wakeup ------------------------------------ */
static void RTC_SetAlarm_ForStandby(uint32_t seconds)
{
    uint32_t current = RTC->CNTH << 16 | RTC->CNTL;
    uint32_t alarm_val = current + seconds;
    HAL_RTC_DeactivateAlarm(&hrtc, RTC_ALARM_A);
    while (!(RTC->CRL & RTC_CRL_RTOFF));
    RTC->CRL |= RTC_CRL_CNF;
    RTC->ALRH = (alarm_val >> 16) & 0xFFFF;
    RTC->ALRL = alarm_val & 0xFFFF;
    RTC->CRL &= ~RTC_CRL_CNF;
    while (!(RTC->CRL & RTC_CRL_RTOFF));
    __HAL_RTC_ALARM_ENABLE_IT(&hrtc, RTC_IT_ALRA);
    EXTI->IMR  |= (1 << 17);
    EXTI->RTSR |= (1 << 17);
    EXTI->PR    = (1 << 17);
    UART_Printf("[RTC] Alarm set: +%lu sec (counter: %lu -> %lu)\r\n",
                seconds, current, alarm_val);
}
/* ---- LED Blink ----------------------------------------------------------- */
static void LED_Blink(uint32_t count, uint32_t delay_ms)
{
    for (uint32_t i = 0; i < count; i++) {
        HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_RESET);
        vTaskDelay(pdMS_TO_TICKS(delay_ms));
        HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET);
        vTaskDelay(pdMS_TO_TICKS(delay_ms));
    }
}
/* ---- Data Retention Task ------------------------------------------------- */
static void DataRetentionTask(void *pvParameters)
{
    (void)pvParameters;
    UART_Printf("\r\n");
    UART_Printf("============================================\r\n");
    UART_Printf("  STM32F103 Backup Register Persistence\r\n");
    UART_Printf("============================================\r\n");
    UART_Printf("  Standby wakeup interval: %d sec\r\n", STANDBY_SECONDS);
    UART_Printf("  Backup registers: 10 x 16-bit (DR1-DR10)\r\n");
    UART_Printf("  SRAM: Lost in Standby\r\n");
    UART_Printf("  Backup domain: Survives Standby\r\n");
    UART_Printf("============================================\r\n\r\n");
    /* Initialize RTC and backup domain */
    RTC_Init();
    /* Read existing backup data */
    uint32_t bkp_boot   = HAL_RTCEx_BKUPRead(&hrtc, BKP_BOOT_COUNT);
    uint32_t bkp_sensor = HAL_RTCEx_BKUPRead(&hrtc, BKP_SENSOR_VALUE);
    uint32_t bkp_magic  = HAL_RTCEx_BKUPRead(&hrtc, BKP_MAGIC_KEY);
    if (from_standby) {
        UART_Printf(">>> WAKEUP FROM STANDBY <<<\r\n\r\n");
        if (bkp_magic == MAGIC_VALUE) {
            UART_Printf("[BKP] Magic key VALID - backup data intact\r\n");
            UART_Printf("[BKP] Previous boot count: %lu\r\n", bkp_boot);
            UART_Printf("[BKP] Previous sensor val: %lu\r\n", bkp_sensor);
        } else {
            UART_Printf("[BKP] Magic key INVALID - backup was cleared\r\n");
            bkp_boot = 0;
        }
    } else {
        UART_Printf(">>> NORMAL BOOT (first power-on) <<<\r\n\r\n");
        bkp_boot = 0;
    }
    /* Show current state of all backup registers */
    Print_Backup_Registers();
    /* SRAM variables always start at 0 after reset */
    sram_boot_count = 0;
    sram_sensor_val = 0;
    /* Show comparison */
    Print_Persistence_Comparison(bkp_boot, bkp_sensor);
    /* Active phase */
    UART_Printf(">>> ACTIVE phase: LED blink + sensor read...\r\n");
    LED_Blink(3, 300);
    /* Simulate sensor reading */
    uint16_t new_sensor = Read_Simulated_Sensor();
    uint32_t new_boot   = bkp_boot + 1;
    uint32_t rtc_counter = RTC->CNTH << 16 | RTC->CNTL;
    uint32_t prev_runtime = HAL_RTCEx_BKUPRead(&hrtc, BKP_TOTAL_RUNTIME);
    uint32_t prev_timestamp = HAL_RTCEx_BKUPRead(&hrtc, BKP_TIMESTAMP);
    uint32_t delta = (prev_timestamp > 0) ? (rtc_counter - prev_timestamp) : 0;
    uint32_t new_runtime = prev_runtime + delta;
    UART_Printf("\r\n[DATA] New boot count   : %lu\r\n", new_boot);
    UART_Printf("[DATA] New sensor value : %u\r\n", new_sensor);
    UART_Printf("[DATA] RTC counter      : %lu\r\n", rtc_counter);
    UART_Printf("[DATA] Accumulated RT   : %lu sec\r\n\r\n", new_runtime);
    /* Write new data to backup registers */
    BKP_Init();
    HAL_RTCEx_BKUPWrite(&hrtc, BKP_BOOT_COUNT,    new_boot);
    HAL_RTCEx_BKUPWrite(&hrtc, BKP_SENSOR_VALUE,   new_sensor);
    HAL_RTCEx_BKUPWrite(&hrtc, BKP_MAGIC_KEY,      MAGIC_VALUE);
    HAL_RTCEx_BKUPWrite(&hrtc, BKP_TIMESTAMP,      rtc_counter);
    HAL_RTCEx_BKUPWrite(&hrtc, BKP_TOTAL_RUNTIME,  new_runtime & 0xFFFF);
    HAL_RTCEx_BKUPWrite(&hrtc, BKP_STATUS_FLAGS,    0x0001); /* bit 0 = data valid */
    UART_Printf("[BKP] Data written to backup registers:\r\n");
    Print_Backup_Registers();
    /* Also update SRAM (will be lost) */
    sram_boot_count = new_boot;
    sram_sensor_val = new_sensor;
    sram_magic      = MAGIC_VALUE;
    UART_Printf("[INFO] SRAM variables also set (will be LOST in Standby)\r\n\r\n");
    /* Prepare for Standby */
    UART_Printf("[STANDBY] Entering Standby in 2 seconds...\r\n");
    vTaskDelay(pdMS_TO_TICKS(2000));
    RTC_SetAlarm_ForStandby(STANDBY_SECONDS);
    UART_Printf("[STANDBY] Goodbye! See you after wakeup...\r\n\r\n");
    vTaskDelay(pdMS_TO_TICKS(100));
    HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET);
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);
    HAL_PWR_EnterSTANDBYMode();
    /* Should never reach here */
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
    /* Check standby flag early */
    __HAL_RCC_PWR_CLK_ENABLE();
    if (__HAL_PWR_GET_FLAG(PWR_FLAG_SB) != RESET) {
        from_standby = 1;
        __HAL_PWR_CLEAR_FLAG(PWR_FLAG_SB);
        __HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);
    }
    GPIO_Init();
    UART_Init();
    xTaskCreate(DataRetentionTask, "DataRet", MAIN_TASK_STACK_SIZE,
                NULL, MAIN_TASK_PRIORITY, NULL);
    vTaskStartScheduler();
    while (1);
    return 0;
}
