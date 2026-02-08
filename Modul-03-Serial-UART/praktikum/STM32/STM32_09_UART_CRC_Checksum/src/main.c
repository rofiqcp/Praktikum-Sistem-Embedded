/**
 * STM32_09_UART_CRC_Checksum
 * 
 * CRC-8 calculation and verification over UART.
 * Protocol: [LENGTH][DATA...][CRC8]
 * CRC-8 polynomial=0x07, init=0x00
 * 
 * Self-test on startup, then receives data via UART interrupt
 * and validates CRC on each received packet.
 */

#include "config.h"

#ifdef STM32F103xB
  #include "stm32f1xx_hal.h"
#elif defined(STM32F401xC)
  #include "stm32f4xx_hal.h"
#elif defined(STM32F411xE)
  #include "stm32f4xx_hal.h"
#else
  #error "Unsupported MCU"
#endif

#include <stdio.h>
#include <string.h>

/* ==================== Global Variables ==================== */
UART_HandleTypeDef huart1;

/* RX state machine */
static uint8_t rx_byte;
static uint8_t rx_buffer[MAX_DATA + 2];  /* LENGTH + DATA + CRC */
static volatile uint16_t rx_index = 0;
static volatile uint8_t rx_expected_len = 0;
static volatile uint8_t rx_complete = 0;

/* ==================== CRC-8 Functions ==================== */

/**
 * Calculate CRC-8 over data buffer
 * Polynomial: 0x07, Init: 0x00
 */
uint8_t crc8_calc(const uint8_t *data, uint16_t len)
{
    uint8_t crc = CRC_INIT;
    
    for (uint16_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t bit = 0; bit < 8; bit++) {
            if (crc & 0x80) {
                crc = (crc << 1) ^ CRC_POLY;
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}

/**
 * Send data with CRC-8 appended
 * Format: [LENGTH][DATA...][CRC8]
 */
void send_with_crc(const uint8_t *data, uint8_t len)
{
    uint8_t packet[MAX_DATA + 2];
    
    if (len > MAX_DATA) len = MAX_DATA;
    
    packet[0] = len;                          /* LENGTH byte */
    memcpy(&packet[1], data, len);            /* DATA bytes */
    
    uint8_t crc = crc8_calc(data, len);
    packet[1 + len] = crc;                    /* CRC8 byte */
    
    HAL_UART_Transmit(&huart1, packet, len + 2, HAL_MAX_DELAY);
    
    printf("TX: [LEN=%u] Data:", len);
    for (uint8_t i = 0; i < len; i++) {
        printf(" %02X", data[i]);
    }
    printf(" [CRC=0x%02X]\r\n", crc);
}

/**
 * Verify CRC of received packet
 * rx_buffer contains: [LENGTH][DATA...][CRC8]
 */
uint8_t verify_received_crc(void)
{
    uint8_t len = rx_buffer[0];
    uint8_t *data = &rx_buffer[1];
    uint8_t received_crc = rx_buffer[1 + len];
    uint8_t calculated_crc = crc8_calc(data, len);
    
    if (received_crc == calculated_crc) {
        return 1;  /* CRC valid */
    }
    return 0;  /* CRC mismatch */
}

/* ==================== UART Callback ==================== */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1) {
        if (rx_complete) {
            /* Previous packet not yet processed, ignore */
            HAL_UART_Receive_IT(&huart1, &rx_byte, 1);
            return;
        }
        
        if (rx_index == 0) {
            /* First byte = LENGTH */
            rx_buffer[0] = rx_byte;
            rx_expected_len = rx_byte;
            rx_index = 1;
        } else {
            rx_buffer[rx_index] = rx_byte;
            rx_index++;
            
            /* Check if we received LENGTH + DATA + CRC */
            if (rx_index >= (uint16_t)(rx_expected_len + 2)) {
                rx_complete = 1;
            }
        }
        
        HAL_UART_Receive_IT(&huart1, &rx_byte, 1);
    }
}

/* ==================== System Configuration ==================== */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

#ifdef STM32F103xB
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
    HAL_RCC_OscConfig(&RCC_OscInitStruct);

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                  RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2);
#else
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = 25;
    RCC_OscInitStruct.PLL.PLLN = 168;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ = 4;
    HAL_RCC_OscConfig(&RCC_OscInitStruct);

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                  RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2);
#endif
}

