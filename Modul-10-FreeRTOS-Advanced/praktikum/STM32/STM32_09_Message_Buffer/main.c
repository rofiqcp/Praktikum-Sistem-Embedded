#include "config.h"
#include "FreeRTOS.h"
#include "task.h"
#include "message_buffer.h"

MessageBufferHandle_t xMessageBuffer;

void vSenderTask(void *pvParameters) {
  const char *pcMessage = "Hello";
  for (;;) {
    xMessageBufferSend(xMessageBuffer, pcMessage, strlen(pcMessage), portMAX_DELAY);
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

void vReceiverTask(void *pvParameters) {
  char cBuffer[20];
  for (;;) {
    xMessageBufferReceive(xMessageBuffer, cBuffer, sizeof(cBuffer), portMAX_DELAY);
  }
}

int main(void) {
  HAL_Init();
  SystemClock_Config();
  
  xMessageBuffer = xMessageBufferCreate(100);
  configASSERT(xMessageBuffer != NULL);
  
  xTaskCreate(vSenderTask, "Sender", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
  xTaskCreate(vReceiverTask, "Receiver", configMINIMAL_STACK_SIZE, NULL, 2, NULL);
  
  vTaskStartScheduler();
  
  for (;;);
}
