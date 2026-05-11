/* STM32_09_SPI_Ethernet_RTOS_Semaphore
 * W5500 Ethernet dengan FreeRTOS Semaphore untuk koordinasi TX/RX
 * Task 1: vEthTxTask - kirim data via SPI setiap TX_INTERVAL_MS (tunggu semaphore)
 * Task 2: vEthRxTask - cek penerimaan data setiap RX_CHECK_MS
 * Task 3: vAppTask   - logic aplikasi, generate data, beri semaphore TX
 * Mendukung bluepill_f103c8, stm32f401cc, stm32f411ce
 */

#include "config.h"
#include "FreeRTOS.h"
#include <stdbool.h>
#include "task.h"
#include "semphr.h"
#include <stdio.h>
#include <string.h>

/* =========================================================
 * Hardware handles
 * ========================================================= */
SPI_HandleTypeDef  hspi1;
UART_HandleTypeDef huart1;

/* =========================================================
 * FreeRTOS semaphores
 * ========================================================= */
SemaphoreHandle_t xTxSemaphore  = NULL;
SemaphoreHandle_t xRxSemaphore  = NULL;
SemaphoreHandle_t xSPIMutex     = NULL;

/* =========================================================
 * Statistics
 * ========================================================= */
static volatile uint32_t tx_count   = 0;
static volatile uint32_t rx_count   = 0;
static volatile uint32_t app_count  = 0;
static volatile bool     eth_ready  = false;

/* =========================================================
 * Function prototypes
 * ========================================================= */
void SystemClock_Config(void);
void MX_GPIO_Init(void);
void MX_SPI1_Init(void);
void MX_USART1_UART_Init(void);
void Error_Handler(void);

/* =========================================================
 * UART helper
 * ========================================================= */
static void UART_Print(const char *msg) {
    HAL_UART_Transmit(&huart1, (uint8_t *)msg, (uint16_t)strlen(msg), 200);
}

/* =========================================================
 * W5500 SPI raw access
 * BSB = Block Select Byte: (block_sel << 3) | rw_bit
 * For common registers: block_sel = 0
 * RW bit: 0 = read, 1 = write (bit 2 of control byte)
 * ========================================================= */
static void W5500_CS_Low(void)  { HAL_GPIO_WritePin(ETH_CS_PORT, ETH_CS_PIN, GPIO_PIN_RESET); }
static void W5500_CS_High(void) { HAL_GPIO_WritePin(ETH_CS_PORT, ETH_CS_PIN, GPIO_PIN_SET);   }

static void W5500_Write(uint16_t addr, uint8_t bsb, uint8_t data) {
    uint8_t buf[4];
    buf[0] = (uint8_t)(addr >> 8);
    buf[1] = (uint8_t)(addr & 0xFF);
    buf[2] = (bsb << 3) | W5500_RW_WRITE;
    buf[3] = data;
    W5500_CS_Low();
    HAL_SPI_Transmit(&hspi1, buf, 4, HAL_MAX_DELAY);
    W5500_CS_High();
}

static uint8_t W5500_Read(uint16_t addr, uint8_t bsb) {
    uint8_t buf[3];
    uint8_t result = 0;
    buf[0] = (uint8_t)(addr >> 8);
    buf[1] = (uint8_t)(addr & 0xFF);
    buf[2] = (bsb << 3) | W5500_RW_READ;
    W5500_CS_Low();
    HAL_SPI_Transmit(&hspi1, buf, 3, HAL_MAX_DELAY);
    HAL_SPI_Receive(&hspi1, &result, 1, HAL_MAX_DELAY);
    W5500_CS_High();
    return result;
}

/* Write multiple bytes (burst) */
static void W5500_WriteBurst(uint16_t addr, uint8_t bsb, const uint8_t *data, uint16_t len) {
    uint8_t header[3];
    header[0] = (uint8_t)(addr >> 8);
    header[1] = (uint8_t)(addr & 0xFF);
    header[2] = (bsb << 3) | W5500_RW_WRITE;
    W5500_CS_Low();
    HAL_SPI_Transmit(&hspi1, header, 3, HAL_MAX_DELAY);
    HAL_SPI_Transmit(&hspi1, (uint8_t *)data, len, HAL_MAX_DELAY);
    W5500_CS_High();
}

/* =========================================================
 * W5500 Initialization
 * ========================================================= */
static void W5500_Init(void) {
    /* Hardware reset */
    HAL_GPIO_WritePin(ETH_RST_PORT, ETH_RST_PIN, GPIO_PIN_RESET);
    HAL_Delay(10);
    HAL_GPIO_WritePin(ETH_RST_PORT, ETH_RST_PIN, GPIO_PIN_SET);
    HAL_Delay(200);

    /* Software reset via MR register bit 7 */
    W5500_Write(W5500_MR, W5500_BSB_COMMON, 0x80);
    HAL_Delay(50);

    /* Write MAC address */
    const uint8_t mac[6] = {ETH_MAC_0, ETH_MAC_1, ETH_MAC_2,
                             ETH_MAC_3, ETH_MAC_4, ETH_MAC_5};
    W5500_WriteBurst(W5500_SHAR0, W5500_BSB_COMMON, mac, 6);

    /* Write IP: 192.168.1.100 */
    const uint8_t ip[4] = {192, 168, 1, 100};
    W5500_WriteBurst(W5500_SIPR0, W5500_BSB_COMMON, ip, 4);

    /* Write Subnet: 255.255.255.0 */
    const uint8_t subnet[4] = {255, 255, 255, 0};
    W5500_WriteBurst(W5500_SUBR0, W5500_BSB_COMMON, subnet, 4);

    /* Write Gateway: 192.168.1.1 */
    const uint8_t gateway[4] = {192, 168, 1, 1};
    W5500_WriteBurst(W5500_GAR0, W5500_BSB_COMMON, gateway, 4);

    eth_ready = true;
    UART_Print("[W5500] Initialized OK. IP=192.168.1.100\r\n");
}

