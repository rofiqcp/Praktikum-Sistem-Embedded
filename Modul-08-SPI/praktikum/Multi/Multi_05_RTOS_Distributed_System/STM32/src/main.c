/* Multi_05_RTOS_Distributed_System - STM32 Node (SPI Master, bluepill F103C8)
 * STM32 sebagai node sensor terdistribusi:
 *   - SPI1: Master ke ESP32 hub (PA4-PA7, HS=PA3)
 *   - SPI2: SD Card logger (PB12-PB15)
 *
 * Task:
 *   vSensorTask  - generate data sensor, kirim ke xTxQueue + xSDQueue
 *   vSDLogTask   - terima dari xSDQueue, simpan ke SD card via SPI2
 *   vTxTask      - terima dari xTxQueue, kirim ke ESP32 via SPI1
 *   vMonitorTask - print statistik via UART
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
SPI_HandleTypeDef  hspi1;  /* To ESP32 */
SPI_HandleTypeDef  hspi2;  /* To SD Card */
UART_HandleTypeDef huart1;

/* =========================================================
 * FreeRTOS objects
 * ========================================================= */
static QueueHandle_t xTxQueue = NULL;
static QueueHandle_t xSDQueue = NULL;

/* =========================================================
 * Statistics
 * ========================================================= */
static volatile uint32_t sensor_count = 0;
static volatile uint32_t tx_count     = 0;
static volatile uint32_t sd_count     = 0;
static volatile uint32_t tx_dropped   = 0;

/* =========================================================
 * UART helper
 * ========================================================= */
static void UART_Print(const char *msg) {
    HAL_UART_Transmit(&huart1, (uint8_t *)msg, (uint16_t)strlen(msg), 200);
}

/* =========================================================
 * SPI CS helpers
 * ========================================================= */
static void SPI1_CS_Low(void)  { HAL_GPIO_WritePin(SPI1_CS_PORT, SPI1_CS_PIN, GPIO_PIN_RESET); }
static void SPI1_CS_High(void) { HAL_GPIO_WritePin(SPI1_CS_PORT, SPI1_CS_PIN, GPIO_PIN_SET); }
static void SD_CS_Low(void)    { HAL_GPIO_WritePin(SD_CS_PORT, SD_CS_PIN, GPIO_PIN_RESET); }
static void SD_CS_High(void)   { HAL_GPIO_WritePin(SD_CS_PORT, SD_CS_PIN, GPIO_PIN_SET); }

/* =========================================================
 * Packet helpers
 * ========================================================= */
static uint8_t dist_checksum(const DistPacket_t *p) {
    const uint8_t *b = (const uint8_t *)p;
    uint8_t sum = 0;
    for (size_t i = 0; i < sizeof(DistPacket_t) - 1; i++) sum ^= b[i];
    return sum;
}

static void build_dist_packet(DistPacket_t *p, uint16_t seq,
                               int16_t temp, uint16_t hum) {
    p->magic            = DIST_MAGIC;
    p->node_id          = NODE_ID;
    p->packet_type      = PKT_TYPE_SENSOR;
    p->seq_num          = seq;
    p->timestamp        = HAL_GetTick();
    p->temperature_x100 = temp;
    p->humidity_x100    = hum;
    p->sd_write_count   = (uint16_t)sd_count;
    p->checksum         = dist_checksum(p);
}

/* =========================================================
 * SD Card minimal interface (SPI2)
 * ========================================================= */
static bool SD_Init(void) {
    uint8_t dummy = 0xFF;
    SD_CS_High();
    for (int i = 0; i < 10; i++) HAL_SPI_Transmit(&hspi2, &dummy, 1, 10);

    uint8_t cmd0[6] = {0x40, 0, 0, 0, 0, 0x95};
    SD_CS_Low();
    HAL_SPI_Transmit(&hspi2, cmd0, 6, 50);
    uint8_t resp = 0xFF;
    for (int i = 0; i < 8; i++) {
        HAL_SPI_Receive(&hspi2, &resp, 1, 10);
        if (resp != 0xFF) break;
    }
    SD_CS_High();
    return (resp == 0x01);
}

static void SD_WriteEntry(const DistPacket_t *p) {
    /* Simulate write: pulse CS, real impl would use FatFS */
    SD_CS_Low();
    HAL_SPI_Transmit(&hspi2, (uint8_t *)p, 2, 10); /* Just send magic+node_id */
    SD_CS_High();
}

/* Wait for ESP32 handshake high */
static bool SPI1_WaitHS(uint32_t timeout_ms) {
    uint32_t t0 = HAL_GetTick();
    while (HAL_GPIO_ReadPin(HS_PORT, HS_PIN) == GPIO_PIN_RESET) {
        if ((HAL_GetTick() - t0) > timeout_ms) return false;
    }
    return true;
}

/* =========================================================
 * FreeRTOS Tasks
 * ========================================================= */
void vSensorTask(void *pvParameters) {
    (void)pvParameters;
    TickType_t xLastWakeTime = xTaskGetTickCount();
    DistPacket_t pkt;
    uint16_t seq = 0;

    for (;;) {
        seq++;
        sensor_count = seq;

        int16_t  temp = (int16_t)(2500 + (seq % 500));
        uint16_t hum  = (uint16_t)(6000 + (seq % 3000));

        build_dist_packet(&pkt, seq, temp, hum);

        /* Send to both queues */
        if (xQueueSend(xTxQueue, &pkt, 0) != pdTRUE) tx_dropped++;
        xQueueSend(xSDQueue, &pkt, 0);

        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(SENSOR_PERIOD_MS));
    }
}

