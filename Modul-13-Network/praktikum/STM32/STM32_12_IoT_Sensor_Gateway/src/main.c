/* ==========================================================================
 * Program     : STM32_12_IoT_Sensor_Gateway
 * Description : Full IoT gateway: STM32 reads sensors -> sends via ESP-01
 * Board       : Blue Pill (STM32F103C8)
 * Framework   : STM32Cube HAL + FreeRTOS
 *
 * Wiring:
 *   UART1 (Debug):  PA9=TX, PA10=RX -> USB-Serial
 *   UART2 (ESP-01): PA2=TX -> ESP RX, PA3=RX -> ESP TX
 *   ADC:            PA0 (ADC1 CH0) - external sensor (pot)
 *   ADC:            Internal temp sensor (ADC1 CH16)
 *   LED Activity:   PC13 (active low)
 *   LED Error:      PB12 (active high)
 * ========================================================================== */
#include "config.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
/* ----- Configuration ----- */
#define WIFI_SSID           "YourSSID"
#define WIFI_PASS           "YourPassword"
#define SERVER_HOST         "192.168.1.100"
#define SERVER_PORT         8080
#define GATEWAY_ID          "gw_stm32_01"
#define SENSOR_READ_INTERVAL  5000
#define UPLOAD_INTERVAL       10000
#define ESP_RX_BUF_SIZE       256
#define SENSOR_QUEUE_LEN      8
/* ----- Sensor Data Structure ----- */
typedef struct {
    float internal_temp;
    float external_adc;
    float ext_voltage;
    uint32_t timestamp;
    uint32_t seq;
} SensorData_t;
typedef enum {
    GW_STATE_INIT,
    GW_STATE_WIFI_CONNECTING,
    GW_STATE_WIFI_CONNECTED,
    GW_STATE_RUNNING,
    GW_STATE_ERROR
} GwState_t;
/* ----- Peripheral Handles ----- */
static UART_HandleTypeDef huart1;
static UART_HandleTypeDef huart2;
static ADC_HandleTypeDef hadc1;
/* ----- FreeRTOS Objects ----- */
static QueueHandle_t xSensorQueue;
static SemaphoreHandle_t xEspMutex;
static SemaphoreHandle_t xAdcMutex;
static TaskHandle_t xSensorTaskHandle;
static TaskHandle_t xUploadTaskHandle;
/* ----- State ----- */
static volatile GwState_t gw_state = GW_STATE_INIT;
static char esp_rx_buf[ESP_RX_BUF_SIZE];
static volatile uint16_t esp_rx_idx = 0;
static volatile uint32_t readings_taken = 0;
static volatile uint32_t uploads_done = 0;
static volatile uint32_t upload_errors = 0;
/* ----- Function Prototypes ----- */
static void SystemClock_Config(void);
static void GPIO_Init(void);
static void UART1_Init(void);
static void UART2_Init(void);
static void ADC1_Init(void);
static void UART_Printf(const char *fmt, ...);
static uint16_t ADC_Read_Channel(uint32_t channel);
static float Read_Internal_Temp(void);
static float Read_External_ADC(void);
static int ESP_SendCmd(const char *cmd, const char *expect, uint32_t timeout);
static int WiFi_Init(void);
static int HTTP_Post(const char *json_data);
static void vSensorTask(void *pvParameters);
static void vUploadTask(void *pvParameters);
static void vMonitorTask(void *pvParameters);
/* ----- UART Printf Helper ----- */
static void UART_Printf(const char *fmt, ...) {
    char buf[192];
    va_list args;
    va_start(args, fmt);
    int n = vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    if (n > 0) {
        HAL_UART_Transmit(&huart1, (uint8_t *)buf, (uint16_t)n, HAL_MAX_DELAY);
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
/* ----- GPIO Init ----- */
static void GPIO_Init(void) {
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    GPIO_InitTypeDef gpio = {0};
    /* PC13 - Activity LED (active low) */
    gpio.Pin = GPIO_PIN_13;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOC, &gpio);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
    /* PB12 - Error LED (active high) */
    gpio.Pin = GPIO_PIN_12;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &gpio);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET);
    /* PA0 - ADC input (analog) */
    gpio.Pin = GPIO_PIN_0;
    gpio.Mode = GPIO_MODE_ANALOG;
    HAL_GPIO_Init(GPIOA, &gpio);
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
}
/* ----- ADC1 Init ----- */
static void ADC1_Init(void) {
    __HAL_RCC_ADC1_CLK_ENABLE();
    hadc1.Instance = ADC1;
#ifdef STM32F103xB
    hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
    hadc1.Init.ContinuousConvMode = DISABLE;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc1.Init.NbrOfConversion = 1;
    HAL_ADC_Init(&hadc1);
    ADC1->CR2 |= ADC_CR2_TSVREFE;
#elif defined(STM32F401xC) || defined(STM32F411xE)
    hadc1.Init.ClockPrescaler        = ADC_CLOCK_SYNC_PCLK_DIV4;
    hadc1.Init.Resolution            = ADC_RESOLUTION_12B;
    hadc1.Init.ScanConvMode          = DISABLE;
    hadc1.Init.ContinuousConvMode    = DISABLE;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConvEdge  = ADC_EXTERNALTRIGCONVEDGE_NONE;
    hadc1.Init.ExternalTrigConv      = ADC_SOFTWARE_START;
    hadc1.Init.DataAlign             = ADC_DATAALIGN_RIGHT;
    hadc1.Init.NbrOfConversion       = 1;
    hadc1.Init.DMAContinuousRequests = DISABLE;
    hadc1.Init.EOCSelection          = ADC_EOC_SINGLE_CONV;
    HAL_ADC_Init(&hadc1);
    /* Enable temperature sensor & Vrefint on F4 via common CCR */
    ADC->CCR |= ADC_CCR_TSVREFE;
#endif
}
/* ----- Read ADC channel ----- */
static uint16_t ADC_Read_Channel(uint32_t channel) {
    ADC_ChannelConfTypeDef cfg = {0};
    cfg.Channel = channel;
#ifdef STM32F103xB
    cfg.Rank = ADC_REGULAR_RANK_1;
    cfg.SamplingTime = ADC_SAMPLETIME_239CYCLES_5;
#elif defined(STM32F401xC) || defined(STM32F411xE)
    cfg.Rank = 1;
    cfg.SamplingTime = ADC_SAMPLETIME_480CYCLES;
#endif
    HAL_ADC_ConfigChannel(&hadc1, &cfg);
    HAL_ADC_Start(&hadc1);
    HAL_ADC_PollForConversion(&hadc1, 100);
    uint16_t val = (uint16_t)HAL_ADC_GetValue(&hadc1);
    HAL_ADC_Stop(&hadc1);
    return val;
}
/* ----- Read internal temperature sensor ----- */
static float Read_Internal_Temp(void) {
    xSemaphoreTake(xAdcMutex, portMAX_DELAY);
#ifdef STM32F103xB
    uint16_t raw = ADC_Read_Channel(ADC_CHANNEL_16);
#elif defined(STM32F401xC) || defined(STM32F411xE)
    uint16_t raw = ADC_Read_Channel(ADC_CHANNEL_TEMPSENSOR);
#endif
    xSemaphoreGive(xAdcMutex);
    /* V_sense = raw * 3.3 / 4096 */
    /* Temp = ((V25 - V_sense) / Avg_Slope) + 25 */
    /* V25 = 1.43V, Avg_Slope = 4.3mV/C */
    float voltage = (float)raw * 3.3f / 4096.0f;
    float temp = ((1.43f - voltage) / 0.0043f) + 25.0f;
    return temp;
}
/* ----- Read external ADC (PA0) ----- */
static float Read_External_ADC(void) {
    xSemaphoreTake(xAdcMutex, portMAX_DELAY);
    uint16_t raw = ADC_Read_Channel(ADC_CHANNEL_0);
    xSemaphoreGive(xAdcMutex);
    float voltage = (float)raw * 3.3f / 4096.0f;
    return voltage;
}
/* ----- ESP-01 AT Command ----- */
static int ESP_SendCmd(const char *cmd, const char *expect, uint32_t timeout) {
    xSemaphoreTake(xEspMutex, portMAX_DELAY);
    UART_Printf("[ESP] >> %s\r\n", cmd);
    char full_cmd[128];
    snprintf(full_cmd, sizeof(full_cmd), "%s\r\n", cmd);
    HAL_UART_Transmit(&huart2, (uint8_t *)full_cmd, strlen(full_cmd), 300);
    memset(esp_rx_buf, 0, sizeof(esp_rx_buf));
    esp_rx_idx = 0;
    uint32_t start = HAL_GetTick();
    uint8_t byte;
    while ((HAL_GetTick() - start) < timeout) {
        if (HAL_UART_Receive(&huart2, &byte, 1, 50) == HAL_OK) {
            if (esp_rx_idx < ESP_RX_BUF_SIZE - 1) {
                esp_rx_buf[esp_rx_idx++] = (char)byte;
                esp_rx_buf[esp_rx_idx] = '\0';
            }
            if (expect && strstr(esp_rx_buf, expect)) {
                UART_Printf("[ESP] << OK\r\n");
                xSemaphoreGive(xEspMutex);
                return 1;
            }
            if (strstr(esp_rx_buf, "ERROR") || strstr(esp_rx_buf, "FAIL")) {
                UART_Printf("[ESP] << ERROR\r\n");
                xSemaphoreGive(xEspMutex);
                return 0;
            }
        }
    }
    UART_Printf("[ESP] << TIMEOUT\r\n");
    xSemaphoreGive(xEspMutex);
    return 0;
}
/* ----- WiFi Initialization ----- */
static int WiFi_Init(void) {
    gw_state = GW_STATE_WIFI_CONNECTING;
    UART_Printf("[GW] WiFi connecting to %s...\r\n", WIFI_SSID);
    ESP_SendCmd("AT+RST", "ready", 3000);
    vTaskDelay(pdMS_TO_TICKS(1000));
    if (!ESP_SendCmd("AT", "OK", 1000)) return 0;
    if (!ESP_SendCmd("AT+CWMODE=1", "OK", 1000)) return 0;
    char join[128];
    snprintf(join, sizeof(join), "AT+CWJAP=\"%s\",\"%s\"", WIFI_SSID, WIFI_PASS);
    if (!ESP_SendCmd(join, "OK", 15000)) return 0;
    gw_state = GW_STATE_WIFI_CONNECTED;
    UART_Printf("[GW] WiFi connected!\r\n");
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET);  /* Error LED off */
    return 1;
}
/* ----- HTTP POST via ESP-01 ----- */
static int HTTP_Post(const char *json_data) {
    char cmd[128];
    /* Open TCP connection */
    snprintf(cmd, sizeof(cmd), "AT+CIPSTART=\"TCP\",\"%s\",%d", SERVER_HOST, SERVER_PORT);
    if (!ESP_SendCmd(cmd, "OK", 5000)) return 0;
    vTaskDelay(pdMS_TO_TICKS(200));
    /* Build HTTP POST request */
    char http_req[384];
    int content_len = strlen(json_data);
    int req_len = snprintf(http_req, sizeof(http_req),
        "POST /api/sensor HTTP/1.1\r\n"
        "Host: %s:%d\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: %d\r\n"
        "Connection: close\r\n"
        "\r\n"
        "%s",
        SERVER_HOST, SERVER_PORT, content_len, json_data);
    /* Send data length */
    snprintf(cmd, sizeof(cmd), "AT+CIPSEND=%d", req_len);
    if (!ESP_SendCmd(cmd, ">", 2000)) return 0;
    /* Send HTTP request */
    xSemaphoreTake(xEspMutex, portMAX_DELAY);
    HAL_UART_Transmit(&huart2, (uint8_t *)http_req, (uint16_t)req_len, 2000);
    /* Wait for SEND OK */
    uint32_t start = HAL_GetTick();
    memset(esp_rx_buf, 0, sizeof(esp_rx_buf));
    esp_rx_idx = 0;
    uint8_t byte;
    int result = 0;
    while ((HAL_GetTick() - start) < 5000) {
        if (HAL_UART_Receive(&huart2, &byte, 1, 50) == HAL_OK) {
            if (esp_rx_idx < ESP_RX_BUF_SIZE - 1) {
                esp_rx_buf[esp_rx_idx++] = (char)byte;
            }
            if (strstr(esp_rx_buf, "SEND OK")) {
                result = 1;
                break;
            }
        }
    }
    xSemaphoreGive(xEspMutex);
    vTaskDelay(pdMS_TO_TICKS(500));
    ESP_SendCmd("AT+CIPCLOSE", "OK", 2000);
    return result;
}
/* ----- Sensor Reading Task ----- */
static void vSensorTask(void *pvParameters) {
    (void)pvParameters;
    uint32_t seq = 0;
    UART_Printf("[SENSOR] Task started\r\n");
    for (;;) {
        SensorData_t data;
        data.internal_temp = Read_Internal_Temp();
        data.ext_voltage = Read_External_ADC();
        data.external_adc = data.ext_voltage * 100.0f / 3.3f;  /* Scale to 0-100 */
        data.timestamp = HAL_GetTick();
        data.seq = seq++;
        UART_Printf("[SENSOR] Temp=%.1fC ExtV=%.2fV Ext=%.1f%% seq=%lu\r\n",
                    (double)data.internal_temp, (double)data.ext_voltage,
                    (double)data.external_adc, data.seq);
        /* Queue data for upload */
        if (xQueueSend(xSensorQueue, &data, pdMS_TO_TICKS(100)) != pdTRUE) {
            UART_Printf("[SENSOR] Queue full, dropping\r\n");
        }
        readings_taken++;
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);  /* Activity blink */
        vTaskDelay(pdMS_TO_TICKS(SENSOR_READ_INTERVAL));
    }
}
/* ----- Upload Task: send sensor data via WiFi ----- */
static void vUploadTask(void *pvParameters) {
    (void)pvParameters;
    SensorData_t data;
    UART_Printf("[UPLOAD] Task started\r\n");
    /* Initialize WiFi */
    while (!WiFi_Init()) {
        UART_Printf("[UPLOAD] WiFi retry in 5s...\r\n");
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);  /* Error LED */
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
    gw_state = GW_STATE_RUNNING;
    for (;;) {
        if (xQueueReceive(xSensorQueue, &data, pdMS_TO_TICKS(UPLOAD_INTERVAL)) == pdTRUE) {
            /* Package as JSON */
            char json[192];
            snprintf(json, sizeof(json),
                     "{\"gw\":\"%s\",\"temp\":%.1f,\"ext_v\":%.2f,\"ext_pct\":%.1f,"
                     "\"ts\":%lu,\"seq\":%lu}",
                     GATEWAY_ID,
                     (double)data.internal_temp,
                     (double)data.ext_voltage,
                     (double)data.external_adc,
                     data.timestamp,
                     data.seq);
            UART_Printf("[UPLOAD] Sending: %s\r\n", json);
            if (HTTP_Post(json)) {
                uploads_done++;
                UART_Printf("[UPLOAD] Success (#%lu)\r\n", uploads_done);
                HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET);
            } else {
                upload_errors++;
                UART_Printf("[UPLOAD] FAILED (#%lu errors)\r\n", upload_errors);
                HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);
                /* Re-queue data */
                xQueueSend(xSensorQueue, &data, 0);
                /* Reconnect WiFi if too many errors */
                if (upload_errors % 3 == 0) {
                    UART_Printf("[UPLOAD] Reconnecting WiFi...\r\n");
                    WiFi_Init();
                }
            }
        }
    }
}
/* ----- Monitor Task ----- */
static void vMonitorTask(void *pvParameters) {
    (void)pvParameters;
    const char *states[] = {"INIT", "WIFI_CONN", "WIFI_OK", "RUNNING", "ERROR"};
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(10000));
        UART_Printf("\r\n===== IoT Gateway Status =====\r\n");
        UART_Printf("  ID       : %s\r\n", GATEWAY_ID);
        UART_Printf("  State    : %s\r\n", states[gw_state]);
        UART_Printf("  Readings : %lu\r\n", readings_taken);
        UART_Printf("  Uploads  : %lu OK, %lu ERR\r\n", uploads_done, upload_errors);
        UART_Printf("  Queue    : %u/%u\r\n",
                    (unsigned)uxQueueMessagesWaiting(xSensorQueue), SENSOR_QUEUE_LEN);
        UART_Printf("  Heap     : %u bytes\r\n", (unsigned)xPortGetFreeHeapSize());
        UART_Printf("==============================\r\n\r\n");
    }
}
/* ----- FreeRTOS Hooks ----- */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
    (void)xTask;
    UART_Printf("STACK OVERFLOW: %s\r\n", pcTaskName);
    for (;;) { }
}
void vApplicationMallocFailedHook(void) {
    UART_Printf("MALLOC FAILED\r\n");
    for (;;) { }
}
static StaticTask_t xIdleTaskTCB;
static StackType_t uxIdleTaskStack[configMINIMAL_STACK_SIZE];
void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTCB, StackType_t **ppxIdleStack, uint32_t *pulIdleStackSize) {
    *ppxIdleTCB = &xIdleTaskTCB;
    *ppxIdleStack = uxIdleTaskStack;
    *pulIdleStackSize = configMINIMAL_STACK_SIZE;
}
static StaticTask_t xTimerTaskTCB;
static StackType_t uxTimerTaskStack[configTIMER_TASK_STACK_DEPTH];
void vApplicationGetTimerTaskMemory(StaticTask_t **ppxTimerTCB, StackType_t **ppxTimerStack, uint32_t *pulTimerStackSize) {
    *ppxTimerTCB = &xTimerTaskTCB;
    *ppxTimerStack = uxTimerTaskStack;
    *pulTimerStackSize = configTIMER_TASK_STACK_DEPTH;
}
/* ----- Main ----- */
int main(void) {
    HAL_Init();
    SystemClock_Config();
    GPIO_Init();
    UART1_Init();
    UART2_Init();
    ADC1_Init();
    UART_Printf("\r\n========================================\r\n");
    UART_Printf("  STM32 IoT Sensor Gateway\r\n");
    UART_Printf("  ID: %s\r\n", GATEWAY_ID);
    UART_Printf("  Server: %s:%d\r\n", SERVER_HOST, SERVER_PORT);
    UART_Printf("  Sensors: Internal Temp + PA0 ADC\r\n");
    UART_Printf("========================================\r\n\r\n");
    xSensorQueue = xQueueCreate(SENSOR_QUEUE_LEN, sizeof(SensorData_t));
    xEspMutex = xSemaphoreCreateMutex();
    xAdcMutex = xSemaphoreCreateMutex();
    xTaskCreate(vSensorTask, "Sensor", 320, NULL, 4, &xSensorTaskHandle);
    xTaskCreate(vUploadTask, "Upload", 512, NULL, 3, &xUploadTaskHandle);
    xTaskCreate(vMonitorTask, "Mon", 256, NULL, 2, NULL);
    UART_Printf("[MAIN] Starting scheduler...\r\n");
    vTaskStartScheduler();
    for (;;) { }
}
