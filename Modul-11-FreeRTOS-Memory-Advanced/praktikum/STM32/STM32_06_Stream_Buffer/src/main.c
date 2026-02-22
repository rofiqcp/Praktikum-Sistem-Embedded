/**
 * STM32_06_Stream_Buffer
 * 
 * Uses xStreamBufferCreate() for variable-length byte stream communication.
 * Sender task writes variable-length data, receiver reads.
 * Demonstrates trigger level functionality.
 * 
 * Hardware: STM32F103C8 BluePill
 * - UART1: PA9 (TX), PA10 (RX) @ 115200
 * - LED: PC13 (active low)
 */

#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "stream_buffer.h"
#include <stdio.h>
#include <string.h>

/* ---- Defines ---- */
#define STREAM_BUFFER_SIZE      256
#define TRIGGER_LEVEL           16
#define LED_PIN                 GPIO_PIN_13
#define LED_PORT                GPIOC

/* ---- Globals ---- */
static UART_HandleTypeDef huart1;
static SemaphoreHandle_t xPrintMutex;

static StreamBufferHandle_t xStreamBuffer;
static volatile uint32_t ulTotalBytesSent = 0;
static volatile uint32_t ulTotalBytesRecv = 0;
static volatile uint32_t ulSendCount = 0;
static volatile uint32_t ulRecvCount = 0;
static volatile uint32_t ulSendFailCount = 0;

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

    /* PC13 LED */
    GPIO_InitStruct.Pin = LED_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_PORT, &GPIO_InitStruct);
    HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET);
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

/* ---- Sender Task: writes variable-length data to stream ---- */
static void vSenderTask(void *pvParameters)
{
    (void)pvParameters;
    uint32_t ulSeqNum = 0;

    /* Different message patterns to send */
    const char *pcMessages[] = {
        "Hello",
        "Stream buffer test data",
        "Short",
        "This is a longer message to test variable-length streaming",
        "ABCDEFGHIJ",
        "12345",
        "Variable length data transfer via FreeRTOS stream buffer"
    };
    const uint32_t ulNumMessages = sizeof(pcMessages) / sizeof(pcMessages[0]);

    for (;;)
    {
        const char *pcMsg = pcMessages[ulSeqNum % ulNumMessages];
        size_t xLen = strlen(pcMsg);

        size_t xBytesSent = xStreamBufferSend(xStreamBuffer,
                                               (const void *)pcMsg,
                                               xLen,
                                               pdMS_TO_TICKS(200));

        ulSendCount++;

        if (xBytesSent > 0)
        {
            ulTotalBytesSent += xBytesSent;

            if (xSemaphoreTake(xPrintMutex, pdMS_TO_TICKS(50)) == pdTRUE)
            {
                printf("[SEND] #%lu len=%u sent=%u buf_space=%u\r\n",
                       ulSeqNum, (unsigned)xLen, (unsigned)xBytesSent,
                       (unsigned)xStreamBufferSpacesAvailable(xStreamBuffer));
                xSemaphoreGive(xPrintMutex);
            }
        }
        else
        {
            ulSendFailCount++;
            if (xSemaphoreTake(xPrintMutex, pdMS_TO_TICKS(50)) == pdTRUE)
            {
                printf("[SEND] #%lu FAILED - buffer full\r\n", ulSeqNum);
                xSemaphoreGive(xPrintMutex);
            }
        }

        ulSeqNum++;
        vTaskDelay(pdMS_TO_TICKS(300 + (ulSeqNum % 5) * 50));
    }
}

/* ---- Receiver Task: reads from stream with trigger level ---- */
static void vReceiverTask(void *pvParameters)
{
    (void)pvParameters;
    uint8_t ucRxBuffer[128];
    uint32_t ulRxSeq = 0;

    for (;;)
    {
        /* Read available bytes - will block until trigger level reached */
        size_t xBytesRecv = xStreamBufferReceive(xStreamBuffer,
                                                  ucRxBuffer,
                                                  sizeof(ucRxBuffer) - 1,
                                                  pdMS_TO_TICKS(1000));

        if (xBytesRecv > 0)
        {
            ucRxBuffer[xBytesRecv] = '\0';
            ulTotalBytesRecv += xBytesRecv;
            ulRecvCount++;

            if (xSemaphoreTake(xPrintMutex, pdMS_TO_TICKS(50)) == pdTRUE)
            {
                printf("[RECV] #%lu bytes=%u data=[%s]\r\n",
                       ulRxSeq, (unsigned)xBytesRecv, ucRxBuffer);
                printf("[RECV] buf_used=%u trigger=%d\r\n",
                       (unsigned)(STREAM_BUFFER_SIZE - 1 -
                       xStreamBufferSpacesAvailable(xStreamBuffer)),
                       TRIGGER_LEVEL);
                xSemaphoreGive(xPrintMutex);
            }
            ulRxSeq++;
        }
        else
        {
            if (xSemaphoreTake(xPrintMutex, pdMS_TO_TICKS(50)) == pdTRUE)
            {
                printf("[RECV] Timeout - no data (trigger=%d)\r\n", TRIGGER_LEVEL);
                xSemaphoreGive(xPrintMutex);
            }
        }
    }
}

