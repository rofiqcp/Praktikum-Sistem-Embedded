/**
 * ============================================================================
 * File        : main.c
 * Program     : STM32_05_SPI_MCP3208_ADC
 * Description : MCP3208 8-channel 12-bit ADC driver via SPI1
 * 
 * Board       : STM32F103C8 (Blue Pill)
 * Framework   : STM32Cube HAL
 * 
 * Wiring:
 *   PA5  -> MCP3208 CLK  (SPI1 SCK)
 *   PA7  -> MCP3208 DIN  (SPI1 MOSI)
 *   PA6  <- MCP3208 DOUT (SPI1 MISO)
 *   PA4  -> MCP3208 CS   (GPIO output, active low)
 *   PA9  -> USB-TTL RX   (USART1 TX)
 *   PA10 <- USB-TTL TX   (USART1 RX)
 *   PC13 -> Onboard LED
 *
 * MCP3208 SPI Protocol:
 *   TX: [0x06|(ch>>2)] [(ch&0x03)<<6] [0x00]
 *   RX: [x] [xxxx_D11..D8] [D7..D0]
 *   Result = ((rx[1] & 0x0F) << 8) | rx[2]
 * ============================================================================
 */

#include "stm32f1xx_hal.h"
#include "config.h"
#include <stdio.h>
#include <string.h>

/* ==================== Global Handles ==================== */
SPI_HandleTypeDef  hspi1;
UART_HandleTypeDef huart1;

/* ==================== Function Prototypes ==================== */
void SystemClock_Config(void);
static void GPIO_Init(void);
static void UART1_Init(void);
static void SPI1_Init(void);
static void MCP3208_CS_Init(void);

uint16_t mcp3208_read_channel(uint8_t channel);
float    mcp3208_raw_to_voltage(uint16_t raw);
void     print_bar_graph(float voltage, float vref);
void     read_all_channels(void);
void     read_channel_statistics(void);

void Error_Handler(void);

/* ==================== Printf Retarget ==================== */
int _write(int file, char *ptr, int len)
{
    HAL_UART_Transmit(&huart1, (uint8_t *)ptr, len, HAL_MAX_DELAY);
    return len;
}

/* ==================== System Clock Configuration ==================== */
/**
 * HSE 8MHz -> PLL x9 -> SYSCLK 72MHz
 * AHB = 72MHz, APB1 = 36MHz, APB2 = 72MHz
 */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    /* Enable HSE and configure PLL */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState       = RCC_HSE_ON;
    RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    RCC_OscInitStruct.PLL.PLLState   = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource  = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL     = RCC_PLL_MUL9;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }

    /* Configure system clock, AHB, APB1, APB2 */
    RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                       RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;   /* APB1 = 36MHz */
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;   /* APB2 = 72MHz */
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
    {
        Error_Handler();
    }
}

/* ==================== GPIO Initialization ==================== */
static void GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* Enable GPIO clocks */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    /* Configure PC13 LED */
    GPIO_InitStruct.Pin   = LED_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_PORT, &GPIO_InitStruct);

    /* LED off (active low on Blue Pill) */
    HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET);
}

/* ==================== UART1 Initialization ==================== */
static void UART1_Init(void)
{
    __HAL_RCC_USART1_CLK_ENABLE();

    /* Configure PA9 (TX) as AF push-pull, PA10 (RX) as input floating */
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Pin   = UART_TX_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(UART_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin   = UART_RX_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    HAL_GPIO_Init(UART_PORT, &GPIO_InitStruct);

    /* UART configuration */
    huart1.Instance          = USART1;
    huart1.Init.BaudRate     = UART_BAUDRATE;
    huart1.Init.WordLength   = UART_WORDLENGTH_8B;
    huart1.Init.StopBits     = UART_STOPBITS_1;
    huart1.Init.Parity       = UART_PARITY_NONE;
    huart1.Init.Mode         = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart1) != HAL_OK)
    {
        Error_Handler();
    }
}

