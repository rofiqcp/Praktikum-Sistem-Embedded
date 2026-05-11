// File utama program STM32_09_GPIO_Interrupt_Combined
// Combined GPIO + interrupt + RTOS
// Multiple buttons dengan interrupt
// Task untuk setiap fungsi button
// Semaphore dan queue

#include "config.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>
#include <string.h>
#include "semphr.h"
#include "queue.h"

// Mendeklarasikan handle untuk UART
UART_HandleTypeDef huart1;
// Mendeklarasikan handle untuk semaphore button 1
SemaphoreHandle_t btn1Semaphore = NULL;
// Mendeklarasikan handle untuk semaphore button 2
SemaphoreHandle_t btn2Semaphore = NULL;
// Mendeklarasikan handle untuk semaphore button 3
SemaphoreHandle_t btn3Semaphore = NULL;
// Mendeklarasikan handle untuk queue button events
QueueHandle_t buttonQueue = NULL;
// Mendeklarasikan handle untuk task button 1
TaskHandle_t btn1TaskHandle = NULL;
// Mendeklarasikan handle untuk task button 2
TaskHandle_t btn2TaskHandle = NULL;
// Mendeklarasikan handle untuk task button 3
TaskHandle_t btn3TaskHandle = NULL;
// Mendeklarasikan handle untuk task LED control
TaskHandle_t ledTaskHandle = NULL;

typedef struct
{
    uint8_t button_id;
    TickType_t press_time;
} ButtonEvent_t;

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART1_UART_Init(void);
void Button1Task(void *argument);
void Button2Task(void *argument);
void Button3Task(void *argument);
void LedControlTask(void *argument);
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin);
void UART_SendString(char *str);
void UART_SendNumber(uint32_t num);

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USART1_UART_Init();

    btn1Semaphore = xSemaphoreCreateBinary();
    btn2Semaphore = xSemaphoreCreateBinary();
    btn3Semaphore = xSemaphoreCreateBinary();

    buttonQueue = xQueueCreate(10, sizeof(ButtonEvent_t));

    xTaskCreate(Button1Task, "BTN1_Task", TASK_STACK_SIZE, NULL, BTN1_TASK_PRIORITY, &btn1TaskHandle);
    xTaskCreate(Button2Task, "BTN2_Task", TASK_STACK_SIZE, NULL, BTN2_TASK_PRIORITY, &btn2TaskHandle);
    xTaskCreate(Button3Task, "BTN3_Task", TASK_STACK_SIZE, NULL, BTN3_TASK_PRIORITY, &btn3TaskHandle);
    xTaskCreate(LedControlTask, "LED_Task", TASK_STACK_SIZE, NULL, 1, &ledTaskHandle);

    UART_SendString("STM32 GPIO Interrupt Combined RTOS Ready\r\n");

    vTaskStartScheduler();

    while (1)
    {
    }
}

void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
    HAL_RCC_OscConfig(&RCC_OscInitStruct);

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK|RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0);
}

static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitStruct.Pin = LED_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = BUTTON1_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(BUTTON1_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = BUTTON2_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
    HAL_GPIO_Init(BUTTON2_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = BUTTON3_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
    HAL_GPIO_Init(BUTTON3_PORT, &GPIO_InitStruct);

    HAL_NVIC_SetPriority(EXTI0_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(EXTI0_IRQn);

    HAL_NVIC_SetPriority(EXTI1_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(EXTI1_IRQn);

    HAL_NVIC_SetPriority(EXTI2_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(EXTI2_IRQn);
}

static void MX_USART1_UART_Init(void)
{
    __HAL_RCC_USART1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Pin = GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    huart1.Instance = USART1;
    huart1.Init.BaudRate = UART_BAUDRATE;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(&huart1);
}

void UART_SendString(char *str)
{
    HAL_UART_Transmit(&huart1, (uint8_t *)str, strlen(str), HAL_MAX_DELAY);
}

void UART_SendNumber(uint32_t num)
{
    char buf[16];
    snprintf(buf, sizeof(buf), "%lu", num);
    UART_SendString(buf);
}

void Button1Task(void *argument)
{
    while (1)
    {
        if (xSemaphoreTake(btn1Semaphore, portMAX_DELAY) == pdTRUE)
        {
            UART_SendString("Button 1 Pressed\r\n");
            HAL_GPIO_TogglePin(LED_PORT, LED_PIN);
        }
    }
}

void Button2Task(void *argument)
{
    ButtonEvent_t event;
    while (1)
    {
        if (xSemaphoreTake(btn2Semaphore, portMAX_DELAY) == pdTRUE)
        {
            event.button_id = 2;
            event.press_time = xTaskGetTickCount();
            xQueueSend(buttonQueue, &event, 0);
            UART_SendString("Button 2 Pressed, sending to queue\r\n");
        }
    }
}

void Button3Task(void *argument)
{
    while (1)
    {
        if (xSemaphoreTake(btn3Semaphore, portMAX_DELAY) == pdTRUE)
        {
            UART_SendString("Button 3 Pressed - LED Blink Fast\r\n");
            for (int i = 0; i < 6; i++)
            {
                HAL_GPIO_TogglePin(LED_PORT, LED_PIN);
                vTaskDelay(pdMS_TO_TICKS(100));
            }
        }
    }
}

void LedControlTask(void *argument)
{
    ButtonEvent_t received_event;
    while (1)
    {
        if (xQueueReceive(buttonQueue, &received_event, pdMS_TO_TICKS(500)) == pdTRUE)
        {
            UART_SendString("LED Task: Button ");
            UART_SendNumber(received_event.button_id);
            UART_SendString(" at tick ");
            UART_SendNumber(received_event.press_time);
            UART_SendString("\r\n");
        }
    }
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    if (GPIO_Pin == BUTTON1_PIN)
    {
        xSemaphoreGiveFromISR(btn1Semaphore, &xHigherPriorityTaskWoken);
    }
    else if (GPIO_Pin == BUTTON2_PIN)
    {
        xSemaphoreGiveFromISR(btn2Semaphore, &xHigherPriorityTaskWoken);
    }
    else if (GPIO_Pin == BUTTON3_PIN)
    {
        xSemaphoreGiveFromISR(btn3Semaphore, &xHigherPriorityTaskWoken);
    }

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void EXTI0_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(BUTTON1_PIN);
}

void EXTI1_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(BUTTON2_PIN);
}

void EXTI2_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(BUTTON3_PIN);
}
