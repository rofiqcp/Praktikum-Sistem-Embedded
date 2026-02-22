/**
 * STM32_07_Message_Buffer
 * 
 * Uses xMessageBufferCreate() for discrete message passing.
 * Multiple message types (command, data, status) with different lengths.
 * message_buffer.h is a wrapper over stream_buffer.
 * 
 * Hardware: STM32F103C8 BluePill
 * - UART1: PA9 (TX), PA10 (RX) @ 115200
 * - LED: PC13 (active low)
 */

#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "message_buffer.h"
#include <stdio.h>
#include <string.h>

/* ---- Defines ---- */
#define MSG_BUFFER_SIZE     512
#define LED_PIN             GPIO_PIN_13
#define LED_PORT            GPIOC

/* ---- Message Types ---- */
typedef enum {
    MSG_TYPE_COMMAND = 0x01,
    MSG_TYPE_DATA    = 0x02,
    MSG_TYPE_STATUS  = 0x03
} MsgType_t;

/* Command message (short) */
typedef struct {
    uint8_t  ucType;
    uint8_t  ucCmdId;
    uint16_t usParam;
} __attribute__((packed)) CmdMsg_t;

/* Data message (medium) */
typedef struct {
    uint8_t  ucType;
    uint8_t  ucSensorId;
    uint32_t ulTimestamp;
    int16_t  sValues[4];
} __attribute__((packed)) DataMsg_t;

/* Status message (long) */
typedef struct {
    uint8_t  ucType;
    uint8_t  ucNodeId;
    uint32_t ulUptime;
    uint16_t usBattery;
    int8_t   cRssi;
    uint8_t  ucFlags;
    char     cDescription[24];
} __attribute__((packed)) StatusMsg_t;

/* ---- Globals ---- */
static UART_HandleTypeDef huart1;
static SemaphoreHandle_t xPrintMutex;
static MessageBufferHandle_t xMsgBuffer;

static volatile uint32_t ulCmdSent = 0;
static volatile uint32_t ulDataSent = 0;
static volatile uint32_t ulStatusSent = 0;
static volatile uint32_t ulCmdRecv = 0;
static volatile uint32_t ulDataRecv = 0;
static volatile uint32_t ulStatusRecv = 0;
static volatile uint32_t ulUnknownRecv = 0;
static volatile uint32_t ulTotalBytesSent = 0;
static volatile uint32_t ulTotalBytesRecv = 0;

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

/* ---- Command Sender Task ---- */
static void vCmdSenderTask(void *pvParameters)
{
    (void)pvParameters;
    uint32_t ulSeq = 0;

    uint8_t ucCmds[] = {0x10, 0x20, 0x30, 0x40, 0x50};
    uint16_t usParams[] = {100, 250, 500, 1000, 2000};

    for (;;)
    {
        CmdMsg_t msg;
        msg.ucType = MSG_TYPE_COMMAND;
        msg.ucCmdId = ucCmds[ulSeq % 5];
        msg.usParam = usParams[ulSeq % 5];

        size_t xSent = xMessageBufferSend(xMsgBuffer, &msg, sizeof(msg),
                                           pdMS_TO_TICKS(200));

        if (xSent > 0)
        {
            ulCmdSent++;
            ulTotalBytesSent += xSent;

            if (xSemaphoreTake(xPrintMutex, pdMS_TO_TICKS(50)) == pdTRUE)
            {
                printf("[CMD_TX] #%lu cmd=0x%02X param=%u size=%u\r\n",
                       ulSeq, msg.ucCmdId, msg.usParam, (unsigned)sizeof(msg));
                xSemaphoreGive(xPrintMutex);
            }
        }

        ulSeq++;
        vTaskDelay(pdMS_TO_TICKS(800));
    }
}

