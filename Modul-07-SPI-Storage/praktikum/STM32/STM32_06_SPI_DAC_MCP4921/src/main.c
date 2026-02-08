/**
 * ============================================================================
 * File        : main.c
 * Program     : STM32_06_SPI_DAC_MCP4921
 * Description : MCP4921 12-bit DAC driver via SPI1 with waveform generation
 *
 * Board       : STM32F103C8 (Blue Pill)
 * Framework   : STM32Cube HAL
 *
 * Wiring:
 *   PA5  -> MCP4921 SCK   (SPI1 SCK)
 *   PA7  -> MCP4921 SDI   (SPI1 MOSI)
 *   PA6  <- MCP4921 SDO   (SPI1 MISO, unused for DAC)
 *   PA4  -> MCP4921 CS    (GPIO output, active low)
 *   PB0  -> MCP4921 LDAC  (GPIO output, active low)
 *   PA9  -> USB-TTL RX    (USART1 TX)
 *   PA10 <- USB-TTL TX    (USART1 RX)
 *   PC13 -> Onboard LED
 *
 * MCP4921 16-bit Command:
 *   [15] DAC select: 0=DACA
 *   [14] Buffer: 0=unbuffered
 *   [13] Gain: 1=1x
 *   [12] Shutdown: 1=active
 *   [11:0] Data (0-4095)
 *   -> Command = 0x3000 | (data & 0x0FFF)
 * ============================================================================
 */

#include "stm32f1xx_hal.h"
#include "config.h"
#include <stdio.h>
#include <string.h>

/* ==================== Global Handles ==================== */
SPI_HandleTypeDef  hspi1;
UART_HandleTypeDef huart1;

/* ==================== Sine Look-Up Table ==================== */
/* Pre-computed sine table (256 entries, 0-4095 range)
 * Generated using integer approximation of: 2047.5 + 2047.5 * sin(2*PI*i/256)
 * This avoids using float in ISR / real-time loop */
static uint16_t sine_lut[LUT_SIZE];

/* ==================== Waveform State ==================== */
static volatile Waveform_TypeDef current_wave = WAVE_SINE;
static volatile uint16_t sample_index = 0;
static volatile uint32_t wave_switch_tick = 0;

/* ==================== Waveform Names ==================== */
static const char *wave_names[] = {
    "SINE",
    "SAWTOOTH",
    "TRIANGLE",
    "SQUARE"
};

/* ==================== Function Prototypes ==================== */
void SystemClock_Config(void);
static void GPIO_Init(void);
static void UART1_Init(void);
static void SPI1_Init(void);
static void MCP4921_GPIO_Init(void);
static void generate_sine_lut(void);

void     mcp4921_write(uint16_t value);
void     mcp4921_ldac_pulse(void);
uint16_t generate_waveform_sample(Waveform_TypeDef wave, uint16_t index);
float    dac_value_to_voltage(uint16_t value);

void Error_Handler(void);

/* ==================== Printf Retarget ==================== */
int _write(int file, char *ptr, int len)
{
    HAL_UART_Transmit(&huart1, (uint8_t *)ptr, len, HAL_MAX_DELAY);
    return len;
}

/* ==================== System Clock Configuration ==================== */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

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

    RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                       RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
    {
        Error_Handler();
    }
}

/* ==================== GPIO Initialization ==================== */
static void GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    /* Configure PC13 LED */
    GPIO_InitStruct.Pin   = LED_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_PORT, &GPIO_InitStruct);
    HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET);
}

