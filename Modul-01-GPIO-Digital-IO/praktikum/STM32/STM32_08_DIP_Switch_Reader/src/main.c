/**
 * ============================================================================
 * @file    main.c
 * @brief   STM32_08 - 4-Bit DIP Switch Reader with Binary Decoding
 * @project STM32_08_DIP_Switch_Reader
 * ============================================================================
 *
 * Description:
 *   Reads a 4-position DIP switch connected to Port B. Each switch is
 *   configured as input with internal pull-up resistor. Switch ON connects
 *   pin to GND (reads LOW = logic 1). The 4 individual bits are combined
 *   into a nibble (0-15) using bit masking and shifting.
 *
 *   The decoded value is displayed as binary and decimal, and interpreted
 *   as a device address or configuration setting.
 *
 * Pin Mapping (avoids PB2 = BOOT1):
 *   PB0 -> DIP SW1 -> Bit 0 (LSB)
 *   PB1 -> DIP SW2 -> Bit 1
 *   PB3 -> DIP SW3 -> Bit 2
 *   PB4 -> DIP SW4 -> Bit 3 (MSB)
 *
 * Hardware:
 *   - 4x DIP switch module (or 4 individual toggle switches)
 *   - Each switch: one side to PBx, other side to GND
 *   - Internal pull-ups used (no external resistors needed)
 *
 * Board Support: Blue Pill (F103C8), F401CC, F411CE
 *
 * ============================================================================
 */

#include "config.h"
#include <stdio.h>

/* ========================= Function Prototypes =========================== */
void SystemClock_Config(void);
void Error_Handler(void);
void GPIO_Init(void);
uint8_t Read_DIP_Switches(void);
void Print_Binary(uint8_t value, uint8_t bits);
void Decode_Setting(uint8_t value);

/* ========================= Printf Redirect (ITM/SWO) ==================== */
int _write(int file, char *ptr, int len) {
    (void)file;
    for (int i = 0; i < len; i++) {
        ITM_SendChar(*ptr++);
    }
    return len;
}

/* ========================= Main Program ================================== */
int main(void) {
    HAL_Init();
    SystemClock_Config();

    /* Enable GPIO port clocks */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    /* Initialize DIP switch GPIO */
    GPIO_Init();

    printf("\r\n========================================\r\n");
    printf("  STM32 4-Bit DIP Switch Reader\r\n");
    printf("  Pins: PB0, PB1, PB3, PB4\r\n");
    printf("========================================\r\n\r\n");
    printf("Switch ON = GND = bit 1\r\n");
    printf("Switch OFF = pull-up = bit 0\r\n\r\n");

    uint8_t last_value = 0xFF; /* Force first print */

    while (1) {
        uint8_t value = Read_DIP_Switches();

        /* Only print when value changes */
        if (value != last_value) {
            printf("[%7lu ms] DIP Value: ", HAL_GetTick());
            Print_Binary(value, 4);
            printf(" (0x%X = %d)\r\n", value, value);

            /* Show individual switch states */
            printf("           SW4=%d  SW3=%d  SW2=%d  SW1=%d\r\n",
                   (value >> 3) & 1,
                   (value >> 2) & 1,
                   (value >> 1) & 1,
                   (value >> 0) & 1);

            /* Decode as address/setting */
            Decode_Setting(value);
            printf("\r\n");

            last_value = value;
        }

        HAL_Delay(READ_INTERVAL_MS);
    }
}

/* ========================= GPIO Initialization =========================== */
void GPIO_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* Configure DIP switch pins as input with pull-up */
    GPIO_InitStruct.Pin  = DIP_ALL_PINS;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(DIP_PORT, &GPIO_InitStruct);
}

/* ========================= Read DIP Switches ============================= */
/**
 * @brief  Read all 4 DIP switches and combine into a nibble.
 *         Switches are active-low (ON = GND = LOW), so we invert.
 *         PB0->bit0, PB1->bit1, PB3->bit2, PB4->bit3
 * @retval Combined 4-bit value (0-15)
 */
uint8_t Read_DIP_Switches(void) {
    uint8_t value = 0;

    /* Read each pin - active LOW, invert for positive logic */
    if (HAL_GPIO_ReadPin(DIP_PORT, DIP1_PIN) == GPIO_PIN_RESET) {
        value |= (1 << 0);  /* Bit 0 */
    }
    if (HAL_GPIO_ReadPin(DIP_PORT, DIP2_PIN) == GPIO_PIN_RESET) {
        value |= (1 << 1);  /* Bit 1 */
    }
    if (HAL_GPIO_ReadPin(DIP_PORT, DIP3_PIN) == GPIO_PIN_RESET) {
        value |= (1 << 2);  /* Bit 2 */
    }
    if (HAL_GPIO_ReadPin(DIP_PORT, DIP4_PIN) == GPIO_PIN_RESET) {
        value |= (1 << 3);  /* Bit 3 */
    }

    return value;
}

/* ========================= Print Binary ================================== */
void Print_Binary(uint8_t value, uint8_t bits) {
    printf("0b");
    for (int i = bits - 1; i >= 0; i--) {
        printf("%c", (value & (1 << i)) ? '1' : '0');
    }
}

/* ========================= Decode Setting ================================ */
void Decode_Setting(uint8_t value) {
    printf("           Address/Setting: ");
    switch (value) {
        case 0:  printf("Default (all OFF)"); break;
        case 1:  printf("Mode 1 - Slow blink"); break;
        case 2:  printf("Mode 2 - Fast blink"); break;
        case 3:  printf("Mode 3 - PWM mode"); break;
        case 4:  printf("Mode 4 - Sensor read"); break;
        case 5:  printf("Mode 5 - Debug output"); break;
        case 6:  printf("Mode 6 - Calibration"); break;
        case 7:  printf("Mode 7 - Test pattern"); break;
        case 8:  printf("Address 8"); break;
        case 9:  printf("Address 9"); break;
        case 10: printf("Address 10 (0xA)"); break;
        case 11: printf("Address 11 (0xB)"); break;
        case 12: printf("Address 12 (0xC)"); break;
        case 13: printf("Address 13 (0xD)"); break;
        case 14: printf("Address 14 (0xE)"); break;
        case 15: printf("ALL ON - Factory reset mode"); break;
        default: printf("Unknown"); break;
    }
    printf("\r\n");
}

/* ========================= System Clock Configuration ==================== */
void SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

#ifdef STM32F103xB
    /*--- F103: 8MHz HSE -> PLL x9 -> 72MHz SYSCLK ---*/
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

#elif defined(STM32F401xC) || defined(STM32F411xE)
    /*--- F401/F411: 25MHz HSE -> PLL -> 84MHz SYSCLK ---*/
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState       = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState   = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource  = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM       = 25;
    RCC_OscInitStruct.PLL.PLLN       = 336;
    RCC_OscInitStruct.PLL.PLLP       = RCC_PLLP_DIV4;
    RCC_OscInitStruct.PLL.PLLQ       = 7;
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

#else
    /*--- Fallback: HSI ---*/
    RCC_OscInitStruct.OscillatorType      = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState            = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }

    RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                     | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_HSI;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK) {
        Error_Handler();
    }
#endif
}

/* ========================= Error Handler ================================= */
void Error_Handler(void) {
    __disable_irq();
    while (1) {
        /* Stay here on error */
    }
}
