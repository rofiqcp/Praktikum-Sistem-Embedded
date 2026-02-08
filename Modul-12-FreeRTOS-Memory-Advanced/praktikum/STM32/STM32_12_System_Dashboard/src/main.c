/**
 * STM32_12_System_Dashboard
 * 
 * CAPSTONE program combining all FreeRTOS concepts from Modul 09-12.
 * Displays comprehensive system dashboard every 5 seconds:
 *   1. Task list with: name, state, priority, stack high water mark, runtime %
 *   2. Heap stats: free, min-ever free, total allocated
 *   3. Queue/semaphore status
 *   4. Uptime, tick count
 * Uses vTaskList() and vTaskGetRunTimeStats() for task info.
 * Multiple worker tasks doing different things.
 * LED heartbeat on PC13.
 * 
 * Platform: STM32F103C8 BluePill
 * UART1: PA9(TX), PA10(RX) @ 115200
 * LED: PC13 (active low)
 */

#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "timers.h"
#include <stdio.h>
#include <string.h>

/* ---- UART handle ---- */
UART_HandleTypeDef huart1;

/* ---- Printf redirect ---- */
int _write(int file, char *ptr, int len)
{
    HAL_UART_Transmit(&huart1, (uint8_t *)ptr, len, HAL_MAX_DELAY);
    return len;
}

/* ---- Clock Configuration: 72 MHz ---- */
static void SystemClock_Config(void)
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

/* ---- GPIO Init: PC13 LED ---- */
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

/* ---- UART1 Init ---- */
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

/* ---- Shared Resources ---- */
static QueueHandle_t     xSensorQueue    = NULL;
static SemaphoreHandle_t xPrintMutex     = NULL;
static SemaphoreHandle_t xCountingSem    = NULL;
static TimerHandle_t     xHeartbeatTimer = NULL;

/* Sensor data structure */
typedef struct {
    uint32_t sensor_id;
    int32_t  value;
    uint32_t timestamp;
} SensorData_t;

/* Dashboard report counter */
static volatile uint32_t dashboard_count = 0;

/* ---- Heartbeat Timer Callback ---- */
static void vHeartbeatTimerCallback(TimerHandle_t xTimer)
{
    (void)xTimer;
    HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
}

