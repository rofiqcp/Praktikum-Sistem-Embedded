#include "main.h"
#include "config.h"
#include "FreeRTOS.h"
#include "task.h"
#include "event_groups.h"
#include "semphr.h"
#include <stdio.h>

// Event bit definitions
#define EVENT_BIT_0 (1 << 0)
#define EVENT_BIT_1 (1 << 1)
#define EVENT_BIT_2 (1 << 2)

// Event Group handle
EventGroupHandle_t xEventGroup;

// Task handles
TaskHandle_t xTask1Handle = NULL;
TaskHandle_t xTask2Handle = NULL;
TaskHandle_t xTask3Handle = NULL;
TaskHandle_t xMonitorTaskHandle = NULL;

// Task prototypes
void vTask1(void *pvParameters);
void vTask2(void *pvParameters);
void vTask3(void *pvParameters);
void vEventMonitorTask(void *pvParameters);

int main(void) {
    // Initialize HAL
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();

    // Create Event Group
    xEventGroup = xEventGroupCreate();
    if (xEventGroup == NULL) {
        Error_Handler();
    }

    // Create tasks
    xTaskCreate(vTask1, "Task1", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, &xTask1Handle);
    xTaskCreate(vTask2, "Task2", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, &xTask2Handle);
    xTaskCreate(vTask3, "Task3", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, &xTask3Handle);
    xTaskCreate(vEventMonitorTask, "EventMonitor", configMINIMAL_STACK_SIZE * 2, NULL, tskIDLE_PRIORITY + 2, &xMonitorTaskHandle);

    // Start scheduler
    vTaskStartScheduler();

    // Should not reach here
    while (1) {
        Error_Handler();
    }
}

// Task1 - set bit 0 periodically
void vTask1(void *pvParameters) {
    while (1) {
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
        xEventGroupSetBits(xEventGroup, EVENT_BIT_0);
        vTaskDelay(pdMS_TO_TICKS(1000));
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

// Task2 - set bit 1 periodically
void vTask2(void *pvParameters) {
    while (1) {
        xEventGroupSetBits(xEventGroup, EVENT_BIT_1);
        vTaskDelay(pdMS_TO_TICKS(1500));
        vTaskDelay(pdMS_TO_TICKS(2500));
    }
}

// Task3 - set bit 2 periodically
void vTask3(void *pvParameters) {
    while (1) {
        xEventGroupSetBits(xEventGroup, EVENT_BIT_2);
        vTaskDelay(pdMS_TO_TICKS(3000));
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

// Event Monitor Task - wait for AND and OR conditions
void vEventMonitorTask(void *pvParameters) {
    EventBits_t uxBits;
    while (1) {
        // Wait for AND condition (BIT0 AND BIT1)
        uxBits = xEventGroupWaitBits(
            xEventGroup,
            EVENT_BIT_0 | EVENT_BIT_1,
            pdTRUE,
            pdTRUE,
            portMAX_DELAY
        );

        // Wait for OR condition (BIT1 OR BIT2)
        uxBits = xEventGroupWaitBits(
            xEventGroup,
            EVENT_BIT_1 | EVENT_BIT_2,
            pdTRUE,
            pdFALSE,
            portMAX_DELAY
        );
    }
}

// GPIO Init
void MX_GPIO_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    __HAL_RCC_GPIOC_CLK_ENABLE();

    GPIO_InitStruct.Pin = GPIO_PIN_13;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
}

// System Clock Config
void SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
    RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;

    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) {
        Error_Handler();
    }
}

// Error Handler
void Error_Handler(void) {
    __disable_irq();
    while (1) {
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
        HAL_Delay(500);
    }
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line) {
}
#endif
