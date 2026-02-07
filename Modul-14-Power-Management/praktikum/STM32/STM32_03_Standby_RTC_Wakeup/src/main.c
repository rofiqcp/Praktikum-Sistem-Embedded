/**
 * ================================================================================
 * PROGRAM 3: STANDBY MODE + RTC ALARM WAKEUP - STM32F103C8T6
 * ================================================================================
 * Deskripsi:
 *   Demonstrasi Standby Mode, mode daya terendah STM32F103:
 *   - 1.8V domain DIMATIKAN total
 *   - SRAM hilang, register hilang
 *   - Hanya RTC, Backup register, dan WKUP pin yang tetap hidup
 *   - Konsumsi: ~2µA
 *   - Setelah wake-up → MCU melakukan RESET (seperti power-on)
 *
 *   Wake-up source: RTC Alarm (bangun otomatis setelah N detik)
 *   Backup Register digunakan untuk menyimpan counter antar reset.
 *
 * Hardware:
 *   - STM32F103C8T6 (Blue Pill)
 *   - LED PC13 / UART1 (PA9/PA10)
 *   - RTC menggunakan LSI (~40kHz internal oscillator)
 *
 * ================================================================================
 */

#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include <string.h>
#include <stdio.h>

UART_HandleTypeDef huart1;
RTC_HandleTypeDef hrtc;

/* ================================================================================
 * FUNGSI UTILITAS
 * ================================================================================ */

static void UART_SendString(const char *str)
{
    HAL_UART_Transmit(&huart1, (uint8_t *)str, strlen(str), HAL_MAX_DELAY);
}

static void UART_Printf(const char *fmt, ...)
{
    char buf[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    UART_SendString(buf);
}

/* ================================================================================
 * KONFIGURASI HARDWARE
 * ================================================================================ */

static void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

    /* HSE → PLL → 72MHz */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE | RCC_OSCILLATORTYPE_LSI;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    RCC_OscInitStruct.LSIState = RCC_LSI_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
    HAL_RCC_OscConfig(&RCC_OscInitStruct);

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                  RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2);

    /* RTC clock dari LSI */
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_RTC;
    PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_LSI;
    HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit);
}

static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    GPIO_InitStruct.Pin = GPIO_PIN_13;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
}

static void MX_USART1_UART_Init(void)
{
    __HAL_RCC_USART1_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    huart1.Instance = USART1;
    huart1.Init.BaudRate = 115200;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    HAL_UART_Init(&huart1);
}

static void MX_RTC_Init(void)
{
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_RCC_BKP_CLK_ENABLE();
    HAL_PWR_EnableBkUpAccess();

    hrtc.Instance = RTC;
    hrtc.Init.AsynchPrediv = RTC_AUTO_1_SECOND;
    hrtc.Init.OutPut = RTC_OUTPUTSOURCE_NONE;
    HAL_RTC_Init(&hrtc);
}

static void Set_RTC_Alarm_Seconds(uint32_t seconds)
{
    RTC_AlarmTypeDef sAlarm = {0};

    /* Baca counter RTC saat ini */
    uint32_t current = HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR1);
    /* Gunakan counter RTC langsung */
    uint32_t cnt = RTC->CNTH;
    cnt = (cnt << 16) | RTC->CNTL;

    /* Set alarm = current + seconds */
    sAlarm.AlarmTime.Hours = 0;
    sAlarm.AlarmTime.Minutes = 0;
    sAlarm.AlarmTime.Seconds = 0;
    sAlarm.Alarm = RTC_ALARM_A;

    /* Set alarm counter langsung */
    HAL_RTC_SetAlarm_IT(&hrtc, &sAlarm, RTC_FORMAT_BIN);

    /* Override alarm register secara manual */
    while (!(RTC->CRL & RTC_CRL_RTOFF)) {}
    RTC->CRL |= RTC_CRL_CNF;
    uint32_t alarm_val = cnt + seconds;
    RTC->ALRH = (alarm_val >> 16) & 0xFFFF;
    RTC->ALRL = alarm_val & 0xFFFF;
    RTC->CRL &= ~RTC_CRL_CNF;
    while (!(RTC->CRL & RTC_CRL_RTOFF)) {}
}

/* ================================================================================
 * FREERTOS TASK
 * ================================================================================ */