/* ==================== UART1 Initialization ==================== */
static void UART1_Init(void)
{
    __HAL_RCC_USART1_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Pin   = UART_TX_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(UART_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin   = UART_RX_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    HAL_GPIO_Init(UART_PORT, &GPIO_InitStruct);

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

    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* PA5 (SCK) and PA7 (MOSI) as AF push-pull */
    GPIO_InitStruct.Pin   = SPI1_SCK_PIN | SPI1_MOSI_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* PA6 (MISO) as input floating (not used by DAC, but keep for full-duplex) */
    GPIO_InitStruct.Pin   = SPI1_MISO_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

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

/* ==================== MCP4921 GPIO Init (CS + LDAC) ==================== */
static void MCP4921_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* CS pin (PA4) */
    GPIO_InitStruct.Pin   = MCP4921_CS_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(MCP4921_CS_PORT, &GPIO_InitStruct);
    HAL_GPIO_WritePin(MCP4921_CS_PORT, MCP4921_CS_PIN, GPIO_PIN_SET);

    /* LDAC pin (PB0) */
    GPIO_InitStruct.Pin   = MCP4921_LDAC_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(MCP4921_LDAC_PORT, &GPIO_InitStruct);
    HAL_GPIO_WritePin(MCP4921_LDAC_PORT, MCP4921_LDAC_PIN, GPIO_PIN_SET);
}

/* ==================== Generate Sine LUT ==================== */
/**
 * Pre-compute sine lookup table using integer math.
 * Uses a 3rd-order polynomial approximation for sin() over [0, 2*PI].
 * Output range: 0 to 4095 (12-bit DAC).
 *
 * Integer approximation of sin using a Taylor-like series:
 *   sin(x) ≈ x - x^3/6 + x^5/120, scaled to fixed-point
 *
 * For simplicity, we use a quadrant-based approach with
 * linear interpolation for reasonable accuracy.
 */
static void generate_sine_lut(void)
{
    /*
     * Use a simple quadrant-based integer sine generation.
     * For each of 256 steps around a full circle:
     *   angle_deg = i * 360 / 256
     * We compute sin() using the Bhaskara I approximation:
     *   sin(x) ≈ 16x(PI-x) / (5*PI^2 - 4x(PI-x))  for 0 <= x <= PI
     *
     * But for embedded simplicity, we'll pre-compute using a
     * step-by-step integer approach.
     */

    /* We'll use a fixed-point scaled sine computation.
     * sin values scaled by 10000 for the first quadrant (64 entries per quadrant).
     * Then map to 0-4095 DAC range. */

    /* Precomputed sine values for first quadrant (0..90 degrees, 65 entries)
     * sin(i * 90/64) * 10000, where i = 0..64 */
    static const int16_t sin_q1[65] = {
            0,   245,   490,   735,   980,  1224,  1467,  1710,
         1951,  2191,  2429,  2667,  2903,  3137,  3369,  3599,
         3827,  4052,  4276,  4496,  4714,  4929,  5141,  5350,
         5556,  5758,  5957,  6152,  6344,  6532,  6716,  6895,
         7071,  7242,  7409,  7572,  7730,  7883,  8032,  8176,
         8315,  8449,  8577,  8701,  8819,  8932,  9040,  9143,
         9239,  9330,  9415,  9495,  9569,  9638,  9700,  9757,
         9808,  9853,  9892,  9925,  9952,  9973,  9988,  9997,
        10000
    };

    for (int i = 0; i < LUT_SIZE; i++)
    {
        int quadrant = i / 64;    /* 0-3 */
        int index    = i % 64;    /* 0-63 */
        int16_t sin_val;

        switch (quadrant)
        {
            case 0:  /* 0-90 degrees */
                sin_val = sin_q1[index];
                break;
            case 1:  /* 90-180 degrees */
                sin_val = sin_q1[64 - index];
                break;
            case 2:  /* 180-270 degrees */
                sin_val = -sin_q1[index];
                break;
            case 3:  /* 270-360 degrees */
                sin_val = -sin_q1[64 - index];
                break;
            default:
                sin_val = 0;
                break;
        }

        /* Map from -10000..+10000 to 0..4095 */
        /* midpoint = 2047, amplitude = 2047 */
        int32_t dac_val = 2047 + (int32_t)(sin_val * 2047L / 10000L);
        if (dac_val < 0) dac_val = 0;
        if (dac_val > 4095) dac_val = 4095;
        sine_lut[i] = (uint16_t)dac_val;
    }
}

/* ==================== MCP4921 Write ==================== */
/**
 * Write a 12-bit value to MCP4921 DAC.
 *
 * 16-bit command word:
 *   Bit 15   = 0: DAC A select
 *   Bit 14   = 0: Unbuffered VREF
 *   Bit 13   = 1: 1x output gain
 *   Bit 12   = 1: Active mode (not shutdown)
 *   Bit 11-0 = DAC data (0-4095)
 *
 * Command = 0x3000 | (value & 0x0FFF)
 *
 * @param value  12-bit DAC value (0-4095)
 */
void mcp4921_write(uint16_t value)
{
    uint16_t cmd = MCP4921_CMD_MASK | (value & 0x0FFF);
    uint8_t tx[2];

    tx[0] = (uint8_t)(cmd >> 8);    /* High byte first */
    tx[1] = (uint8_t)(cmd & 0xFF);  /* Low byte */

    /* CS low */
    HAL_GPIO_WritePin(MCP4921_CS_PORT, MCP4921_CS_PIN, GPIO_PIN_RESET);

    /* Send 2 bytes */
    HAL_SPI_Transmit(&hspi1, tx, 2, HAL_MAX_DELAY);

    /* CS high */
    HAL_GPIO_WritePin(MCP4921_CS_PORT, MCP4921_CS_PIN, GPIO_PIN_SET);
}

/* ==================== LDAC Pulse ==================== */
/**
 * Pulse LDAC low to update DAC output synchronously.
 * LDAC low latches the data register to the output.
 */
void mcp4921_ldac_pulse(void)
{
    HAL_GPIO_WritePin(MCP4921_LDAC_PORT, MCP4921_LDAC_PIN, GPIO_PIN_RESET);
    /* Brief delay - a few nops is sufficient for MCP4921 */
    __NOP(); __NOP(); __NOP(); __NOP();
    HAL_GPIO_WritePin(MCP4921_LDAC_PORT, MCP4921_LDAC_PIN, GPIO_PIN_SET);
}

/* ==================== Generate Waveform Sample ==================== */
/**
 * Generate a single waveform sample based on type and index.
 *
 * @param wave   Waveform type
 * @param index  Sample index (0 to LUT_SIZE-1)
 * @return       12-bit DAC value (0-4095)
 */
uint16_t generate_waveform_sample(Waveform_TypeDef wave, uint16_t index)
{
    uint16_t idx = index % LUT_SIZE;

    switch (wave)
    {
        case WAVE_SINE:
            return sine_lut[idx];

        case WAVE_SAWTOOTH:
            /* Linear ramp from 0 to 4095 over 256 steps */
            return (uint16_t)((uint32_t)idx * 4095 / (LUT_SIZE - 1));

        case WAVE_TRIANGLE:
        {
            /* Triangle: 0->4095->0 over 256 steps */
            uint16_t half = LUT_SIZE / 2;
            if (idx < half)
                return (uint16_t)((uint32_t)idx * 4095 / (half - 1));
            else
                return (uint16_t)((uint32_t)(LUT_SIZE - 1 - idx) * 4095 / (half - 1));
        }

        case WAVE_SQUARE:
            /* Square wave: 0 for first half, 4095 for second half */
            return (idx < LUT_SIZE / 2) ? 0 : 4095;

        default:
            return 0;
    }
}

/* ==================== DAC Value to Voltage ==================== */
float dac_value_to_voltage(uint16_t value)
{
    return ((float)value * MCP4921_VREF) / (float)MCP4921_RESOLUTION;
}

/* ==================== Error Handler ==================== */
void Error_Handler(void)
{
    __disable_irq();
    while (1)
    {
        HAL_GPIO_TogglePin(LED_PORT, LED_PIN);
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
    uint32_t last_switch_tick = 0;
    uint32_t last_print_tick  = 0;
    uint32_t total_samples    = 0;
    uint16_t dac_value;
    float    voltage;

    /* Initialize HAL */
    HAL_Init();

    /* Configure system clock */
    SystemClock_Config();

    /* Initialize peripherals */
    GPIO_Init();
    UART1_Init();
    SPI1_Init();
    MCP4921_GPIO_Init();

    /* Pre-compute sine LUT */
    generate_sine_lut();

    /* Startup banner */
    printf("\r\n");
    printf("============================================================\r\n");
    printf("  STM32F103 + MCP4921 12-bit DAC Waveform Generator\r\n");
    printf("============================================================\r\n");
    printf("  SPI Clock : PA5 (SCK)\r\n");
    printf("  SPI MOSI  : PA7 (SDI)\r\n");
    printf("  CS Pin    : PA4\r\n");
    printf("  LDAC Pin  : PB0\r\n");
    printf("  VREF      : %.1fV\r\n", MCP4921_VREF);
    printf("  Resolution: 12-bit (0-4095)\r\n");
    printf("  LUT Size  : %d entries\r\n", LUT_SIZE);
    printf("  SPI Speed : 72MHz/4 = 18MHz\r\n");
    printf("  Wave Switch: every %d seconds\r\n", WAVEFORM_SWITCH_SEC);
    printf("============================================================\r\n");

    /* Verify sine LUT */
    printf("\r\n[INFO] Sine LUT verification:\r\n");
    printf("  LUT[0]   = %u (expect ~2047, 0 deg)\r\n",   sine_lut[0]);
    printf("  LUT[64]  = %u (expect ~4094, 90 deg)\r\n",  sine_lut[64]);
    printf("  LUT[128] = %u (expect ~2047, 180 deg)\r\n", sine_lut[128]);
    printf("  LUT[192] = %u (expect ~0, 270 deg)\r\n",    sine_lut[192]);

    /* Test DAC write */
    printf("\r\n[TEST] Writing mid-scale (2048) to DAC...\r\n");
    mcp4921_write(2048);
    mcp4921_ldac_pulse();
    printf("[TEST] Expected output: %.3fV\r\n", dac_value_to_voltage(2048));

    printf("\r\n[START] Waveform generation starting...\r\n");
    printf("  Current waveform: %s\r\n\r\n", wave_names[current_wave]);

    last_switch_tick = HAL_GetTick();
    last_print_tick  = HAL_GetTick();

    /* Main loop */
    while (1)
    {
        /* Generate current waveform sample */
        dac_value = generate_waveform_sample(current_wave, sample_index);

        /* Write to DAC */
        mcp4921_write(dac_value);
        mcp4921_ldac_pulse();

        total_samples++;
        sample_index = (sample_index + 1) % LUT_SIZE;

        /* Print status every 500ms */
        if (HAL_GetTick() - last_print_tick >= 500)
        {
            last_print_tick = HAL_GetTick();
            voltage = dac_value_to_voltage(dac_value);

            printf("DAC:%s,idx:%3u,val:%4u,V:%.3f,total:%lu\r\n",
                   wave_names[current_wave], sample_index,
                   dac_value, voltage, total_samples);
        }

        /* Switch waveform every WAVEFORM_SWITCH_SEC seconds */
        if (HAL_GetTick() - last_switch_tick >= (WAVEFORM_SWITCH_SEC * 1000))
        {
            last_switch_tick = HAL_GetTick();
            current_wave = (Waveform_TypeDef)((current_wave + 1) % WAVE_COUNT);
            sample_index = 0;

            printf("\r\n>>> Waveform switched to: %s <<<\r\n\r\n",
                   wave_names[current_wave]);

            /* Toggle LED on waveform change */
            HAL_GPIO_TogglePin(LED_PORT, LED_PIN);
        }

        /* Small delay to control output rate */
        HAL_Delay(1);
    }
}
