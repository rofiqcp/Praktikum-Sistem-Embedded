/* ==========================================================================
 * Program     : STM32_09_UART_Bridge_ESP32
 * Description : Custom UART protocol to ESP32 as WiFi co-processor
 * Board       : Blue Pill (STM32F103C8)
 * Framework   : STM32Cube HAL + FreeRTOS
 * 
 * Wiring:
 *   UART1 (Debug):  PA9=TX, PA10=RX -> USB-Serial
 *   UART2 (ESP32):  PA2=TX, PA3=RX  -> ESP32 UART
 *   LED:            PC13 (active low)
 *
 * Protocol Frame:
 *   STX(0x02) + CMD(1B) + LEN(2B, little-endian) + DATA(0..N) + CHECKSUM(1B) + ETX(0x03)
 *   Checksum = XOR of CMD + LEN_L + LEN_H + DATA[0..N-1]
 * ========================================================================== */

#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

/* ----- Protocol Definitions ----- */
#define PROTO_STX           0x02
#define PROTO_ETX           0x03
#define CMD_WIFI_CONNECT    0x01
#define CMD_SEND_DATA       0x02
#define CMD_GET_STATUS      0x03
#define CMD_GET_IP          0x04
#define CMD_ACK             0x10
#define CMD_NACK            0x11

#define MAX_DATA_LEN        128
#define CMD_QUEUE_LEN       8
#define RX_BUFFER_SIZE      256

/* ----- Protocol Packet Structure ----- */
typedef struct {
    uint8_t cmd;
    uint16_t data_len;
    uint8_t data[MAX_DATA_LEN];
} ProtoPacket_t;

/* ----- Peripheral Handles ----- */
static UART_HandleTypeDef huart1;  /* Debug console */
static UART_HandleTypeDef huart2;  /* ESP32 bridge  */

/* ----- FreeRTOS Objects ----- */
static QueueHandle_t xCmdQueue;
static SemaphoreHandle_t xUart2Mutex;
static TaskHandle_t xSendTaskHandle;
static TaskHandle_t xRecvTaskHandle;

/* ----- RX State Machine ----- */
typedef enum { RX_WAIT_STX, RX_CMD, RX_LEN_L, RX_LEN_H, RX_DATA, RX_CHECKSUM, RX_ETX } RxState_t;

static volatile uint8_t uart2_rx_byte;
static volatile uint32_t packets_sent = 0;
static volatile uint32_t packets_received = 0;
static volatile uint32_t checksum_errors = 0;

/* ----- Function Prototypes ----- */
static void SystemClock_Config(void);
static void GPIO_Init(void);
static void UART1_Init(void);
static void UART2_Init(void);
static void UART_Printf(const char *fmt, ...);
static uint8_t Calculate_Checksum(const ProtoPacket_t *pkt);
static void Build_Frame(const ProtoPacket_t *pkt, uint8_t *frame, uint16_t *frame_len);
static void Send_Command(uint8_t cmd, const uint8_t *data, uint16_t len);
static void vSendTask(void *pvParameters);
static void vRecvTask(void *pvParameters);
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

/* ----- Clock: HSE 8MHz -> PLL -> 72MHz ----- */
static void SystemClock_Config(void) {
    RCC_OscInitTypeDef osc = {0};
    osc.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    osc.HSEState = RCC_HSE_ON;
    osc.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    osc.PLL.PLLState = RCC_PLL_ON;
    osc.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    osc.PLL.PLLMUL = RCC_PLL_MUL9;
    HAL_RCC_OscConfig(&osc);

    RCC_ClkInitTypeDef clk = {0};
    clk.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                    RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    clk.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    clk.AHBCLKDivider = RCC_SYSCLK_DIV1;
    clk.APB1CLKDivider = RCC_HCLK_DIV2;
    clk.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_2);
}

/* ----- GPIO Init ----- */
static void GPIO_Init(void) {
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef gpio = {0};
    gpio.Pin = LED_PIN;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_PORT, &gpio);
    HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET);  /* LED off (active low) */
}

/* ----- UART1 Init (Debug) ----- */
static void UART1_Init(void) {
    __HAL_RCC_USART1_CLK_ENABLE();

    GPIO_InitTypeDef gpio = {0};
    gpio.Pin = GPIO_PIN_9;
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &gpio);

    gpio.Pin = GPIO_PIN_10;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &gpio);

    huart1.Instance = USART1;
    huart1.Init.BaudRate = UART_BAUDRATE;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    HAL_UART_Init(&huart1);
}