static void StandbyTask(void *pvParameters)
{
    /* Baca boot counter dari Backup Register */
    uint32_t boot_count = HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR2);
    boot_count++;
    HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR2, boot_count);

    UART_SendString("\r\n");
    UART_SendString("========================================\r\n");
    UART_SendString("  STM32 Standby Mode + RTC Alarm\r\n");
    UART_SendString("  Modul 14: Power Management\r\n");
    UART_SendString("========================================\r\n");
    UART_SendString("\r\n");

    /* Cek apakah bangun dari Standby */
    if (__HAL_PWR_GET_FLAG(PWR_FLAG_SB) != RESET) {
        UART_SendString("[WAKEUP] Woke from STANDBY mode!\r\n");
        UART_SendString("[WAKEUP] Source: RTC Alarm\r\n");
        __HAL_PWR_CLEAR_FLAG(PWR_FLAG_SB);
        __HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);
    } else {
        UART_SendString("[BOOT] Normal power-on reset\r\n");
    }

    UART_Printf("[INFO] Boot count (from backup reg): %lu\r\n", boot_count);
    UART_SendString("[INFO] SRAM data is LOST after standby!\r\n");

    /* LED indikator: blink sesuai boot_count (max 5) */
    uint32_t blinks = (boot_count > 5) ? 5 : boot_count;
    for (uint32_t i = 0; i < blinks; i++) {
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
        vTaskDelay(pdMS_TO_TICKS(200));
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
        vTaskDelay(pdMS_TO_TICKS(200));
    }

    /* Fase aktif 5 detik */
    UART_SendString("\r\n[ACTIVE] Active for 5 seconds...\r\n");
    UART_SendString("[ACTIVE] Simulating sensor read...\r\n");
    vTaskDelay(pdMS_TO_TICKS(5000));

    /* Set RTC Alarm untuk 10 detik */
    UART_SendString("\r\n[STANDBY] Setting RTC alarm: 10 seconds\r\n");
    UART_SendString("[STANDBY] Entering STANDBY mode...\r\n");
    UART_SendString("[STANDBY] Current: ~2uA\r\n");
    UART_SendString("[STANDBY] ALL SRAM will be LOST!\r\n");
    UART_SendString("[STANDBY] Only Backup Registers survive\r\n");
    vTaskDelay(pdMS_TO_TICKS(50));

    /* Konfigurasi RTC Alarm */
    Set_RTC_Alarm_Seconds(10);

    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET); /* LED off */

    /* === MASUK STANDBY MODE === */
    /*
     * Setelah ini, MCU akan RESET total
     * Hanya Backup Register yang bertahan
     * Bangun oleh: RTC Alarm, WKUP pin, NRST, IWDG
     */
    HAL_PWR_EnterSTANDBYMode();

    /* Kode ini TIDAK PERNAH tercapai! */
    UART_SendString("[ERROR] Should never reach here!\r\n");
    while (1) {}
}

/* ================================================================================
 * MAIN
 * ================================================================================ */

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USART1_UART_Init();
    MX_RTC_Init();

    __HAL_RCC_PWR_CLK_ENABLE();

    xTaskCreate(StandbyTask, "Standby", 512, NULL,
                tskIDLE_PRIORITY + 2, NULL);

    vTaskStartScheduler();
    while (1) {}
}

/* ================================================================================
 * EXCEPTION HANDLERS
 * ================================================================================ */

void NMI_Handler(void) {}
void HardFault_Handler(void) { while (1) {} }
void MemManage_Handler(void) { while (1) {} }
void BusFault_Handler(void) { while (1) {} }
void UsageFault_Handler(void) { while (1) {} }
void DebugMon_Handler(void) {}

void SysTick_Handler(void)
{
    HAL_IncTick();
    if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED) {
        xPortSysTickHandler();
    }
}

void RTC_Alarm_IRQHandler(void)
{
    HAL_RTC_AlarmIRQHandler(&hrtc);
}

/* ================================================================================
 * FREERTOS HOOKS
 * ================================================================================ */

void vApplicationMallocFailedHook(void)
{
    while (1) {}
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    (void)pcTaskName;
    while (1) {}
}

static StaticTask_t xIdleTaskTCB;
static StackType_t uxIdleTaskStack[configMINIMAL_STACK_SIZE];

void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer,
                                   StackType_t **ppxIdleTaskStackBuffer,
                                   uint32_t *pulIdleTaskStackSize)
{
    *ppxIdleTaskTCBBuffer = &xIdleTaskTCB;
    *ppxIdleTaskStackBuffer = uxIdleTaskStack;
    *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}

static StaticTask_t xTimerTaskTCB;
static StackType_t uxTimerTaskStack[configTIMER_TASK_STACK_DEPTH];

void vApplicationGetTimerTaskMemory(StaticTask_t **ppxTimerTaskTCBBuffer,
                                    StackType_t **ppxTimerTaskStackBuffer,
                                    uint32_t *pulTimerTaskStackSize)
{
    *ppxTimerTaskTCBBuffer = &xTimerTaskTCB;
    *ppxTimerTaskStackBuffer = uxTimerTaskStack;
    *pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
}
