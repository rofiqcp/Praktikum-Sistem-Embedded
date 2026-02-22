/**
 * ============================================================================
 * STM32_02_ESP01_WiFi_Connect
 * ============================================================================
 * Connect to WiFi Access Point via ESP-01 AT Commands
 *
 * Hardware:
 *   - Blue Pill STM32F103C8T6
 *   - ESP-01 (ESP8266) module
 *   - LED: PC13 (active low)
 *   - UART1: PA9(TX)/PA10(RX) - Debug output (115200 baud)
 *   - UART2: PA2(TX)/PA3(RX)  - ESP-01 communication (115200 baud)
 *
 * Sequence:
 *   1. AT            -> test connection
 *   2. AT+CWMODE=1   -> station mode
 *   3. AT+CWJAP="SSID","PASSWORD" -> connect to AP
 *   4. AT+CIFSR      -> get assigned IP address
 *   5. Periodic status check with retry logic
 * ============================================================================
 */
#include "config.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
/* ---- Handles ---- */
static UART_HandleTypeDef huart1;
static UART_HandleTypeDef huart2;
/* ---- WiFi credentials (change as needed) ---- */
#define WIFI_SSID       "YourSSID"
#define WIFI_PASSWORD   "YourPassword"
#define MAX_RETRIES     5
/* ---- Buffer definitions ---- */
#define ESP_RX_BUF_SIZE   512
#define AT_TIMEOUT_MS     5000
#define WIFI_TIMEOUT_MS   15000
static char esp_rx_buf[ESP_RX_BUF_SIZE];
static volatile uint16_t esp_rx_idx = 0;
static uint8_t esp_rx_byte;
/* ---- Forward declarations ---- */
static void SystemClock_Config(void);
static void GPIO_Init(void);
static void UART1_Init(void);
static void UART2_Init(void);
static void UART_Printf(const char *fmt, ...);
static void ESP_SendCommand(const char *cmd);
static int  ESP_WaitResponse(const char *expect, uint32_t timeout_ms);
static void ESP_ClearBuffer(void);
static int  ESP_SendAndCheck(const char *cmd, const char *expect, uint32_t timeout);
static void WiFiConnectTask(void *pvParameters);
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
/* ============================================================
 *  System Clock Configuration (Multi-platform)
 * ============================================================ */
#ifdef STM32F103xB
/* F103: 8 MHz HSE -> PLL x9 -> 72 MHz SYSCLK */
static void SystemClock_Config(void)
{
    RCC_OscInitTypeDef osc = {0};
    RCC_ClkInitTypeDef clk = {0};
    osc.OscillatorType      = RCC_OSCILLATORTYPE_HSE;
    osc.HSEState            = RCC_HSE_ON;
    osc.HSEPredivValue      = RCC_HSE_PREDIV_DIV1;
    osc.PLL.PLLState        = RCC_PLL_ON;
    osc.PLL.PLLSource       = RCC_PLLSOURCE_HSE;
    osc.PLL.PLLMUL          = RCC_PLL_MUL9;
    HAL_RCC_OscConfig(&osc);
    clk.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                       | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    clk.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    clk.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    clk.APB1CLKDivider = RCC_HCLK_DIV2;
    clk.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_2);
}
#elif defined(STM32F401xC) || defined(STM32F411xE)
/* F4xx: 25 MHz HSE -> PLL -> 84 MHz SYSCLK */
static void SystemClock_Config(void)
{
    RCC_OscInitTypeDef osc = {0};
    RCC_ClkInitTypeDef clk = {0};
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);
    osc.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    osc.HSEState       = RCC_HSE_ON;
    osc.PLL.PLLState   = RCC_PLL_ON;
    osc.PLL.PLLSource  = RCC_PLLSOURCE_HSE;
    osc.PLL.PLLM       = 25;
    osc.PLL.PLLN       = 336;
    osc.PLL.PLLP       = RCC_PLLP_DIV4;   /* 336/4 = 84 MHz */
    osc.PLL.PLLQ       = 7;
    HAL_RCC_OscConfig(&osc);
    clk.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                       | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    clk.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    clk.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    clk.APB1CLKDivider = RCC_HCLK_DIV2;
    clk.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_2);
}
#endif
/* ---- GPIO: PC13 LED ---- */
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

/* ---- UART1: Debug (PA9 TX, PA10 RX) ---- */
static void UART1_Init(void)
{
    __HAL_RCC_USART1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef g = {0};
#ifdef STM32F103xB
    g.Pin   = GPIO_PIN_9;
    g.Mode  = GPIO_MODE_AF_PP;
    g.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &g);
    g.Pin   = GPIO_PIN_10;
    g.Mode  = GPIO_MODE_INPUT;
    g.Pull  = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &g);
#elif defined(STM32F401xC) || defined(STM32F411xE)
    g.Pin       = GPIO_PIN_9 | GPIO_PIN_10;
    g.Mode      = GPIO_MODE_AF_PP;
    g.Pull      = GPIO_PULLUP;
    g.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
    g.Alternate = GPIO_AF7_USART1;
    HAL_GPIO_Init(GPIOA, &g);
#endif

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

/* ---- UART2: Module comm (PA2 TX, PA3 RX) ---- */
static void UART2_Init(void)
{
    __HAL_RCC_USART2_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    GPIO_InitTypeDef g = {0};
#ifdef STM32F103xB
    g.Pin   = GPIO_PIN_2;
    g.Mode  = GPIO_MODE_AF_PP;
    g.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &g);
    g.Pin   = GPIO_PIN_3;
    g.Mode  = GPIO_MODE_INPUT;
    g.Pull  = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOA, &g);