/* ----- UART2 Init (ESP32 Bridge) ----- */
static void UART2_Init(void) {
    __HAL_RCC_USART2_CLK_ENABLE();

    GPIO_InitTypeDef gpio = {0};
    gpio.Pin = GPIO_PIN_2;  /* TX */
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &gpio);

    gpio.Pin = GPIO_PIN_3;  /* RX */
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOA, &gpio);

    huart2.Instance = USART2;
    huart2.Init.BaudRate = 115200;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    HAL_UART_Init(&huart2);

    HAL_NVIC_SetPriority(USART2_IRQn, 6, 0);
    HAL_NVIC_EnableIRQ(USART2_IRQn);
    HAL_UART_Receive_IT(&huart2, (uint8_t *)&uart2_rx_byte, 1);
}

/* ----- Checksum: XOR of CMD + LEN_L + LEN_H + DATA ----- */
static uint8_t Calculate_Checksum(const ProtoPacket_t *pkt) {
    uint8_t cs = pkt->cmd;
    cs ^= (uint8_t)(pkt->data_len & 0xFF);
    cs ^= (uint8_t)((pkt->data_len >> 8) & 0xFF);
    for (uint16_t i = 0; i < pkt->data_len; i++) {
        cs ^= pkt->data[i];
    }
    return cs;
}

/* ----- Build wire frame from packet ----- */
static void Build_Frame(const ProtoPacket_t *pkt, uint8_t *frame, uint16_t *frame_len) {
    uint16_t idx = 0;
    frame[idx++] = PROTO_STX;
    frame[idx++] = pkt->cmd;
    frame[idx++] = (uint8_t)(pkt->data_len & 0xFF);
    frame[idx++] = (uint8_t)((pkt->data_len >> 8) & 0xFF);
    for (uint16_t i = 0; i < pkt->data_len; i++) {
        frame[idx++] = pkt->data[i];
    }
    frame[idx++] = Calculate_Checksum(pkt);
    frame[idx++] = PROTO_ETX;
    *frame_len = idx;
}

/* ----- Send a command to ESP32 ----- */
static void Send_Command(uint8_t cmd, const uint8_t *data, uint16_t len) {
    ProtoPacket_t pkt;
    pkt.cmd = cmd;
    pkt.data_len = (len > MAX_DATA_LEN) ? MAX_DATA_LEN : len;
    if (data && pkt.data_len > 0) {
        memcpy(pkt.data, data, pkt.data_len);
    }
    xQueueSend(xCmdQueue, &pkt, pdMS_TO_TICKS(100));
}

/* ----- Send Task: dequeue commands and transmit ----- */
static void vSendTask(void *pvParameters) {
    (void)pvParameters;
    ProtoPacket_t pkt;
    uint8_t frame[MAX_DATA_LEN + 8];
    uint16_t frame_len;
    const char *cmd_names[] = {"?", "WIFI_CONNECT", "SEND_DATA", "GET_STATUS", "GET_IP"};

    UART_Printf("[SEND] Task started\r\n");

    /* Initial WiFi connect command */
    const char *ssid_pass = "MySSID:MyPassword";
    Send_Command(CMD_WIFI_CONNECT, (const uint8_t *)ssid_pass, strlen(ssid_pass));

    TickType_t xLastWake = xTaskGetTickCount();
    uint32_t cycle = 0;

    for (;;) {
        /* Periodically enqueue status/data commands */
        if (cycle % 10 == 3) {
            Send_Command(CMD_GET_STATUS, NULL, 0);
        }
        if (cycle % 10 == 6) {
            Send_Command(CMD_GET_IP, NULL, 0);
        }
        if (cycle % 5 == 0) {
            char sensor_data[48];
            int n = snprintf(sensor_data, sizeof(sensor_data), "{\"temp\":%.1f,\"hum\":%.1f}",
                             25.0 + (cycle % 10) * 0.5, 60.0 + (cycle % 8));
            Send_Command(CMD_SEND_DATA, (const uint8_t *)sensor_data, (uint16_t)n);
        }

        /* Process queued commands */
        while (xQueueReceive(xCmdQueue, &pkt, 0) == pdTRUE) {
            Build_Frame(&pkt, frame, &frame_len);

            const char *name = (pkt.cmd >= 1 && pkt.cmd <= 4) ? cmd_names[pkt.cmd] : "UNKNOWN";
            UART_Printf("[SEND] CMD=%s(0x%02X) LEN=%u CS=0x%02X\r\n",
                        name, pkt.cmd, pkt.data_len, Calculate_Checksum(&pkt));

            xSemaphoreTake(xUart2Mutex, portMAX_DELAY);
            HAL_UART_Transmit(&huart2, frame, frame_len, 200);
            xSemaphoreGive(xUart2Mutex);

            packets_sent++;
            HAL_GPIO_TogglePin(LED_PORT, LED_PIN);
        }

        cycle++;
        vTaskDelayUntil(&xLastWake, pdMS_TO_TICKS(1000));
    }
}

