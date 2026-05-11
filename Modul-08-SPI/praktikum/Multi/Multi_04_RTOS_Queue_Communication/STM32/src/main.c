/* Multi_04_RTOS_Queue_Communication - STM32 (SPI Master, bluepill F103C8)
 * FreeRTOS dengan Queue komunikasi ke ESP32 via SPI
 *
 * Task:
 *   vSensorTask   - generate data sensor setiap SENSOR_PERIOD_MS, kirim ke xSpiQueue
 *   vTxTask       - terima dari queue, kirim ke ESP32 via SPI
 *   vMonitorTask  - print statistik via UART setiap MONITOR_PERIOD_MS
 *
 * Protokol paket ke ESP32:
 *   [magic=0xA5][cmd][temp_hi][temp_lo][hum_hi][hum_lo][ts0][ts1][ts2][ts3][chksum]
 */

#include "config.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

/* =========================================================
 * Hardware handles
 * ========================================================= */
SPI_HandleTypeDef  hspi1;
UART_HandleTypeDef huart1;

/* =========================================================
 * FreeRTOS objects
 * ========================================================= */
static QueueHandle_t xSpiQueue = NULL;

/* =========================================================
 * Statistics
 * ========================================================= */
static volatile uint32_t generated = 0;
static volatile uint32_t sent      = 0;
static volatile uint32_t dropped   = 0;

/* =========================================================
 * UART helper
 * ========================================================= */
static void UART_Print(const char *msg) {
    HAL_UART_Transmit(&huart1, (uint8_t *)msg, (uint16_t)strlen(msg), 200);
}

/* =========================================================
 * SPI CS helpers
 * ========================================================= */
static void SPI_CS_Low(void)  { HAL_GPIO_WritePin(SPI_CS_PORT, SPI_CS_PIN, GPIO_PIN_RESET); }
static void SPI_CS_High(void) { HAL_GPIO_WritePin(SPI_CS_PORT, SPI_CS_PIN, GPIO_PIN_SET);   }

/* Wait for ESP32 handshake HIGH (slave ready) */
static bool SPI_WaitHS(uint32_t timeout_ms) {
    uint32_t t0 = HAL_GetTick();
    while (HAL_GPIO_ReadPin(HS_PORT, HS_PIN) == GPIO_PIN_RESET) {
        if ((HAL_GetTick() - t0) > timeout_ms) return false;
    }
    return true;
}

/* =========================================================
 * Packet build + checksum
 * ========================================================= */
static uint8_t packet_checksum(const SpiPacket_t *p) {
    const uint8_t *b = (const uint8_t *)p;
    uint8_t sum = 0;
    for (size_t i = 0; i < sizeof(SpiPacket_t) - 1; i++) sum ^= b[i];
    return sum;
}

static void build_packet(SpiPacket_t *p, uint8_t cmd,
                          int16_t temp, uint16_t hum, uint32_t ts) {
    p->magic            = PACKET_MAGIC;
    p->cmd              = cmd;
    p->temperature_x100 = temp;
    p->humidity_x100    = hum;
    p->timestamp        = ts;
    p->checksum         = packet_checksum(p);
}

/* =========================================================
 * FreeRTOS Tasks
 * ========================================================= */
void vSensorTask(void *pvParameters) {
    (void)pvParameters;
    TickType_t xLastWakeTime = xTaskGetTickCount();
    SpiPacket_t pkt;
    uint32_t count = 0;

    for (;;) {
        count++;
        generated = count;

        int16_t  temp = (int16_t)(2500 + (count % 500));
        uint16_t hum  = (uint16_t)(6000 + (count % 3000));
        uint32_t ts   = HAL_GetTick();

        build_packet(&pkt, CMD_SENSOR_DATA, temp, hum, ts);

        if (xQueueSend(xSpiQueue, &pkt, 0) != pdTRUE) {
            dropped++;
        }

        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(SENSOR_PERIOD_MS));
    }
}

void vTxTask(void *pvParameters) {
    (void)pvParameters;
    SpiPacket_t pkt;
    uint8_t rx_buf[sizeof(SpiPacket_t)];
    char buf[80];

    for (;;) {
        if (xQueueReceive(xSpiQueue, &pkt, portMAX_DELAY) == pdTRUE) {
            /* Wait for ESP32 ready */
            if (!SPI_WaitHS(200)) {
                UART_Print("[TX] HS timeout\r\n");
                continue;
            }

            SPI_CS_Low();
            HAL_SPI_TransmitReceive(&hspi1, (uint8_t *)&pkt, rx_buf,
                                    sizeof(SpiPacket_t), 100);
            SPI_CS_High();
            sent++;

            /* Check ACK from ESP32 (first byte of rx should be 0xAA) */
            snprintf(buf, sizeof(buf),
                     "[TX] Pkt#%lu T=%d.%02d H=%d.%02d ACK=0x%02X\r\n",
                     (unsigned long)sent,
                     pkt.temperature_x100 / 100, pkt.temperature_x100 % 100,
                     pkt.humidity_x100 / 100, pkt.humidity_x100 % 100,
                     rx_buf[0]);
            UART_Print(buf);
        }
    }
}

