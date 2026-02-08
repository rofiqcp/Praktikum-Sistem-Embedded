/**
 * ============================================================================
 * @file    main.c
 * @brief   STM32_11 - Emergency Stop System with NC Button (Fail-Safe)
 * @project STM32_11_Emergency_Stop
 * ============================================================================
 *
 * Description:
 *   Implements a fail-safe emergency stop system using a Normally Closed (NC)
 *   push button. The NC design ensures safety: if the wire breaks or the
 *   button contact opens unexpectedly, the system immediately enters
 *   emergency mode (fail-safe principle).
 *
 *   System States:
 *     NORMAL:    PB0 = HIGH -> LED slow blink, buzzer OFF
 *     EMERGENCY: PB0 = LOW  -> LED fast blink, buzzer ON
 *     RESET:     PB0 must stay HIGH for 2 seconds to clear emergency
 *
 *   The NC button is wired so that:
 *     - Normal operation: button closed, PB0 pulled HIGH
 *     - Emergency: button opened (or wire break) -> PB0 goes LOW
 *     - This is the OPPOSITE of a normal NO (Normally Open) button
 *
 * Hardware:
 *   - 1x NC (Normally Closed) push button on PB0
 *   - External 10K pull-up resistor PB0 to VCC recommended
 *   - NC button connects PB0 through button to VCC
 *   - 1x LED on PC13 (Blue Pill onboard, active-LOW)
 *   - 1x Active buzzer on PA1 (through NPN transistor or MOSFET)
 *
 * Wiring:
 *   VCC ---[NC_BUTTON]--- PB0 ---[10K]--- GND
 *                               (or use internal pull-up + external pull-down)
 *   PC13 --- onboard LED (active LOW)
 *   PA1  ---[1K]--- NPN_BASE, NPN_COLLECTOR --- BUZZER+ --- VCC
 *                             NPN_EMITTER --- GND, BUZZER- --- NPN_COLLECTOR
 *
 * Board Support: Blue Pill (F103C8), F401CC, F411CE
 *
 * ============================================================================
 */

#include "config.h"
#include <stdio.h>

/* ========================= System States ================================= */
typedef enum {
    STATE_NORMAL,
    STATE_EMERGENCY,
    STATE_RESETTING
} SystemState_t;

/* ========================= Function Prototypes =========================== */
void SystemClock_Config(void);
void Error_Handler(void);
void GPIO_Init(void);
void State_Normal(uint32_t now);
void State_Emergency(uint32_t now);
void State_Resetting(uint32_t now);
void LED_On(void);
void LED_Off(void);
void Buzzer_On(void);
void Buzzer_Off(void);
const char *State_To_String(SystemState_t state);

/* ========================= Printf Redirect (ITM/SWO) ==================== */
int _write(int file, char *ptr, int len) {
    (void)file;
    for (int i = 0; i < len; i++) {
        ITM_SendChar(*ptr++);
    }
    return len;
}

/* ========================= Global State ================================== */
static SystemState_t system_state   = STATE_NORMAL;
static uint32_t      last_blink     = 0;
static uint32_t      reset_start    = 0;
static uint8_t       state_changed  = 1;   /* Flag to print state on change */

/* ========================= Main Program ================================== */
int main(void) {
    HAL_Init();
    SystemClock_Config();

    /* Enable GPIO port clocks */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    /* Initialize GPIO */
    GPIO_Init();

    printf("\r\n========================================\r\n");
    printf("  STM32 Emergency Stop System\r\n");
    printf("  NC Button=PB0, LED=PC13, Buzzer=PA1\r\n");
    printf("========================================\r\n\r\n");
    printf("Fail-safe design: wire break = EMERGENCY\r\n");
    printf("PB0 HIGH=Normal, PB0 LOW=Emergency\r\n");
    printf("Hold PB0 HIGH for %d ms to reset\r\n\r\n", RESET_HOLD_MS);

    /* Initial state */
    LED_On();
    Buzzer_Off();

    while (1) {
        uint32_t now = HAL_GetTick();

        /* Print state transitions */
        if (state_changed) {
            printf("[%7lu ms] State: %s\r\n", now, State_To_String(system_state));
            state_changed = 0;
        }

        /* State machine */
        switch (system_state) {
            case STATE_NORMAL:
                State_Normal(now);
                break;

            case STATE_EMERGENCY:
                State_Emergency(now);
                break;

            case STATE_RESETTING:
                State_Resetting(now);
                break;
        }
    }
}

