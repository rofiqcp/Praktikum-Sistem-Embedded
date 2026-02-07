/**
 * ================================================================================
 * PROGRAM 2: STOP MODE DENGAN EXTI WAKEUP - STM32F103C8T6
 * ================================================================================
 * Deskripsi:
 *   Demonstrasi Stop Mode pada STM32F103. Pada Stop Mode:
 *   - Semua clock BERHENTI (HSE, HSI, PLL OFF)
 *   - Voltage regulator dalam mode low-power
 *   - SRAM dan register TETAP tersimpan
 *   - Hanya EXTI (external interrupt) yang bisa membangunkan
 *   - Konsumsi: ~20µA (vs ~30mA saat aktif)
 *
 *   PENTING: Setelah wake-up dari Stop Mode, clock kembali ke HSI 8MHz!
 *   Harus rekonfigurasi SystemClock ke 72MHz (PLL).
 *
 * Hardware:
 *   - STM32F103C8T6 (Blue Pill)
 *   - LED built-in di PC13 (active low)
 *   - Button di PA0 (EXTI0, falling edge, internal pull-up)
 *   - UART1 (PA9=TX, PA10=RX, 115200 baud)
 *
 * ================================================================================
 */

#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include <string.h>
#include <stdio.h>

UART_HandleTypeDef huart1;
static volatile uint8_t wakeup_flag = 0;
static uint32_t stop_count = 0;

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

    /* Button PA0 EXTI */
    GPIO_InitStruct.Pin = GPIO_PIN_0;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    HAL_NVIC_SetPriority(EXTI0_IRQn, 6, 0);
    HAL_NVIC_EnableIRQ(EXTI0_IRQn);
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

/* ================================================================================
 * INTERRUPT HANDLERS
 * ================================================================================ */

void EXTI0_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_0);
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == GPIO_PIN_0) {
        wakeup_flag = 1;
    }
}

/* ================================================================================
 * FREERTOS TASK
 * ================================================================================ */

static void StopModeTask(void *pvParameters)
{
    UART_SendString("\r\n");
    UART_SendString("========================================\r\n");
    UART_SendString("  STM32 Stop Mode + EXTI Wakeup Demo\r\n");
    UART_SendString("  Modul 14: Power Management\r\n");
    UART_SendString("========================================\r\n");
    UART_SendString("\r\n");
    UART_SendString("Stop Mode: ALL clocks OFF, ~20uA\r\n");
    UART_SendString("Wake source: EXTI0 (PA0 button)\r\n");
    UART_SendString("NOTE: Clock resets to HSI 8MHz after wake!\r\n");
    UART_SendString("\r\n");

    while (1) {
        /* Fase Aktif */
        UART_Printf("[ACTIVE] SYSCLK = %lu Hz\r\n", HAL_RCC_GetSysClockFreq());
        UART_SendString("[ACTIVE] Running for 3 seconds...\r\n");

        for (int i = 0; i < 6; i++) {
            HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
            vTaskDelay(pdMS_TO_TICKS(500));
        }

        /* Masuk Stop Mode */
        stop_count++;
        UART_Printf("[STOP] Entering Stop Mode #%lu...\r\n", stop_count);
        UART_SendString("[STOP] All clocks will be OFF\r\n");
        UART_SendString("[STOP] Press PA0 button to wake!\r\n");
        vTaskDelay(pdMS_TO_TICKS(50));

        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET); /* LED off */
        wakeup_flag = 0;

        /* === MASUK STOP MODE === */
        /*
         * HAL_PWR_EnterSTOPMode:
         * - PWR_LOWPOWERREGULATOR_ON: regulator low-power aktif
         * - PWR_STOPENTRY_WFI: tunggu interrupt
         *
         * Setelah wake-up: clock = HSI 8MHz!
         * HARUS rekonfigurasi PLL ke 72MHz!
         */
        HAL_SuspendTick();
        HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);
        /* === MCU BERHENTI DI SINI === */
        /* === LANJUT SETELAH EXTI === */

        /* KRITIS: Rekonfigurasi clock ke 72MHz! */
        SystemClock_Config();
        HAL_ResumeTick();

        /* Re-init UART karena clock berubah */
        MX_USART1_UART_Init();

        UART_SendString("\r\n[WAKEUP] Woke from STOP mode!\r\n");
        UART_Printf("[WAKEUP] Clock restored: %lu Hz\r\n",
                     HAL_RCC_GetSysClockFreq());

        if (wakeup_flag) {
            UART_SendString("[WAKEUP] Source: EXTI0 (Button PA0)\r\n");
        }

        /* LED indikator wake-up (pola berbeda: 2 blink cepat) */
        for (int i = 0; i < 4; i++) {
            HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
            vTaskDelay(pdMS_TO_TICKS(80));
        }
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);

        UART_Printf("[INFO] Total stop cycles: %lu\r\n\r\n", stop_count);
    }
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

    __HAL_RCC_PWR_CLK_ENABLE();

    xTaskCreate(StopModeTask, "StopMode", 256, NULL,
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

/* ================================================================================
 * FREERTOS HOOKS
 * ================================================================================ */

void vApplicationMallocFailedHook(void)
{
    UART_SendString("[ERROR] Malloc failed!\r\n");
    while (1) {}
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    UART_Printf("[ERROR] Stack overflow: %s\r\n", pcTaskName);
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
