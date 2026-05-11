#include "config.h"
#include "FreeRTOS.h"
#include "task.h"

StaticTask_t xTaskBuffer;
StackType_t xStack[configMINIMAL_STACK_SIZE];

void vStaticTask(void *pvParameters) {
  for (;;) {
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

int main(void) {
  HAL_Init();
  SystemClock_Config();
  
  TaskHandle_t xTaskHandle = xTaskCreateStatic(
    vStaticTask,
    "Static",
    configMINIMAL_STACK_SIZE,
    NULL,
    1,
    xStack,
    &xTaskBuffer
  );
  configASSERT(xTaskHandle != NULL);
  
  vTaskStartScheduler();
  
  for (;;);
}
