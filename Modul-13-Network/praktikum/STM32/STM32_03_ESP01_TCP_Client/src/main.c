/**
 * ============================================================================
 * STM32_03_ESP01_TCP_Client
 * ============================================================================
 * TCP Client Connection via ESP-01 AT Commands
 *
 * Hardware:
 *   - Blue Pill STM32F103C8T6
 *   - ESP-01 (ESP8266) module
 *   - LED: PC13 (active low)
 *   - UART1: PA9(TX)/PA10(RX) - Debug (115200 baud)
 *   - UART2: PA2(TX)/PA3(RX)  - ESP-01 (115200 baud)
 *
 * Sequence:
 *   1. Connect to WiFi (AT+CWJAP)
 *   2. Open TCP connection (AT+CIPSTART)
 *   3. Send data (AT+CIPSEND)
 *   4. Receive response
 *   5. Close connection (AT+CIPCLOSE)
 * ============================================================================
 */

#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

/* ---- Handles ---- */
static UART_HandleTypeDef huart1;
static UART_HandleTypeDef huart2;

/* ---- Network config ---- */
#define WIFI_SSID       "YourSSID"
#define WIFI_PASSWORD   "YourPassword"
#define TCP_SERVER_IP   "192.168.1.100"
#define TCP_SERVER_PORT "8080"
#define MAX_RETRIES     5

/* ---- Buffers ---- */
#define ESP_RX_BUF_SIZE   1024
#define AT_TIMEOUT_MS     5000
#define WIFI_TIMEOUT_MS   15000
#define TCP_TIMEOUT_MS    10000

static char esp_rx_buf[ESP_RX_BUF_SIZE];
static volatile uint16_t esp_rx_idx = 0;
static uint8_t esp_rx_byte;

/* ---- Forward declarations ---- */
static void SystemClock_Config(void);
static void GPIO_Init(void);
static void UART1_Init(void);
static void UART2_Init(void);
static void UART_Printf(const char *fmt, ...);
static void ESP_SendRaw(const char *data, uint16_t len);
static void ESP_SendCommand(const char *cmd);
static int  ESP_WaitResponse(const char *expect, uint32_t timeout_ms);
static void ESP_ClearBuffer(void);
static int  ESP_SendAndCheck(const char *cmd, const char *expect, uint32_t timeout);
static int  WiFi_Connect(void);
static int  TCP_Connect(const char *ip, const char *port);
static int  TCP_Send(const char *data, uint16_t len);
static void TCP_Close(void);
static void TCPClientTask(void *pvParameters);

/* ---- UART printf ---- */
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

/* ---- System clock: HSE 8MHz -> PLL -> 72MHz ---- */
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

/* ---- GPIO ---- */
static void GPIO_Init(void)
{
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin   = LED_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_PORT, &GPIO_InitStruct);
    HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET);
}

/* ---- UART1: Debug ---- */
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

/* ---- UART2: ESP-01 ---- */
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

/* ---- UART2 RX interrupt ---- */
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

static void ESP_SendRaw(const char *data, uint16_t len)
{
    HAL_UART_Transmit(&huart2, (uint8_t *)data, len, 2000);
}

