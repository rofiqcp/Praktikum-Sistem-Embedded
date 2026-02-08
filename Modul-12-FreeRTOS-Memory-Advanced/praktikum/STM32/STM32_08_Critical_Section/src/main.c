/**
 * STM32_08_Critical_Section
 * 
 * Demonstrates taskENTER_CRITICAL() / taskEXIT_CRITICAL().
 * Two tasks modifying a shared multi-word struct.
 * Shows: without critical section (corrupted) then with critical section (correct).
 * Two LEDs: PA0 and PA1 show task execution.
 * 
 * Hardware: STM32F103C8 BluePill
 * - UART1: PA9 (TX), PA10 (RX) @ 115200
 * - LED: PC13 (active low, heartbeat)
 * - LED1: PA0 (task A indicator)
 * - LED2: PA1 (task B indicator)
 */

#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include <stdio.h>
#include <string.h>

/* ---- Defines ---- */
#define LED_PIN             GPIO_PIN_13
#define LED_PORT            GPIOC
#define LED1_PIN            GPIO_PIN_0
#define LED2_PIN            GPIO_PIN_1
#define LED_EXT_PORT        GPIOA

#define TEST_DURATION_MS    10000   /* Duration per phase */

/* ---- Shared Data Structure (multi-word, vulnerable to tearing) ---- */
typedef struct {
    uint32_t ulField1;
    uint32_t ulField2;
    uint32_t ulField3;
    uint32_t ulField4;
    uint32_t ulChecksum;
} SharedData_t;

/* ---- Globals ---- */
static UART_HandleTypeDef huart1;
static SemaphoreHandle_t xPrintMutex;

static volatile SharedData_t xSharedData;
static volatile uint32_t ulCorruptionCount = 0;
static volatile uint32_t ulVerifyCount = 0;
static volatile uint32_t ulWriteCountA = 0;
static volatile uint32_t ulWriteCountB = 0;
static volatile BaseType_t xUseCriticalSection = pdFALSE;
static volatile BaseType_t xTestRunning = pdTRUE;

/* ---- _write for printf ---- */
int _write(int file, char *ptr, int len)
{
    HAL_UART_Transmit(&huart1, (uint8_t *)ptr, len, HAL_MAX_DELAY);
    return len;
}

/* ---- Clock Config ---- */
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

/* ---- GPIO Init ---- */
static void GPIO_Init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* PC13 LED (heartbeat) */
    GPIO_InitStruct.Pin = LED_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_PORT, &GPIO_InitStruct);
    HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET);

    /* PA0 LED1 (Task A) */
    GPIO_InitStruct.Pin = LED1_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_EXT_PORT, &GPIO_InitStruct);
    HAL_GPIO_WritePin(LED_EXT_PORT, LED1_PIN, GPIO_PIN_RESET);

    /* PA1 LED2 (Task B) */
    GPIO_InitStruct.Pin = LED2_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_EXT_PORT, &GPIO_InitStruct);
    HAL_GPIO_WritePin(LED_EXT_PORT, LED2_PIN, GPIO_PIN_RESET);
}

