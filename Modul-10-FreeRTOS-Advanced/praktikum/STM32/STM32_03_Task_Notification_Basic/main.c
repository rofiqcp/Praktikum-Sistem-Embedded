#include "config.h"
#include "FreeRTOS.h"
#include "task.h"

TaskHandle_t xTask1Handle, xTask2Handle;

void vTaskSender(void *pvParameters) {
  for (;;) {
    xTaskNotify(xTask2Handle, 0x01, eSetBits);
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

void vTaskReceiver(void *pvParameters) {
  uint32_t ulNotificationValue;
  for (;;) {
    xTaskNotifyWait(0x00, 0xFFFFFFFF, &ulNotificationValue, portMAX_DELAY);
  }
}

int main(void) {
  HAL_Init();
  SystemClock_Config();
  
  xTaskCreate(vTaskSender, "Sender", configMINIMAL_STACK_SIZE, NULL, 1, &xTask1Handle);
  xTaskCreate(vTaskReceiver, "Receiver", configMINIMAL_STACK_SIZE, NULL, 2, &xTask2Handle);
  
  vTaskStartScheduler();
  
  for (;;);
}
