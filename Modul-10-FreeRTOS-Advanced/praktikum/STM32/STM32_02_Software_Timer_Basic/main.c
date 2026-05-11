#include "config.h"
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"

TimerHandle_t xTimer;

void vTimerCallback(TimerHandle_t xTimer) {
  // Timer callback
}

void vTaskFunction(void *pvParameters) {
  for (;;) {
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

int main(void) {
  HAL_Init();
  SystemClock_Config();
  
  xTimer = xTimerCreate("Timer", pdMS_TO_TICKS(500), pdTRUE, 0, vTimerCallback);
  configASSERT(xTimer != NULL);
  xTimerStart(xTimer, 0);
  
  xTaskCreate(vTaskFunction, "Task", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
  
  vTaskStartScheduler();
  
  for (;;);
}
