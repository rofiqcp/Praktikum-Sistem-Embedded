/**
 * ================================================================================
 * PROGRAM 6: BACKUP REGISTER PERSISTENCE - STM32F103C8T6
 * ================================================================================
 * Deskripsi:
 *   Demonstrasi penggunaan Backup Register untuk menyimpan data yang
 *   bertahan saat MCU dalam Standby Mode atau setelah system reset.
 *
 *   STM32F103 memiliki 10 Backup Registers (BKP_DR1..BKP_DR10),
 *   masing-masing 16-bit. Data hilang hanya jika VBAT terputus.
 *
 *   Program menyimpan:
 *   - Boot counter (BKP_DR1)
 *   - Timestamp terakhir (BKP_DR2-DR3, 32-bit)
 *   - Sensor data terakhir (BKP_DR4)
 *   - Status flag (BKP_DR5)
 *
 * Hardware:
 *   - STM32F103C8T6 (Blue Pill)
 *   - LED PC13 / UART1 (PA9/PA10)
 *   - Optional: VBAT pin ke coin cell untuk persistence tanpa VDD
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

/* Backup Register mapping */
#define BKP_BOOT_COUNT      RTC_BKP_DR1
#define BKP_TIMESTAMP_H     RTC_BKP_DR2
#define BKP_TIMESTAMP_L     RTC_BKP_DR3
#define BKP_SENSOR_DATA     RTC_BKP_DR4
#define BKP_STATUS_FLAGS    RTC_BKP_DR5
#define BKP_MAGIC           RTC_BKP_DR6

#define MAGIC_VALUE         0xBEEF

/* Status flags */
#define FLAG_NORMAL_BOOT    0x0001
#define FLAG_STANDBY_WAKE   0x0002
#define FLAG_DATA_VALID     0x0004

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

/* ================================================================================
 * BACKUP REGISTER FUNCTIONS
 * ================================================================================ */

static void BKP_Write32(uint32_t reg_h, uint32_t reg_l, uint32_t value)
{
    HAL_RTCEx_BKUPWrite(&hrtc, reg_h, (value >> 16) & 0xFFFF);
    HAL_RTCEx_BKUPWrite(&hrtc, reg_l, value & 0xFFFF);
}

static uint32_t BKP_Read32(uint32_t reg_h, uint32_t reg_l)
{
    uint32_t val = HAL_RTCEx_BKUPRead(&hrtc, reg_h);
    val = (val << 16) | HAL_RTCEx_BKUPRead(&hrtc, reg_l);
    return val;
}

/* ================================================================================
 * FREERTOS TASK
 * ================================================================================ */

