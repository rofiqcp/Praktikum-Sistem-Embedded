/**
 * ============================================================================
 * STM32_04_ESP01_HTTP_GET
 * ============================================================================
 * HTTP GET Request via ESP-01 AT Commands
 *
 * Hardware:
 *   - Blue Pill STM32F103C8T6
 *   - ESP-01 (ESP8266) module
 *   - LED: PC13 (active low)
 *   - UART1: PA9(TX)/PA10(RX) - Debug (115200 baud)
 *   - UART2: PA2(TX)/PA3(RX)  - ESP-01 (115200 baud)
 *
 * Sequence:
 *   1. Connect to WiFi
 *   2. Open TCP to web server port 80
 *   3. Send raw HTTP GET request
 *   4. Receive and parse HTTP response (status, headers, body)
 *   5. Close connection
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
/* ---- Config ---- */
#define WIFI_SSID       "YourSSID"
#define WIFI_PASSWORD   "YourPassword"
#define HTTP_HOST       "httpbin.org"
#define HTTP_PORT       "80"
#define HTTP_PATH       "/get"
#define MAX_RETRIES     5
/* ---- Buffers ---- */
#define ESP_RX_BUF_SIZE   2048
#define AT_TIMEOUT_MS     5000
#define WIFI_TIMEOUT_MS   15000
#define TCP_TIMEOUT_MS    10000
#define HTTP_TIMEOUT_MS   15000
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
static void HTTP_ParseResponse(const char *response);
static void HTTPGetTask(void *pvParameters);
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
    ESP_SendAndCheck("AT+CWMODE=1\r\n", "OK", AT_TIMEOUT_MS);
    vTaskDelay(pdMS_TO_TICKS(500));
    snprintf(cmd, sizeof(cmd), "AT+CWJAP=\"%s\",\"%s\"\r\n", WIFI_SSID, WIFI_PASSWORD);
    for (attempt = 0; attempt < MAX_RETRIES; attempt++) {
        UART_Printf("[WIFI] Attempt %d/%d...\r\n", attempt + 1, MAX_RETRIES);
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
/* ---- Parse HTTP response ---- */
static void HTTP_ParseResponse(const char *response)
{
    UART_Printf("\r\n--- HTTP Response Parsing ---\r\n");
    /* Find +IPD header from ESP-01 */
    const char *ipd = strstr(response, "+IPD,");
    if (ipd) {
        int data_len = 0;
        sscanf(ipd, "+IPD,%d:", &data_len);
        UART_Printf("[HTTP] Data length: %d bytes\r\n", data_len);
    }
    /* Find HTTP status line */
    const char *http = strstr(response, "HTTP/");
    if (http) {
        /* Extract status code */
        int status_code = 0;
        const char *sp = strchr(http, ' ');
        if (sp) {
            sscanf(sp + 1, "%d", &status_code);
        }
        UART_Printf("[HTTP] Status Code: %d\r\n", status_code);
        /* Print status line */
        const char *eol = strstr(http, "\r\n");
        if (eol) {
            int line_len = (int)(eol - http);
            UART_Printf("[HTTP] Status: %.*s\r\n", line_len, http);
        }
    } else {
        UART_Printf("[HTTP] No HTTP status line found\r\n");
    }
    /* Find Content-Type header */
    const char *ct = strstr(response, "Content-Type:");
    if (ct) {
        const char *eol = strstr(ct, "\r\n");
        if (eol) {
            int len = (int)(eol - ct);
            UART_Printf("[HTTP] %.*s\r\n", len, ct);
        }
    }
    /* Find body (after \r\n\r\n) */
    const char *body = strstr(response, "\r\n\r\n");
    if (body) {
        body += 4;
        int body_len = strlen(body);
        UART_Printf("[HTTP] Body (%d chars):\r\n", body_len);
        /* Print first 256 chars of body */
        int print_len = body_len > 256 ? 256 : body_len;
        UART_Printf("%.*s\r\n", print_len, body);
        if (body_len > 256) {
            UART_Printf("... (truncated)\r\n");
        }
    }
    UART_Printf("--- End HTTP Parse ---\r\n");
}
/* ---- HTTP GET Task ---- */
static void HTTPGetTask(void *pvParameters)
{
    (void)pvParameters;
    uint32_t request_count = 0;
    UART_Printf("\r\n========================================\r\n");
    UART_Printf("  STM32 + ESP-01 HTTP GET Client\r\n");
    UART_Printf("  Host: %s%s\r\n", HTTP_HOST, HTTP_PATH);
    UART_Printf("========================================\r\n\n");
    vTaskDelay(pdMS_TO_TICKS(2000));
    ESP_ClearBuffer();
    /* Test AT */
    ESP_SendAndCheck("AT\r\n", "OK", AT_TIMEOUT_MS);
    vTaskDelay(pdMS_TO_TICKS(500));
    /* Connect WiFi */
    if (!WiFi_Connect()) {
        UART_Printf("[FATAL] WiFi failed\r\n");
        for (;;) { vTaskDelay(pdMS_TO_TICKS(5000)); }
    }
    /* Set single connection mode */
    ESP_SendAndCheck("AT+CIPMUX=0\r\n", "OK", AT_TIMEOUT_MS);
    vTaskDelay(pdMS_TO_TICKS(500));
    /* Main HTTP GET loop */
    for (;;) {
        request_count++;
        UART_Printf("\r\n=== HTTP GET Request #%lu ===\r\n", request_count);
        HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_RESET);
        /* Build HTTP request */
        char http_req[256];
        int req_len = snprintf(http_req, sizeof(http_req),
            "GET %s HTTP/1.1\r\n"
            "Host: %s\r\n"
            "Connection: close\r\n"
            "User-Agent: STM32-ESP01/1.0\r\n"
            "\r\n",
            HTTP_PATH, HTTP_HOST);
        /* Open TCP to HTTP server */
        char cipstart[128];
        snprintf(cipstart, sizeof(cipstart),
                 "AT+CIPSTART=\"TCP\",\"%s\",%s\r\n", HTTP_HOST, HTTP_PORT);
        if (ESP_SendAndCheck(cipstart, "OK", TCP_TIMEOUT_MS) != 1) {
            UART_Printf("[HTTP] TCP connect failed\r\n");
            HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET);
            vTaskDelay(pdMS_TO_TICKS(10000));
            continue;
        }
        vTaskDelay(pdMS_TO_TICKS(500));
        /* Send CIPSEND with length */
        char cipsend[32];
        snprintf(cipsend, sizeof(cipsend), "AT+CIPSEND=%d\r\n", req_len);
        ESP_SendCommand(cipsend);
        if (ESP_WaitResponse(">", AT_TIMEOUT_MS) != 1) {
            UART_Printf("[HTTP] No '>' prompt\r\n");
            ESP_SendAndCheck("AT+CIPCLOSE\r\n", "OK", AT_TIMEOUT_MS);
            HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET);
            vTaskDelay(pdMS_TO_TICKS(10000));
            continue;
        }
        /* Send HTTP request */
        UART_Printf("[HTTP] Sending request...\r\n");
        UART_Printf("[TX] %s", http_req);
        ESP_ClearBuffer();
        ESP_SendRaw(http_req, (uint16_t)req_len);
        /* Wait for SEND OK */
        if (ESP_WaitResponse("SEND OK", AT_TIMEOUT_MS) != 1) {
            UART_Printf("[HTTP] SEND failed\r\n");
        }
        /* Wait for HTTP response (+IPD data) */
        UART_Printf("[HTTP] Waiting for response...\r\n");
        vTaskDelay(pdMS_TO_TICKS(5000));
        /* Parse response */
        if (esp_rx_idx > 0) {
            HTTP_ParseResponse(esp_rx_buf);
        } else {
            UART_Printf("[HTTP] No response received\r\n");
        }
        /* Close connection */
        ESP_SendAndCheck("AT+CIPCLOSE\r\n", "OK", AT_TIMEOUT_MS);
        HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET);
        UBaseType_t hwm = uxTaskGetStackHighWaterMark(NULL);
        UART_Printf("[SYS] Stack HWM: %u | Next request in 30s\r\n", (unsigned int)hwm);
        vTaskDelay(pdMS_TO_TICKS(30000));
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
    xTaskCreate(HTTPGetTask, "HTTP", 512, NULL, MAIN_TASK_PRIORITY, NULL);
    vTaskStartScheduler();
    for (;;) {}
}