/* ---- UART1 Init ---- */
static void UART1_Init(void)
{
    __HAL_RCC_USART1_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* PA9 TX */
    GPIO_InitStruct.Pin = GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* PA10 RX */
    GPIO_InitStruct.Pin = GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
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

/* ---- Write shared data (writer A pattern: all fields = value) ---- */
static void WriteSharedData_A(uint32_t ulValue)
{
    if (xUseCriticalSection)
    {
        taskENTER_CRITICAL();
    }

    /* LED1 ON during write */
    HAL_GPIO_WritePin(LED_EXT_PORT, LED1_PIN, GPIO_PIN_SET);

    /* Write all fields with consistent pattern */
    xSharedData.ulField1 = ulValue;
    xSharedData.ulField2 = ulValue;
    /* Small busy loop to increase chance of preemption without critical section */
    for (volatile int i = 0; i < 10; i++) {}
    xSharedData.ulField3 = ulValue;
    xSharedData.ulField4 = ulValue;
    xSharedData.ulChecksum = ulValue * 4;

    HAL_GPIO_WritePin(LED_EXT_PORT, LED1_PIN, GPIO_PIN_RESET);

    if (xUseCriticalSection)
    {
        taskEXIT_CRITICAL();
    }

    ulWriteCountA++;
}

/* ---- Write shared data (writer B pattern: all fields = value | 0x80000000) ---- */
static void WriteSharedData_B(uint32_t ulValue)
{
    uint32_t ulTagged = ulValue | 0x80000000UL;

    if (xUseCriticalSection)
    {
        taskENTER_CRITICAL();
    }

    /* LED2 ON during write */
    HAL_GPIO_WritePin(LED_EXT_PORT, LED2_PIN, GPIO_PIN_SET);

    xSharedData.ulField1 = ulTagged;
    xSharedData.ulField2 = ulTagged;
    for (volatile int i = 0; i < 10; i++) {}
    xSharedData.ulField3 = ulTagged;
    xSharedData.ulField4 = ulTagged;
    xSharedData.ulChecksum = ulTagged * 4;

    HAL_GPIO_WritePin(LED_EXT_PORT, LED2_PIN, GPIO_PIN_RESET);

    if (xUseCriticalSection)
    {
        taskEXIT_CRITICAL();
    }

    ulWriteCountB++;
}

/* ---- Verify shared data consistency ---- */
static BaseType_t VerifySharedData(void)
{
    uint32_t f1, f2, f3, f4, cs;

    if (xUseCriticalSection)
    {
        taskENTER_CRITICAL();
    }

    f1 = xSharedData.ulField1;
    f2 = xSharedData.ulField2;
    f3 = xSharedData.ulField3;
    f4 = xSharedData.ulField4;
    cs = xSharedData.ulChecksum;

    if (xUseCriticalSection)
    {
        taskEXIT_CRITICAL();
    }

    ulVerifyCount++;

    /* All fields must be the same, and checksum must match */
    if ((f1 == f2) && (f2 == f3) && (f3 == f4) && (cs == f1 * 4))
    {
        return pdTRUE;
    }
    else
    {
        ulCorruptionCount++;
        if (xSemaphoreTake(xPrintMutex, pdMS_TO_TICKS(50)) == pdTRUE)
        {
            printf("[CORRUPT] f1=0x%08lX f2=0x%08lX f3=0x%08lX f4=0x%08lX cs=0x%08lX expected_cs=0x%08lX\r\n",
                   f1, f2, f3, f4, cs, f1 * 4);
            xSemaphoreGive(xPrintMutex);
        }
        return pdFALSE;
    }
}

/* ---- Writer Task A ---- */
static void vWriterTaskA(void *pvParameters)
{
    (void)pvParameters;
    uint32_t ulValue = 0;

    for (;;)
    {
        if (!xTestRunning)
        {
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        ulValue++;
        WriteSharedData_A(ulValue);

        /* Minimal delay to maximize contention */
        taskYIELD();
    }
}

/* ---- Writer Task B ---- */
static void vWriterTaskB(void *pvParameters)
{
    (void)pvParameters;
    uint32_t ulValue = 0;

    for (;;)
    {
        if (!xTestRunning)
        {
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        ulValue++;
        WriteSharedData_B(ulValue);

        taskYIELD();
    }
}

/* ---- Verifier Task ---- */
static void vVerifierTask(void *pvParameters)
{
    (void)pvParameters;

    for (;;)
    {
        if (!xTestRunning)
        {
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        VerifySharedData();

        /* Verify at high rate */
        taskYIELD();
    }
}

/* ---- Controller Task: runs both test phases ---- */
static void vControllerTask(void *pvParameters)
{
    (void)pvParameters;

    for (;;)
    {
        /* ======== PHASE 1: WITHOUT Critical Section ======== */
        if (xSemaphoreTake(xPrintMutex, pdMS_TO_TICKS(100)) == pdTRUE)
        {
            printf("\r\n");
            printf("############################################\r\n");
            printf("  PHASE 1: WITHOUT Critical Section\r\n");
            printf("  Duration: %d seconds\r\n", TEST_DURATION_MS / 1000);
            printf("  Expecting CORRUPTIONS!\r\n");
            printf("############################################\r\n\r\n");
            xSemaphoreGive(xPrintMutex);
        }

        /* Reset counters */
        ulCorruptionCount = 0;
        ulVerifyCount = 0;
        ulWriteCountA = 0;
        ulWriteCountB = 0;
        xUseCriticalSection = pdFALSE;
        xTestRunning = pdTRUE;

        vTaskDelay(pdMS_TO_TICKS(TEST_DURATION_MS));

        xTestRunning = pdFALSE;
        vTaskDelay(pdMS_TO_TICKS(100)); /* Let tasks pause */

        /* Report Phase 1 */
        if (xSemaphoreTake(xPrintMutex, pdMS_TO_TICKS(100)) == pdTRUE)
        {
            printf("\r\n===== PHASE 1 RESULTS (NO CRITICAL SECTION) =====\r\n");
            printf("[RESULT] Write A count  : %lu\r\n", ulWriteCountA);
            printf("[RESULT] Write B count  : %lu\r\n", ulWriteCountB);
            printf("[RESULT] Verify count   : %lu\r\n", ulVerifyCount);
            printf("[RESULT] Corruptions    : %lu\r\n", ulCorruptionCount);
            if (ulVerifyCount > 0)
            {
                printf("[RESULT] Corruption rate: %lu.%02lu%%\r\n",
                       (ulCorruptionCount * 100) / ulVerifyCount,
                       ((ulCorruptionCount * 10000) / ulVerifyCount) % 100);
            }
            printf("[RESULT] Status         : %s\r\n",
                   ulCorruptionCount > 0 ? "DATA CORRUPTED!" : "No corruption detected");
            printf("==================================================\r\n\r\n");
            xSemaphoreGive(xPrintMutex);
        }

        vTaskDelay(pdMS_TO_TICKS(2000));

        /* ======== PHASE 2: WITH Critical Section ======== */
        if (xSemaphoreTake(xPrintMutex, pdMS_TO_TICKS(100)) == pdTRUE)
        {
            printf("\r\n");
            printf("############################################\r\n");
            printf("  PHASE 2: WITH Critical Section\r\n");
            printf("  Duration: %d seconds\r\n", TEST_DURATION_MS / 1000);
            printf("  Expecting NO corruptions\r\n");
            printf("############################################\r\n\r\n");
            xSemaphoreGive(xPrintMutex);
        }

        /* Reset counters */
        ulCorruptionCount = 0;
        ulVerifyCount = 0;
        ulWriteCountA = 0;
        ulWriteCountB = 0;
        xUseCriticalSection = pdTRUE;
        xTestRunning = pdTRUE;

        vTaskDelay(pdMS_TO_TICKS(TEST_DURATION_MS));

        xTestRunning = pdFALSE;
        vTaskDelay(pdMS_TO_TICKS(100));

        /* Report Phase 2 */
        if (xSemaphoreTake(xPrintMutex, pdMS_TO_TICKS(100)) == pdTRUE)
        {
            printf("\r\n===== PHASE 2 RESULTS (WITH CRITICAL SECTION) =====\r\n");
            printf("[RESULT] Write A count  : %lu\r\n", ulWriteCountA);
            printf("[RESULT] Write B count  : %lu\r\n", ulWriteCountB);
            printf("[RESULT] Verify count   : %lu\r\n", ulVerifyCount);
            printf("[RESULT] Corruptions    : %lu\r\n", ulCorruptionCount);
            if (ulVerifyCount > 0)
            {
                printf("[RESULT] Corruption rate: %lu.%02lu%%\r\n",
                       (ulCorruptionCount * 100) / ulVerifyCount,
                       ((ulCorruptionCount * 10000) / ulVerifyCount) % 100);
            }
            printf("[RESULT] Status         : %s\r\n",
                   ulCorruptionCount > 0 ? "DATA CORRUPTED!" : "DATA INTACT - Protected!");
            printf("====================================================\r\n\r\n");

            printf("[MAIN] Heap remaining: %u bytes\r\n\r\n",
                   (unsigned)xPortGetFreeHeapSize());

            HAL_GPIO_TogglePin(LED_PORT, LED_PIN);
            xSemaphoreGive(xPrintMutex);
        }

        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}

/* ---- FreeRTOS Hooks ---- */
void vApplicationMallocFailedHook(void)
{
    printf("[ERROR] Malloc failed!\r\n");
    for (;;)
    {
        HAL_GPIO_TogglePin(LED_PORT, LED_PIN);
        HAL_Delay(100);
    }
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    printf("[ERROR] Stack overflow in task: %s\r\n", pcTaskName);
    for (;;)
    {
        HAL_GPIO_TogglePin(LED_PORT, LED_PIN);
        HAL_Delay(100);
    }
}

/* ---- Main ---- */
int main(void)
{
    HAL_Init();
    SystemClock_Config();
    GPIO_Init();
    UART1_Init();

    printf("\r\n\r\n========================================\r\n");
    printf("  STM32_08 Critical Section Demo\r\n");
    printf("  Shared struct protection test\r\n");
    printf("  LED PA0=TaskA, PA1=TaskB\r\n");
    printf("========================================\r\n\r\n");

    xPrintMutex = xSemaphoreCreateMutex();
    configASSERT(xPrintMutex != NULL);

    /* Initialize shared data */
    xSharedData.ulField1 = 0;
    xSharedData.ulField2 = 0;
    xSharedData.ulField3 = 0;
    xSharedData.ulField4 = 0;
    xSharedData.ulChecksum = 0;

    /* Writers at same priority to force time-slicing contention */
    xTaskCreate(vWriterTaskA,    "WriterA", 256, NULL, 2, NULL);
    xTaskCreate(vWriterTaskB,    "WriterB", 256, NULL, 2, NULL);
    xTaskCreate(vVerifierTask,   "Verify",  256, NULL, 2, NULL);
    xTaskCreate(vControllerTask, "Ctrl",    384, NULL, 3, NULL);

    printf("[MAIN] Starting scheduler...\r\n\r\n");
    vTaskStartScheduler();

    for (;;) {}
}