/* ==================== SPI1 Initialization ==================== */
static void SPI1_Init(void)
{
    __HAL_RCC_SPI1_CLK_ENABLE();

    /* Configure SPI1 pins */
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* PA5 (SCK) and PA7 (MOSI) as AF push-pull */
    GPIO_InitStruct.Pin   = SPI1_SCK_PIN | SPI1_MOSI_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* PA6 (MISO) as input floating */
    GPIO_InitStruct.Pin   = SPI1_MISO_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* SPI1 configuration: Master, CPOL=0, CPHA=0, 8-bit, MSB first */
    hspi1.Instance               = SPI1;
    hspi1.Init.Mode              = SPI_MODE_MASTER;
    hspi1.Init.Direction         = SPI_DIRECTION_2LINES;
    hspi1.Init.DataSize          = SPI_DATASIZE_8BIT;
    hspi1.Init.CLKPolarity       = SPI_POLARITY_LOW;
    hspi1.Init.CLKPhase          = SPI_PHASE_1EDGE;
    hspi1.Init.NSS               = SPI_NSS_SOFT;
    hspi1.Init.BaudRatePrescaler = SPI_PRESCALER;
    hspi1.Init.FirstBit          = SPI_FIRSTBIT_MSB;
    hspi1.Init.TIMode            = SPI_TIMODE_DISABLE;
    hspi1.Init.CRCCalculation    = SPI_CRCCALCULATION_DISABLE;
    hspi1.Init.CRCPolynomial     = 10;
    if (HAL_SPI_Init(&hspi1) != HAL_OK)
    {
        Error_Handler();
    }
}

/* ==================== MCP3208 CS Pin Init ==================== */
static void MCP3208_CS_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Pin   = MCP3208_CS_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(MCP3208_CS_PORT, &GPIO_InitStruct);

    /* CS high (deselected) */
    HAL_GPIO_WritePin(MCP3208_CS_PORT, MCP3208_CS_PIN, GPIO_PIN_SET);
}

/* ==================== MCP3208 Read Channel ==================== */
/**
 * Read a single channel from MCP3208 (single-ended mode).
 *
 * SPI Frame (3 bytes):
 *   TX byte 0: 0x06 | (channel >> 2)    -- Start bit + single/diff + D2
 *   TX byte 1: (channel & 0x03) << 6    -- D1, D0 + don't care
 *   TX byte 2: 0x00                     -- Clock out result
 *
 * RX byte 1 lower nibble = D11..D8
 * RX byte 2              = D7..D0
 *
 * @param channel  Channel number (0-7)
 * @return         12-bit ADC value (0-4095)
 */
uint16_t mcp3208_read_channel(uint8_t channel)
{
    uint8_t tx[3];
    uint8_t rx[3];
    uint16_t result;

    if (channel > 7) channel = 7;

    /* Build command frame (single-ended) */
    tx[0] = 0x06 | (channel >> 2);       /* Start=1, Single=1, D2 */
    tx[1] = (channel & 0x03) << 6;       /* D1, D0 */
    tx[2] = 0x00;                         /* Don't care */

    /* CS low - select MCP3208 */
    HAL_GPIO_WritePin(MCP3208_CS_PORT, MCP3208_CS_PIN, GPIO_PIN_RESET);

    /* Transmit command and receive response simultaneously */
    HAL_SPI_TransmitReceive(&hspi1, tx, rx, 3, HAL_MAX_DELAY);

    /* CS high - deselect MCP3208 */
    HAL_GPIO_WritePin(MCP3208_CS_PORT, MCP3208_CS_PIN, GPIO_PIN_SET);

    /* Extract 12-bit result from response */
    result = ((uint16_t)(rx[1] & 0x0F) << 8) | rx[2];

    return result;
}

/* ==================== Raw to Voltage Conversion ==================== */
float mcp3208_raw_to_voltage(uint16_t raw)
{
    return ((float)raw * MCP3208_VREF) / (float)MCP3208_RESOLUTION;
}

/* ==================== Print Bar Graph ==================== */
void print_bar_graph(float voltage, float vref)
{
    int bar_len = (int)((voltage / vref) * BAR_MAX_WIDTH);
    if (bar_len > BAR_MAX_WIDTH) bar_len = BAR_MAX_WIDTH;

    printf("[");
    for (int i = 0; i < BAR_MAX_WIDTH; i++)
    {
        if (i < bar_len)
            printf("#");
        else
            printf(" ");
    }
    printf("]");
}

/* ==================== Read All 8 Channels ==================== */
void read_all_channels(void)
{
    uint16_t raw;
    float voltage;

    printf("\r\n");
    printf("========================================================\r\n");
    printf(" MCP3208 8-Channel ADC Reading (VREF=%.1fV)\r\n", MCP3208_VREF);
    printf("========================================================\r\n");
    printf(" CH | Raw  | Voltage | Bar Graph\r\n");
    printf("----+------+---------+--------------------------------\r\n");

    for (uint8_t ch = 0; ch < MCP3208_CHANNELS; ch++)
    {
        raw     = mcp3208_read_channel(ch);
        voltage = mcp3208_raw_to_voltage(raw);

        printf(" %d  | %4u | %5.2fV  | ", ch, raw, voltage);
        print_bar_graph(voltage, MCP3208_VREF);
        printf("\r\n");
    }

    printf("========================================================\r\n");
}