static int ESP_WaitResponse(const char *expect, uint32_t timeout_ms)
{
    uint32_t start = HAL_GetTick();
    while ((HAL_GetTick() - start) < timeout_ms) {
        if (strstr(esp_rx_buf, expect) != NULL) return 1;
        if (strstr(esp_rx_buf, "FAIL") != NULL) return -2;
        if (strstr(esp_rx_buf, "ERROR") != NULL) return -1;
        if (strstr(esp_rx_buf, "CLOSED") != NULL) return -3;
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    return 0;
}

static int ESP_SendAndCheck(const char *cmd, const char *expect, uint32_t timeout)
{
    UART_Printf("[TX] %s", cmd);
    ESP_SendCommand(cmd);
    int ret = ESP_WaitResponse(expect, timeout);
    if (ret == 1)       UART_Printf("[OK] Response OK\r\n");
    else if (ret < 0)   UART_Printf("[ERR] Error (code %d)\r\n", ret);
    else                UART_Printf("[TMO] Timeout\r\n");
    return ret;
}

/* ---- WiFi connect ---- */
static int WiFi_Connect(void)
{
    char cmd[128];
    int attempt;

    UART_Printf("[WIFI] Setting station mode...\r\n");
    ESP_SendAndCheck("AT+CWMODE=1\r\n", "OK", AT_TIMEOUT_MS);
    vTaskDelay(pdMS_TO_TICKS(500));

    snprintf(cmd, sizeof(cmd), "AT+CWJAP=\"%s\",\"%s\"\r\n", WIFI_SSID, WIFI_PASSWORD);

    for (attempt = 0; attempt < MAX_RETRIES; attempt++) {
        UART_Printf("[WIFI] Connect attempt %d/%d...\r\n", attempt + 1, MAX_RETRIES);
        if (ESP_SendAndCheck(cmd, "OK", WIFI_TIMEOUT_MS) == 1) {
            UART_Printf("[WIFI] Connected!\r\n");
            vTaskDelay(pdMS_TO_TICKS(500));
            ESP_SendAndCheck("AT+CIFSR\r\n", "OK", AT_TIMEOUT_MS);
            return 1;
        }
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
    return 0;
}

/* ---- TCP connect ---- */
static int TCP_Connect(const char *ip, const char *port)
{
    char cmd[128];
    snprintf(cmd, sizeof(cmd), "AT+CIPSTART=\"TCP\",\"%s\",%s\r\n", ip, port);

    UART_Printf("[TCP] Connecting to %s:%s...\r\n", ip, port);
    if (ESP_SendAndCheck(cmd, "OK", TCP_TIMEOUT_MS) == 1) {
        UART_Printf("[TCP] Connected!\r\n");
        return 1;
    }
    /* Also accept ALREADY CONNECTED */
    if (strstr(esp_rx_buf, "ALREADY") != NULL) {
        UART_Printf("[TCP] Already connected\r\n");
        return 1;
    }
    return 0;
}

/* ---- TCP send ---- */
static int TCP_Send(const char *data, uint16_t len)
{
    char cmd[32];
    snprintf(cmd, sizeof(cmd), "AT+CIPSEND=%u\r\n", len);

    UART_Printf("[TCP] Sending %u bytes...\r\n", len);
    ESP_SendCommand(cmd);

    /* Wait for '>' prompt */
    int ret = ESP_WaitResponse(">", AT_TIMEOUT_MS);
    if (ret != 1) {
        UART_Printf("[TCP] No '>' prompt\r\n");
        return 0;
    }

    /* Send actual data */
    ESP_ClearBuffer();
    ESP_SendRaw(data, len);

    /* Wait for SEND OK */
    ret = ESP_WaitResponse("SEND OK", TCP_TIMEOUT_MS);
    if (ret == 1) {
        UART_Printf("[TCP] Data sent OK\r\n");
        return 1;
    }
    UART_Printf("[TCP] Send failed\r\n");
    return 0;
}

/* ---- TCP close ---- */
static void TCP_Close(void)
{
    UART_Printf("[TCP] Closing connection...\r\n");
    ESP_SendAndCheck("AT+CIPCLOSE\r\n", "OK", AT_TIMEOUT_MS);
}

/* ---- TCP Client Task ---- */
static void TCPClientTask(void *pvParameters)
{
    (void)pvParameters;
    uint32_t msg_count = 0;

    UART_Printf("\r\n========================================\r\n");
    UART_Printf("  STM32 + ESP-01 TCP Client\r\n");
    UART_Printf("  Server: %s:%s\r\n", TCP_SERVER_IP, TCP_SERVER_PORT);
    UART_Printf("========================================\r\n\n");

    vTaskDelay(pdMS_TO_TICKS(2000));
    ESP_ClearBuffer();

    /* Test AT */
    ESP_SendAndCheck("AT\r\n", "OK", AT_TIMEOUT_MS);
    vTaskDelay(pdMS_TO_TICKS(500));

    /* Connect WiFi */
    if (!WiFi_Connect()) {
        UART_Printf("[FATAL] WiFi connection failed\r\n");
        for (;;) { vTaskDelay(pdMS_TO_TICKS(5000)); }
    }

    /* Main TCP loop */
    for (;;) {
        msg_count++;
        char payload[64];
        int plen = snprintf(payload, sizeof(payload),
                            "Hello from STM32! Msg#%lu\r\n", msg_count);

        HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_RESET);

        /* Connect TCP */
        if (TCP_Connect(TCP_SERVER_IP, TCP_SERVER_PORT)) {
            /* Send data */
            TCP_Send(payload, (uint16_t)plen);

            /* Wait for response */
            UART_Printf("[TCP] Waiting for response...\r\n");
            vTaskDelay(pdMS_TO_TICKS(2000));
            if (esp_rx_idx > 0) {
                UART_Printf("[TCP] Response:\r\n%s\r\n", esp_rx_buf);
            }

            TCP_Close();
        } else {
            UART_Printf("[TCP] Connection failed\r\n");
        }

        HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET);

        UBaseType_t hwm = uxTaskGetStackHighWaterMark(NULL);
        UART_Printf("[SYS] Stack HWM: %u | Next send in 10s\r\n", (unsigned int)hwm);
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}

/* ---- FreeRTOS hooks ---- */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    UART_Printf("\r\n[FATAL] Stack overflow: %s\r\n", pcTaskName);
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

/* ---- Main ---- */
int main(void)
{
    HAL_Init();
    SystemClock_Config();
    GPIO_Init();
    UART1_Init();
    UART2_Init();

    xTaskCreate(TCPClientTask, "TCP", 512, NULL, MAIN_TASK_PRIORITY, NULL);
    vTaskStartScheduler();
    for (;;) {}
}
