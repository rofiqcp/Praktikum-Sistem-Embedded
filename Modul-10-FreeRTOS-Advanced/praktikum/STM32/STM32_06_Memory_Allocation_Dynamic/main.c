#include "config.h"
#include "FreeRTOS.h"
#include "task.h"

void vTaskDynamicMem(void *pvParameters) {
  uint8_t *pucBuffer;
  for (;;) {
    pucBuffer = pvPortMalloc(128);
    if (pucBuffer != NULL) {
      vPortFree(pucBuffer);
    }
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

int main(void) {
  HAL_Init();
  SystemClock_Config();
  
  xTaskCreate(vTaskDynamicMem, "Dynamic", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
  
  vTaskStartScheduler();
  
  for (;;);
}
