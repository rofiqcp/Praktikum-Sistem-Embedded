/**
 * STM32_04_Queue_ISR
 * Button on PA0 with EXTI interrupt sends event via xQueueSendFromISR
 * to LED handler task. Demonstrates ISR-to-task communication.
 */

#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include <stdio.h>
#include <string.h>

static UART_HandleTypeDef huart1;
static QueueHandle_t xButtonQueue;

/* Button event structure */
typedef struct {
    uint32_t press_count;
    uint32_t timestamp;
} ButtonEvent_t;

static volatile uint32_t ulButtonPressCount = 0;
static volatile uint32_t ulEventsProcessed = 0;

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

/* Button GPIO Init - PA0 with EXTI rising edge */
static void Button_GPIO_Init(void) {
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_AFIO_CLK_ENABLE();

    GPIO_InitTypeDef gpio = {0};
    gpio.Pin = GPIO_PIN_0;
    gpio.Mode = GPIO_MODE_IT_RISING;
    gpio.Pull = GPIO_PULLDOWN;
    HAL_GPIO_Init(GPIOA, &gpio);

    HAL_NVIC_SetPriority(EXTI0_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY, 0);
    HAL_NVIC_EnableIRQ(EXTI0_IRQn);
}

void vApplicationMallocFailedHook(void) {
    printf("[ERROR] Malloc failed!\r\n");
    while (1);
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
    printf("[ERROR] Stack overflow in task: %s\r\n", pcTaskName);
    while (1);
}

/* EXTI0 ISR */
void EXTI0_IRQHandler(void) {
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_0);
}

/* HAL GPIO EXTI Callback - called from ISR context */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    if (GPIO_Pin == GPIO_PIN_0) {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        ButtonEvent_t event;

        ulButtonPressCount++;
        event.press_count = ulButtonPressCount;
        event.timestamp = xTaskGetTickCountFromISR();

        xQueueSendFromISR(xButtonQueue, &event, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

/* LED Handler Task - processes button events from ISR queue */
static void vLEDHandlerTask(void *pvParameters) {
    ButtonEvent_t event;

    printf("[LEDHandler] Task started, waiting for button events...\r\n");

    for (;;) {
        if (xQueueReceive(xButtonQueue, &event, portMAX_DELAY) == pdPASS) {
            ulEventsProcessed++;
            HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);

            printf("[LEDHandler] Button #%lu at tick=%lu - LED toggled (processed=%lu)\r\n",
                   event.press_count, event.timestamp, ulEventsProcessed);
        }
    }
}

/* Monitor Task - periodic status */
static void vMonitorTask(void *pvParameters) {
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(5000));
        printf("\r\n===== ISR Queue Statistics =====\r\n");
        printf("  Button presses (ISR): %lu\r\n", ulButtonPressCount);
        printf("  Events processed:     %lu\r\n", ulEventsProcessed);
        printf("  Queue pending:        %u\r\n",
               (unsigned int)uxQueueMessagesWaiting(xButtonQueue));
        printf("  Free heap:            %u bytes\r\n",
               (unsigned int)xPortGetFreeHeapSize());
        printf("================================\r\n\r\n");
    }
}

/* Simulated button press task (for testing without hardware button) */
static void vSimButtonTask(void *pvParameters) {
    printf("[SimButton] Simulated button press task started\r\n");
    printf("[SimButton] Press PA0 button or wait for simulated presses\r\n");

    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(3000));
        /* Simulate button press by calling the callback */
        HAL_GPIO_EXTI_Callback(GPIO_PIN_0);
        printf("[SimButton] Simulated button press\r\n");
    }
}

int main(void) {
    HAL_Init();
    SystemClock_Config();
    UART1_Init();
    LED_GPIO_Init();
    Button_GPIO_Init();

    printf("\r\n========================================\r\n");
    printf("  STM32 FreeRTOS Queue ISR Demo\r\n");
    printf("  Button PA0 -> ISR -> Queue -> Task\r\n");
    printf("========================================\r\n\r\n");

    xButtonQueue = xQueueCreate(10, sizeof(ButtonEvent_t));
    if (xButtonQueue == NULL) {
        printf("[ERROR] Failed to create button queue!\r\n");
        while (1);
    }

    xTaskCreate(vLEDHandlerTask, "LEDHandler", 256, NULL, 3, NULL);
    xTaskCreate(vMonitorTask, "Monitor", 256, NULL, 1, NULL);
    xTaskCreate(vSimButtonTask, "SimButton", 256, NULL, 1, NULL);

    printf("[INIT] Tasks created, starting scheduler...\r\n\r\n");
    vTaskStartScheduler();

    while (1);
}