static void MX_USART1_UART_Init(void)
{
    huart1.Instance = USART1;
    huart1.Init.BaudRate = UART_BAUD;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(&huart1);
}

void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    if (huart->Instance == USART1) {
        __HAL_RCC_USART1_CLK_ENABLE();

#ifdef STM32F103xB
        __HAL_RCC_GPIOA_CLK_ENABLE();

        /* PA9 = TX (AF Push-Pull) */
        GPIO_InitStruct.Pin = GPIO_PIN_9;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

        /* PA10 = RX (Input) */
        GPIO_InitStruct.Pin = GPIO_PIN_10;
        GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
        GPIO_InitStruct.Pull = GPIO_PULLUP;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
#else
        __HAL_RCC_GPIOA_CLK_ENABLE();

        GPIO_InitStruct.Pin = GPIO_PIN_9 | GPIO_PIN_10;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_PULLUP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
#endif

        HAL_NVIC_SetPriority(USART1_IRQn, 0, 0);
        HAL_NVIC_EnableIRQ(USART1_IRQn);
    }
}

/* ==================== IRQ Handler ==================== */
void USART1_IRQHandler(void)
{
    HAL_UART_IRQHandler(&huart1);
}

/* ==================== Self-Test ==================== */
static void run_self_test(void)
{
    printf("\r\n===== CRC-8 Self-Test =====\r\n");
    printf("Polynomial: 0x%02X, Init: 0x%02X\r\n", CRC_POLY, CRC_INIT);

    /* Test 1: Known data */
    uint8_t test1[] = {0x01, 0x02, 0x03, 0x04};
    uint8_t crc1 = crc8_calc(test1, sizeof(test1));
    printf("Test 1: Data={01 02 03 04} CRC=0x%02X\r\n", crc1);

    /* Test 2: "Hello" */
    uint8_t test2[] = "Hello";
    uint8_t crc2 = crc8_calc(test2, 5);
    printf("Test 2: Data=\"Hello\" CRC=0x%02X\r\n", crc2);

    /* Test 3: Verify consistency */
    uint8_t crc2_again = crc8_calc(test2, 5);
    printf("Test 3: Repeat CRC = 0x%02X %s\r\n", crc2_again,
           (crc2 == crc2_again) ? "[PASS]" : "[FAIL]");

    /* Test 4: Single byte */
    uint8_t test4[] = {0xAA};
    uint8_t crc4 = crc8_calc(test4, 1);
    printf("Test 4: Data={AA} CRC=0x%02X\r\n", crc4);

    /* Test 5: Send a packet with CRC */
    printf("\r\nSending test packet with CRC...\r\n");
    send_with_crc(test1, sizeof(test1));

    printf("===========================\r\n\r\n");
    printf("Ready to receive packets: [LEN][DATA...][CRC8]\r\n\r\n");
}

/* ==================== Retarget printf ==================== */
int _write(int file, char *ptr, int len)
{
    HAL_UART_Transmit(&huart1, (uint8_t *)ptr, len, HAL_MAX_DELAY);
    return len;
}

void Error_Handler(void)
{
    while (1) {
        /* Stay here */
    }
}

/* ==================== Main ==================== */
int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_USART1_UART_Init();

    /* Run self-test */
    run_self_test();

    /* Start receiving via interrupt */
    HAL_UART_Receive_IT(&huart1, &rx_byte, 1);

    while (1) {
        if (rx_complete) {
            uint8_t len = rx_buffer[0];
            
            printf("RX: [LEN=%u] Data:", len);
            for (uint8_t i = 0; i < len; i++) {
                printf(" %02X", rx_buffer[1 + i]);
            }
            printf(" [CRC=0x%02X]", rx_buffer[1 + len]);
            
            if (verify_received_crc()) {
                printf(" -> CRC OK\r\n");
            } else {
                uint8_t expected = crc8_calc(&rx_buffer[1], len);
                printf(" -> CRC FAIL (expected 0x%02X)\r\n", expected);
            }
            
            /* Reset for next packet */
            rx_index = 0;
            rx_expected_len = 0;
            rx_complete = 0;
        }
    }
}
