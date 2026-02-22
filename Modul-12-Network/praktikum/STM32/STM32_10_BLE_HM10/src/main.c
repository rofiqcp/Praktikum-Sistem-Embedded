/* ==========================================================================
 * Program     : STM32_10_BLE_HM10
 * Description : BLE communication via HM-10 module using UART AT commands
 * Board       : Blue Pill (STM32F103C8)
 * Framework   : STM32Cube HAL + FreeRTOS
 *
 * Wiring:
 *   UART1 (Debug):  PA9=TX, PA10=RX -> USB-Serial
 *   UART2 (HM-10):  PA2=TX -> HM-10 RX, PA3=RX -> HM-10 TX
 *   HM-10 VCC -> 3.3V, HM-10 GND -> GND
 *   LED:            PC13 (active low) - BLE connection status
 * ========================================================================== */
#include "config.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
/* ----- BLE Definitions ----- */
#define HM10_RX_BUF_SIZE    128
#define HM10_CMD_TIMEOUT    1000
#define AT_RESPONSE_TIMEOUT 500
#define BLE_DATA_QUEUE_LEN  8
#define BLE_DATA_MAX_LEN    64
typedef struct {
    char data[BLE_DATA_MAX_LEN];
    uint16_t len;
} BleData_t;
typedef enum {
    BLE_STATE_INIT,
    BLE_STATE_READY,
    BLE_STATE_ADVERTISING,
    BLE_STATE_CONNECTED,
    BLE_STATE_ERROR
} BleState_t;
/* ----- Peripheral Handles ----- */
static UART_HandleTypeDef huart1;
static UART_HandleTypeDef huart2;
/* ----- FreeRTOS Objects ----- */
static QueueHandle_t xBleRxQueue;
static SemaphoreHandle_t xUart2Mutex;
static TaskHandle_t xBleTaskHandle;
static TaskHandle_t xAppTaskHandle;
/* ----- State Variables ----- */
static volatile BleState_t ble_state = BLE_STATE_INIT;
static char hm10_rx_buf[HM10_RX_BUF_SIZE];
static volatile uint16_t hm10_rx_idx = 0;
static volatile uint32_t ble_rx_count = 0;
static volatile uint32_t ble_tx_count = 0;
static char ble_name[20] = "STM32_BLE";
static char ble_addr[20] = "unknown";
/* ----- Function Prototypes ----- */
static void SystemClock_Config(void);
static void GPIO_Init(void);
static void UART1_Init(void);
static void UART2_Init(void);
static void UART_Printf(const char *fmt, ...);
static int HM10_SendAT(const char *cmd, char *response, uint16_t resp_size, uint32_t timeout);
static void HM10_SendData(const char *data, uint16_t len);
static int HM10_Init_Module(void);
static void vBleTask(void *pvParameters);
static void vAppTask(void *pvParameters);
static void vStatusTask(void *pvParameters);
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
/* ----- Send AT command and wait for response ----- */
static int HM10_SendAT(const char *cmd, char *response, uint16_t resp_size, uint32_t timeout) {
    xSemaphoreTake(xUart2Mutex, portMAX_DELAY);
    memset(hm10_rx_buf, 0, sizeof(hm10_rx_buf));
    hm10_rx_idx = 0;
    HAL_UART_Transmit(&huart2, (uint8_t *)cmd, strlen(cmd), 200);
    UART_Printf("[BLE] TX: %s\r\n", cmd);
    /* Collect response bytes with timeout */
    uint32_t start = HAL_GetTick();
    uint8_t byte;
    while ((HAL_GetTick() - start) < timeout) {
        if (HAL_UART_Receive(&huart2, &byte, 1, 50) == HAL_OK) {
            if (hm10_rx_idx < HM10_RX_BUF_SIZE - 1) {
                hm10_rx_buf[hm10_rx_idx++] = (char)byte;
                hm10_rx_buf[hm10_rx_idx] = '\0';
            }
            start = HAL_GetTick();  /* Reset timeout on data */
        }
        if (hm10_rx_idx > 0 && (HAL_GetTick() - start) > 100) {
            break;  /* No more data coming */
        }
    }
    if (response && resp_size > 0) {
        strncpy(response, hm10_rx_buf, resp_size - 1);
        response[resp_size - 1] = '\0';
    }
    UART_Printf("[BLE] RX: %s\r\n", hm10_rx_buf);
    xSemaphoreGive(xUart2Mutex);
    return (hm10_rx_idx > 0) ? 1 : 0;
}
/* ----- Send data over BLE connection ----- */
static void HM10_SendData(const char *data, uint16_t len) {
    xSemaphoreTake(xUart2Mutex, portMAX_DELAY);
    HAL_UART_Transmit(&huart2, (uint8_t *)data, len, 200);
    ble_tx_count++;
    xSemaphoreGive(xUart2Mutex);
}
/* ----- Initialize HM-10 module ----- */
static int HM10_Init_Module(void) {
    char resp[64];
    UART_Printf("[BLE] Initializing HM-10 module...\r\n");
    /* Test connection */
    if (!HM10_SendAT("AT", resp, sizeof(resp), AT_RESPONSE_TIMEOUT)) {
        UART_Printf("[BLE] ERROR: HM-10 not responding\r\n");
        return 0;
    }
    if (strstr(resp, "OK") == NULL) {
        UART_Printf("[BLE] WARNING: unexpected response\r\n");
    }
    vTaskDelay(pdMS_TO_TICKS(200));
    /* Set device name */
    HM10_SendAT("AT+NAMEstm32ble", resp, sizeof(resp), AT_RESPONSE_TIMEOUT);
    strncpy(ble_name, "stm32ble", sizeof(ble_name));
    vTaskDelay(pdMS_TO_TICKS(200));
    /* Set role to peripheral (slave) */
    HM10_SendAT("AT+ROLE0", resp, sizeof(resp), AT_RESPONSE_TIMEOUT);
    vTaskDelay(pdMS_TO_TICKS(200));
    /* Get MAC address */
    HM10_SendAT("AT+ADDR?", resp, sizeof(resp), AT_RESPONSE_TIMEOUT);
    if (strlen(resp) > 8) {
        strncpy(ble_addr, resp + 8, sizeof(ble_addr) - 1);  /* Skip "OK+ADDR:" */
    }
    vTaskDelay(pdMS_TO_TICKS(200));
    /* Set service UUID (optional) */
    HM10_SendAT("AT+UUID0xFFE0", resp, sizeof(resp), AT_RESPONSE_TIMEOUT);
    vTaskDelay(pdMS_TO_TICKS(200));
    UART_Printf("[BLE] Init complete - Name: %s, Addr: %s\r\n", ble_name, ble_addr);
    return 1;
}
/* ----- BLE Communication Task ----- */
static void vBleTask(void *pvParameters) {
    (void)pvParameters;
    BleData_t rx_data;
    uint8_t byte;
    UART_Printf("[BLE] Task started\r\n");
    /* Initialize HM-10 */
    if (HM10_Init_Module()) {
        ble_state = BLE_STATE_ADVERTISING;
        UART_Printf("[BLE] Advertising...\r\n");
    } else {
        ble_state = BLE_STATE_ERROR;
        UART_Printf("[BLE] ERROR: init failed\r\n");
    }
    /* Main receive loop */
    memset(&rx_data, 0, sizeof(rx_data));
    for (;;) {
        if (ble_state == BLE_STATE_ERROR) {
            HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET);
            vTaskDelay(pdMS_TO_TICKS(2000));
            /* Retry init */
            if (HM10_Init_Module()) {
                ble_state = BLE_STATE_ADVERTISING;
            }
            continue;
        }
        /* Read incoming BLE data */
        if (HAL_UART_Receive(&huart2, &byte, 1, 100) == HAL_OK) {
            /* Check for connection indicator */
            if (byte == 'O' || byte == 'C') {
                /* Could be OK+CONN or OK+LOST */
                char status_buf[16] = {0};
                status_buf[0] = (char)byte;
                uint8_t si = 1;
                while (si < 15) {
                    if (HAL_UART_Receive(&huart2, &byte, 1, 50) == HAL_OK) {
                        status_buf[si++] = (char)byte;
                    } else {
                        break;
                    }
                }
                if (strstr(status_buf, "OK+CONN")) {
                    ble_state = BLE_STATE_CONNECTED;
                    HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_RESET);
                    UART_Printf("[BLE] CONNECTED!\r\n");
                    continue;
                } else if (strstr(status_buf, "OK+LOST")) {
                    ble_state = BLE_STATE_ADVERTISING;
                    HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET);
                    UART_Printf("[BLE] DISCONNECTED\r\n");
                    continue;
                }
                /* Not a status message - treat as data */
                for (uint8_t i = 0; i < si; i++) {
                    if (rx_data.len < BLE_DATA_MAX_LEN - 1) {
                        rx_data.data[rx_data.len++] = status_buf[i];
                    }
                }
            } else {
                if (rx_data.len < BLE_DATA_MAX_LEN - 1) {
                    rx_data.data[rx_data.len++] = (char)byte;
                }
            }
            /* If buffer has data and timeout, queue it */
            if (rx_data.len > 0) {
                uint8_t more;
                if (HAL_UART_Receive(&huart2, &more, 1, 30) != HAL_OK) {
                    rx_data.data[rx_data.len] = '\0';
                    xQueueSend(xBleRxQueue, &rx_data, pdMS_TO_TICKS(50));
                    ble_rx_count++;
                    UART_Printf("[BLE] RX Data(%u): %s\r\n", rx_data.len, rx_data.data);
                    memset(&rx_data, 0, sizeof(rx_data));
                } else {
                    rx_data.data[rx_data.len++] = (char)more;
                }
            }
        }
        /* Blink LED in advertising mode */
        if (ble_state == BLE_STATE_ADVERTISING) {
            HAL_GPIO_TogglePin(LED_PORT, LED_PIN);
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
/* ----- Application Task: process BLE data and send responses ----- */
static void vAppTask(void *pvParameters) {
    (void)pvParameters;
    BleData_t rx_data;
    uint32_t cycle = 0;
    UART_Printf("[APP] Task started\r\n");
    for (;;) {
        /* Process received BLE data */
        if (xQueueReceive(xBleRxQueue, &rx_data, pdMS_TO_TICKS(100)) == pdTRUE) {
            UART_Printf("[APP] Processing: '%s'\r\n", rx_data.data);
            /* Echo back with response */
            if (ble_state == BLE_STATE_CONNECTED) {
                char reply[BLE_DATA_MAX_LEN];
                int n = snprintf(reply, sizeof(reply), "ACK:%s", rx_data.data);
                HM10_SendData(reply, (uint16_t)n);
                UART_Printf("[APP] Sent reply: %s\r\n", reply);
            }
        }
        /* Periodic heartbeat to connected device */
        if (ble_state == BLE_STATE_CONNECTED && (cycle % 50 == 0)) {
            char hb[32];
            int n = snprintf(hb, sizeof(hb), "HB:%lu", cycle / 50);
            HM10_SendData(hb, (uint16_t)n);
        }
        cycle++;
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
/* ----- Status Task ----- */
static void vStatusTask(void *pvParameters) {
    (void)pvParameters;
    const char *state_names[] = {"INIT", "READY", "ADVERTISING", "CONNECTED", "ERROR"};
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(5000));
        UART_Printf("\r\n===== BLE HM-10 Status =====\r\n");
        UART_Printf("  State   : %s\r\n", state_names[ble_state]);
        UART_Printf("  Name    : %s\r\n", ble_name);
        UART_Printf("  Address : %s\r\n", ble_addr);
        UART_Printf("  RX Count: %lu\r\n", ble_rx_count);
        UART_Printf("  TX Count: %lu\r\n", ble_tx_count);
        UART_Printf("  Heap    : %u bytes\r\n", (unsigned)xPortGetFreeHeapSize());
        UART_Printf("============================\r\n\r\n");
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
    UART_Printf("  STM32 BLE HM-10 Communication\r\n");
    UART_Printf("  HM-10: PA2(TX)->RX, PA3(RX)->TX\r\n");
    UART_Printf("  Debug: UART1 @ %d baud\r\n", UART_BAUDRATE);
    UART_Printf("========================================\r\n\r\n");
    xBleRxQueue = xQueueCreate(BLE_DATA_QUEUE_LEN, sizeof(BleData_t));
    xUart2Mutex = xSemaphoreCreateMutex();
    xTaskCreate(vBleTask, "BLE", 384, NULL, 4, &xBleTaskHandle);
    xTaskCreate(vAppTask, "App", 320, NULL, 3, &xAppTaskHandle);
    xTaskCreate(vStatusTask, "Stat", 256, NULL, 2, NULL);
    UART_Printf("[MAIN] Starting scheduler...\r\n");
    vTaskStartScheduler();
    for (;;) { }
}