/* =========================================================
 * FreeRTOS Tasks
 * ========================================================= */
void vEthTxTask(void *pvParameters) {
    (void)pvParameters;
    char buf[64];
    uint8_t tx_data[16];

    for (;;) {
        /* Wait for application to signal data ready */
        if (xSemaphoreTake(xTxSemaphore, portMAX_DELAY) == pdTRUE) {
            /* Acquire SPI mutex */
            if (xSemaphoreTake(xSPIMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                /* Simulate SPI Ethernet TX: toggle CS and send dummy packet */
                tx_count++;
                tx_data[0] = (uint8_t)(tx_count >> 8);
                tx_data[1] = (uint8_t)(tx_count & 0xFF);
                tx_data[2] = 0xDE; tx_data[3] = 0xAD;

                W5500_CS_Low();
                HAL_SPI_Transmit(&hspi1, tx_data, 4, HAL_MAX_DELAY);
                W5500_CS_High();

                xSemaphoreGive(xSPIMutex);

                snprintf(buf, sizeof(buf), "[TX] Packet #%lu sent\r\n",
                         (unsigned long)tx_count);
                UART_Print(buf);

                /* Signal RX task to check for response */
                xSemaphoreGive(xRxSemaphore);
            }
        }
    }
}

void vEthRxTask(void *pvParameters) {
    (void)pvParameters;
    char buf[64];
    uint8_t rx_data[4];

    for (;;) {
        if (xSemaphoreTake(xRxSemaphore, pdMS_TO_TICKS(RX_CHECK_MS)) == pdTRUE) {
            /* Acquire SPI to read from W5500 */
            if (xSemaphoreTake(xSPIMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
                rx_count++;
                /* Simulate read from W5500 socket RX register */
                W5500_CS_Low();
                HAL_SPI_Receive(&hspi1, rx_data, 4, HAL_MAX_DELAY);
                W5500_CS_High();

                xSemaphoreGive(xSPIMutex);

                snprintf(buf, sizeof(buf), "[RX] Response #%lu received\r\n",
                         (unsigned long)rx_count);
                UART_Print(buf);
            }
        }
    }
}

void vAppTask(void *pvParameters) {
    (void)pvParameters;
    char buf[80];
    TickType_t xLastWakeTime = xTaskGetTickCount();

    for (;;) {
        app_count++;

        /* Generate application data and signal TX */
        snprintf(buf, sizeof(buf),
                 "[APP] #%lu | TX=%lu RX=%lu | ETH=%s\r\n",
                 (unsigned long)app_count,
                 (unsigned long)tx_count,
                 (unsigned long)rx_count,
                 eth_ready ? "UP" : "DOWN");
        UART_Print(buf);

        /* Toggle LED */
        HAL_GPIO_TogglePin(LED_PORT, LED_PIN);

        /* Signal TX task to send a packet */
        xSemaphoreGive(xTxSemaphore);

        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(APP_UPDATE_MS));
    }
}

/* =========================================================
 * SysTick Handler (FreeRTOS integration)
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

    /* LED */
    HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET);
    GPIO_InitStruct.Pin   = LED_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_PORT, &GPIO_InitStruct);

    /* ETH CS, RST */
    HAL_GPIO_WritePin(ETH_CS_PORT,  ETH_CS_PIN,  GPIO_PIN_SET);
    HAL_GPIO_WritePin(ETH_RST_PORT, ETH_RST_PIN, GPIO_PIN_SET);
    GPIO_InitStruct.Pin   = ETH_CS_PIN | ETH_RST_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* ETH INT as input */
    GPIO_InitStruct.Pin  = ETH_INT_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(ETH_INT_PORT, &GPIO_InitStruct);
}

void MX_SPI1_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_SPI1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    /* PA5=SCK, PA7=MOSI */
    GPIO_InitStruct.Pin   = SPI1_SCK_PIN | SPI1_MOSI_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
#if defined(STM32F401xC) || defined(STM32F411xE)
    GPIO_InitStruct.Alternate = GPIO_AF5_SPI1;
#endif
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* PA6=MISO */
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

    UART_Print("\r\n=== STM32 SPI Ethernet RTOS Semaphore ===\r\n");

    /* Initialize W5500 */
    W5500_Init();

    /* Create semaphores */
    xTxSemaphore = xSemaphoreCreateBinary();
    xRxSemaphore = xSemaphoreCreateBinary();
    xSPIMutex    = xSemaphoreCreateMutex();

    if (xTxSemaphore == NULL || xRxSemaphore == NULL || xSPIMutex == NULL) {
        UART_Print("ERROR: Failed to create semaphores!\r\n");
        Error_Handler();
    }

    /* Create tasks */
    xTaskCreate(vEthTxTask, "EthTX", ETH_TX_TASK_STACK, NULL, ETH_TX_TASK_PRIORITY, NULL);
    xTaskCreate(vEthRxTask, "EthRX", ETH_RX_TASK_STACK, NULL, ETH_RX_TASK_PRIORITY, NULL);
    xTaskCreate(vAppTask,   "App",   APP_TASK_STACK,     NULL, APP_TASK_PRIORITY,     NULL);

    UART_Print("Starting FreeRTOS scheduler...\r\n");
    vTaskStartScheduler();

    while (1) {}
}
