/**
 * STM32_07_Notification_Value
 * xTaskNotify() with eSetValueWithOverwrite sends command values.
 * Receiver xTaskNotifyWait() reads value and acts accordingly.
 */

#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>
#include <string.h>

UART_HandleTypeDef huart1;

static TaskHandle_t xReceiverTask = NULL;

/* Command definitions */
#define CMD_LED_ON      0x01
#define CMD_LED_OFF     0x02
#define CMD_LED_BLINK   0x03
#define CMD_STATUS      0x04
#define CMD_RESET       0x05

static volatile uint32_t ulCommandsSent = 0;
static volatile uint32_t ulCommandsProcessed = 0;

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

static const char *pcCmdName(uint32_t cmd)
{
    switch (cmd)
    {
        case CMD_LED_ON:    return "LED_ON";
        case CMD_LED_OFF:   return "LED_OFF";
        case CMD_LED_BLINK: return "LED_BLINK";
        case CMD_STATUS:    return "STATUS";
        case CMD_RESET:     return "RESET";
        default:            return "UNKNOWN";
    }
}

/* Sender task - cycles through commands */
static void vSenderTask(void *pvParameters)
{
    const uint32_t commands[] = {CMD_LED_ON, CMD_LED_BLINK, CMD_STATUS,
                                 CMD_LED_OFF, CMD_RESET};
    uint32_t idx = 0;

    for (;;)
    {
        uint32_t cmd = commands[idx % 5];
        ulCommandsSent++;

        printf("[SEND] Command: 0x%02lX (%s), #%lu, Tick=%lu\r\n",
               cmd, pcCmdName(cmd), ulCommandsSent,
               (unsigned long)xTaskGetTickCount());

        xTaskNotify(xReceiverTask, cmd, eSetValueWithOverwrite);

        idx++;
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

/* Receiver task - processes command values */
static void vReceiverTask(void *pvParameters)
{
    uint32_t ulNotifiedValue;

    for (;;)
    {
        if (xTaskNotifyWait(0x00, 0xFFFFFFFF, &ulNotifiedValue, portMAX_DELAY) == pdTRUE)
        {
            ulCommandsProcessed++;
            printf("[RECV] Command: 0x%02lX (%s), #%lu\r\n",
                   ulNotifiedValue, pcCmdName(ulNotifiedValue),
                   ulCommandsProcessed);

            switch (ulNotifiedValue)
            {
                case CMD_LED_ON:
                    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
                    printf("  -> LED ON\r\n");
                    break;

                case CMD_LED_OFF:
                    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
                    printf("  -> LED OFF\r\n");
                    break;

                case CMD_LED_BLINK:
                    for (int i = 0; i < 6; i++)
                    {
                        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
                        vTaskDelay(pdMS_TO_TICKS(100));
                    }
                    printf("  -> LED BLINK done\r\n");
                    break;

                case CMD_STATUS:
                    printf("  -> Sent=%lu, Processed=%lu, HWM=%lu\r\n",
                           ulCommandsSent, ulCommandsProcessed,
                           (unsigned long)uxTaskGetStackHighWaterMark(NULL));
                    break;

                case CMD_RESET:
                    printf("  -> RESET counters\r\n");
                    break;

                default:
                    printf("  -> Unknown command: 0x%02lX\r\n", ulNotifiedValue);
                    break;
            }
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

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    UART1_Init();
    GPIO_Init();

    printf("\r\n=== STM32 Notification Value ===\r\n");
    printf("xTaskNotify with eSetValueWithOverwrite\r\n\r\n");

    xTaskCreate(vReceiverTask, "Receiver", 256, NULL, 2, &xReceiverTask);
    xTaskCreate(vSenderTask, "Sender", 256, NULL, 1, NULL);

    vTaskStartScheduler();
    for (;;);
}

void SysTick_Handler(void)
{
    HAL_IncTick();
    if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED)
        xPortSysTickHandler();
}
