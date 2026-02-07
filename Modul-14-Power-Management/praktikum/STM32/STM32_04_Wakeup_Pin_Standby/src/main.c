/**
 * ================================================================================
 * PROGRAM 4: WAKEUP PIN + STANDBY MODE - STM32F103C8T6
 * ================================================================================
 * Deskripsi:
 *   Demonstrasi Standby Mode dengan WKUP Pin (PA0).
 *   Berbeda dari EXTI, WKUP pin adalah fitur khusus PWR yang dapat
 *   membangunkan MCU dari Standby Mode (yang merupakan mode EXTI
 *   TIDAK bisa membangunkan).
 *
 *   PA0 = WKUP pin pada STM32F103 (rising edge)
 *   Konsumsi: ~2µA dalam Standby
 *
 * Hardware:
 *   - STM32F103C8T6 (Blue Pill)
 *   - Button di PA0 (active high untuk WKUP - rising edge)
 *   - LED built-in PC13
 *   - UART1 (PA9/PA10)
 *
 * CATATAN: WKUP pin = rising edge pada PA0!
 *          Berbeda dari EXTI yang bisa falling/rising.
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

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
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
}

static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    /* LED PC13 */
    GPIO_InitStruct.Pin = GPIO_PIN_13;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);

    /* PA0 sebagai input (WKUP pin) */
    GPIO_InitStruct.Pin = GPIO_PIN_0;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;  /* Pull-down karena WKUP = rising edge */
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
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
 * FREERTOS TASK
 * ================================================================================ */

static void WakeupPinTask(void *pvParameters)
{
    /* Baca boot counter dari Backup Register */
    uint32_t boot_count = HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR3);
    boot_count++;
    HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR3, boot_count);

    UART_SendString("\r\n");
    UART_SendString("========================================\r\n");
    UART_SendString("  STM32 Wakeup Pin + Standby Demo\r\n");
    UART_SendString("  Modul 14: Power Management\r\n");
    UART_SendString("========================================\r\n");
    UART_SendString("\r\n");

    /* Cek source reset */
    if (__HAL_PWR_GET_FLAG(PWR_FLAG_SB) != RESET) {
        UART_SendString("[WAKEUP] Woke from STANDBY mode!\r\n");

        if (__HAL_PWR_GET_FLAG(PWR_FLAG_WU) != RESET) {
            UART_SendString("[WAKEUP] Source: WKUP Pin (PA0 rising)\r\n");
        }

        __HAL_PWR_CLEAR_FLAG(PWR_FLAG_SB);
        __HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);
    } else {
        UART_SendString("[BOOT] Normal power-on reset\r\n");
    }

    UART_Printf("[INFO] Boot count: %lu\r\n", boot_count);
    UART_SendString("\r\n");

    /* Tampilkan info WKUP pin */
    UART_SendString("WKUP Pin Info:\r\n");
    UART_SendString("  Pin    : PA0\r\n");
    UART_SendString("  Trigger: Rising edge ONLY\r\n");
    UART_SendString("  Note   : Different from EXTI!\r\n");
    UART_SendString("  Use    : External pull-down + button to VCC\r\n");
    UART_SendString("\r\n");

    /* LED pattern: boot_count blinks */
    UART_Printf("[LED] Blinking %lu times...\r\n",
                (boot_count > 10) ? (uint32_t)10 : boot_count);

    uint32_t blinks = (boot_count > 10) ? 10 : boot_count;
    for (uint32_t i = 0; i < blinks; i++) {
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
        vTaskDelay(pdMS_TO_TICKS(150));
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
        vTaskDelay(pdMS_TO_TICKS(150));
    }

    /* Aktif 3 detik */
    UART_SendString("\r\n[ACTIVE] Active for 3 seconds...\r\n");
    vTaskDelay(pdMS_TO_TICKS(3000));

    /* Masuk Standby */
    UART_SendString("\r\n[STANDBY] Entering STANDBY mode...\r\n");
    UART_SendString("[STANDBY] Press PA0 (rising edge) to wake!\r\n");
    UART_SendString("[STANDBY] Current: ~2uA\r\n");
    vTaskDelay(pdMS_TO_TICKS(50));

    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);

    /* Enable WKUP pin */
    HAL_PWR_EnableWakeUpPin(PWR_WAKEUP_PIN1);  /* PA0 */

    /* Clear wake-up flag */
    __HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);

    /* === MASUK STANDBY === */
    HAL_PWR_EnterSTANDBYMode();

    /* Tidak pernah tercapai */
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

    xTaskCreate(WakeupPinTask, "WkupPin", 512, NULL,
                tskIDLE_PRIORITY + 2, NULL);

    vTaskStartScheduler();
    while (1) {}
}

/* ================================================================================
 * EXCEPTION & FREERTOS HANDLERS
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
    (void)xTask; (void)pcTaskName;
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
