/* Multi_03_Bidirectional_DataExchange - STM32 side (SPI Master)
 * STM32 bertindak sebagai SPI Master, berkomunikasi dengan ESP32 Slave
 * Protocol:
 *   CMD_READ_SENSOR (0x01): kirim cmd, terima 4 byte (temp_hi, temp_lo, hum_hi, hum_lo)
 *   CMD_WRITE_LED   (0x02): kirim cmd + value (0x00/0x01), terima ACK (0xAA)
 *   CMD_GET_STATUS  (0x03): kirim cmd, terima 2 byte status
 *   CMD_ECHO        (0x04): kirim cmd + byte, terima byte yang sama
 *
 * Hardware (bluepill F103C8):
 *   SPI1 Master: SCK=PA5, MISO=PA6, MOSI=PA7, CS=PA4
 *   Handshake (ESP32 ready): PA3 (input)
 *   LED: PC13 (active LOW)
 *   UART1: PA9(TX), PA10(RX) untuk debug
 */

#include "config.h"
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

/* =========================================================
 * Hardware handles
 * ========================================================= */
SPI_HandleTypeDef  hspi1;
UART_HandleTypeDef huart1;

/* =========================================================
 * UART helper
 * ========================================================= */
static void UART_Print(const char *msg) {
    HAL_UART_Transmit(&huart1, (uint8_t *)msg, (uint16_t)strlen(msg), 200);
}

/* =========================================================
 * SPI Master helpers
 * ========================================================= */
static void SPI_CS_Low(void)  { HAL_GPIO_WritePin(SPI_GPIO_PORT, SPI_CS_PIN, GPIO_PIN_RESET); }
static void SPI_CS_High(void) { HAL_GPIO_WritePin(SPI_GPIO_PORT, SPI_CS_PIN, GPIO_PIN_SET); }

/* Wait for ESP32 handshake pin to go HIGH (ready) */
static bool SPI_WaitReady(uint32_t timeout_ms) {
    uint32_t start = HAL_GetTick();
    while (HAL_GPIO_ReadPin(HANDSHAKE_GPIO_PORT, HANDSHAKE_PIN) == GPIO_PIN_RESET) {
        if ((HAL_GetTick() - start) > timeout_ms) return false;
    }
    return true;
}

static HAL_StatusTypeDef SPI_Transfer(uint8_t *tx, uint8_t *rx, uint16_t len) {
    return HAL_SPI_TransmitReceive(&hspi1, tx, rx, len, 100);
}

/* =========================================================
 * Protocol functions
 * ========================================================= */
typedef struct {
    int16_t temperature_x100; /* e.g. 2512 = 25.12 degC */
    int16_t humidity_x100;    /* e.g. 6234 = 62.34 % */
} SensorPacket_t;

static bool cmd_read_sensor(SensorPacket_t *out) {
    if (!SPI_WaitReady(100)) return false;

    uint8_t tx[5] = {CMD_READ_SENSOR, 0xFF, 0xFF, 0xFF, 0xFF};
    uint8_t rx[5] = {0};

    SPI_CS_Low();
    SPI_Transfer(tx, rx, 5);
    SPI_CS_High();

    out->temperature_x100 = (int16_t)((rx[1] << 8) | rx[2]);
    out->humidity_x100    = (int16_t)((rx[3] << 8) | rx[4]);
    return true;
}

static bool cmd_write_led(uint8_t value) {
    if (!SPI_WaitReady(100)) return false;

    uint8_t tx[2] = {CMD_WRITE_LED, value};
    uint8_t rx[2] = {0};

    SPI_CS_Low();
    SPI_Transfer(tx, rx, 2);
    SPI_CS_High();

    return (rx[1] == 0xAA); /* ACK */
}

static bool cmd_get_status(uint8_t *status_out) {
    if (!SPI_WaitReady(100)) return false;

    uint8_t tx[3] = {CMD_GET_STATUS, 0xFF, 0xFF};
    uint8_t rx[3] = {0};

    SPI_CS_Low();
    SPI_Transfer(tx, rx, 3);
    SPI_CS_High();

    if (status_out) { status_out[0] = rx[1]; status_out[1] = rx[2]; }
    return true;
}

static bool cmd_echo(uint8_t data, uint8_t *echo_out) {
    if (!SPI_WaitReady(100)) return false;

    uint8_t tx[2] = {CMD_ECHO, data};
    uint8_t rx[2] = {0};

    SPI_CS_Low();
    SPI_Transfer(tx, rx, 2);
    SPI_CS_High();

    if (echo_out) *echo_out = rx[1];
    return (rx[1] == data);
}

