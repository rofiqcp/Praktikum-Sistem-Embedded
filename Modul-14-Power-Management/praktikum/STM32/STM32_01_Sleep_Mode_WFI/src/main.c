/**
 * ================================================================================
 * PROGRAM 1: SLEEP MODE WFI (Wait For Interrupt) - STM32F103C8T6
 * ================================================================================
 * Deskripsi:
 *   Demonstrasi Sleep Mode pada STM32F103. Pada sleep mode, CPU berhenti 
 *   tetapi semua peripheral (SRAM, Flash, clock tree) tetap aktif.
 *   MCU bangun dengan interrupt apapun (EXTI, timer, UART, dll).
 *   Konsumsi arus: ~2mA (vs ~30mA saat aktif pada 72MHz)
 *
 * Hardware:
 *   - STM32F103C8T6 (Blue Pill)
 *   - LED built-in di PC13 (active low)
 *   - Button di PA0 (active low, internal pull-up)
 *   - UART1 (PA9=TX, PA10=RX, 115200 baud)
 *
 * Framework: STM32Cube HAL + FreeRTOS
 *
 * Cara Kerja:
 *   1. Task utama berjalan dan LED berkedip
 *   2. Setelah 5 detik, MCU masuk Sleep Mode (WFI)
 *   3. Button press (EXTI0) membangunkan MCU
 *   4. LED berkedip cepat sebagai indikator wake-up
 *   5. Kembali ke langkah 2
 *
 * ================================================================================
 */

#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include <string.h>
#include <stdio.h>

/* ================================================================================
 * VARIABEL GLOBAL
 * ================================================================================ */

UART_HandleTypeDef huart1;
static volatile uint8_t wakeup_flag = 0;
static uint32_t sleep_count = 0;

/* ================================================================================
 * FUNGSI UTILITAS UART
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

    /* HSE 8MHz → PLL → 72MHz */
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

    /* LED PC13 (active low) */
    GPIO_InitStruct.Pin = GPIO_PIN_13;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET); /* LED off */

    /* Button PA0 with EXTI (interrupt on falling edge) */
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

    /* PA9 = TX */
    GPIO_InitStruct.Pin = GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* PA10 = RX */
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
 * EXTI INTERRUPT HANDLER
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

static void PowerManagementTask(void *pvParameters)
{
    UART_SendString("\r\n");
    UART_SendString("========================================\r\n");
    UART_SendString("  STM32 Sleep Mode WFI Demo\r\n");
    UART_SendString("  Modul 14: Power Management\r\n");
    UART_SendString("========================================\r\n");
    UART_SendString("\r\n");

    while (1) {
        /* Fase Aktif: LED berkedip selama 5 detik */
        UART_SendString("[ACTIVE] Running for 5 seconds...\r\n");
        for (int i = 0; i < 10; i++) {
            HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
            vTaskDelay(pdMS_TO_TICKS(500));
        }

        /* Masuk Sleep Mode */
        sleep_count++;
        UART_Printf("[SLEEP] Entering Sleep Mode #%lu (WFI)...\r\n", sleep_count);
        UART_SendString("[SLEEP] Press PA0 button to wake up!\r\n");
        UART_SendString("[SLEEP] CPU stops, peripherals stay on\r\n");
        UART_SendString("[SLEEP] Expected current: ~2mA\r\n");
        vTaskDelay(pdMS_TO_TICKS(50)); /* Flush UART */

        /* LED mati sebelum sleep */
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);

        wakeup_flag = 0;

        /* === MASUK SLEEP MODE === */
        /* 
         * HAL_PWR_EnterSLEEPMode:
         * - Parameter 1: PWR_MAINREGULATOR_ON = regulator utama tetap on
         * - Parameter 2: PWR_SLEEPENTRY_WFI = Wait For Interrupt
         * - CPU clock BERHENTI, semua peripheral tetap aktif
         * - Interrupt apapun akan membangunkan CPU
         */
        HAL_SuspendTick();  /* Suspend SysTick agar tidak self-wake */
        HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);
        /* === CPU BERHENTI DI SINI === */
        /* === LANJUT SETELAH INTERRUPT === */
        HAL_ResumeTick();

        /* Wake-up! */
        UART_SendString("\r\n[WAKEUP] MCU woke up from Sleep!\r\n");

        if (wakeup_flag) {
            UART_SendString("[WAKEUP] Source: Button PA0 (EXTI0)\r\n");
        } else {
            UART_SendString("[WAKEUP] Source: Other interrupt\r\n");
        }

        /* LED berkedip cepat sebagai indikator wake-up */
        UART_SendString("[WAKEUP] LED fast blink = wake indicator\r\n");
        for (int i = 0; i < 6; i++) {
            HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
            vTaskDelay(pdMS_TO_TICKS(100));
        }
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);

        UART_Printf("[INFO] Total sleep cycles: %lu\r\n\r\n", sleep_count);
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

    xTaskCreate(PowerManagementTask, "PwrMgmt", 256, NULL,
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
    UART_Printf("[ERROR] Stack overflow in: %s\r\n", pcTaskName);
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
