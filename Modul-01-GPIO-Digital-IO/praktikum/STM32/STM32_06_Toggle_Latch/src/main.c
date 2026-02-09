/**
 * @file main.c
 * @brief STM32_06_Toggle_Latch - Toggle Latch - Toggle On/Off dengan Tombol
 * 
 * FUNGSI: Toggle LED state pada setiap tekan tombol dengan edge detection
 * Supported: STM32F103C8T6, STM32F401CCU6, STM32F411CEU6
 */



#include "config.h"

/* ---- Function Prototypes ---- */
void SystemClock_Config(void);
void MX_GPIO_Init(void);
void Error_Handler(void);

/* ---- printf stub (no UART needed) ---- */

/* ============================================================
 *  MAIN
 * ============================================================ */
int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();

    GPIO_PinState prev_btn_state = GPIO_PIN_SET;  /* Not pressed (pull-up) */
    GPIO_PinState curr_btn_state;

    while (1)
    {
        /* Read current button state */
        curr_btn_state = HAL_GPIO_ReadPin(BTN_PORT, BTN_PIN);

        /* Detect falling edge: prev was HIGH (not pressed), now LOW (pressed) */
        if (prev_btn_state != curr_btn_state)
        {
            /* State changed - apply debounce */
            HAL_Delay(DEBOUNCE_DELAY_MS);

            /* Re-read to confirm */
            curr_btn_state = HAL_GPIO_ReadPin(BTN_PORT, BTN_PIN);

            if (curr_btn_state == BTN_PRESSED && prev_btn_state != BTN_PRESSED)
            {
                /* Confirmed falling edge (rising edge of press) -> Toggle LED */
                HAL_GPIO_TogglePin(LED_PORT, LED_PIN);
            }

            /* Update previous state */
            prev_btn_state = curr_btn_state;
        }
    }
}

/* ============================================================
 *  GPIO Initialization
 * ============================================================ */
void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* Enable GPIO Clocks */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    /* ---- Configure LED pin (PC13) as push-pull output ---- */
    HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET); /* LED OFF initially */

    GPIO_InitStruct.Pin   = LED_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_PORT, &GPIO_InitStruct);

    /* ---- Configure Button pin (PB0) as input with pull-up ---- */
    GPIO_InitStruct.Pin   = BTN_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull  = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(BTN_PORT, &GPIO_InitStruct);
}

/* ============================================================
 *  System Clock Configuration
 * ============================================================ */
#ifdef STM32F103xC
/* F103: 8MHz HSE -> PLL x9 -> 72MHz SYSCLK */
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
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }

    RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                     | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) {
        Error_Handler();
    }
}

#elif defined(STM32F401xC) || defined(STM32F411xE)
/* F4xx: 25MHz HSE -> PLL -> 84MHz SYSCLK */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState       = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState   = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource  = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM       = 25;
    RCC_OscInitStruct.PLL.PLLN       = 168;
    RCC_OscInitStruct.PLL.PLLP       = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ       = 4;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }

    RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                     | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) {
        Error_Handler();
    }
}
#endif

/* ============================================================
 *  Error Handler
 * ============================================================ */
void Error_Handler(void)
{
    __disable_irq();
    while (1) { }
}
