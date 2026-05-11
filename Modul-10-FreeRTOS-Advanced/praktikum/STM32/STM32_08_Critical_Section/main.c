#include "config.h"
#include "FreeRTOS.h"
#include "task.h"

void vCriticalTask(void *pvParameters) {
  for (;;) {
    taskENTER_CRITICAL();
    // Critical section code
    taskEXIT_CRITICAL();
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

int main(void) {
  HAL_Init();
  SystemClock_Config();
  
  xTaskCreate(vCriticalTask, "Critical", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
  
  vTaskStartScheduler();
  
  for (;;);
}
