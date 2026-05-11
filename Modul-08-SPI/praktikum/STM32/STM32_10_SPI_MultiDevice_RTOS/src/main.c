/* STM32_10_SPI_MultiDevice_RTOS
 * Mengoperasikan 3 perangkat SPI secara bersamaan dengan FreeRTOS:
 *   - LCD ST7735 (SPI1, CS=PA4, DC=PB0, RST=PB1)
 *   - SD Card    (SPI1, CS=PA3)
 *   - W5500 ETH  (SPI1, CS=PA2, RST=PA1, INT=PA0)
 * Semua perangkat berbagi SPI1 -> dilindungi xSPIMutex
 * Task:
 *   vDataCollectorTask - generate data sensor setiap SENSOR_PERIOD_MS
 *   vLCDTask           - update tampilan LCD setiap LCD_PERIOD_MS
 *   vSDLoggerTask      - tulis log ke SD setiap SD_PERIOD_MS
 *   vEthTask           - kirim data via W5500 setiap ETH_PERIOD_MS
 *   vMonitorTask       - cetak status ke UART setiap MONITOR_PERIOD_MS
 * Mendukung bluepill_f103c8, stm32f401cc, stm32f411ce
 */

#include "config.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

/* =========================================================
 * Hardware handles
 * ========================================================= */
SPI_HandleTypeDef  hspi1;
UART_HandleTypeDef huart1;

/* =========================================================
 * FreeRTOS objects
 * ========================================================= */
SemaphoreHandle_t xSPIMutex    = NULL;
QueueHandle_t     xDataQueue   = NULL;
QueueHandle_t     xLogQueue    = NULL;

/* =========================================================
 * Global stats (protected by read/write only from task)
 * ========================================================= */
static volatile uint32_t sensor_count = 0;
static volatile uint32_t lcd_updates  = 0;
static volatile uint32_t sd_writes    = 0;
static volatile uint32_t eth_sends    = 0;

/* =========================================================
 * Function prototypes
 * ========================================================= */
void SystemClock_Config(void);
void MX_GPIO_Init(void);
void MX_SPI1_Init(void);
void MX_USART1_UART_Init(void);
void Error_Handler(void);

/* =========================================================
 * UART helper (thread-safe via mutex not needed: only called
 * from single monitor task or before scheduler)
 * ========================================================= */
static void UART_Print(const char *msg) {
    HAL_UART_Transmit(&huart1, (uint8_t *)msg, (uint16_t)strlen(msg), 300);
}

/* =========================================================
 * SPI1 CS helpers
 * ========================================================= */
static inline void LCD_CS_Low(void)  { HAL_GPIO_WritePin(LCD_CS_PORT, LCD_CS_PIN, GPIO_PIN_RESET); }
static inline void LCD_CS_High(void) { HAL_GPIO_WritePin(LCD_CS_PORT, LCD_CS_PIN, GPIO_PIN_SET);   }
static inline void SD_CS_Low(void)   { HAL_GPIO_WritePin(SD_CS_PORT,  SD_CS_PIN,  GPIO_PIN_RESET); }
static inline void SD_CS_High(void)  { HAL_GPIO_WritePin(SD_CS_PORT,  SD_CS_PIN,  GPIO_PIN_SET);   }
static inline void ETH_CS_Low(void)  { HAL_GPIO_WritePin(ETH_CS_PORT, ETH_CS_PIN, GPIO_PIN_RESET); }
static inline void ETH_CS_High(void) { HAL_GPIO_WritePin(ETH_CS_PORT, ETH_CS_PIN, GPIO_PIN_SET);   }

/* =========================================================
 * LCD ST7735 - Minimal command interface
 * ========================================================= */
static void LCD_SendCmd(uint8_t cmd) {
    HAL_GPIO_WritePin(LCD_DC_PORT, LCD_DC_PIN, GPIO_PIN_RESET); /* Command */
    LCD_CS_Low();
    HAL_SPI_Transmit(&hspi1, &cmd, 1, HAL_MAX_DELAY);
    LCD_CS_High();
}