/* ----- Receive Task: parse incoming frames ----- */
static void vRecvTask(void *pvParameters) {
    (void)pvParameters;
    UART_Printf("[RECV] Task started\r\n");

    RxState_t state = RX_WAIT_STX;
    ProtoPacket_t rx_pkt;
    uint16_t data_idx = 0;
    uint8_t rx_checksum = 0;
    uint8_t byte;

    for (;;) {
        /* Poll UART2 for received bytes */
        if (HAL_UART_Receive(&huart2, &byte, 1, 50) == HAL_OK) {
            switch (state) {
            case RX_WAIT_STX:
                if (byte == PROTO_STX) {
                    memset(&rx_pkt, 0, sizeof(rx_pkt));
                    data_idx = 0;
                    state = RX_CMD;
                }
                break;
            case RX_CMD:
                rx_pkt.cmd = byte;
                state = RX_LEN_L;
                break;
            case RX_LEN_L:
                rx_pkt.data_len = byte;
                state = RX_LEN_H;
                break;
            case RX_LEN_H:
                rx_pkt.data_len |= ((uint16_t)byte << 8);
                if (rx_pkt.data_len > MAX_DATA_LEN) {
                    UART_Printf("[RECV] ERROR: data_len=%u exceeds max\r\n", rx_pkt.data_len);
                    state = RX_WAIT_STX;
                } else if (rx_pkt.data_len == 0) {
                    state = RX_CHECKSUM;
                } else {
                    data_idx = 0;
                    state = RX_DATA;
                }
                break;
            case RX_DATA:
                rx_pkt.data[data_idx++] = byte;
                if (data_idx >= rx_pkt.data_len) {
                    state = RX_CHECKSUM;
                }
                break;
            case RX_CHECKSUM:
                rx_checksum = byte;
                state = RX_ETX;
                break;
            case RX_ETX:
                if (byte == PROTO_ETX) {
                    uint8_t calc_cs = Calculate_Checksum(&rx_pkt);
                    if (calc_cs == rx_checksum) {
                        packets_received++;
                        UART_Printf("[RECV] OK CMD=0x%02X LEN=%u DATA='%.*s'\r\n",
                                    rx_pkt.cmd, rx_pkt.data_len,
                                    rx_pkt.data_len, rx_pkt.data);
                    } else {
                        checksum_errors++;
                        UART_Printf("[RECV] CHECKSUM ERROR: got=0x%02X exp=0x%02X\r\n",
                                    rx_checksum, calc_cs);
                    }
                } else {
                    UART_Printf("[RECV] ERROR: expected ETX(0x03), got 0x%02X\r\n", byte);
                }
                state = RX_WAIT_STX;
                break;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

/* ----- Monitor Task: periodic stats ----- */
static void vMonitorTask(void *pvParameters) {
    (void)pvParameters;
    UART_Printf("[MON] Monitor task started\r\n");

    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(5000));
        UART_Printf("\r\n===== Bridge Stats =====\r\n");
        UART_Printf("  Sent    : %lu packets\r\n", packets_sent);
        UART_Printf("  Received: %lu packets\r\n", packets_received);
        UART_Printf("  CS Errors: %lu\r\n", checksum_errors);
        UART_Printf("  Heap Free: %u bytes\r\n", (unsigned)xPortGetFreeHeapSize());
        UART_Printf("========================\r\n\r\n");
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
    UART_Printf("  STM32 UART Bridge to ESP32\r\n");
    UART_Printf("  Protocol: STX+CMD+LEN+DATA+CS+ETX\r\n");
    UART_Printf("  UART2: PA2(TX), PA3(RX) @ 115200\r\n");
    UART_Printf("========================================\r\n\r\n");

    xCmdQueue = xQueueCreate(CMD_QUEUE_LEN, sizeof(ProtoPacket_t));
    xUart2Mutex = xSemaphoreCreateMutex();

    xTaskCreate(vSendTask, "Send", 384, NULL, 3, &xSendTaskHandle);
    xTaskCreate(vRecvTask, "Recv", 384, NULL, 4, &xRecvTaskHandle);
    xTaskCreate(vMonitorTask, "Mon", 256, NULL, 2, NULL);

    UART_Printf("[MAIN] Starting scheduler...\r\n");
    vTaskStartScheduler();

    for (;;) { }
}