void vMonitorTask(void *pvParameters) {
    (void)pvParameters;
    char buf[96];
    TickType_t xLastWakeTime = xTaskGetTickCount();

    for (;;) {
        snprintf(buf, sizeof(buf),
                 "[MON] Gen=%lu Sent=%lu Drop=%lu QLen=%u\r\n",
                 (unsigned long)generated,
                 (unsigned long)sent,
                 (unsigned long)dropped,
                 (unsigned)uxQueueMessagesWaiting(xSpiQueue));
        UART_Print(buf);
        HAL_GPIO_TogglePin(LED_PORT, LED_PIN);
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(MONITOR_PERIOD_MS));
    }
}

/* =========================================================
 * SysTick Handler
 * ========================================================= */
void SysTick_Handler(void) {
    HAL_IncTick();
    if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED) {
        extern void xPortSysTickHandler(void);
        xPortSysTickHandler();
    }
}

/* =========================================================
 * Hardware Initialization
 * ========================================================= */
void SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    RCC_OscInitStruct.OscillatorType      = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState            = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState        = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource       = RCC_PLLSOURCE_HSI_DIV2;
    RCC_OscInitStruct.PLL.PLLMUL          = RCC_PLL_MUL16;
    HAL_RCC_OscConfig(&RCC_OscInitStruct);

    RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                     | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2);
}

static void GPIO_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    /* CS high */
    HAL_GPIO_WritePin(SPI_CS_PORT, SPI_CS_PIN, GPIO_PIN_SET);
    GPIO_InitStruct.Pin   = SPI_CS_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(SPI_CS_PORT, &GPIO_InitStruct);

    /* Handshake input */
    GPIO_InitStruct.Pin  = HS_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;
    HAL_GPIO_Init(HS_PORT, &GPIO_InitStruct);

    /* LED */
    HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET);
    GPIO_InitStruct.Pin   = LED_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_PORT, &GPIO_InitStruct);
}

static void SPI1_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_SPI1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitStruct.Pin   = SPI_SCK_PIN | SPI_MOSI_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(SPI_SCK_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin  = SPI_MISO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    HAL_GPIO_Init(SPI_MISO_PORT, &GPIO_InitStruct);

    hspi1.Instance               = SPI1;
    hspi1.Init.Mode              = SPI_MODE_MASTER;
    hspi1.Init.Direction         = SPI_DIRECTION_2LINES;
    hspi1.Init.DataSize          = SPI_DATASIZE_8BIT;
    hspi1.Init.CLKPolarity       = SPI_POLARITY_LOW;
    hspi1.Init.CLKPhase          = SPI_PHASE_1EDGE;
    hspi1.Init.NSS               = SPI_NSS_SOFT;
    hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
    hspi1.Init.FirstBit          = SPI_FIRSTBIT_MSB;
    hspi1.Init.TIMode            = SPI_TIMODE_DISABLE;
    hspi1.Init.CRCCalculation    = SPI_CRCCALCULATION_DISABLE;
    HAL_SPI_Init(&hspi1);
}

static void USART1_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_USART1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitStruct.Pin   = GPIO_PIN_9;
    GPIO_InitStruct.Mode  = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin  = GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
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

void Error_Handler(void) {
    __disable_irq();
    while (1) {}
}

/* =========================================================
 * Main
 * ========================================================= */
int main(void) {
    HAL_Init();
    SystemClock_Config();
    GPIO_Init();
    SPI1_Init();
    USART1_Init();

    UART_Print("\r\n=== Multi_04 STM32 RTOS Queue Comm (Master) ===\r\n");

    /* Create queue */
    xSpiQueue = xQueueCreate(SPI_QUEUE_LENGTH, sizeof(SpiPacket_t));
    if (xSpiQueue == NULL) {
        UART_Print("ERROR: Queue creation failed!\r\n");
        Error_Handler();
    }

    /* Create tasks */
    xTaskCreate(vSensorTask,  "Sensor",  SENSOR_TASK_STACK,  NULL, SENSOR_TASK_PRIORITY,  NULL);
    xTaskCreate(vTxTask,      "Tx",      TX_TASK_STACK,      NULL, TX_TASK_PRIORITY,      NULL);
    xTaskCreate(vMonitorTask, "Monitor", MONITOR_TASK_STACK, NULL, MONITOR_TASK_PRIORITY, NULL);

    UART_Print("Starting scheduler...\r\n");
    vTaskStartScheduler();

    while (1) {}
}