/* ========================= GPIO Initialization =========================== */
void GPIO_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* Emergency Stop button: PB0 input with pull-up */
    GPIO_InitStruct.Pin  = ESTOP_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(ESTOP_PORT, &GPIO_InitStruct);

    /* Status LED: PC13 output push-pull */
    GPIO_InitStruct.Pin   = LED_STATUS_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_STATUS_PORT, &GPIO_InitStruct);

    /* Buzzer: PA1 output push-pull */
    GPIO_InitStruct.Pin   = BUZZER_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(BUZZER_PORT, &GPIO_InitStruct);
}

/* ========================= Normal State Handler ========================== */
void State_Normal(uint32_t now) {
    /* Slow heartbeat blink to show system is alive */
    if (now - last_blink >= NORMAL_BLINK_MS) {
        HAL_GPIO_TogglePin(LED_STATUS_PORT, LED_STATUS_PIN);
        last_blink = now;
    }

    /* Buzzer must be OFF in normal mode */
    Buzzer_Off();

    /* Check for emergency condition */
    if (HAL_GPIO_ReadPin(ESTOP_PORT, ESTOP_PIN) == ESTOP_ACTIVE_STATE) {
        /* Debounce: wait and re-check */
        HAL_Delay(DEBOUNCE_MS);
        if (HAL_GPIO_ReadPin(ESTOP_PORT, ESTOP_PIN) == ESTOP_ACTIVE_STATE) {
            system_state  = STATE_EMERGENCY;
            state_changed = 1;
            printf("[%7lu ms] !!! EMERGENCY TRIGGERED !!!\r\n", now);
            printf("           PB0 went LOW (wire break or button opened)\r\n");
        }
    }
}

/* ========================= Emergency State Handler ======================= */
void State_Emergency(uint32_t now) {
    /* Fast blink LED */
    if (now - last_blink >= EMERGENCY_BLINK_MS) {
        HAL_GPIO_TogglePin(LED_STATUS_PORT, LED_STATUS_PIN);
        last_blink = now;
    }

    /* Buzzer ON continuously */
    Buzzer_On();

    /* Check if button is back to HIGH (safe) to begin reset */
    if (HAL_GPIO_ReadPin(ESTOP_PORT, ESTOP_PIN) != ESTOP_ACTIVE_STATE) {
        system_state  = STATE_RESETTING;
        reset_start   = now;
        state_changed = 1;
        printf("[%7lu ms] Reset initiated - hold for %d ms...\r\n",
               now, RESET_HOLD_MS);
    }
}

/* ========================= Resetting State Handler ======================= */
void State_Resetting(uint32_t now) {
    /* Continue fast blink during reset attempt */
    if (now - last_blink >= (EMERGENCY_BLINK_MS * 2)) {
        HAL_GPIO_TogglePin(LED_STATUS_PORT, LED_STATUS_PIN);
        last_blink = now;
    }

    /* Buzzer intermittent beep during reset */
    if ((now / 200) % 2) {
        Buzzer_On();
    } else {
        Buzzer_Off();
    }

    /* Check if button is still HIGH (safe) */
    if (HAL_GPIO_ReadPin(ESTOP_PORT, ESTOP_PIN) == ESTOP_ACTIVE_STATE) {
        /* Button went LOW again during reset - back to emergency */
        system_state  = STATE_EMERGENCY;
        state_changed = 1;
        printf("[%7lu ms] Reset ABORTED - emergency still active!\r\n", now);
        return;
    }

    /* Check if held long enough */
    if (now - reset_start >= RESET_HOLD_MS) {
        /* Reset successful */
        system_state  = STATE_NORMAL;
        state_changed = 1;
        Buzzer_Off();
        LED_On();
        printf("[%7lu ms] >>> SYSTEM RESET - Returning to normal <<<\r\n", now);
    }
}

/* ========================= LED Control =================================== */
void LED_On(void) {
    HAL_GPIO_WritePin(LED_STATUS_PORT, LED_STATUS_PIN, LED_ON_STATE);
}

void LED_Off(void) {
    HAL_GPIO_WritePin(LED_STATUS_PORT, LED_STATUS_PIN, LED_OFF_STATE);
}

/* ========================= Buzzer Control ================================ */
void Buzzer_On(void) {
    HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, GPIO_PIN_SET);
}

void Buzzer_Off(void) {
    HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, GPIO_PIN_RESET);
}

/* ========================= State to String =============================== */
const char *State_To_String(SystemState_t state) {
    switch (state) {
        case STATE_NORMAL:    return "NORMAL (safe)";
        case STATE_EMERGENCY: return "*** EMERGENCY ***";
        case STATE_RESETTING: return "RESETTING (hold button)";
        default:              return "UNKNOWN";
    }
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
