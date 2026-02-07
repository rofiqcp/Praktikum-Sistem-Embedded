/**
 * STM32_03_Queue_Multiple
 * Two queues: command queue (Commander->Executor) and status queue (Executor->Monitor).
 * Demonstrates multi-queue pipeline pattern.
 */

#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include <stdio.h>
#include <string.h>

static UART_HandleTypeDef huart1;

/* Command types */
typedef enum {
    CMD_LED_ON = 0,
    CMD_LED_OFF,
    CMD_LED_TOGGLE,
    CMD_LED_BLINK,
    CMD_STATUS_REQUEST
} CommandType_t;

/* Command structure */
typedef struct {
    CommandType_t type;
    uint32_t      param;
    uint32_t      cmd_id;
} Command_t;

/* Status structure */
typedef struct {
    uint32_t cmd_id;
    uint8_t  success;
    uint32_t exec_time_ms;
    uint32_t timestamp;
} Status_t;

static QueueHandle_t xCommandQueue;
static QueueHandle_t xStatusQueue;
static volatile uint32_t ulCmdsSent = 0;
static volatile uint32_t ulCmdsExecuted = 0;
static volatile uint32_t ulStatusReceived = 0;

int _write(int file, char *ptr, int len) {
    HAL_UART_Transmit(&huart1, (uint8_t*)ptr, len, HAL_MAX_DELAY);
    return len;
}

static void SystemClock_Config(void) {
    RCC_OscInitTypeDef osc = {0};
    RCC_ClkInitTypeDef clk = {0};
    osc.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    osc.HSEState = RCC_HSE_ON;
    osc.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    osc.PLL.PLLState = RCC_PLL_ON;
    osc.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    osc.PLL.PLLMUL = RCC_PLL_MUL9;
    HAL_RCC_OscConfig(&osc);
    clk.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                    RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    clk.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    clk.AHBCLKDivider = RCC_SYSCLK_DIV1;
    clk.APB1CLKDivider = RCC_HCLK_DIV2;
    clk.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_2);
}

static void UART1_Init(void) {
    __HAL_RCC_USART1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    GPIO_InitTypeDef gpio = {0};
    gpio.Pin = GPIO_PIN_9;
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &gpio);
    gpio.Pin = GPIO_PIN_10;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &gpio);
    huart1.Instance = USART1;
    huart1.Init.BaudRate = 115200;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    HAL_UART_Init(&huart1);
}

static void LED_GPIO_Init(void) {
    __HAL_RCC_GPIOC_CLK_ENABLE();
    GPIO_InitTypeDef gpio = {0};
    gpio.Pin = GPIO_PIN_13;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOC, &gpio);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
}

void vApplicationMallocFailedHook(void) {
    printf("[ERROR] Malloc failed!\r\n");
    while (1);
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
    printf("[ERROR] Stack overflow in task: %s\r\n", pcTaskName);
    while (1);
}

static const char *cmd_name(CommandType_t t) {
    switch (t) {
        case CMD_LED_ON:         return "LED_ON";
        case CMD_LED_OFF:        return "LED_OFF";
        case CMD_LED_TOGGLE:     return "LED_TOGGLE";
        case CMD_LED_BLINK:      return "LED_BLINK";
        case CMD_STATUS_REQUEST: return "STATUS_REQ";
        default: return "UNKNOWN";
    }
}