#elif defined(STM32F401xC) || defined(STM32F411xE)
    g.Pin       = GPIO_PIN_2 | GPIO_PIN_3;
    g.Mode      = GPIO_MODE_AF_PP;
    g.Pull      = GPIO_PULLUP;
    g.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
    g.Alternate = GPIO_AF7_USART2;
    HAL_GPIO_Init(GPIOA, &g);
#endif
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
/* ---- ESP helpers ---- */
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
        if (strstr(esp_rx_buf, expect) != NULL) return 1;
        if (strstr(esp_rx_buf, "FAIL") != NULL) return -2;
        if (strstr(esp_rx_buf, "ERROR") != NULL) return -1;
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    return 0;
}
static int ESP_SendAndCheck(const char *cmd, const char *expect, uint32_t timeout)
{
    UART_Printf("[TX] %s", cmd);
    ESP_SendCommand(cmd);
    int ret = ESP_WaitResponse(expect, timeout);
    if (ret == 1)       UART_Printf("[OK] %s\r\n", expect);
    else if (ret == -1) UART_Printf("[ERR] ERROR response\r\n");
    else if (ret == -2) UART_Printf("[ERR] FAIL response\r\n");
    else                UART_Printf("[TMO] Timeout\r\n");
    if (esp_rx_idx > 0) UART_Printf("[RX] %s\r\n", esp_rx_buf);
    return ret;
}
/* ---- WiFi connection task ---- */
static void WiFiConnectTask(void *pvParameters)
{
    (void)pvParameters;
    char cmd_buf[128];
    int attempt;
    UART_Printf("\r\n========================================\r\n");
    UART_Printf("  STM32 + ESP-01 WiFi Connect\r\n");
    UART_Printf("  SSID: %s\r\n", WIFI_SSID);
    UART_Printf("========================================\r\n\n");
    /* Wait for ESP-01 boot */
    UART_Printf("[INFO] Waiting for ESP-01 boot...\r\n");
    vTaskDelay(pdMS_TO_TICKS(2000));
    ESP_ClearBuffer();
    /* Step 1: Test AT */
    UART_Printf("\r\n[STEP 1] Test AT connection\r\n");
    for (attempt = 0; attempt < MAX_RETRIES; attempt++) {
        if (ESP_SendAndCheck("AT\r\n", "OK", AT_TIMEOUT_MS) == 1) break;
        UART_Printf("[RETRY] Attempt %d/%d\r\n", attempt + 1, MAX_RETRIES);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    /* Step 2: Set station mode */
    UART_Printf("\r\n[STEP 2] Set WiFi Station mode\r\n");
    ESP_SendAndCheck("AT+CWMODE=1\r\n", "OK", AT_TIMEOUT_MS);
    vTaskDelay(pdMS_TO_TICKS(500));
    /* Step 3: Connect to WiFi with retry */
    UART_Printf("\r\n[STEP 3] Connecting to WiFi: %s\r\n", WIFI_SSID);
    snprintf(cmd_buf, sizeof(cmd_buf), "AT+CWJAP=\"%s\",\"%s\"\r\n",
             WIFI_SSID, WIFI_PASSWORD);
    int connected = 0;
    for (attempt = 0; attempt < MAX_RETRIES; attempt++) {
        HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_RESET); /* LED on */
        int ret = ESP_SendAndCheck(cmd_buf, "OK", WIFI_TIMEOUT_MS);
        HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET);
        if (ret == 1) {
            connected = 1;
            UART_Printf("[OK] WiFi connected!\r\n");
            break;
        }
        UART_Printf("[RETRY] WiFi attempt %d/%d failed\r\n", attempt + 1, MAX_RETRIES);
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
    if (!connected) {
        UART_Printf("[FAIL] Could not connect to WiFi after %d attempts\r\n", MAX_RETRIES);
    }
    /* Step 4: Get IP address */
    UART_Printf("\r\n[STEP 4] Query IP address\r\n");
    ESP_SendAndCheck("AT+CIFSR\r\n", "OK", AT_TIMEOUT_MS);
    /* Periodic status monitoring */
    UART_Printf("\r\n[INFO] Entering status monitor loop...\r\n");
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(15000));
        UART_Printf("\r\n--- WiFi Status Check ---\r\n");
        ESP_SendAndCheck("AT+CWJAP?\r\n", "OK", AT_TIMEOUT_MS);
        vTaskDelay(pdMS_TO_TICKS(500));
        ESP_SendAndCheck("AT+CIFSR\r\n", "OK", AT_TIMEOUT_MS);
        /* Check connection lost -> reconnect */
        if (strstr(esp_rx_buf, "No AP") != NULL) {
            UART_Printf("[WARN] WiFi disconnected! Reconnecting...\r\n");
            snprintf(cmd_buf, sizeof(cmd_buf), "AT+CWJAP=\"%s\",\"%s\"\r\n",
                     WIFI_SSID, WIFI_PASSWORD);
            ESP_SendAndCheck(cmd_buf, "OK", WIFI_TIMEOUT_MS);
        }
        UBaseType_t hwm = uxTaskGetStackHighWaterMark(NULL);
        UART_Printf("[SYS] Stack HWM: %u words\r\n", (unsigned int)hwm);
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
    xTaskCreate(WiFiConnectTask, "WiFi", 384, NULL, MAIN_TASK_PRIORITY, NULL);
    vTaskStartScheduler();
    for (;;) {}
}