/* ---- Worker Task 1: Sensor Producer ---- */
static void vSensorProducerTask(void *pvParameters)
{
    (void)pvParameters;
    uint32_t reading = 0;

    while (1) {
        SensorData_t data;
        data.sensor_id = 1;
        data.value     = 200 + (reading % 100) - 50; /* Simulate temperature-like */
        data.timestamp = xTaskGetTickCount();

        if (xQueueSend(xSensorQueue, &data, pdMS_TO_TICKS(100)) == pdPASS) {
            xSemaphoreGive(xCountingSem);
        }

        reading++;
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/* ---- Worker Task 2: Sensor Consumer ---- */
static void vSensorConsumerTask(void *pvParameters)
{
    (void)pvParameters;
    SensorData_t data;
    uint32_t processed = 0;

    while (1) {
        if (xSemaphoreTake(xCountingSem, pdMS_TO_TICKS(2000)) == pdTRUE) {
            if (xQueueReceive(xSensorQueue, &data, pdMS_TO_TICKS(100)) == pdPASS) {
                processed++;
                /* Simulate processing */
                volatile uint32_t dummy = 0;
                for (int i = 0; i < 1000; i++) {
                    dummy += i;
                }
                (void)dummy;
            }
        }
    }
}

/* ---- Worker Task 3: Computation (CPU intensive) ---- */
static void vComputeTask(void *pvParameters)
{
    (void)pvParameters;

    while (1) {
        /* Simulate moderate CPU work */
        volatile uint32_t result = 0;
        for (int i = 0; i < 5000; i++) {
            result += i * i;
        }
        (void)result;

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

/* ---- Worker Task 4: Memory exerciser ---- */
static void vMemoryTask(void *pvParameters)
{
    (void)pvParameters;

    while (1) {
        /* Allocate, use, free - exercising the heap */
        void *buf = pvPortMalloc(64);
        if (buf) {
            memset(buf, 0xAB, 64);
            vTaskDelay(pdMS_TO_TICKS(200));
            vPortFree(buf);
        }

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

/* ---- Format uptime ---- */
static void format_uptime(uint32_t ticks, char *buf, size_t buf_size)
{
    uint32_t total_seconds = ticks / configTICK_RATE_HZ;
    uint32_t hours   = total_seconds / 3600;
    uint32_t minutes = (total_seconds % 3600) / 60;
    uint32_t seconds = total_seconds % 60;
    uint32_t ms      = ticks % configTICK_RATE_HZ;
    snprintf(buf, buf_size, "%02lu:%02lu:%02lu.%03lu",
             hours, minutes, seconds, ms);
}

/* ---- Dashboard Task ---- */
static void vDashboardTask(void *pvParameters)
{
    (void)pvParameters;
    static char task_list_buf[512];
    static char runtime_stats_buf[512];
    char uptime_str[32];

    /* Let system stabilize */
    vTaskDelay(pdMS_TO_TICKS(3000));

    while (1) {
        dashboard_count++;
        uint32_t tick_now = xTaskGetTickCount();
        format_uptime(tick_now, uptime_str, sizeof(uptime_str));

        /* Take print mutex to avoid interleaved output */
        if (xSemaphoreTake(xPrintMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {

            printf("\r\n");
            printf("[DASHBOARD] ╔══════════════════════════════════════════════╗\r\n");
            printf("[DASHBOARD] ║     FreeRTOS System Dashboard #%04lu          ║\r\n",
                   dashboard_count);
            printf("[DASHBOARD] ╚══════════════════════════════════════════════╝\r\n");
            printf("\r\n");

            /* Section 1: System Info */
            printf("[DASH_SYS] --- System Information ---\r\n");
            printf("[DASH_SYS] Uptime         : %s\r\n", uptime_str);
            printf("[DASH_SYS] Tick count      : %lu\r\n", tick_now);
            printf("[DASH_SYS] CPU Clock       : %lu MHz\r\n", SystemCoreClock / 1000000);
            printf("[DASH_SYS] Total tasks     : %lu\r\n", (uint32_t)uxTaskGetNumberOfTasks());
            printf("[DASH_SYS] Dashboard report: #%lu\r\n\r\n", dashboard_count);

            /* Section 2: Task List (vTaskList) */
            printf("[DASH_TASKS] --- Task List ---\r\n");
            printf("[DASH_TASKS] Name            State  Prio  Stack  Num\r\n");
            printf("[DASH_TASKS] --------------- -----  ----  -----  ---\r\n");
            memset(task_list_buf, 0, sizeof(task_list_buf));
            vTaskList(task_list_buf);
            /* Print line by line with our prefix */
            char *line_start = task_list_buf;
            while (*line_start) {
                char *line_end = strchr(line_start, '\n');
                if (line_end) {
                    *line_end = '\0';
                    if (strlen(line_start) > 0) {
                        printf("[DASH_TASKS] %s\r\n", line_start);
                    }
                    line_start = line_end + 1;
                } else {
                    if (strlen(line_start) > 0) {
                        printf("[DASH_TASKS] %s\r\n", line_start);
                    }
                    break;
                }
            }
            printf("\r\n");

            /* Section 3: Runtime Stats (vTaskGetRunTimeStats) */
            printf("[DASH_RUNTIME] --- Runtime Statistics ---\r\n");
            printf("[DASH_RUNTIME] Task             Abs Time    %%Time\r\n");
            printf("[DASH_RUNTIME] --------------- ----------  ------\r\n");
            memset(runtime_stats_buf, 0, sizeof(runtime_stats_buf));
            vTaskGetRunTimeStats(runtime_stats_buf);
            line_start = runtime_stats_buf;
            while (*line_start) {
                char *line_end = strchr(line_start, '\n');
                if (line_end) {
                    *line_end = '\0';
                    if (strlen(line_start) > 0) {
                        printf("[DASH_RUNTIME] %s\r\n", line_start);
                    }
                    line_start = line_end + 1;
                } else {
                    if (strlen(line_start) > 0) {
                        printf("[DASH_RUNTIME] %s\r\n", line_start);
                    }
                    break;
                }
            }
            printf("\r\n");

            /* Section 4: Heap Statistics */
            HeapStats_t heap_stats;
            vPortGetHeapStats(&heap_stats);
            size_t used = configTOTAL_HEAP_SIZE - heap_stats.xAvailableHeapSpaceInBytes;

            printf("[DASH_HEAP] --- Heap Statistics ---\r\n");
            printf("[DASH_HEAP] Total heap size      : %u bytes\r\n",
                   (unsigned)configTOTAL_HEAP_SIZE);
            printf("[DASH_HEAP] Free                 : %u bytes\r\n",
                   (unsigned)heap_stats.xAvailableHeapSpaceInBytes);
            printf("[DASH_HEAP] Used                 : %u bytes\r\n",
                   (unsigned)used);
            printf("[DASH_HEAP] Usage                : %u%%\r\n",
                   (unsigned)(used * 100 / configTOTAL_HEAP_SIZE));
            printf("[DASH_HEAP] Largest free block   : %u bytes\r\n",
                   (unsigned)heap_stats.xSizeOfLargestFreeBlockInBytes);
            printf("[DASH_HEAP] Smallest free block  : %u bytes\r\n",
                   (unsigned)heap_stats.xSizeOfSmallestFreeBlockInBytes);
            printf("[DASH_HEAP] Free block count     : %u\r\n",
                   (unsigned)heap_stats.xNumberOfFreeBlocks);
            printf("[DASH_HEAP] Min ever free        : %u bytes\r\n",
                   (unsigned)heap_stats.xMinimumEverFreeBytesRemaining);
            printf("[DASH_HEAP] Successful allocs    : %u\r\n",
                   (unsigned)heap_stats.xNumberOfSuccessfulAllocations);
            printf("[DASH_HEAP] Successful frees     : %u\r\n",
                   (unsigned)heap_stats.xNumberOfSuccessfulFrees);
            printf("\r\n");

            /* Section 5: Queue/Semaphore Status */
            printf("[DASH_SYNC] --- Synchronization Objects ---\r\n");
            if (xSensorQueue != NULL) {
                printf("[DASH_SYNC] SensorQueue: msgs waiting=%lu, spaces=%lu\r\n",
                       (uint32_t)uxQueueMessagesWaiting(xSensorQueue),
                       (uint32_t)uxQueueSpacesAvailable(xSensorQueue));
            }
            if (xCountingSem != NULL) {
                printf("[DASH_SYNC] CountingSem: count=%lu\r\n",
                       (uint32_t)uxSemaphoreGetCount(xCountingSem));
            }
            printf("[DASH_SYNC] PrintMutex: (this task holds it)\r\n");
            printf("\r\n");

            /* Section 6: Per-task stack high water mark */
            printf("[DASH_STACK] --- Stack High Water Marks ---\r\n");
            TaskHandle_t task_handles[8];
            const char *task_names[] = {"Dashboard", "SensProd", "SensCons",
                                         "Compute", "MemTask"};
            int num_named_tasks = 5;
            for (int i = 0; i < num_named_tasks; i++) {
                TaskHandle_t h = xTaskGetHandle(task_names[i]);
                if (h != NULL) {
                    UBaseType_t hwm = uxTaskGetStackHighWaterMark(h);
                    printf("[DASH_STACK] %-15s: %lu words free (min)\r\n",
                           task_names[i], (uint32_t)hwm);
                }
            }
            printf("\r\n");

            printf("[DASHBOARD] ══════════════════════════════════════════════\r\n\r\n");

            xSemaphoreGive(xPrintMutex);
        }

        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

/* ---- FreeRTOS hooks ---- */
void vApplicationMallocFailedHook(void)
{
    printf("[ERROR] Malloc failed!\r\n");
    while (1) {
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
        HAL_Delay(100);
    }
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    printf("[ERROR] Stack overflow in task: %s\r\n", pcTaskName);
    while (1) {
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
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

    printf("\r\n\r\n--- System Boot ---\r\n");
    printf("STM32_12: FreeRTOS System Dashboard (Capstone)\r\n");
    printf("CPU: %lu MHz\r\n\r\n", SystemCoreClock / 1000000);

    /* Create synchronization objects */
    xSensorQueue = xQueueCreate(10, sizeof(SensorData_t));
    xPrintMutex  = xSemaphoreCreateMutex();
    xCountingSem = xSemaphoreCreateCounting(10, 0);

    if (xSensorQueue == NULL || xPrintMutex == NULL || xCountingSem == NULL) {
        printf("[ERROR] Failed to create sync objects\r\n");
        while (1);
    }

    /* Create heartbeat timer (LED toggle every 500ms) */
    xHeartbeatTimer = xTimerCreate("Heartbeat", pdMS_TO_TICKS(500),
                                    pdTRUE, NULL, vHeartbeatTimerCallback);
    if (xHeartbeatTimer != NULL) {
        xTimerStart(xHeartbeatTimer, 0);
    }

    /* Create tasks */
    xTaskCreate(vDashboardTask,      "Dashboard", 768, NULL, 4, NULL);
    xTaskCreate(vSensorProducerTask, "SensProd",  256, NULL, 2, NULL);
    xTaskCreate(vSensorConsumerTask, "SensCons",  256, NULL, 2, NULL);
    xTaskCreate(vComputeTask,        "Compute",   256, NULL, 1, NULL);
    xTaskCreate(vMemoryTask,         "MemTask",   256, NULL, 1, NULL);

    printf("All tasks created. Starting scheduler...\r\n\r\n");

    vTaskStartScheduler();

    while (1) {
    }
}