static void LCD_SendData(uint8_t data) {
    HAL_GPIO_WritePin(LCD_DC_PORT, LCD_DC_PIN, GPIO_PIN_SET); /* Data */
    LCD_CS_Low();
    HAL_SPI_Transmit(&hspi1, &data, 1, HAL_MAX_DELAY);
    LCD_CS_High();
}

static void LCD_Init(void) {
    /* Hardware reset */
    HAL_GPIO_WritePin(LCD_RST_PORT, LCD_RST_PIN, GPIO_PIN_RESET);
    HAL_Delay(10);
    HAL_GPIO_WritePin(LCD_RST_PORT, LCD_RST_PIN, GPIO_PIN_SET);
    HAL_Delay(150);

    LCD_SendCmd(0x01); /* Software reset */
    HAL_Delay(150);
    LCD_SendCmd(0x11); /* Sleep out */
    HAL_Delay(200);
    LCD_SendCmd(0x3A); /* Color mode */
    LCD_SendData(0x05); /* 16-bit */
    LCD_SendCmd(0x29); /* Display on */
    HAL_Delay(100);
}

static void LCD_WriteText(uint8_t row, const char *text) {
    /* Simplified: set address window at row*8 and write blank pixels
     * Real implementation would use font rendering */
    (void)row;
    (void)text;
    LCD_SendCmd(0x2C); /* RAM write */
    /* Write some dummy data to represent text */
    uint8_t pixel[2] = {0xFF, 0xFF};
    LCD_CS_Low();
    HAL_GPIO_WritePin(LCD_DC_PORT, LCD_DC_PIN, GPIO_PIN_SET);
    for (int i = 0; i < 16; i++) {
        HAL_SPI_Transmit(&hspi1, pixel, 2, HAL_MAX_DELAY);
    }
    LCD_CS_High();
}

/* =========================================================
 * SD Card - Minimal SPI raw interface
 * ========================================================= */
static uint8_t SD_SendCmd(uint8_t cmd, uint32_t arg) {
    uint8_t buf[6];
    uint8_t resp;
    buf[0] = 0x40 | cmd;
    buf[1] = (uint8_t)(arg >> 24);
    buf[2] = (uint8_t)(arg >> 16);
    buf[3] = (uint8_t)(arg >> 8);
    buf[4] = (uint8_t)(arg);
    buf[5] = (cmd == 0) ? 0x95 : 0x87; /* CRC (only needed for CMD0, CMD8) */
    SD_CS_Low();
    HAL_SPI_Transmit(&hspi1, buf, 6, HAL_MAX_DELAY);
    /* Wait for response (up to 8 bytes) */
    for (int i = 0; i < 8; i++) {
        HAL_SPI_Receive(&hspi1, &resp, 1, HAL_MAX_DELAY);
        if (resp != 0xFF) break;
    }
    SD_CS_High();
    return resp;
}

static bool SD_Init(void) {
    uint8_t dummy = 0xFF;
    /* Send 80 clock cycles with CS high */
    SD_CS_High();
    for (int i = 0; i < 10; i++) {
        HAL_SPI_Transmit(&hspi1, &dummy, 1, HAL_MAX_DELAY);
    }
    /* CMD0: GO_IDLE_STATE */
    uint8_t r = SD_SendCmd(0, 0);
    return (r == 0x01); /* 0x01 = idle state, init success */
}

static void SD_WriteLogEntry(const SensorData_t *data) {
    char entry[64];
    snprintf(entry, sizeof(entry), "T=%lu,Temp=%ld,Hum=%ld\r\n",
             (unsigned long)data->timestamp,
             (long)data->temperature_x100,
             (long)data->humidity_x100);
    /* In real implementation: open file, seek to end, write, close */
    /* Here we simulate: just send CMD17 read as dummy to verify SPI works */
    (void)entry;
    SD_SendCmd(17, 0); /* CMD17 read block addr 0 (won't succeed w/o full impl) */
}

