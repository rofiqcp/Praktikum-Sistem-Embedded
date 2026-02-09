/**
 * @file main.c
 * @brief STM32F411CEU6 LED Blink via USB
 * 
 * FUNGSI: Membuat LED berkedip dengan interval 500ms
 * - Inisialisasi GPIO sebagai output
 * - Kontrol LED ON/OFF dengan HAL_GPIO_WritePin()
 * - Menggunakan HAL_Delay() untuk timing
 * 
 * Hardware: STM32F411CEU6 dengan LED pada PC13
 * Upload: Via USB DFU Bootloader (hold BOOT0 saat upload)
 */

#include "config.h"

/* ============================================================
 *  Function Prototypes
 * ============================================================ */
void SystemClock_Config(void);
void MX_GPIO_Init(void);
void Error_Handler(void);

/* ============================================================
 *  MAIN
 * ============================================================ */
int main(void)
{
    /* Initialize HAL and System Clock */
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();

    /* Main LED Blink Loop */
    while (1)
    {
        /* LED ON  (active-low: RESET = ON) */
        HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_RESET);
        HAL_Delay(BLINK_DELAY_MS);

        /* LED OFF (active-low: SET = OFF) */
        HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET);
        HAL_Delay(BLINK_DELAY_MS);
    }

    return 0;
}

/* ============================================================
 *  GPIO Initialization
 * ============================================================ */
void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* Enable GPIO Port C Clock */
    __HAL_RCC_GPIOC_CLK_ENABLE();

    /* Pre-set LED OFF before configuring as output */
    HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET);

    /* Configure LED pin (PC13) as push-pull output */
    GPIO_InitStruct.Pin   = LED_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_PORT, &GPIO_InitStruct);
}

/* ============================================================
 *  System Clock Configuration (84 MHz)
 * ============================================================ */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    /* Enable Power Control Clock */
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

    /* Configure Oscillator: 25MHz HSE -> PLL -> 84MHz */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState       = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState   = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource  = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM       = 25;    /* Divide by 25 -> 1 MHz */
    RCC_OscInitStruct.PLL.PLLN       = 168;   /* Multiply by 168 -> 168 MHz */
    RCC_OscInitStruct.PLL.PLLP       = RCC_PLLP_DIV2;  /* Divide by 2 -> 84 MHz */
    RCC_OscInitStruct.PLL.PLLQ       = 4;

    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }

    /* Configure Clock Dividers */
    RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                     | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
    {
        Error_Handler();
    }
}

/* ============================================================
 *  Error Handler
 * ============================================================ */
void Error_Handler(void)
{
    __disable_irq();
    while (1);
}
