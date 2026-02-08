/**
 * ============================================================================
 * STM32_01_UART_AT_Command
 * ============================================================================
 * AT Command Interface to ESP-01 (ESP8266) via UART2
 *
 * Hardware:
 *   - Blue Pill STM32F103C8T6
 *   - ESP-01 (ESP8266) module
 *   - LED: PC13 (active low)
 *   - UART1: PA9(TX)/PA10(RX) - Debug output (115200 baud)
 *   - UART2: PA2(TX)/PA3(RX)  - ESP-01 communication (115200 baud)
 *
 * ESP-01 Wiring:
 *   VCC  -> 3.3V
 *   GND  -> GND
 *   TX   -> PA3 (STM32 UART2 RX)
 *   RX   -> PA2 (STM32 UART2 TX)
 *   CH_PD-> 3.3V (chip enable)
 *
 * Sends basic AT commands: AT, AT+GMR, AT+RST
 * Receives and parses responses with timeout handling
 * ============================================================================
 */

#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

/* ---- Handle declarations ---- */
static UART_HandleTypeDef huart1;  /* Debug console */
static UART_HandleTypeDef huart2;  /* ESP-01 */

/* ---- Buffer definitions ---- */
#define ESP_RX_BUF_SIZE   512
#define ESP_TX_BUF_SIZE   128
#define AT_TIMEOUT_MS     5000
#define AT_RST_TIMEOUT_MS 8000

static char esp_rx_buf[ESP_RX_BUF_SIZE];
static volatile uint16_t esp_rx_idx = 0;
static uint8_t esp_rx_byte;

/* ---- AT command list ---- */
typedef struct {
    const char *command;
    const char *description;
    uint32_t timeout_ms;
} AT_Command_t;

static const AT_Command_t at_commands[] = {
    {"AT\r\n",       "Test AT connection",       AT_TIMEOUT_MS},
    {"AT+GMR\r\n",   "Get firmware version",     AT_TIMEOUT_MS},
    {"AT+CWMODE?\r\n","Query WiFi mode",         AT_TIMEOUT_MS},
    {"AT+RST\r\n",   "Reset ESP-01 module",      AT_RST_TIMEOUT_MS},
};
#define NUM_AT_COMMANDS  (sizeof(at_commands)/sizeof(at_commands[0]))

/* ---- Forward declarations ---- */
static void SystemClock_Config(void);
static void GPIO_Init(void);
static void UART1_Init(void);
static void UART2_Init(void);
static void UART_Printf(const char *fmt, ...);
static void ESP_SendCommand(const char *cmd);
static int  ESP_WaitResponse(const char *expect, uint32_t timeout_ms);
static void ESP_ClearBuffer(void);
static void AT_CommandTask(void *pvParameters);

/* ---- UART printf helper (UART1 debug) ---- */
static void UART_Printf(const char *fmt, ...)
{
    char buf[256];
    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    if (len > 0) {
        HAL_UART_Transmit(&huart1, (uint8_t *)buf, (uint16_t)len, HAL_MAX_DELAY);
    }
}

/* ---- Clock: HSE 8MHz -> PLL -> 72MHz ---- */
static void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState       = RCC_HSE_ON;
    RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    RCC_OscInitStruct.PLL.PLLState   = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource  = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL     = RCC_PLL_MUL9;
    HAL_RCC_OscConfig(&RCC_OscInitStruct);

    RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                       RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2);
}

/* ---- GPIO init: PC13 LED ---- */
static void GPIO_Init(void)
{
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin   = LED_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_PORT, &GPIO_InitStruct);
    HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET); /* LED off */
}

/* ---- UART1: Debug (PA9 TX, PA10 RX) ---- */
static void UART1_Init(void)
{
    __HAL_RCC_USART1_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin   = GPIO_PIN_9;
    GPIO_InitStruct.Mode  = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin   = GPIO_PIN_10;
    GPIO_InitStruct.Mode  = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    huart1.Instance          = USART1;
    huart1.Init.BaudRate     = UART_BAUDRATE;
    huart1.Init.WordLength   = UART_WORDLENGTH_8B;
    huart1.Init.StopBits     = UART_STOPBITS_1;
    huart1.Init.Parity       = UART_PARITY_NONE;
    huart1.Init.Mode         = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(&huart1);
}

/* ---- UART2: ESP-01 (PA2 TX, PA3 RX) ---- */
static void UART2_Init(void)
{
    __HAL_RCC_USART2_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin   = GPIO_PIN_2;
    GPIO_InitStruct.Mode  = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin   = GPIO_PIN_3;
    GPIO_InitStruct.Mode  = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull  = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    huart2.Instance          = USART2;
    huart2.Init.BaudRate     = UART_BAUDRATE;
    huart2.Init.WordLength   = UART_WORDLENGTH_8B;
    huart2.Init.StopBits     = UART_STOPBITS_1;
    huart2.Init.Parity       = UART_PARITY_NONE;
    huart2.Init.Mode         = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(&huart2);

    HAL_NVIC_SetPriority(USART2_IRQn, 6, 0);
    HAL_NVIC_EnableIRQ(USART2_IRQn);
    HAL_UART_Receive_IT(&huart2, &esp_rx_byte, 1);
}