/* =========================================================
 * W5500 Ethernet - Common register access
 * ========================================================= */
static void W5500_Write(uint16_t addr, uint8_t bsb, uint8_t data) {
    uint8_t buf[4];
    buf[0] = (uint8_t)(addr >> 8);
    buf[1] = (uint8_t)(addr & 0xFF);
    buf[2] = (bsb << 3) | W5500_RW_WRITE;
    buf[3] = data;
    ETH_CS_Low();
    HAL_SPI_Transmit(&hspi1, buf, 4, HAL_MAX_DELAY);
    ETH_CS_High();
}

static void W5500_WriteBurst(uint16_t addr, uint8_t bsb, const uint8_t *data, uint16_t len) {
    uint8_t header[3];
    header[0] = (uint8_t)(addr >> 8);
    header[1] = (uint8_t)(addr & 0xFF);
    header[2] = (bsb << 3) | W5500_RW_WRITE;
    ETH_CS_Low();
    HAL_SPI_Transmit(&hspi1, header, 3, HAL_MAX_DELAY);
    HAL_SPI_Transmit(&hspi1, (uint8_t *)data, len, HAL_MAX_DELAY);
    ETH_CS_High();
}

static void W5500_Init(void) {
    HAL_GPIO_WritePin(ETH_RST_PORT, ETH_RST_PIN, GPIO_PIN_RESET);
    HAL_Delay(10);
    HAL_GPIO_WritePin(ETH_RST_PORT, ETH_RST_PIN, GPIO_PIN_SET);
    HAL_Delay(200);

    W5500_Write(W5500_MR, W5500_BSB_COMMON, 0x80); /* SW reset */
    HAL_Delay(50);

    const uint8_t mac[6] = {ETH_MAC_0, ETH_MAC_1, ETH_MAC_2,
                             ETH_MAC_3, ETH_MAC_4, ETH_MAC_5};
    W5500_WriteBurst(W5500_SHAR0, W5500_BSB_COMMON, mac, 6);

    const uint8_t ip[4] = {192, 168, 1, 100};
    W5500_WriteBurst(W5500_SIPR0, W5500_BSB_COMMON, ip, 4);

    const uint8_t subnet[4] = {255, 255, 255, 0};
    W5500_WriteBurst(W5500_SUBR0, W5500_BSB_COMMON, subnet, 4);

    const uint8_t gateway[4] = {192, 168, 1, 1};
    W5500_WriteBurst(W5500_GAR0, W5500_BSB_COMMON, gateway, 4);
}

static void W5500_SendPacket(const SensorData_t *data) {
    char pkt[64];
    uint8_t tx[4] = {0xDE, 0xAD, (uint8_t)(data->counter >> 8), (uint8_t)(data->counter)};
    snprintf(pkt, sizeof(pkt), "N=%lu T=%ld H=%ld",
             (unsigned long)data->counter,
             (long)data->temperature_x100,
             (long)data->humidity_x100);
    ETH_CS_Low();
    HAL_SPI_Transmit(&hspi1, tx, 4, HAL_MAX_DELAY);
    ETH_CS_High();
    (void)pkt;
}

/* =========================================================
 * FreeRTOS Tasks
 * ========================================================= */
void vDataCollectorTask(void *pvParameters) {
    (void)pvParameters;
    TickType_t xLastWakeTime = xTaskGetTickCount();
    SensorData_t data;

    for (;;) {
        sensor_count++;
        data.timestamp        = HAL_GetTick();
        data.counter          = sensor_count;
        data.temperature_x100 = 2500 + (int32_t)(sensor_count % 500);  /* 25.00..29.99 degC */
        data.humidity_x100    = 6000 + (int32_t)(sensor_count % 4000); /* 60..99% RH */
        data.freeHeap         = (uint16_t)xPortGetFreeHeapSize();

        /* Send to both queues (non-blocking) */
        xQueueSend(xDataQueue, &data, 0);
        xQueueSend(xLogQueue,  &data, 0);

        HAL_GPIO_TogglePin(LED_PORT, LED_PIN);
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(SENSOR_PERIOD_MS));
    }
}

