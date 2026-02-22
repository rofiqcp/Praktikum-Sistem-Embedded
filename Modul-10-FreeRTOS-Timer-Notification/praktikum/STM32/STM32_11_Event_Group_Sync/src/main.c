/**
 * STM32_11_Event_Group_Sync
 * 3 worker tasks use xEventGroupSync() for barrier synchronization.
 * Each prints when reaching barrier, all continue simultaneously.
 * Demonstrates rendezvous pattern.
 */

#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "event_groups.h"
#include <stdio.h>
#include <string.h>

UART_HandleTypeDef huart1;

static EventGroupHandle_t xSyncEventGroup;

/* Sync bits - one per worker */
#define WORKER_0_BIT   (1 << 0)
#define WORKER_1_BIT   (1 << 1)
#define WORKER_2_BIT   (1 << 2)
#define ALL_SYNC_BITS  (WORKER_0_BIT | WORKER_1_BIT | WORKER_2_BIT)

static volatile uint32_t ulSyncCount = 0;

void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState       = RCC_HSE_ON;
    RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    RCC_OscInitStruct.PLL.PLLState   = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource  = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL     = RCC_PLL_MUL9;
    HAL_RCC_OscConfig(&RCC_OscInitStruct);

    RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                       RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2);
}

static void UART1_Init(void)
{
    __HAL_RCC_USART1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin   = GPIO_PIN_9;
    GPIO_InitStruct.Mode  = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin  = GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    huart1.Instance          = USART1;
    huart1.Init.BaudRate     = 115200;
    huart1.Init.WordLength   = UART_WORDLENGTH_8B;
    huart1.Init.StopBits     = UART_STOPBITS_1;
    huart1.Init.Parity       = UART_PARITY_NONE;
    huart1.Init.Mode         = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(&huart1);
}

int _write(int file, char *ptr, int len)
{
    HAL_UART_Transmit(&huart1, (uint8_t *)ptr, len, HAL_MAX_DELAY);
    return len;
}

static void GPIO_Init(void)
{
    __HAL_RCC_GPIOC_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin   = GPIO_PIN_13;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
}

typedef struct {
    uint32_t    ulWorkerID;
    EventBits_t uxMyBit;
    uint32_t    ulWorkTimeMs;
} WorkerConfig_t;

static void vWorkerTask(void *pvParameters)
{
    WorkerConfig_t *cfg = (WorkerConfig_t *)pvParameters;
    uint32_t ulRound = 0;

    for (;;)
    {
        ulRound++;

        /* Phase 1: Do some work (variable time) */
        printf("[WORKER-%lu] Round %lu: Working for %lums...\r\n",
               cfg->ulWorkerID, ulRound, cfg->ulWorkTimeMs);
        vTaskDelay(pdMS_TO_TICKS(cfg->ulWorkTimeMs));

        /* Phase 2: Reach barrier and wait for others */
        printf("[WORKER-%lu] Round %lu: Reached barrier @ Tick=%lu\r\n",
               cfg->ulWorkerID, ulRound,
               (unsigned long)xTaskGetTickCount());

        EventBits_t uxReturn = xEventGroupSync(
            xSyncEventGroup,
            cfg->uxMyBit,      /* Set my bit */
            ALL_SYNC_BITS,     /* Wait for all bits */
            portMAX_DELAY
        );

        /* Phase 3: All workers synchronized - continue together */
        if ((uxReturn & ALL_SYNC_BITS) == ALL_SYNC_BITS)
        {
            printf("[WORKER-%lu] Round %lu: *** SYNC OK *** All passed barrier @ Tick=%lu\r\n",
                   cfg->ulWorkerID, ulRound,
                   (unsigned long)xTaskGetTickCount());

            /* Only worker 0 updates global counter and LED */
            if (cfg->ulWorkerID == 0)
            {
                ulSyncCount++;
                HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
                printf("[SYNC] === Barrier #%lu completed ===\r\n\r\n", ulSyncCount);
            }
        }
        else
        {
            printf("[WORKER-%lu] Round %lu: Sync TIMEOUT!\r\n",
                   cfg->ulWorkerID, ulRound);
        }
    }
}

void vApplicationMallocFailedHook(void)
{
    printf("[ERROR] Malloc failed!\r\n");
    taskDISABLE_INTERRUPTS();
    for (;;);
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    printf("[ERROR] Stack overflow: %s\r\n", pcTaskName);
    taskDISABLE_INTERRUPTS();
    for (;;);
}

static WorkerConfig_t workerConfigs[3] = {
    {0, WORKER_0_BIT, 500},
    {1, WORKER_1_BIT, 1000},
    {2, WORKER_2_BIT, 1500},
};

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    UART1_Init();
    GPIO_Init();

    printf("\r\n=== STM32 Event Group Sync ===\r\n");
    printf("3 workers with xEventGroupSync barrier\r\n");
    printf("Work times: W0=500ms, W1=1000ms, W2=1500ms\r\n\r\n");

    xSyncEventGroup = xEventGroupCreate();

    if (xSyncEventGroup != NULL)
    {
        xTaskCreate(vWorkerTask, "Worker0", 256, &workerConfigs[0], 2, NULL);
        xTaskCreate(vWorkerTask, "Worker1", 256, &workerConfigs[1], 2, NULL);
        xTaskCreate(vWorkerTask, "Worker2", 256, &workerConfigs[2], 2, NULL);
    }

    vTaskStartScheduler();
    for (;;);
}

void SysTick_Handler(void)
{
    HAL_IncTick();
    if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED)
        xPortSysTickHandler();
}