/* =========================================================
 * Hardware initialization
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

    /* CS pin */
    HAL_GPIO_WritePin(SPI_GPIO_PORT, SPI_CS_PIN, GPIO_PIN_SET);
    GPIO_InitStruct.Pin   = SPI_CS_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(SPI_GPIO_PORT, &GPIO_InitStruct);

    /* Handshake input */
    GPIO_InitStruct.Pin  = HANDSHAKE_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;
    HAL_GPIO_Init(HANDSHAKE_GPIO_PORT, &GPIO_InitStruct);

    /* LED output */
    HAL_GPIO_WritePin(LED_GPIO_PORT, LED_PIN, GPIO_PIN_SET);
    GPIO_InitStruct.Pin   = LED_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_GPIO_PORT, &GPIO_InitStruct);
}

static void SPI1_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_SPI1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    /* SCK, MOSI */
    GPIO_InitStruct.Pin   = SPI_SCK_PIN | SPI_MOSI_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(SPI_GPIO_PORT, &GPIO_InitStruct);

    /* MISO */
    GPIO_InitStruct.Pin  = SPI_MISO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    HAL_GPIO_Init(SPI_GPIO_PORT, &GPIO_InitStruct);

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

    GPIO_InitStruct.Pin   = GPIO_PIN_9; /* TX */
    GPIO_InitStruct.Mode  = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
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

/* =========================================================
 * Main
 * ========================================================= */
int main(void) {
    HAL_Init();
    SystemClock_Config();
    GPIO_Init();
    SPI1_Init();
    USART1_Init();

    UART_Print("\r\n=== Multi_03 STM32 SPI Master ===\r\n");
    UART_Print("Waiting for ESP32 ready signal...\r\n");

    /* Wait for ESP32 to be ready */
    uint32_t wait_start = HAL_GetTick();
    while (HAL_GPIO_ReadPin(HANDSHAKE_GPIO_PORT, HANDSHAKE_PIN) == GPIO_PIN_RESET) {
        if ((HAL_GetTick() - wait_start) > 5000) {
            UART_Print("WARNING: ESP32 not responding, continuing anyway\r\n");
            break;
        }
    }
    UART_Print("ESP32 ready.\r\n");

    uint32_t loop_count = 0;
    char buf[96];

    for (;;) {
        loop_count++;
        snprintf(buf, sizeof(buf), "\r\n--- Cycle %lu ---\r\n", (unsigned long)loop_count);
        UART_Print(buf);

        /* 1. Read sensor data from ESP32 */
        SensorPacket_t sensor;
        if (cmd_read_sensor(&sensor)) {
            snprintf(buf, sizeof(buf),
                     "[READ_SENSOR] Temp=%d.%02d C, Hum=%d.%02d%%\r\n",
                     sensor.temperature_x100 / 100, sensor.temperature_x100 % 100,
                     sensor.humidity_x100 / 100, sensor.humidity_x100 % 100);
            UART_Print(buf);
        } else {
            UART_Print("[READ_SENSOR] TIMEOUT\r\n");
        }

        HAL_Delay(10);

        /* 2. Write LED command (toggle) */
        uint8_t led_val = (loop_count % 2) ? 0x01 : 0x00;
        if (cmd_write_led(led_val)) {
            snprintf(buf, sizeof(buf), "[WRITE_LED] LED=%s, ACK OK\r\n",
                     led_val ? "ON" : "OFF");
            UART_Print(buf);
        } else {
            UART_Print("[WRITE_LED] No ACK\r\n");
        }

        HAL_Delay(10);

        /* 3. Get status */
        uint8_t status[2] = {0};
        if (cmd_get_status(status)) {
            snprintf(buf, sizeof(buf), "[GET_STATUS] 0x%02X 0x%02X\r\n",
                     status[0], status[1]);
            UART_Print(buf);
        } else {
            UART_Print("[GET_STATUS] TIMEOUT\r\n");
        }

        HAL_Delay(10);

        /* 4. Echo test */
        uint8_t echo_val = (uint8_t)(loop_count & 0xFF);
        uint8_t echo_rx  = 0;
        if (cmd_echo(echo_val, &echo_rx)) {
            snprintf(buf, sizeof(buf), "[ECHO] Sent=0x%02X Got=0x%02X OK\r\n",
                     echo_val, echo_rx);
            UART_Print(buf);
        } else {
            snprintf(buf, sizeof(buf), "[ECHO] MISMATCH Sent=0x%02X Got=0x%02X\r\n",
                     echo_val, echo_rx);
            UART_Print(buf);
        }

        /* Toggle local LED */
        HAL_GPIO_TogglePin(LED_GPIO_PORT, LED_PIN);

        HAL_Delay(2000);
    }
}
