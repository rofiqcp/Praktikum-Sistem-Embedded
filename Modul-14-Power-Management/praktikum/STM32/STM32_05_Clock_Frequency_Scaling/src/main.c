/**
 * ================================================================================
 * PROGRAM 5: CLOCK FREQUENCY SCALING - STM32F103C8T6
 * ================================================================================
 * Deskripsi:
 *   Demonstrasi penghematan daya dengan mengubah frekuensi clock MCU.
 *   Konsumsi daya berbanding lurus dengan frekuensi:
 *   - 72MHz (PLL dari HSE) : ~30mA (performa maksimum)
 *   - 8MHz  (HSE langsung) : ~10mA (hemat daya sedang)
 *   - 8MHz  (HSI internal) : ~8mA  (tanpa crystal external)
 *
 *   Program melakukan benchmark sederhana pada setiap frekuensi
 *   dan menampilkan hasilnya via UART.
 *
 * Hardware:
 *   - STM32F103C8T6 (Blue Pill)
 *   - LED PC13 / UART1 (PA9/PA10)
 *
 * ================================================================================
 */

#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include <string.h>
#include <stdio.h>

UART_HandleTypeDef huart1;

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
 * KONFIGURASI CLOCK
 * ================================================================================ */

static void Clock_72MHz_PLL(void)
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

static void Clock_8MHz_HSE(void)
{
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    /* Switch to HSE directly (8MHz) */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                  RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSE;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0);

    /* Matikan PLL untuk hemat daya */
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_NONE;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_OFF;
    HAL_RCC_OscConfig(&RCC_OscInitStruct);
}

static void Clock_8MHz_HSI(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    /* Enable HSI */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
    HAL_RCC_OscConfig(&RCC_OscInitStruct);

    /* Switch SYSCLK ke HSI */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                  RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0);
}

static void UART_Reinit(uint32_t sysclk)
{
    /* Reinit UART setelah clock berubah */
    (void)sysclk;
    huart1.Init.BaudRate = 115200;
    HAL_UART_Init(&huart1);
}

/* ================================================================================
 * KONFIGURASI GPIO & UART
 * ================================================================================ */

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

/* ================================================================================
 * BENCHMARK
 * ================================================================================ */

static uint32_t run_benchmark(void)
{
    volatile uint32_t sum = 0;
    uint32_t start = HAL_GetTick();

    for (volatile uint32_t i = 0; i < 100000; i++) {
        sum += i;
        sum ^= (i << 3);
        sum += (sum >> 2);
    }

    uint32_t elapsed = HAL_GetTick() - start;
    return elapsed;
}

/* ================================================================================
 * FREERTOS TASK
 * ================================================================================ */

static void ClockScalingTask(void *pvParameters)
{
    UART_SendString("\r\n");
    UART_SendString("==========================================\r\n");
    UART_SendString("  STM32 Clock Frequency Scaling Demo\r\n");
    UART_SendString("  Modul 14: Power Management\r\n");
    UART_SendString("==========================================\r\n");
    UART_SendString("\r\n");

    while (1) {
        /* === TEST 1: 72MHz PLL === */
        Clock_72MHz_PLL();
        UART_Reinit(72000000);

        UART_Printf("\r\n[TEST 1] SYSCLK = %lu Hz (HSE+PLL)\r\n",
                     HAL_RCC_GetSysClockFreq());
        UART_SendString("  Expected current: ~30mA\r\n");

        /* LED cepat pada 72MHz */
        for (int i = 0; i < 6; i++) {
            HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
            vTaskDelay(pdMS_TO_TICKS(100));
        }

        uint32_t t1 = run_benchmark();
        UART_Printf("  Benchmark: %lu ms\r\n", t1);

        vTaskDelay(pdMS_TO_TICKS(2000));

        /* === TEST 2: 8MHz HSE === */
        Clock_8MHz_HSE();
        /* Update SysTick untuk 8MHz */
        SystemCoreClock = 8000000;
        HAL_SYSTICK_Config(SystemCoreClock / 1000);
        UART_Reinit(8000000);

        UART_Printf("\r\n[TEST 2] SYSCLK = %lu Hz (HSE direct)\r\n",
                     HAL_RCC_GetSysClockFreq());
        UART_SendString("  Expected current: ~10mA\r\n");
        UART_SendString("  PLL is OFF for power saving\r\n");

        /* LED lambat pada 8MHz */
        for (int i = 0; i < 4; i++) {
            HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
            vTaskDelay(pdMS_TO_TICKS(250));
        }

        uint32_t t2 = run_benchmark();
        UART_Printf("  Benchmark: %lu ms\r\n", t2);

        vTaskDelay(pdMS_TO_TICKS(2000));

        /* === TEST 3: 8MHz HSI === */
        Clock_8MHz_HSI();
        SystemCoreClock = 8000000;
        HAL_SYSTICK_Config(SystemCoreClock / 1000);
        UART_Reinit(8000000);

        UART_Printf("\r\n[TEST 3] SYSCLK = %lu Hz (HSI internal)\r\n",
                     HAL_RCC_GetSysClockFreq());
        UART_SendString("  Expected current: ~8mA\r\n");
        UART_SendString("  No external crystal needed\r\n");

        for (int i = 0; i < 4; i++) {
            HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
            vTaskDelay(pdMS_TO_TICKS(250));
        }

        uint32_t t3 = run_benchmark();
        UART_Printf("  Benchmark: %lu ms\r\n", t3);

        /* Kembali ke 72MHz */
        Clock_72MHz_PLL();
        SystemCoreClock = 72000000;
        HAL_SYSTICK_Config(SystemCoreClock / 1000);
        UART_Reinit(72000000);

        /* Summary */
        UART_SendString("\r\n");
        UART_SendString("=== SUMMARY ===\r\n");
        UART_Printf("  72MHz PLL : %3lu ms  ~30mA\r\n", t1);
        UART_Printf("   8MHz HSE : %3lu ms  ~10mA\r\n", t2);
        UART_Printf("   8MHz HSI : %3lu ms   ~8mA\r\n", t3);
        UART_SendString("  Sleep Mode:          ~2mA\r\n");
        UART_SendString("  Stop Mode :         ~20uA\r\n");
        UART_SendString("  Standby   :          ~2uA\r\n");
        UART_SendString("\r\nNext cycle in 5 seconds...\r\n");

        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

/* ================================================================================
 * MAIN
 * ================================================================================ */

int main(void)
{
    HAL_Init();
    Clock_72MHz_PLL();
    MX_GPIO_Init();
    MX_USART1_UART_Init();

    xTaskCreate(ClockScalingTask, "ClkScale", 512, NULL,
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