void vSDLogTask(void *pvParameters) {
    (void)pvParameters;
    DistPacket_t pkt;

    for (;;) {
        if (xQueueReceive(xSDQueue, &pkt, portMAX_DELAY) == pdTRUE) {
            SD_WriteEntry(&pkt);
            sd_count++;
        }
    }
}

void vTxTask(void *pvParameters) {
    (void)pvParameters;
    DistPacket_t pkt;
    uint8_t rx_buf[sizeof(DistPacket_t)];
    char buf[80];

    for (;;) {
        if (xQueueReceive(xTxQueue, &pkt, portMAX_DELAY) == pdTRUE) {
            if (!SPI1_WaitHS(300)) {
                UART_Print("[TX] HS timeout\r\n");
                continue;
            }
            SPI1_CS_Low();
            HAL_SPI_TransmitReceive(&hspi1, (uint8_t *)&pkt, rx_buf,
                                    sizeof(DistPacket_t), 200);
            SPI1_CS_High();
            tx_count++;

            snprintf(buf, sizeof(buf),
                     "[TX#%lu] Seq=%u T=%d.%02d H=%d.%02d ACK=0x%02X\r\n",
                     (unsigned long)tx_count, pkt.seq_num,
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
                 "[MON] Sensor=%lu TX=%lu SD=%lu Drop=%lu Heap=%u\r\n",
                 (unsigned long)sensor_count,
                 (unsigned long)tx_count,
                 (unsigned long)sd_count,
                 (unsigned long)tx_dropped,
                 (unsigned)xPortGetFreeHeapSize());
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
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    /* SPI1 CS */
    HAL_GPIO_WritePin(SPI1_CS_PORT, SPI1_CS_PIN, GPIO_PIN_SET);
    GPIO_InitStruct.Pin   = SPI1_CS_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(SPI1_CS_PORT, &GPIO_InitStruct);

    /* Handshake input */
    GPIO_InitStruct.Pin  = HS_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;
    HAL_GPIO_Init(HS_PORT, &GPIO_InitStruct);

    /* SD CS */
    HAL_GPIO_WritePin(SD_CS_PORT, SD_CS_PIN, GPIO_PIN_SET);
    GPIO_InitStruct.Pin   = SD_CS_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(SD_CS_PORT, &GPIO_InitStruct);

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

    GPIO_InitStruct.Pin   = SPI1_SCK_PIN | SPI1_MOSI_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin  = SPI1_MISO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

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

static void SPI2_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    __HAL_RCC_SPI2_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    GPIO_InitStruct.Pin   = SD_SCK_PIN | SD_MOSI_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    GPIO_InitStruct.Pin  = SD_MISO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    hspi2.Instance               = SPI2;
    hspi2.Init.Mode              = SPI_MODE_MASTER;
    hspi2.Init.Direction         = SPI_DIRECTION_2LINES;
    hspi2.Init.DataSize          = SPI_DATASIZE_8BIT;
    hspi2.Init.CLKPolarity       = SPI_POLARITY_LOW;
    hspi2.Init.CLKPhase          = SPI_PHASE_1EDGE;
    hspi2.Init.NSS               = SPI_NSS_SOFT;
    hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_32;
    hspi2.Init.FirstBit          = SPI_FIRSTBIT_MSB;
    hspi2.Init.TIMode            = SPI_TIMODE_DISABLE;
    hspi2.Init.CRCCalculation    = SPI_CRCCALCULATION_DISABLE;
    HAL_SPI_Init(&hspi2);
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
    SPI2_Init();
    USART1_Init();

    UART_Print("\r\n=== Multi_05 STM32 Distributed Node ===\r\n");
    UART_Print("Node ID: 0x01\r\n");

    /* Init SD Card */
    if (SD_Init()) {
        UART_Print("[SD] Init OK\r\n");
    } else {
        UART_Print("[SD] Init FAILED\r\n");
    }

    /* Create queues */
    xTxQueue = xQueueCreate(TX_QUEUE_LEN, sizeof(DistPacket_t));
    xSDQueue = xQueueCreate(SD_QUEUE_LEN, sizeof(DistPacket_t));

    if (!xTxQueue || !xSDQueue) {
        UART_Print("ERROR: Queue creation failed!\r\n");
        Error_Handler();
    }

    /* Create tasks */
    xTaskCreate(vSensorTask,  "Sensor",  SENSOR_TASK_STACK,  NULL, SENSOR_TASK_PRIORITY,  NULL);
    xTaskCreate(vSDLogTask,   "SDLog",   SD_LOG_TASK_STACK,  NULL, SD_LOG_TASK_PRIORITY,  NULL);
    xTaskCreate(vTxTask,      "Tx",      TX_TASK_STACK,      NULL, TX_TASK_PRIORITY,      NULL);
    xTaskCreate(vMonitorTask, "Monitor", MONITOR_TASK_STACK, NULL, MONITOR_TASK_PRIORITY, NULL);

    UART_Print("Starting scheduler...\r\n");
    vTaskStartScheduler();

    while (1) {}
}