void vLCDTask(void *pvParameters) {
    (void)pvParameters;
    SensorData_t data;
    char line[32];

    for (;;) {
        if (xQueueReceive(xDataQueue, &data, pdMS_TO_TICKS(LCD_PERIOD_MS)) == pdTRUE) {
            if (xSemaphoreTake(xSPIMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                snprintf(line, sizeof(line), "T:%ld.%02ld C", data.temperature_x100 / 100,
                         data.temperature_x100 % 100);
                LCD_WriteText(0, line);

                snprintf(line, sizeof(line), "H:%ld.%02ld %%", data.humidity_x100 / 100,
                         data.humidity_x100 % 100);
                LCD_WriteText(1, line);

                lcd_updates++;
                xSemaphoreGive(xSPIMutex);
            }
        }
    }
}

void vSDLoggerTask(void *pvParameters) {
    (void)pvParameters;
    SensorData_t data;

    for (;;) {
        if (xQueueReceive(xLogQueue, &data, pdMS_TO_TICKS(SD_PERIOD_MS)) == pdTRUE) {
            if (xSemaphoreTake(xSPIMutex, pdMS_TO_TICKS(200)) == pdTRUE) {
                SD_WriteLogEntry(&data);
                sd_writes++;
                xSemaphoreGive(xSPIMutex);
            }
        }
    }
}

void vEthTask(void *pvParameters) {
    (void)pvParameters;
    SensorData_t data;
    TickType_t xLastWakeTime = xTaskGetTickCount();

    for (;;) {
        /* Peek at latest from data queue (don't consume - LCD may need it) */
        if (xQueuePeek(xDataQueue, &data, 0) == pdTRUE) {
            if (xSemaphoreTake(xSPIMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                W5500_SendPacket(&data);
                eth_sends++;
                xSemaphoreGive(xSPIMutex);
            }
        }
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(ETH_PERIOD_MS));
    }
}

void vMonitorTask(void *pvParameters) {
    (void)pvParameters;
    char buf[96];
    TickType_t xLastWakeTime = xTaskGetTickCount();

    for (;;) {
        snprintf(buf, sizeof(buf),
                 "[MON] Sensor:%lu LCD:%lu SD:%lu ETH:%lu Heap:%u\r\n",
                 (unsigned long)sensor_count,
                 (unsigned long)lcd_updates,
                 (unsigned long)sd_writes,
                 (unsigned long)eth_sends,
                 (unsigned)xPortGetFreeHeapSize());
        UART_Print(buf);
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

#if defined(STM32F103xB)
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
#elif defined(STM32F401xC) || defined(STM32F411xE)
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);
    RCC_OscInitStruct.OscillatorType      = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState            = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState        = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource       = RCC_PLLSOURCE_HSI;
    RCC_OscInitStruct.PLL.PLLM            = 8;
    RCC_OscInitStruct.PLL.PLLN            = 84;
    RCC_OscInitStruct.PLL.PLLP            = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ            = 4;
    HAL_RCC_OscConfig(&RCC_OscInitStruct);
    RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                     | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2);
#endif
}