static void BackupRegTask(void *pvParameters)
{
    UART_SendString("\r\n");
    UART_SendString("==========================================\r\n");
    UART_SendString("  STM32 Backup Register Persistence\r\n");
    UART_SendString("  Modul 14: Power Management\r\n");
    UART_SendString("==========================================\r\n");
    UART_SendString("\r\n");

    /* Cek apakah ini fresh boot (magic value hilang = VBAT terputus) */
    uint32_t magic = HAL_RTCEx_BKUPRead(&hrtc, BKP_MAGIC);
    uint8_t fresh_boot = (magic != MAGIC_VALUE);

    if (fresh_boot) {
        UART_SendString("[INIT] Fresh boot detected (VBAT was lost)\r\n");
        UART_SendString("[INIT] Initializing backup registers...\r\n");

        HAL_RTCEx_BKUPWrite(&hrtc, BKP_BOOT_COUNT, 0);
        BKP_Write32(BKP_TIMESTAMP_H, BKP_TIMESTAMP_L, 0);
        HAL_RTCEx_BKUPWrite(&hrtc, BKP_SENSOR_DATA, 0);
        HAL_RTCEx_BKUPWrite(&hrtc, BKP_STATUS_FLAGS, FLAG_NORMAL_BOOT);
        HAL_RTCEx_BKUPWrite(&hrtc, BKP_MAGIC, MAGIC_VALUE);
    }

    /* Cek standby wake */
    if (__HAL_PWR_GET_FLAG(PWR_FLAG_SB) != RESET) {
        UART_SendString("[WAKEUP] From Standby Mode\r\n");
        uint16_t flags = HAL_RTCEx_BKUPRead(&hrtc, BKP_STATUS_FLAGS);
        flags |= FLAG_STANDBY_WAKE;
        HAL_RTCEx_BKUPWrite(&hrtc, BKP_STATUS_FLAGS, flags);
        __HAL_PWR_CLEAR_FLAG(PWR_FLAG_SB);
        __HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);
    }

    /* Update boot counter */
    uint16_t boot_count = HAL_RTCEx_BKUPRead(&hrtc, BKP_BOOT_COUNT);
    boot_count++;
    HAL_RTCEx_BKUPWrite(&hrtc, BKP_BOOT_COUNT, boot_count);

    /* Update timestamp */
    uint32_t timestamp = HAL_GetTick();
    BKP_Write32(BKP_TIMESTAMP_H, BKP_TIMESTAMP_L, timestamp);

    /* Simulated sensor data */
    uint16_t sensor = (uint16_t)(HAL_GetTick() & 0x0FFF);
    HAL_RTCEx_BKUPWrite(&hrtc, BKP_SENSOR_DATA, sensor);

    uint16_t flags = HAL_RTCEx_BKUPRead(&hrtc, BKP_STATUS_FLAGS);
    flags |= FLAG_DATA_VALID;
    HAL_RTCEx_BKUPWrite(&hrtc, BKP_STATUS_FLAGS, flags);

    /* Tampilkan semua Backup Register */
    UART_SendString("\r\n");
    UART_SendString("Backup Register Contents:\r\n");
    UART_SendString("+---------+---------+---------------------+\r\n");
    UART_SendString("| Reg     | Value   | Description         |\r\n");
    UART_SendString("+---------+---------+---------------------+\r\n");
    UART_Printf("| BKP_DR1 | 0x%04X  | Boot count: %u      |\r\n",
                boot_count, boot_count);
    UART_Printf("| BKP_DR2 | 0x%04X  | Timestamp (H)       |\r\n",
                HAL_RTCEx_BKUPRead(&hrtc, BKP_TIMESTAMP_H));
    UART_Printf("| BKP_DR3 | 0x%04X  | Timestamp (L)       |\r\n",
                HAL_RTCEx_BKUPRead(&hrtc, BKP_TIMESTAMP_L));
    UART_Printf("| BKP_DR4 | 0x%04X  | Sensor data         |\r\n",
                sensor);
    UART_Printf("| BKP_DR5 | 0x%04X  | Status flags        |\r\n",
                flags);
    UART_Printf("| BKP_DR6 | 0x%04X  | Magic (0xBEEF)      |\r\n",
                HAL_RTCEx_BKUPRead(&hrtc, BKP_MAGIC));
    UART_SendString("+---------+---------+---------------------+\r\n");

    UART_SendString("\r\nFlags:\r\n");
    UART_Printf("  NORMAL_BOOT  : %s\r\n", (flags & FLAG_NORMAL_BOOT) ? "YES" : "NO");
    UART_Printf("  STANDBY_WAKE : %s\r\n", (flags & FLAG_STANDBY_WAKE) ? "YES" : "NO");
    UART_Printf("  DATA_VALID   : %s\r\n", (flags & FLAG_DATA_VALID) ? "YES" : "NO");

    /* LED blink */
    UART_SendString("\r\n[ACTIVE] Active for 5 seconds...\r\n");
    for (int i = 0; i < 10; i++) {
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
        vTaskDelay(pdMS_TO_TICKS(500));
    }

    /* Masuk Standby untuk demo persistence */
    UART_SendString("\r\n[STANDBY] Entering Standby...\r\n");
    UART_SendString("[STANDBY] Backup registers will SURVIVE!\r\n");
    UART_SendString("[STANDBY] Press RESET or wait WKUP pin\r\n");
    vTaskDelay(pdMS_TO_TICKS(50));

    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);

    HAL_PWR_EnableWakeUpPin(PWR_WAKEUP_PIN1);
    __HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);
    HAL_PWR_EnterSTANDBYMode();

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

    xTaskCreate(BackupRegTask, "BkpReg", 512, NULL,
                tskIDLE_PRIORITY + 2, NULL);

    vTaskStartScheduler();
    while (1) {}
}

/* ================================================================================
 * EXCEPTION HANDLERS & FREERTOS HOOKS
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

void vApplicationMallocFailedHook(void) { while (1) {} }
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask; (void)pcTaskName; while (1) {}
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
