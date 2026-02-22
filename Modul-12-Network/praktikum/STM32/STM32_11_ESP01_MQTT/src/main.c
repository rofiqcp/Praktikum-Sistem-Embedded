/* ==========================================================================
 * Program     : STM32_11_ESP01_MQTT
 * Description : MQTT publish/subscribe via ESP-01 AT commands
 * Board       : Blue Pill (STM32F103C8)
 * Framework   : STM32Cube HAL + FreeRTOS
 *
 * Wiring:
 *   UART1 (Debug):  PA9=TX, PA10=RX -> USB-Serial
 *   UART2 (ESP-01): PA2=TX -> ESP RX, PA3=RX -> ESP TX
 *   ESP-01: VCC->3.3V, GND->GND, CH_PD->3.3V
 *   LED:            PC13 (active low) - activity indicator
 *
 * MQTT: Manually constructed packets over raw TCP to broker
 * ========================================================================== */
#include "config.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
/* ----- MQTT Packet Types ----- */
#define MQTT_CONNECT      0x10
#define MQTT_CONNACK      0x20
#define MQTT_PUBLISH      0x30
#define MQTT_SUBSCRIBE    0x82
#define MQTT_SUBACK       0x90
#define MQTT_PINGREQ      0xC0
#define MQTT_PINGRESP     0xD0
/* ----- Configuration ----- */
#define WIFI_SSID          "YourSSID"
#define WIFI_PASS          "YourPassword"
#define MQTT_BROKER        "test.mosquitto.org"
#define MQTT_PORT          1883
#define MQTT_CLIENT_ID     "stm32_iot_01"
#define MQTT_PUB_TOPIC     "stm32/sensor/data"
#define MQTT_SUB_TOPIC     "stm32/cmd/#"
#define MQTT_KEEPALIVE     60
#define ESP_RX_BUF_SIZE    256
#define ESP_CMD_TIMEOUT    5000
#define PUBLISH_INTERVAL   10000
typedef enum {
    MQTT_STATE_INIT,
    MQTT_STATE_WIFI_CONNECTING,
    MQTT_STATE_WIFI_CONNECTED,
    MQTT_STATE_TCP_CONNECTING,
    MQTT_STATE_TCP_CONNECTED,
    MQTT_STATE_MQTT_CONNECTING,
    MQTT_STATE_MQTT_CONNECTED,
    MQTT_STATE_ERROR
} MqttState_t;
/* ----- Peripheral Handles ----- */
static UART_HandleTypeDef huart1;
static UART_HandleTypeDef huart2;
/* ----- FreeRTOS Objects ----- */
static SemaphoreHandle_t xEspMutex;
static TaskHandle_t xMqttTaskHandle;
/* ----- State ----- */
static volatile MqttState_t mqtt_state = MQTT_STATE_INIT;
static char esp_rx_buf[ESP_RX_BUF_SIZE];
static volatile uint16_t esp_rx_idx = 0;
static volatile uint32_t publish_count = 0;
static volatile uint32_t msg_received = 0;
static uint16_t mqtt_packet_id = 1;
/* ----- Function Prototypes ----- */
static void SystemClock_Config(void);
static void GPIO_Init(void);
static void UART1_Init(void);
static void UART2_Init(void);
static void UART_Printf(const char *fmt, ...);
static int ESP_SendCmd(const char *cmd, const char *expect, uint32_t timeout);
static int ESP_SendRaw(const uint8_t *data, uint16_t len);
static int ESP_ReadResponse(char *buf, uint16_t size, uint32_t timeout);
static int WiFi_Connect(void);
static int TCP_Connect(void);
static int MQTT_BuildConnect(uint8_t *buf);
static int MQTT_BuildPublish(uint8_t *buf, const char *topic, const char *payload);
static int MQTT_BuildSubscribe(uint8_t *buf, const char *topic);
static int MQTT_BuildPingReq(uint8_t *buf);
static int MQTT_SendConnect(void);
static int MQTT_Publish(const char *topic, const char *payload);
static int MQTT_Subscribe(const char *topic);
static int MQTT_SendPing(void);
static void vMqttTask(void *pvParameters);
static void vPublishTask(void *pvParameters);
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
    GPIO_InitTypeDef gpio = {0};
    gpio.Pin = LED_PIN;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_PORT, &gpio);
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
}
/* ----- Send AT command and check expected response ----- */
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
                UART_Printf("[ESP] << %s [OK]\r\n", expect);
                xSemaphoreGive(xEspMutex);
                return 1;
            }
            if (strstr(esp_rx_buf, "ERROR") || strstr(esp_rx_buf, "FAIL")) {
                UART_Printf("[ESP] << ERROR in response\r\n");
                xSemaphoreGive(xEspMutex);
                return 0;
            }
        }
    }
    UART_Printf("[ESP] << TIMEOUT\r\n");
    xSemaphoreGive(xEspMutex);
    return 0;
}
/* ----- Send raw binary data via ESP-01 TCP ----- */
static int ESP_SendRaw(const uint8_t *data, uint16_t len) {
    char send_cmd[32];
    snprintf(send_cmd, sizeof(send_cmd), "AT+CIPSEND=%u", len);
    if (!ESP_SendCmd(send_cmd, ">", 2000)) {
        return 0;
    }
    xSemaphoreTake(xEspMutex, portMAX_DELAY);
    HAL_UART_Transmit(&huart2, (uint8_t *)data, len, 1000);
    /* Wait for SEND OK */
    uint32_t start = HAL_GetTick();
    memset(esp_rx_buf, 0, sizeof(esp_rx_buf));
    esp_rx_idx = 0;
    uint8_t byte;
    while ((HAL_GetTick() - start) < 3000) {
        if (HAL_UART_Receive(&huart2, &byte, 1, 50) == HAL_OK) {
            if (esp_rx_idx < ESP_RX_BUF_SIZE - 1) {
                esp_rx_buf[esp_rx_idx++] = (char)byte;
            }
            if (strstr(esp_rx_buf, "SEND OK")) {
                xSemaphoreGive(xEspMutex);
                return 1;
            }
        }
    }
    xSemaphoreGive(xEspMutex);
    return 0;
}
/* ----- WiFi Connect ----- */
static int WiFi_Connect(void) {
    mqtt_state = MQTT_STATE_WIFI_CONNECTING;
    UART_Printf("[WIFI] Connecting to %s...\r\n", WIFI_SSID);
    ESP_SendCmd("AT+RST", "ready", 3000);
    vTaskDelay(pdMS_TO_TICKS(1000));
    if (!ESP_SendCmd("AT", "OK", 1000)) return 0;
    if (!ESP_SendCmd("AT+CWMODE=1", "OK", 1000)) return 0;
    char join_cmd[128];
    snprintf(join_cmd, sizeof(join_cmd), "AT+CWJAP=\"%s\",\"%s\"", WIFI_SSID, WIFI_PASS);
    if (!ESP_SendCmd(join_cmd, "OK", 15000)) return 0;
    mqtt_state = MQTT_STATE_WIFI_CONNECTED;
    UART_Printf("[WIFI] Connected!\r\n");
    return 1;
}
/* ----- TCP Connect to MQTT broker ----- */
static int TCP_Connect(void) {
    mqtt_state = MQTT_STATE_TCP_CONNECTING;
    char tcp_cmd[128];
    snprintf(tcp_cmd, sizeof(tcp_cmd), "AT+CIPSTART=\"TCP\",\"%s\",%d", MQTT_BROKER, MQTT_PORT);
    if (!ESP_SendCmd(tcp_cmd, "OK", 10000)) return 0;
    mqtt_state = MQTT_STATE_TCP_CONNECTED;
    UART_Printf("[TCP] Connected to %s:%d\r\n", MQTT_BROKER, MQTT_PORT);
    return 1;
}
/* ----- Build MQTT CONNECT packet ----- */
static int MQTT_BuildConnect(uint8_t *buf) {
    uint16_t client_id_len = strlen(MQTT_CLIENT_ID);
    /* Variable header: protocol name(6) + protocol level(1) + flags(1) + keepalive(2) = 10 */
    uint16_t var_hdr_len = 10;
    /* Payload: client ID length(2) + client ID */
    uint16_t payload_len = 2 + client_id_len;
    uint16_t remaining = var_hdr_len + payload_len;
    int idx = 0;
    buf[idx++] = MQTT_CONNECT;  /* Fixed header: packet type */
    /* Encode remaining length */
    uint16_t rl = remaining;
    do {
        uint8_t encoded = rl % 128;
        rl /= 128;
        if (rl > 0) encoded |= 0x80;
        buf[idx++] = encoded;
    } while (rl > 0);
    /* Variable header */
    buf[idx++] = 0x00; buf[idx++] = 0x04;  /* Protocol name length */
    buf[idx++] = 'M'; buf[idx++] = 'Q'; buf[idx++] = 'T'; buf[idx++] = 'T';
    buf[idx++] = 0x04;  /* Protocol level (MQTT 3.1.1) */
    buf[idx++] = 0x02;  /* Connect flags: clean session */
    buf[idx++] = (MQTT_KEEPALIVE >> 8) & 0xFF;  /* Keep alive MSB */
    buf[idx++] = MQTT_KEEPALIVE & 0xFF;          /* Keep alive LSB */
    /* Payload: Client ID */
    buf[idx++] = (client_id_len >> 8) & 0xFF;
    buf[idx++] = client_id_len & 0xFF;
    memcpy(&buf[idx], MQTT_CLIENT_ID, client_id_len);
    idx += client_id_len;
    return idx;
}
/* ----- Build MQTT PUBLISH packet (QoS 0) ----- */
static int MQTT_BuildPublish(uint8_t *buf, const char *topic, const char *payload) {
    uint16_t topic_len = strlen(topic);
    uint16_t payload_len = strlen(payload);
    uint16_t remaining = 2 + topic_len + payload_len;
    int idx = 0;
    buf[idx++] = MQTT_PUBLISH;  /* QoS 0, no retain */
    uint16_t rl = remaining;
    do {
        uint8_t encoded = rl % 128;
        rl /= 128;
        if (rl > 0) encoded |= 0x80;
        buf[idx++] = encoded;
    } while (rl > 0);
    buf[idx++] = (topic_len >> 8) & 0xFF;
    buf[idx++] = topic_len & 0xFF;
    memcpy(&buf[idx], topic, topic_len);
    idx += topic_len;
    memcpy(&buf[idx], payload, payload_len);
    idx += payload_len;
    return idx;
}
/* ----- Build MQTT SUBSCRIBE packet ----- */
static int MQTT_BuildSubscribe(uint8_t *buf, const char *topic) {
    uint16_t topic_len = strlen(topic);
    uint16_t remaining = 2 + 2 + topic_len + 1;  /* packet ID + topic len + topic + QoS */
    int idx = 0;
    buf[idx++] = MQTT_SUBSCRIBE;
    uint16_t rl = remaining;
    do {
        uint8_t encoded = rl % 128;
        rl /= 128;
        if (rl > 0) encoded |= 0x80;
        buf[idx++] = encoded;
    } while (rl > 0);
    buf[idx++] = (mqtt_packet_id >> 8) & 0xFF;
    buf[idx++] = mqtt_packet_id & 0xFF;
    mqtt_packet_id++;
    buf[idx++] = (topic_len >> 8) & 0xFF;
    buf[idx++] = topic_len & 0xFF;
    memcpy(&buf[idx], topic, topic_len);
    idx += topic_len;
    buf[idx++] = 0x00;  /* QoS 0 */
    return idx;
}
/* ----- Build MQTT PINGREQ packet ----- */
static int MQTT_BuildPingReq(uint8_t *buf) {
    buf[0] = MQTT_PINGREQ;
    buf[1] = 0x00;
    return 2;
}
/* ----- High-level MQTT operations ----- */
static int MQTT_SendConnect(void) {
    uint8_t pkt[64];
    int len = MQTT_BuildConnect(pkt);
    UART_Printf("[MQTT] Sending CONNECT (%d bytes)\r\n", len);
    return ESP_SendRaw(pkt, (uint16_t)len);
}
static int MQTT_Publish(const char *topic, const char *payload) {
    uint8_t pkt[200];
    int len = MQTT_BuildPublish(pkt, topic, payload);
    UART_Printf("[MQTT] PUBLISH topic='%s' payload='%s' (%d bytes)\r\n", topic, payload, len);
    int result = ESP_SendRaw(pkt, (uint16_t)len);
    if (result) publish_count++;
    return result;
}
static int MQTT_Subscribe(const char *topic) {
    uint8_t pkt[128];
    int len = MQTT_BuildSubscribe(pkt, topic);
    UART_Printf("[MQTT] SUBSCRIBE topic='%s' (%d bytes)\r\n", topic, len);
    return ESP_SendRaw(pkt, (uint16_t)len);
}
static int MQTT_SendPing(void) {
    uint8_t pkt[2];
    int len = MQTT_BuildPingReq(pkt);
    return ESP_SendRaw(pkt, (uint16_t)len);
}
/* ----- MQTT Connection Task ----- */
static void vMqttTask(void *pvParameters) {
    (void)pvParameters;
    UART_Printf("[MQTT] Task started\r\n");
    for (;;) {
        if (mqtt_state == MQTT_STATE_INIT || mqtt_state == MQTT_STATE_ERROR) {
            HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET);
            if (!WiFi_Connect()) {
                UART_Printf("[MQTT] WiFi failed, retry in 5s\r\n");
                mqtt_state = MQTT_STATE_ERROR;
                vTaskDelay(pdMS_TO_TICKS(5000));
                continue;
            }
            if (!TCP_Connect()) {
                UART_Printf("[MQTT] TCP failed, retry in 5s\r\n");
                mqtt_state = MQTT_STATE_ERROR;
                vTaskDelay(pdMS_TO_TICKS(5000));
                continue;
            }
            mqtt_state = MQTT_STATE_MQTT_CONNECTING;
            vTaskDelay(pdMS_TO_TICKS(500));
            if (!MQTT_SendConnect()) {
                UART_Printf("[MQTT] CONNECT packet failed\r\n");
                mqtt_state = MQTT_STATE_ERROR;
                vTaskDelay(pdMS_TO_TICKS(5000));
                continue;
            }
            vTaskDelay(pdMS_TO_TICKS(1000));
            mqtt_state = MQTT_STATE_MQTT_CONNECTED;
            UART_Printf("[MQTT] Connected to broker!\r\n");
            HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_RESET);
            /* Subscribe to command topic */
            MQTT_Subscribe(MQTT_SUB_TOPIC);
            vTaskDelay(pdMS_TO_TICKS(500));
        }
        /* Keep-alive ping every 30 seconds */
        if (mqtt_state == MQTT_STATE_MQTT_CONNECTED) {
            MQTT_SendPing();
            UART_Printf("[MQTT] PINGREQ sent\r\n");
        }
        vTaskDelay(pdMS_TO_TICKS(30000));
    }
}
/* ----- Publish Task: periodic sensor data ----- */
static void vPublishTask(void *pvParameters) {
    (void)pvParameters;
    uint32_t cycle = 0;
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(PUBLISH_INTERVAL));
        if (mqtt_state != MQTT_STATE_MQTT_CONNECTED) {
            continue;
        }
        /* Simulate sensor readings */
        float temperature = 22.5f + (float)(cycle % 20) * 0.3f;
        float humidity = 55.0f + (float)(cycle % 15) * 0.5f;
        char payload[96];
        snprintf(payload, sizeof(payload),
                 "{\"id\":\"%s\",\"temp\":%.1f,\"hum\":%.1f,\"seq\":%lu}",
                 MQTT_CLIENT_ID, (double)temperature, (double)humidity, cycle);
        MQTT_Publish(MQTT_PUB_TOPIC, payload);
        HAL_GPIO_TogglePin(LED_PORT, LED_PIN);
        cycle++;
    }
}
/* ----- Monitor Task ----- */
static void vMonitorTask(void *pvParameters) {
    (void)pvParameters;
    const char *states[] = {"INIT", "WIFI_CONN", "WIFI_OK", "TCP_CONN",
                            "TCP_OK", "MQTT_CONN", "MQTT_OK", "ERROR"};
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(10000));
        UART_Printf("\r\n===== MQTT Status =====\r\n");
        UART_Printf("  State    : %s\r\n", states[mqtt_state]);
        UART_Printf("  Broker   : %s:%d\r\n", MQTT_BROKER, MQTT_PORT);
        UART_Printf("  Published: %lu msgs\r\n", publish_count);
        UART_Printf("  Received : %lu msgs\r\n", msg_received);
        UART_Printf("  Heap     : %u bytes\r\n", (unsigned)xPortGetFreeHeapSize());
        UART_Printf("=======================\r\n\r\n");
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
    UART_Printf("\r\n========================================\r\n");
    UART_Printf("  STM32 ESP-01 MQTT Client\r\n");
    UART_Printf("  Broker: %s:%d\r\n", MQTT_BROKER, MQTT_PORT);
    UART_Printf("  Pub: %s\r\n", MQTT_PUB_TOPIC);
    UART_Printf("  Sub: %s\r\n", MQTT_SUB_TOPIC);
    UART_Printf("========================================\r\n\r\n");
    xEspMutex = xSemaphoreCreateMutex();
    xTaskCreate(vMqttTask, "MQTT", 512, NULL, 4, &xMqttTaskHandle);
    xTaskCreate(vPublishTask, "Pub", 384, NULL, 3, NULL);
    xTaskCreate(vMonitorTask, "Mon", 256, NULL, 2, NULL);
    UART_Printf("[MAIN] Starting scheduler...\r\n");
    vTaskStartScheduler();
    for (;;) { }
}
