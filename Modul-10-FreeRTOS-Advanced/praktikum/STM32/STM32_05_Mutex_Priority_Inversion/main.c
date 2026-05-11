#include "config.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

SemaphoreHandle_t xMutex;

void vLowPriorityTask(void *pvParameters) {
  for (;;) {
    xSemaphoreTake(xMutex, portMAX_DELAY);
    vTaskDelay(pdMS_TO_TICKS(100));
    xSemaphoreGive(xMutex);
  }
}

void vMediumPriorityTask(void *pvParameters) {
  for (;;) {
    vTaskDelay(pdMS_TO_TICKS(50));
  }
}

void vHighPriorityTask(void *pvParameters) {
  for (;;) {
    vTaskDelay(pdMS_TO_TICKS(20));
    xSemaphoreTake(xMutex, portMAX_DELAY);
    xSemaphoreGive(xMutex);
  }
}

int main(void) {
  HAL_Init();
  SystemClock_Config();
  
  xMutex = xSemaphoreCreateMutex();
  configASSERT(xMutex != NULL);
  
  xTaskCreate(vLowPriorityTask, "Low", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
  xTaskCreate(vMediumPriorityTask, "Medium", configMINIMAL_STACK_SIZE, NULL, 2, NULL);
  xTaskCreate(vHighPriorityTask, "High", configMINIMAL_STACK_SIZE, NULL, 3, NULL);
  
  vTaskStartScheduler();
  
  for (;;);
}