/* ==================== Channel Statistics ==================== */
void read_channel_statistics(void)
{
    uint16_t samples[STATS_SAMPLES];
    uint16_t min_val, max_val;
    uint32_t sum;
    float    avg_voltage, min_voltage, max_voltage;

    printf("\r\n");
    printf("==================== STATISTICS (%d samples/ch) ====================\r\n", STATS_SAMPLES);
    printf(" CH | Min Raw | Max Raw | Avg Voltage | Min V  | Max V  | Range V\r\n");
    printf("----+---------+---------+-------------+--------+--------+--------\r\n");

    for (uint8_t ch = 0; ch < MCP3208_CHANNELS; ch++)
    {
        min_val = 4095;
        max_val = 0;
        sum     = 0;

        /* Collect samples */
        for (int s = 0; s < STATS_SAMPLES; s++)
        {
            samples[s] = mcp3208_read_channel(ch);
            if (samples[s] < min_val) min_val = samples[s];
            if (samples[s] > max_val) max_val = samples[s];
            sum += samples[s];
            HAL_Delay(1);  /* Short delay between samples */
        }

        avg_voltage = mcp3208_raw_to_voltage((uint16_t)(sum / STATS_SAMPLES));
        min_voltage = mcp3208_raw_to_voltage(min_val);
        max_voltage = mcp3208_raw_to_voltage(max_val);

        printf(" %d  | %5u   | %5u   | %7.3fV    | %5.3fV | %5.3fV | %5.3fV\r\n",
               ch, min_val, max_val, avg_voltage,
               min_voltage, max_voltage, max_voltage - min_voltage);
    }

    printf("====================================================================\r\n");
}

/* ==================== Error Handler ==================== */
void Error_Handler(void)
{
    __disable_irq();
    while (1)
    {
        HAL_GPIO_TogglePin(LED_PORT, LED_PIN);
        /* Simple delay loop since systick may not work */
        for (volatile uint32_t i = 0; i < 200000; i++);
    }
}

/* ==================== SysTick Handler ==================== */
void SysTick_Handler(void)
{
    HAL_IncTick();
}

/* ==================== Main Function ==================== */
int main(void)
{
    uint32_t cycle_count = 0;

    /* Initialize HAL library */
    HAL_Init();

    /* Configure system clock: HSE 8MHz -> PLL x9 -> 72MHz */
    SystemClock_Config();

    /* Initialize peripherals */
    GPIO_Init();
    UART1_Init();
    SPI1_Init();
    MCP3208_CS_Init();

    /* Startup banner */
    printf("\r\n");
    printf("============================================================\r\n");
    printf("  STM32F103 + MCP3208 8-Channel 12-bit ADC via SPI\r\n");
    printf("============================================================\r\n");
    printf("  SPI Clock : PA5 (SCK)\r\n");
    printf("  SPI MOSI  : PA7 (DIN)\r\n");
    printf("  SPI MISO  : PA6 (DOUT)\r\n");
    printf("  Chip Select: PA4 (CS)\r\n");
    printf("  VREF      : %.1fV\r\n", MCP3208_VREF);
    printf("  Resolution: 12-bit (0-4095)\r\n");
    printf("  Channels  : %d (single-ended)\r\n", MCP3208_CHANNELS);
    printf("  SPI Speed : 72MHz/16 = 4.5MHz\r\n");
    printf("  Interval  : %d ms\r\n", READ_INTERVAL_MS);
    printf("============================================================\r\n");

    /* Quick connection test - read channel 0 */
    printf("\r\n[TEST] Reading channel 0...\r\n");
    uint16_t test_val = mcp3208_read_channel(0);
    printf("[TEST] CH0 raw = %u, voltage = %.3fV\r\n",
           test_val, mcp3208_raw_to_voltage(test_val));

    if (test_val == 0 && mcp3208_read_channel(1) == 0 && mcp3208_read_channel(2) == 0)
    {
        printf("[WARN] All channels reading 0 - check wiring!\r\n");
    }
    else
    {
        printf("[OK] MCP3208 communication established.\r\n");
    }

    /* Toggle LED to indicate running */
    HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_RESET);

    /* Main loop */
    while (1)
    {
        cycle_count++;

        printf("\r\n>>> Cycle #%lu | Uptime: %lu sec <<<\r\n",
               cycle_count, HAL_GetTick() / 1000);

        /* Read all 8 channels with bar graphs */
        read_all_channels();

        /* Print statistics every 5th cycle */
        if (cycle_count % 5 == 0)
        {
            read_channel_statistics();
        }

        /* Toggle LED */
        HAL_GPIO_TogglePin(LED_PORT, LED_PIN);

        /* Wait before next reading */
        HAL_Delay(READ_INTERVAL_MS);
    }
}
