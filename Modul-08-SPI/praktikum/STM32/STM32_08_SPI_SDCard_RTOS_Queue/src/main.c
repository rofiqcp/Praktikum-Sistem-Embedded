/* STM32_08_SPI_SDCard_RTOS_Queue - supports bluepill_f103c8 / stm32f401cc / stm32f411ce */
#include "config.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "config.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

SPI_HandleTypeDef hspi2;
QueueHandle_t xDataQueue;

typedef struct {
    uint32_t timestamp;
    float temperature;
    float humidity;
    uint32_t sequence;
} SensorData_t;

uint32_t data_count = 0;
uint32_t write_count = 0;

void SystemClock_Config(void);
void GPIO_Init(void);
void SPI2_Init(void);
float simulate_temperature(void);
float simulate_humidity(void);

float simulate_temperature(void) {
    return TEMP_MIN + ((float)rand() / RAND_MAX) * (TEMP_MAX - TEMP_MIN);
}

float simulate_humidity(void) {
    return HUMIDITY_MIN + ((float)rand() / RAND_MAX) * (HUMIDITY_MAX - HUMIDITY_MIN);
}

void vSensorTask(void *pvParameters) {
    SensorData_t sensor_data;
    
    while (1) {
        sensor_data.timestamp = xTaskGetTickCount();
        sensor_data.temperature = simulate_temperature();
        sensor_data.humidity = simulate_humidity();
        sensor_data.sequence = data_count++;
        
        if (xQueueSend(xDataQueue, &sensor_data, pdMS_TO_TICKS(100)) == pdPASS) {
            // Data sent to queue successfully
        } else {
            // Queue full, data dropped
        }
        
        vTaskDelay(pdMS_TO_TICKS(SENSOR_READ_MS));
    }
}

void SD_WriteData(SensorData_t *data) {
    // Simulate SD card write operation
    HAL_GPIO_WritePin(SD_CS_PORT, SD_CS_PIN, GPIO_PIN_RESET);
    
    char buffer[128];
    snprintf(buffer, sizeof(buffer), "%lu,%lu,%.2f,%.2f\r\n",
             data->timestamp, data->sequence, data->temperature, data->humidity);
    
    // Simulate SPI write
    HAL_SPI_Transmit(&hspi2, (uint8_t*)buffer, strlen(buffer), HAL_MAX_DELAY);
    
    HAL_GPIO_WritePin(SD_CS_PORT, SD_CS_PIN, GPIO_PIN_SET);
    
    write_count++;
}

void vLoggerTask(void *pvParameters) {
    SensorData_t received_data;
    
    while (1) {
        if (xQueueReceive(xDataQueue, &received_data, portMAX_DELAY) == pdPASS) {
            SD_WriteData(&received_data);
        }
    }
}

void vStatusTask(void *pvParameters) {
    UBaseType_t queue_items;
    
    while (1) {
        queue_items = uxQueueMessagesWaiting(xDataQueue);
        
        // Status output via UART or debug
        // For demonstration, we just track the counts
        
        vTaskDelay(pdMS_TO_TICKS(STATUS_UPDATE_MS));
    }
}

int main(void) {
    HAL_Init();
    SystemClock_Config();
    GPIO_Init();
    SPI2_Init();
    
    xDataQueue = xQueueCreate(DATA_QUEUE_LENGTH, sizeof(SensorData_t));
    
    if (xDataQueue != NULL) {
        xTaskCreate(vSensorTask, "Sensor", SENSOR_TASK_STACK, NULL, SENSOR_TASK_PRIORITY, NULL);
        xTaskCreate(vLoggerTask, "Logger", LOGGER_TASK_STACK, NULL, LOGGER_TASK_PRIORITY, NULL);
        xTaskCreate(vStatusTask, "Status", STATUS_TASK_STACK, NULL, STATUS_TASK_PRIORITY, NULL);
        
        vTaskStartScheduler();
    }
    
    while (1) {}
}

void SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

#if defined(STM32F103xB)
    RCC_OscInitStruct.OscillatorType      = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState            = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState        = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource       = RCC_PLLSOURCE_HSI_DIV2;
    RCC_OscInitStruct.PLL.PLLMUL          = RCC_PLL_MUL16;
    HAL_RCC_OscConfig(&RCC_OscInitStruct);
    RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                     | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2);
#elif defined(STM32F401xC) || defined(STM32F411xE)
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);
    RCC_OscInitStruct.OscillatorType      = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState            = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState        = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource       = RCC_PLLSOURCE_HSI;
    RCC_OscInitStruct.PLL.PLLM            = 8;
    RCC_OscInitStruct.PLL.PLLN            = 84;
    RCC_OscInitStruct.PLL.PLLP            = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ            = 4;
    HAL_RCC_OscConfig(&RCC_OscInitStruct);
    RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                     | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2);
#endif
}

void GPIO_Init(void) {
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* SD CS */
    GPIO_InitStruct.Pin   = SD_CS_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(SD_CS_PORT, &GPIO_InitStruct);
    HAL_GPIO_WritePin(SD_CS_PORT, SD_CS_PIN, GPIO_PIN_SET);

    /* LED */
    GPIO_InitStruct.Pin = LED_PIN;
    HAL_GPIO_Init(LED_PORT, &GPIO_InitStruct);
    HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET);
}

void SPI2_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_SPI2_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    GPIO_InitStruct.Pin   = SD_SCK_PIN | SD_MISO_PIN | SD_MOSI_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
#if defined(STM32F401xC) || defined(STM32F411xE)
    GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
#endif
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    hspi2.Instance               = SPI2;
    hspi2.Init.Mode              = SPI_MODE_MASTER;
    hspi2.Init.Direction         = SPI_DIRECTION_2LINES;
    hspi2.Init.DataSize          = SPI_DATASIZE_8BIT;
    hspi2.Init.CLKPolarity       = SPI_POLARITY_LOW;
    hspi2.Init.CLKPhase          = SPI_PHASE_1EDGE;
    hspi2.Init.NSS               = SPI_NSS_SOFT;
    hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;
    hspi2.Init.FirstBit          = SPI_FIRSTBIT_MSB;
    hspi2.Init.TIMode            = SPI_TIMODE_DISABLE;
    hspi2.Init.CRCCalculation    = SPI_CRCCALCULATION_DISABLE;
    HAL_SPI_Init(&hspi2);
}

void SysTick_Handler(void) {
    HAL_IncTick();
    if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED) {
        extern void xPortSysTickHandler(void);
        xPortSysTickHandler();
    }
}

void vApplicationStackOverflowHook(xTaskHandle xTask, signed char *pcTaskName) {
    (void)xTask; (void)pcTaskName;
    while (1) {}
}

void vApplicationMallocFailedHook(void) {
    while (1) {}
}
