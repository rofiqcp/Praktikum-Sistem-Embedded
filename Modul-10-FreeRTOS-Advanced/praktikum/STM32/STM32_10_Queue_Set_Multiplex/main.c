#include "config.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

QueueSetHandle_t xQueueSet;
QueueHandle_t xQueue1, xQueue2;

void vSenderTask(void *pvParameters) {
  uint32_t ulValue = 0;
  for (;;) {
    xQueueSend(xQueue1, &ulValue, portMAX_DELAY);
    ulValue++;
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

void vReceiverTask(void *pvParameters) {
  QueueSetMemberHandle_t xActivatedMember;
  for (;;) {
    xActivatedMember = xQueueSelectFromSet(xQueueSet, portMAX_DELAY);
    if (xActivatedMember == xQueue1) {
      uint32_t ulValue;
      xQueueReceive(xQueue1, &ulValue, 0);
    } else if (xActivatedMember == xQueue2) {
      uint32_t ulValue;
      xQueueReceive(xQueue2, &ulValue, 0);
    }
  }
}

int main(void) {
  HAL_Init();
  SystemClock_Config();
  
  xQueue1 = xQueueCreate(5, sizeof(uint32_t));
  xQueue2 = xQueueCreate(5, sizeof(uint32_t));
  xQueueSet = xQueueCreateSet(10);
  xQueueAddToSet(xQueue1, xQueueSet);
  xQueueAddToSet(xQueue2, xQueueSet);
  
  xTaskCreate(vSenderTask, "Sender", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
  xTaskCreate(vReceiverTask, "Receiver", configMINIMAL_STACK_SIZE, NULL, 2, NULL);
  
  vTaskStartScheduler();
  
  for (;;);
}
