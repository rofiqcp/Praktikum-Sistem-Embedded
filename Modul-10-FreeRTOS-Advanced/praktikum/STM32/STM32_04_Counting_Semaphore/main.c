#include "config.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

SemaphoreHandle_t xCountingSemaphore;

void vTaskGive(void *pvParameters) {
  for (;;) {
    xSemaphoreGive(xCountingSemaphore);
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

void vTaskTake(void *pvParameters) {
  for (;;) {
    xSemaphoreTake(xCountingSemaphore, portMAX_DELAY);
  }
}

int main(void) {
  HAL_Init();
  SystemClock_Config();
  
  xCountingSemaphore = xSemaphoreCreateCounting(5, 0);
  configASSERT(xCountingSemaphore != NULL);
  
  xTaskCreate(vTaskGive, "Give", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
  xTaskCreate(vTaskTake, "Take", configMINIMAL_STACK_SIZE, NULL, 2, NULL);
  
  vTaskStartScheduler();
  
  for (;;);
}