/* ---- Data Sender Task ---- */
static void vDataSenderTask(void *pvParameters)
{
    (void)pvParameters;
    uint32_t ulSeq = 0;

    for (;;)
    {
        DataMsg_t msg;
        msg.ucType = MSG_TYPE_DATA;
        msg.ucSensorId = (uint8_t)(ulSeq % 4);
        msg.ulTimestamp = (uint32_t)xTaskGetTickCount();
        msg.sValues[0] = (int16_t)(1000 + (ulSeq * 7) % 500);
        msg.sValues[1] = (int16_t)(2000 - (ulSeq * 3) % 300);
        msg.sValues[2] = (int16_t)(ulSeq * 11 % 1024);
        msg.sValues[3] = (int16_t)(-(int16_t)(ulSeq * 5 % 200));

        size_t xSent = xMessageBufferSend(xMsgBuffer, &msg, sizeof(msg),
                                           pdMS_TO_TICKS(200));

        if (xSent > 0)
        {
            ulDataSent++;
            ulTotalBytesSent += xSent;

            if (xSemaphoreTake(xPrintMutex, pdMS_TO_TICKS(50)) == pdTRUE)
            {
                printf("[DAT_TX] #%lu sensor=%u ts=%lu v=[%d,%d,%d,%d] size=%u\r\n",
                       ulSeq, msg.ucSensorId, msg.ulTimestamp,
                       msg.sValues[0], msg.sValues[1],
                       msg.sValues[2], msg.sValues[3],
                       (unsigned)sizeof(msg));
                xSemaphoreGive(xPrintMutex);
            }
        }

        ulSeq++;
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

/* ---- Status Sender Task ---- */
static void vStatusSenderTask(void *pvParameters)
{
    (void)pvParameters;
    uint32_t ulSeq = 0;

    for (;;)
    {
        StatusMsg_t msg;
        msg.ucType = MSG_TYPE_STATUS;
        msg.ucNodeId = (uint8_t)(ulSeq % 3 + 1);
        msg.ulUptime = (uint32_t)xTaskGetTickCount();
        msg.usBattery = (uint16_t)(3300 - (ulSeq * 10) % 600);
        msg.cRssi = (int8_t)(-40 - (int8_t)(ulSeq % 30));
        msg.ucFlags = (uint8_t)(ulSeq & 0x0F);
        snprintf(msg.cDescription, sizeof(msg.cDescription),
                 "Node%u stat#%lu", msg.ucNodeId, ulSeq);

        size_t xSent = xMessageBufferSend(xMsgBuffer, &msg, sizeof(msg),
                                           pdMS_TO_TICKS(200));

        if (xSent > 0)
        {
            ulStatusSent++;
            ulTotalBytesSent += xSent;

            if (xSemaphoreTake(xPrintMutex, pdMS_TO_TICKS(50)) == pdTRUE)
            {
                printf("[STA_TX] #%lu node=%u bat=%umV rssi=%ddBm [%s] size=%u\r\n",
                       ulSeq, msg.ucNodeId, msg.usBattery, msg.cRssi,
                       msg.cDescription, (unsigned)sizeof(msg));
                xSemaphoreGive(xPrintMutex);
            }
        }

        ulSeq++;
        vTaskDelay(pdMS_TO_TICKS(1500));
    }
}

/* ---- Receiver Task: receives and decodes all message types ---- */
static void vReceiverTask(void *pvParameters)
{
    (void)pvParameters;
    uint8_t ucRxBuf[64];
    uint32_t ulRxSeq = 0;

    for (;;)
    {
        size_t xRecvLen = xMessageBufferReceive(xMsgBuffer, ucRxBuf,
                                                 sizeof(ucRxBuf),
                                                 pdMS_TO_TICKS(2000));

        if (xRecvLen > 0)
        {
            ulTotalBytesRecv += xRecvLen;
            ulRxSeq++;

            uint8_t ucType = ucRxBuf[0];

            if (xSemaphoreTake(xPrintMutex, pdMS_TO_TICKS(50)) == pdTRUE)
            {
                switch (ucType)
                {
                    case MSG_TYPE_COMMAND:
                    {
                        CmdMsg_t *pCmd = (CmdMsg_t *)ucRxBuf;
                        ulCmdRecv++;
                        printf("[MSG_RX] #%lu type=CMD cmd=0x%02X param=%u len=%u\r\n",
                               ulRxSeq, pCmd->ucCmdId, pCmd->usParam,
                               (unsigned)xRecvLen);
                        break;
                    }
                    case MSG_TYPE_DATA:
                    {
                        DataMsg_t *pData = (DataMsg_t *)ucRxBuf;
                        ulDataRecv++;
                        printf("[MSG_RX] #%lu type=DATA sensor=%u ts=%lu v=[%d,%d,%d,%d] len=%u\r\n",
                               ulRxSeq, pData->ucSensorId, pData->ulTimestamp,
                               pData->sValues[0], pData->sValues[1],
                               pData->sValues[2], pData->sValues[3],
                               (unsigned)xRecvLen);
                        break;
                    }
                    case MSG_TYPE_STATUS:
                    {
                        StatusMsg_t *pStat = (StatusMsg_t *)ucRxBuf;
                        ulStatusRecv++;
                        printf("[MSG_RX] #%lu type=STATUS node=%u bat=%umV rssi=%d [%s] len=%u\r\n",
                               ulRxSeq, pStat->ucNodeId, pStat->usBattery,
                               pStat->cRssi, pStat->cDescription,
                               (unsigned)xRecvLen);
                        break;
                    }
                    default:
                        ulUnknownRecv++;
                        printf("[MSG_RX] #%lu type=UNKNOWN(0x%02X) len=%u\r\n",
                               ulRxSeq, ucType, (unsigned)xRecvLen);
                        break;
                }
                xSemaphoreGive(xPrintMutex);
            }
        }
        else
        {
            if (xSemaphoreTake(xPrintMutex, pdMS_TO_TICKS(50)) == pdTRUE)
            {
                printf("[MSG_RX] Timeout - no messages\r\n");
                xSemaphoreGive(xPrintMutex);
            }
        }
    }
}

/* ---- Stats Task ---- */
static void vStatsTask(void *pvParameters)
{
    (void)pvParameters;
    TickType_t xLastWake = xTaskGetTickCount();

    for (;;)
    {
        vTaskDelayUntil(&xLastWake, pdMS_TO_TICKS(5000));

        if (xSemaphoreTake(xPrintMutex, pdMS_TO_TICKS(100)) == pdTRUE)
        {
            printf("\r\n===== MESSAGE BUFFER STATS =====\r\n");
            printf("[STATS] Buffer size : %d bytes\r\n", MSG_BUFFER_SIZE);
            printf("[STATS] Msg sizes   : CMD=%u DATA=%u STATUS=%u\r\n",
                   (unsigned)sizeof(CmdMsg_t),
                   (unsigned)sizeof(DataMsg_t),
                   (unsigned)sizeof(StatusMsg_t));
            printf("[STATS] --- Sent ---\r\n");
            printf("[STATS] Commands    : %lu\r\n", ulCmdSent);
            printf("[STATS] Data        : %lu\r\n", ulDataSent);
            printf("[STATS] Status      : %lu\r\n", ulStatusSent);
            printf("[STATS] Total bytes : %lu\r\n", ulTotalBytesSent);
            printf("[STATS] --- Recv ---\r\n");
            printf("[STATS] Commands    : %lu\r\n", ulCmdRecv);
            printf("[STATS] Data        : %lu\r\n", ulDataRecv);
            printf("[STATS] Status      : %lu\r\n", ulStatusRecv);
            printf("[STATS] Unknown     : %lu\r\n", ulUnknownRecv);
            printf("[STATS] Total bytes : %lu\r\n", ulTotalBytesRecv);
            printf("[STATS] Heap remain : %u bytes\r\n",
                   (unsigned)xPortGetFreeHeapSize());
            printf("================================\r\n\r\n");

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
    printf("  STM32_07 Message Buffer Demo\r\n");
    printf("  Multi-type discrete messages\r\n");
    printf("  CMD=%u DATA=%u STATUS=%u bytes\r\n",
           (unsigned)sizeof(CmdMsg_t),
           (unsigned)sizeof(DataMsg_t),
           (unsigned)sizeof(StatusMsg_t));
    printf("========================================\r\n\r\n");

    xPrintMutex = xSemaphoreCreateMutex();
    configASSERT(xPrintMutex != NULL);

    xMsgBuffer = xMessageBufferCreate(MSG_BUFFER_SIZE);
    configASSERT(xMsgBuffer != NULL);

    printf("[MAIN] Message buffer created: %d bytes\r\n", MSG_BUFFER_SIZE);

    xTaskCreate(vCmdSenderTask,    "CmdTx",   256, NULL, 2, NULL);
    xTaskCreate(vDataSenderTask,   "DatTx",   256, NULL, 2, NULL);
    xTaskCreate(vStatusSenderTask, "StaTx",   256, NULL, 2, NULL);
    xTaskCreate(vReceiverTask,     "MsgRx",   384, NULL, 3, NULL);
    xTaskCreate(vStatsTask,        "Stats",   256, NULL, 1, NULL);

    printf("[MAIN] Starting scheduler...\r\n\r\n");
    vTaskStartScheduler();

    for (;;) {}
}