/* ---- UART2 RX interrupt callback ---- */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2) {
        if (esp_rx_idx < ESP_RX_BUF_SIZE - 1) {
            esp_rx_buf[esp_rx_idx++] = (char)esp_rx_byte;
            esp_rx_buf[esp_rx_idx] = '\0';
        }
        HAL_UART_Receive_IT(&huart2, &esp_rx_byte, 1);
    }
}

void USART2_IRQHandler(void)
{
    HAL_UART_IRQHandler(&huart2);
}

/* ---- ESP helper functions ---- */
static void ESP_ClearBuffer(void)
{
    memset(esp_rx_buf, 0, ESP_RX_BUF_SIZE);
    esp_rx_idx = 0;
}

static void ESP_SendCommand(const char *cmd)
{
    ESP_ClearBuffer();
    HAL_UART_Transmit(&huart2, (uint8_t *)cmd, strlen(cmd), 1000);
}

static int ESP_WaitResponse(const char *expect, uint32_t timeout_ms)
{
    uint32_t start = HAL_GetTick();
    while ((HAL_GetTick() - start) < timeout_ms) {
        if (strstr(esp_rx_buf, expect) != NULL) {
            return 1; /* Found */
        }
        if (strstr(esp_rx_buf, "ERROR") != NULL) {
            return -1; /* Error */
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    return 0; /* Timeout */
}

/* ---- Main AT command task ---- */
static void AT_CommandTask(void *pvParameters)
{
    (void)pvParameters;
    int result;
    uint32_t cycle = 0;

    UART_Printf("\r\n========================================\r\n");
    UART_Printf("  STM32 + ESP-01 AT Command Interface\r\n");
    UART_Printf("  UART1: Debug | UART2: ESP-01\r\n");
    UART_Printf("========================================\r\n\n");

    /* Initial delay for ESP-01 boot */
    UART_Printf("[INFO] Waiting for ESP-01 to boot...\r\n");
    vTaskDelay(pdMS_TO_TICKS(2000));
    ESP_ClearBuffer();

    for (;;) {
        cycle++;
        UART_Printf("\r\n--- AT Command Cycle #%lu ---\r\n", cycle);

        for (uint32_t i = 0; i < NUM_AT_COMMANDS; i++) {
            UART_Printf("\r\n[CMD] %s", at_commands[i].description);
            UART_Printf("\r\n[TX ] %s", at_commands[i].command);

            HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_RESET); /* LED on */
            ESP_SendCommand(at_commands[i].command);

            const char *expect = (i == 3) ? "ready" : "OK";
            result = ESP_WaitResponse(expect, at_commands[i].timeout_ms);

            HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET); /* LED off */

            if (result == 1) {
                UART_Printf("\r\n[OK ] Response received:\r\n%s\r\n", esp_rx_buf);
            } else if (result == -1) {
                UART_Printf("\r\n[ERR] ESP-01 returned ERROR:\r\n%s\r\n", esp_rx_buf);
            } else {
                UART_Printf("\r\n[TMO] Timeout after %lu ms\r\n", at_commands[i].timeout_ms);
                if (esp_rx_idx > 0) {
                    UART_Printf("[RX ] Partial: %s\r\n", esp_rx_buf);
                }
            }
            vTaskDelay(pdMS_TO_TICKS(500));
        }

        /* Stack high water mark */
        UBaseType_t hwm = uxTaskGetStackHighWaterMark(NULL);
        UART_Printf("\r\n[SYS] Stack HWM: %u words\r\n", (unsigned int)hwm);
        UART_Printf("[SYS] Next cycle in 10 seconds...\r\n");
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}

/* ---- FreeRTOS hooks ---- */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    UART_Printf("\r\n[FATAL] Stack overflow in task: %s\r\n", pcTaskName);
    for (;;) {}
}

void vApplicationMallocFailedHook(void)
{
    UART_Printf("\r\n[FATAL] Malloc failed!\r\n");
    for (;;) {}
}

static StaticTask_t xIdleTaskTCB;
static StackType_t  uxIdleTaskStack[configMINIMAL_STACK_SIZE];
void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer,
                                   StackType_t **ppxIdleTaskStackBuffer,
                                   uint32_t *pulIdleTaskStackSize)
{
    *ppxIdleTaskTCBBuffer   = &xIdleTaskTCB;
    *ppxIdleTaskStackBuffer = uxIdleTaskStack;
    *pulIdleTaskStackSize   = configMINIMAL_STACK_SIZE;
}

static StaticTask_t xTimerTaskTCB;
static StackType_t  uxTimerTaskStack[configTIMER_TASK_STACK_DEPTH];
void vApplicationGetTimerTaskMemory(StaticTask_t **ppxTimerTaskTCBBuffer,
                                    StackType_t **ppxTimerTaskStackBuffer,
                                    uint32_t *pulTimerTaskStackSize)
{
    *ppxTimerTaskTCBBuffer   = &xTimerTaskTCB;
    *ppxTimerTaskStackBuffer = uxTimerTaskStack;
    *pulTimerTaskStackSize   = configTIMER_TASK_STACK_DEPTH;
}

/* ---- Main entry ---- */
int main(void)
{
    HAL_Init();
    SystemClock_Config();
    GPIO_Init();
    UART1_Init();
    UART2_Init();

    xTaskCreate(AT_CommandTask, "ATCmd", MAIN_TASK_STACK_SIZE, NULL,
                MAIN_TASK_PRIORITY, NULL);

    vTaskStartScheduler();
    for (;;) {}
}