/* Commander Task - sends commands in sequence */
static void vCommanderTask(void *pvParameters) {
    Command_t cmd;
    CommandType_t sequence[] = {CMD_LED_ON, CMD_LED_OFF, CMD_LED_TOGGLE,
                                CMD_LED_BLINK, CMD_STATUS_REQUEST};
    uint32_t idx = 0;

    printf("[Commander] Task started\r\n");

    for (;;) {
        cmd.type = sequence[idx % 5];
        cmd.param = (cmd.type == CMD_LED_BLINK) ? 3 : 0;
        cmd.cmd_id = ++ulCmdsSent;

        if (xQueueSend(xCommandQueue, &cmd, pdMS_TO_TICKS(200)) == pdPASS) {
            printf("[Commander] Sent CMD #%lu: %s (param=%lu)\r\n",
                   cmd.cmd_id, cmd_name(cmd.type), cmd.param);
        } else {
            printf("[Commander] Command queue FULL!\r\n");
        }

        idx++;
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/* Executor Task - executes commands and sends status */
static void vExecutorTask(void *pvParameters) {
    Command_t cmd;
    Status_t status;
    uint32_t start_tick;

    printf("[Executor] Task started\r\n");

    for (;;) {
        if (xQueueReceive(xCommandQueue, &cmd, pdMS_TO_TICKS(2000)) == pdPASS) {
            start_tick = xTaskGetTickCount();
            ulCmdsExecuted++;

            printf("[Executor] Executing CMD #%lu: %s\r\n",
                   cmd.cmd_id, cmd_name(cmd.type));

            status.success = 1;

            switch (cmd.type) {
                case CMD_LED_ON:
                    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
                    vTaskDelay(pdMS_TO_TICKS(50));
                    break;
                case CMD_LED_OFF:
                    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
                    vTaskDelay(pdMS_TO_TICKS(50));
                    break;
                case CMD_LED_TOGGLE:
                    HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
                    vTaskDelay(pdMS_TO_TICKS(50));
                    break;
                case CMD_LED_BLINK:
                    for (uint32_t i = 0; i < cmd.param; i++) {
                        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
                        vTaskDelay(pdMS_TO_TICKS(200));
                        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
                        vTaskDelay(pdMS_TO_TICKS(200));
                    }
                    break;
                case CMD_STATUS_REQUEST:
                    printf("[Executor] Status: executed=%lu, heap=%u\r\n",
                           ulCmdsExecuted,
                           (unsigned int)xPortGetFreeHeapSize());
                    break;
            }

            /* Send status back */
            status.cmd_id = cmd.cmd_id;
            status.exec_time_ms = xTaskGetTickCount() - start_tick;
            status.timestamp = xTaskGetTickCount();

            if (xQueueSend(xStatusQueue, &status, pdMS_TO_TICKS(100)) == pdPASS) {
                printf("[Executor] Status sent for CMD #%lu (took %lums)\r\n",
                       status.cmd_id, status.exec_time_ms);
            }
        }
    }
}

/* Monitor Task - receives status reports */
static void vMonitorTask(void *pvParameters) {
    Status_t status;

    printf("[Monitor] Task started\r\n");

    for (;;) {
        if (xQueueReceive(xStatusQueue, &status, pdMS_TO_TICKS(5000)) == pdPASS) {
            ulStatusReceived++;
            printf("[Monitor] CMD #%lu: %s (exec_time=%lums, t=%lu)\r\n",
                   status.cmd_id,
                   status.success ? "SUCCESS" : "FAILED",
                   status.exec_time_ms,
                   status.timestamp);
        }

        /* Periodic summary */
        if (ulStatusReceived > 0 && ulStatusReceived % 5 == 0) {
            printf("\r\n===== Pipeline Statistics =====\r\n");
            printf("  Commands sent:     %lu\r\n", ulCmdsSent);
            printf("  Commands executed: %lu\r\n", ulCmdsExecuted);
            printf("  Statuses received: %lu\r\n", ulStatusReceived);
            printf("  Cmd queue pending: %u\r\n",
                   (unsigned int)uxQueueMessagesWaiting(xCommandQueue));
            printf("  Sts queue pending: %u\r\n",
                   (unsigned int)uxQueueMessagesWaiting(xStatusQueue));
            printf("  Free heap:         %u bytes\r\n",
                   (unsigned int)xPortGetFreeHeapSize());
            printf("===============================\r\n\r\n");
        }
    }
}

int main(void) {
    HAL_Init();
    SystemClock_Config();
    UART1_Init();
    LED_GPIO_Init();

    printf("\r\n========================================\r\n");
    printf("  STM32 FreeRTOS Multiple Queues Demo\r\n");
    printf("  Commander -> Executor -> Monitor\r\n");
    printf("========================================\r\n\r\n");

    xCommandQueue = xQueueCreate(5, sizeof(Command_t));
    xStatusQueue = xQueueCreate(5, sizeof(Status_t));

    if (xCommandQueue == NULL || xStatusQueue == NULL) {
        printf("[ERROR] Failed to create queues!\r\n");
        while (1);
    }

    xTaskCreate(vCommanderTask, "Commander", 256, NULL, 2, NULL);
    xTaskCreate(vExecutorTask, "Executor", 256, NULL, 3, NULL);
    xTaskCreate(vMonitorTask, "Monitor", 256, NULL, 2, NULL);

    printf("[INIT] All tasks created, starting scheduler...\r\n\r\n");
    vTaskStartScheduler();

    while (1);
}
