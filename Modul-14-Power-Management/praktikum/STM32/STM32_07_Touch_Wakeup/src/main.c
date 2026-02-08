/* ============================================================================
 * STM32_07_Touch_Wakeup - WKUP Pin (PA0) Wakeup from Standby
 * ============================================================================
 * STM32 tidak punya touch sensor seperti ESP32, gunakan WKUP pin sebagai
 * alternatif. PA0 adalah WKUP pin pada STM32F103.
 *
 * - PA0 is the WKUP pin on STM32F103
 * - Enable WKUP pin: HAL_PWR_EnableWakeUpPin(PWR_WAKEUP_PIN1)
 * - Enter standby mode
 * - Rising edge on PA0 wakes MCU
 * - Check if woke from standby: __HAL_PWR_GET_FLAG(PWR_FLAG_SB)
 * - Track wakeup count in backup register
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

/* Backup register map */
#define BKP_WAKEUP_COUNT    RTC_BKP_DR1
#define BKP_MAGIC           RTC_BKP_DR2
#define BKP_LAST_SOURCE     RTC_BKP_DR3
#define BKP_TOTAL_AWAKE_MS  RTC_BKP_DR4
#define BKP_TIMESTAMPS      RTC_BKP_DR5

#define MAGIC_VALUE         0xBEEF
#define AWAKE_PERIOD_MS     5000   /* Stay awake for 5 seconds before standby */

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
    if (len > 0) HAL_UART_Transmit(&huart1, (uint8_t *)buf, len, HAL_MAX_DELAY);
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

/* ---- WKUP Pin Wakeup Task ------------------------------------------------ */
static void vWakeupTask(void *pvParameters)
{
    (void)pvParameters;

    uint32_t wakeup_count = 0;
    uint32_t total_awake  = 0;
    uint8_t  from_standby = 0;

    /* Check if woke from Standby */
    if (__HAL_PWR_GET_FLAG(PWR_FLAG_SB)) {
        __HAL_PWR_CLEAR_FLAG(PWR_FLAG_SB);
        __HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);
        from_standby = 1;
    }

    /* Read backup registers */
    if (HAL_RTCEx_BKUPRead(&hrtc, BKP_MAGIC) == MAGIC_VALUE) {
        wakeup_count = HAL_RTCEx_BKUPRead(&hrtc, BKP_WAKEUP_COUNT);
        total_awake  = HAL_RTCEx_BKUPRead(&hrtc, BKP_TOTAL_AWAKE_MS);
    }

    if (from_standby) {
        wakeup_count++;
    }

    UART_Printf("\r\n========================================\r\n");
    UART_Printf("  STM32 WKUP Pin Wakeup Demo (PA0)\r\n");
    UART_Printf("========================================\r\n");
    UART_Printf("NOTE: STM32 tidak punya touch sensor seperti ESP32,\r\n");
    UART_Printf("      gunakan WKUP pin (PA0) sebagai alternatif.\r\n\r\n");

    if (from_standby) {
        UART_Printf("[WAKEUP] Woke from STANDBY via PA0 WKUP pin!\r\n");
        UART_Printf("[WAKEUP] Source: Rising edge on PA0\r\n");
    } else {
        UART_Printf("[BOOT] Fresh power-on / reset (not from standby)\r\n");
        wakeup_count = 0;
        total_awake  = 0;
    }

    UART_Printf("[INFO] Wakeup count: %lu\r\n", wakeup_count);
    UART_Printf("[INFO] Total awake time: %lu ms\r\n", total_awake);
    UART_Printf("[INFO] System clock: %lu Hz\r\n", HAL_RCC_GetSysClockFreq());

    /* Blink LED to indicate wakeup number */
    UART_Printf("[LED] Blinking %lu times for wakeup count...\r\n",
                wakeup_count > 0 ? wakeup_count : 1);
    uint32_t blinks = (wakeup_count > 0) ? wakeup_count : 1;
    if (blinks > 10) blinks = 10;  /* Cap at 10 blinks */
    for (uint32_t i = 0; i < blinks; i++) {
        HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_RESET); /* ON (active low) */
        vTaskDelay(pdMS_TO_TICKS(200));
        HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET);   /* OFF */
        vTaskDelay(pdMS_TO_TICKS(200));
    }

    /* Stay awake, print countdown */
    UART_Printf("\r\n[COUNTDOWN] Staying awake for %d seconds...\r\n", AWAKE_PERIOD_MS / 1000);
    uint32_t awake_start = HAL_GetTick();
    for (int sec = AWAKE_PERIOD_MS / 1000; sec > 0; sec--) {
        UART_Printf("  Entering standby in %d seconds...\r\n", sec);
        HAL_GPIO_TogglePin(LED_PORT, LED_PIN);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    uint32_t awake_elapsed = HAL_GetTick() - awake_start;
    total_awake += awake_elapsed;

    /* Save state to backup registers */
    HAL_RTCEx_BKUPWrite(&hrtc, BKP_MAGIC, MAGIC_VALUE);
    HAL_RTCEx_BKUPWrite(&hrtc, BKP_WAKEUP_COUNT, wakeup_count);
    HAL_RTCEx_BKUPWrite(&hrtc, BKP_TOTAL_AWAKE_MS, total_awake);
    HAL_RTCEx_BKUPWrite(&hrtc, BKP_LAST_SOURCE, 1); /* 1 = WKUP pin */
    HAL_RTCEx_BKUPWrite(&hrtc, BKP_TIMESTAMPS, HAL_GetTick());

    UART_Printf("\r\n[STANDBY] Saved state to backup registers\r\n");
    UART_Printf("[STANDBY] Wakeup count saved: %lu\r\n", wakeup_count);
    UART_Printf("[STANDBY] Total awake time saved: %lu ms\r\n", total_awake);
    UART_Printf("[STANDBY] Enabling WKUP pin (PA0)...\r\n");
    UART_Printf("[STANDBY] Press button on PA0 (rising edge) to wake up!\r\n");
    UART_Printf("[STANDBY] Entering STANDBY mode NOW.\r\n\r\n");

    /* Small delay for UART to finish */
    vTaskDelay(pdMS_TO_TICKS(100));

    /* Enable WKUP pin and enter standby */
    HAL_PWR_EnableWakeUpPin(PWR_WAKEUP_PIN1);
    __HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);
    HAL_PWR_EnterSTANDBYMode();

    /* Should never reach here */
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/* ---- FreeRTOS Hooks ------------------------------------------------------ */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    UART_Printf("[FATAL] Stack overflow in task: %s\r\n", pcTaskName);
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

    xTaskCreate(vWakeupTask, "WKUP", MAIN_TASK_STACK_SIZE, NULL,
                MAIN_TASK_PRIORITY, NULL);
    vTaskStartScheduler();
    for (;;);
}