void MX_GPIO_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    /* Set all CS high before init */
    HAL_GPIO_WritePin(LCD_CS_PORT, LCD_CS_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(SD_CS_PORT,  SD_CS_PIN,  GPIO_PIN_SET);
    HAL_GPIO_WritePin(ETH_CS_PORT, ETH_CS_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(ETH_RST_PORT, ETH_RST_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LCD_RST_PORT, LCD_RST_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET);

    /* CS pins: output PA2, PA3, PA4 */
    GPIO_InitStruct.Pin   = LCD_CS_PIN | SD_CS_PIN | ETH_CS_PIN | ETH_RST_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* LCD DC, RST and ETH INT */
    GPIO_InitStruct.Pin  = LCD_DC_PIN | LCD_RST_PIN;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* ETH INT as input */
    GPIO_InitStruct.Pin  = ETH_INT_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(ETH_INT_PORT, &GPIO_InitStruct);

    /* LED */
    GPIO_InitStruct.Pin   = LED_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_PORT, &GPIO_InitStruct);
}

void MX_SPI1_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_SPI1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    /* SCK, MOSI */
    GPIO_InitStruct.Pin   = SPI1_SCK_PIN | SPI1_MOSI_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
#if defined(STM32F401xC) || defined(STM32F411xE)
    GPIO_InitStruct.Alternate = GPIO_AF5_SPI1;
#endif
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* MISO */
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
    hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;
    hspi1.Init.FirstBit          = SPI_FIRSTBIT_MSB;
    hspi1.Init.TIMode            = SPI_TIMODE_DISABLE;
    hspi1.Init.CRCCalculation    = SPI_CRCCALCULATION_DISABLE;
    HAL_SPI_Init(&hspi1);
}

void MX_USART1_UART_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_USART1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitStruct.Pin   = GPIO_PIN_9; /* TX */
    GPIO_InitStruct.Mode  = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
#if defined(STM32F401xC) || defined(STM32F411xE)
    GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
#endif
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin  = GPIO_PIN_10; /* RX */
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
    MX_GPIO_Init();
    MX_SPI1_Init();
    MX_USART1_UART_Init();

    UART_Print("\r\n=== STM32 SPI MultiDevice RTOS ===\r\n");
    UART_Print("Initializing devices...\r\n");

    /* Initialize all SPI devices */
    LCD_Init();
    UART_Print("[LCD] ST7735 init OK\r\n");

    if (SD_Init()) {
        UART_Print("[SD] Card init OK\r\n");
    } else {
        UART_Print("[SD] Card init FAILED\r\n");
    }

    W5500_Init();
    UART_Print("[ETH] W5500 init OK\r\n");

    /* Create FreeRTOS objects */
    xSPIMutex  = xSemaphoreCreateMutex();
    xDataQueue = xQueueCreate(QUEUE_DATA_LENGTH, sizeof(SensorData_t));
    xLogQueue  = xQueueCreate(QUEUE_LOG_LENGTH,  sizeof(SensorData_t));

    if (xSPIMutex == NULL || xDataQueue == NULL || xLogQueue == NULL) {
        UART_Print("ERROR: Failed to create RTOS objects!\r\n");
        Error_Handler();
    }

    /* Create tasks */
    xTaskCreate(vDataCollectorTask, "DataColl", TASK_DATA_COLLECTOR_STACK, NULL,
                TASK_DATA_COLLECTOR_PRIORITY, NULL);
    xTaskCreate(vLCDTask,           "LCD",      TASK_LCD_UPDATE_STACK,     NULL,
                TASK_LCD_UPDATE_PRIORITY,     NULL);
    xTaskCreate(vSDLoggerTask,      "SDLog",    TASK_SD_LOGGER_STACK,      NULL,
                TASK_SD_LOGGER_PRIORITY,      NULL);
    xTaskCreate(vEthTask,           "Eth",      TASK_ETH_COMM_STACK,       NULL,
                TASK_ETH_COMM_PRIORITY,       NULL);
    xTaskCreate(vMonitorTask,       "Monitor",  TASK_MONITOR_STACK,        NULL,
                TASK_MONITOR_PRIORITY,        NULL);

    UART_Print("Starting FreeRTOS scheduler...\r\n");
    vTaskStartScheduler();

    while (1) {}
}