/* ---- Burst Sender: sends rapid bursts to test trigger ---- */
static void vBurstSenderTask(void *pvParameters)
{
    (void)pvParameters;
    uint32_t ulBurstNum = 0;

    for (;;)
    {
        vTaskDelay(pdMS_TO_TICKS(5000));

        ulBurstNum++;
        if (xSemaphoreTake(xPrintMutex, pdMS_TO_TICKS(50)) == pdTRUE)
        {
            printf("\r\n[BURST] === Burst #%lu: sending 5 rapid chunks ===\r\n", ulBurstNum);
            xSemaphoreGive(xPrintMutex);
        }

        /* Send 5 small chunks rapidly */
        for (int i = 0; i < 5; i++)
        {
            char cBuf[32];
            int iLen = snprintf(cBuf, sizeof(cBuf), "B%lu_%d,", ulBurstNum, i);
            size_t xSent = xStreamBufferSend(xStreamBuffer, cBuf, (size_t)iLen,
                                              pdMS_TO_TICKS(50));
            if (xSent > 0)
            {
                ulTotalBytesSent += xSent;
            }
            vTaskDelay(pdMS_TO_TICKS(10));
        }

        if (xSemaphoreTake(xPrintMutex, pdMS_TO_TICKS(50)) == pdTRUE)
        {
            printf("[BURST] Burst #%lu complete\r\n\r\n", ulBurstNum);
            xSemaphoreGive(xPrintMutex);
        }
    }
}

/* ---- Stats Task ---- */
static void vStatsTask(void *pvParameters)
{
    (void)pvParameters;
    TickType_t xLastWake = xTaskGetTickCount();
    TickType_t xStartTick = xTaskGetTickCount();

    for (;;)
    {
        vTaskDelayUntil(&xLastWake, pdMS_TO_TICKS(4000));

        if (xSemaphoreTake(xPrintMutex, pdMS_TO_TICKS(100)) == pdTRUE)
        {
            uint32_t ulElapsedMs = (uint32_t)(xTaskGetTickCount() - xStartTick);
            uint32_t ulElapsedSec = ulElapsedMs / 1000;
            uint32_t ulThroughput = (ulElapsedSec > 0) ?
                                    (ulTotalBytesRecv / ulElapsedSec) : 0;

            printf("\r\n===== STREAM BUFFER STATS =====\r\n");
            printf("[STATS] Buffer size  : %d bytes\r\n", STREAM_BUFFER_SIZE);
            printf("[STATS] Trigger level: %d bytes\r\n", TRIGGER_LEVEL);
            printf("[STATS] Bytes sent   : %lu\r\n", ulTotalBytesSent);
            printf("[STATS] Bytes recv   : %lu\r\n", ulTotalBytesRecv);
            printf("[STATS] Send ops     : %lu\r\n", ulSendCount);
            printf("[STATS] Recv ops     : %lu\r\n", ulRecvCount);
            printf("[STATS] Send fails   : %lu\r\n", ulSendFailCount);
            printf("[STATS] Throughput   : %lu bytes/sec\r\n", ulThroughput);
            printf("[STATS] Buf space    : %u bytes\r\n",
                   (unsigned)xStreamBufferSpacesAvailable(xStreamBuffer));
            printf("[STATS] Heap remain  : %u bytes\r\n",
                   (unsigned)xPortGetFreeHeapSize());
            printf("===============================\r\n\r\n");

            HAL_GPIO_TogglePin(LED_PORT, LED_PIN);
            xSemaphoreGive(xPrintMutex);
        }
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
    printf("  STM32_06 Stream Buffer Demo\r\n");
    printf("  Buffer=%d bytes, Trigger=%d\r\n",
           STREAM_BUFFER_SIZE, TRIGGER_LEVEL);
    printf("========================================\r\n\r\n");

    xPrintMutex = xSemaphoreCreateMutex();
    configASSERT(xPrintMutex != NULL);

    /* Create stream buffer with trigger level */
    xStreamBuffer = xStreamBufferCreate(STREAM_BUFFER_SIZE, TRIGGER_LEVEL);
    configASSERT(xStreamBuffer != NULL);

    printf("[MAIN] Stream buffer created: size=%d trigger=%d\r\n",
           STREAM_BUFFER_SIZE, TRIGGER_LEVEL);

    xTaskCreate(vSenderTask,      "Sender",  256, NULL, 2, NULL);
    xTaskCreate(vReceiverTask,    "Recvr",   256, NULL, 3, NULL);
    xTaskCreate(vBurstSenderTask, "Burst",   256, NULL, 2, NULL);
    xTaskCreate(vStatsTask,       "Stats",   256, NULL, 1, NULL);

    printf("[MAIN] Starting scheduler...\r\n\r\n");
    vTaskStartScheduler();

    for (;;) {}
}
