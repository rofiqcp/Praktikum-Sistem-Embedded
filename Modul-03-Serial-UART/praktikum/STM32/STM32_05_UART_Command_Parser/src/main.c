/**
 * STM32_05_UART_Command_Parser
 * 
 * Parse text commands via USART1: LED ON, LED OFF, BEEP, STATUS, HELP
 * UART interrupt receive into buffer, process on newline.
 * LED on PC13, buzzer on PB1 (simple GPIO toggle).
 * Command table: array of struct {name, handler_func}.
 */

#include "config.h"
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#ifdef STM32F103xB
  #include "stm32f1xx_hal.h"
#elif defined(STM32F401xC)
  #include "stm32f4xx_hal.h"
#elif defined(STM32F411xE)
  #include "stm32f4xx_hal.h"
#endif

/* ---- Global Variables ---- */
UART_HandleTypeDef huart1;

static uint8_t  rx_byte;
static char     rx_buffer[MAX_CMD_LEN];
static uint16_t rx_index = 0;
static volatile uint8_t cmd_ready = 0;

static uint8_t led_state   = 0;
static uint8_t buzzer_state = 0;

/* ---- Forward Declarations ---- */
static void SystemClock_Config(void);
static void GPIO_Init(void);
static void UART1_Init(void);
static void UART_SendString(const char *str);
static void process_command(char *cmd);

/* Command handler prototypes */
static void cmd_led_on(void);
static void cmd_led_off(void);
static void cmd_beep(void);
static void cmd_status(void);
static void cmd_help(void);

/* ---- Command Table ---- */
typedef struct {
    const char *name;
    void (*handler)(void);
} Command_t;

static const Command_t command_table[] = {
    { "LED ON",  cmd_led_on  },
    { "LED OFF", cmd_led_off },
    { "BEEP",    cmd_beep    },
    { "STATUS",  cmd_status  },
    { "HELP",    cmd_help    },
};

#define NUM_COMMANDS  (sizeof(command_table) / sizeof(command_table[0]))

/* ---- printf redirect ---- */
int _write(int file, char *ptr, int len) {
    (void)file;
    HAL_UART_Transmit(&huart1, (uint8_t *)ptr, len, HAL_MAX_DELAY);
    return len;
}

/* ---- Main ---- */
int main(void) {
    HAL_Init();
    SystemClock_Config();
    GPIO_Init();
    UART1_Init();

    UART_SendString("\r\n=== UART Command Parser ===\r\n");
    UART_SendString("Commands: LED ON, LED OFF, BEEP, STATUS, HELP\r\n");
    UART_SendString("> ");

    /* Start interrupt reception */
    HAL_UART_Receive_IT(&huart1, &rx_byte, 1);

    while (1) {
        if (cmd_ready) {
            cmd_ready = 0;
            process_command(rx_buffer);
            UART_SendString("> ");
        }
    }
}

/* ---- Command Handlers ---- */
static void cmd_led_on(void) {
    HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_RESET); /* Active low */
    led_state = 1;
    UART_SendString("LED turned ON\r\n");
}

static void cmd_led_off(void) {
    HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET);
    led_state = 0;
    UART_SendString("LED turned OFF\r\n");
}

static void cmd_beep(void) {
    /* Toggle buzzer briefly */
    HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, GPIO_PIN_SET);
    buzzer_state = 1;
    HAL_Delay(200);
    HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, GPIO_PIN_RESET);
    buzzer_state = 0;
    UART_SendString("BEEP!\r\n");
}

static void cmd_status(void) {
    char buf[64];
    snprintf(buf, sizeof(buf), "LED: %s, Buzzer: %s\r\n",
             led_state ? "ON" : "OFF",
             buzzer_state ? "ON" : "OFF");
    UART_SendString(buf);
}

static void cmd_help(void) {
    UART_SendString("Available commands:\r\n");
    UART_SendString("  LED ON   - Turn on PC13 LED\r\n");
    UART_SendString("  LED OFF  - Turn off PC13 LED\r\n");
    UART_SendString("  BEEP     - Short buzzer beep on PB1\r\n");
    UART_SendString("  STATUS   - Show LED/Buzzer state\r\n");
    UART_SendString("  HELP     - Show this help\r\n");
}

/* ---- Process Command ---- */
static void str_to_upper(char *s) {
    while (*s) {
        *s = (char)toupper((unsigned char)*s);
        s++;
    }
}

static void str_trim(char *s) {
    /* Trim leading */
    char *start = s;
    while (*start == ' ' || *start == '\t') start++;
    if (start != s) memmove(s, start, strlen(start) + 1);
    /* Trim trailing */
    int len = (int)strlen(s);
    while (len > 0 && (s[len - 1] == ' ' || s[len - 1] == '\t' ||
           s[len - 1] == '\r' || s[len - 1] == '\n')) {
        s[--len] = '\0';
    }
}

static void process_command(char *cmd) {
    str_trim(cmd);
    if (strlen(cmd) == 0) return;

    /* Convert to uppercase for matching */
    char upper_cmd[MAX_CMD_LEN];
    strncpy(upper_cmd, cmd, MAX_CMD_LEN - 1);
    upper_cmd[MAX_CMD_LEN - 1] = '\0';
    str_to_upper(upper_cmd);

    for (uint32_t i = 0; i < NUM_COMMANDS; i++) {
        if (strcmp(upper_cmd, command_table[i].name) == 0) {
            command_table[i].handler();
            return;
        }
    }
    UART_SendString("Unknown command: ");
    UART_SendString(cmd);
    UART_SendString("\r\nType HELP for available commands.\r\n");
}

/* ---- UART RX Callback ---- */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART1) {
        if (rx_byte == '\r' || rx_byte == '\n') {
            if (rx_index > 0) {
                rx_buffer[rx_index] = '\0';
                rx_index = 0;
                cmd_ready = 1;
                UART_SendString("\r\n");
            }
        } else if (rx_byte == 0x08 || rx_byte == 0x7F) {
            /* Backspace */
            if (rx_index > 0) {
                rx_index--;
                HAL_UART_Transmit(&huart1, (uint8_t *)"\b \b", 3, HAL_MAX_DELAY);
            }
        } else {
            if (rx_index < MAX_CMD_LEN - 1) {
                rx_buffer[rx_index++] = (char)rx_byte;
                HAL_UART_Transmit(&huart1, &rx_byte, 1, HAL_MAX_DELAY);
            }
        }
        HAL_UART_Receive_IT(&huart1, &rx_byte, 1);
    }
}

/* ---- UART Send ---- */
static void UART_SendString(const char *str) {
    HAL_UART_Transmit(&huart1, (uint8_t *)str, (uint16_t)strlen(str), HAL_MAX_DELAY);
}

/* ---- Peripheral Init ---- */
static void GPIO_Init(void) {
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* LED PC13 */
    GPIO_InitStruct.Pin   = LED_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
#ifdef STM32F103xB
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
#else
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
#endif
    HAL_GPIO_Init(LED_PORT, &GPIO_InitStruct);
    HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET); /* OFF initially */

    /* Buzzer PB1 */
    GPIO_InitStruct.Pin   = BUZZER_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    HAL_GPIO_Init(BUZZER_PORT, &GPIO_InitStruct);
    HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, GPIO_PIN_RESET);
}

static void UART1_Init(void) {
    __HAL_RCC_USART1_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};

#ifdef STM32F103xB
    /* PA9 TX: AF Push-Pull */
    GPIO_InitStruct.Pin   = USART1_TX_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(USART1_PORT, &GPIO_InitStruct);

    /* PA10 RX: Input floating */
    GPIO_InitStruct.Pin  = USART1_RX_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(USART1_PORT, &GPIO_InitStruct);
#else
    /* F4: both AF_PP with alternate */
    GPIO_InitStruct.Pin       = USART1_TX_PIN | USART1_RX_PIN;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_PULLUP;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
    HAL_GPIO_Init(USART1_PORT, &GPIO_InitStruct);
#endif

    huart1.Instance          = USART1;
    huart1.Init.BaudRate     = UART_BAUDRATE;
    huart1.Init.WordLength   = UART_WORDLENGTH_8B;
    huart1.Init.StopBits     = UART_STOPBITS_1;
    huart1.Init.Parity       = UART_PARITY_NONE;
    huart1.Init.Mode         = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(&huart1);

    HAL_NVIC_SetPriority(USART1_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(USART1_IRQn);
}

/* ---- IRQ Handler ---- */
void USART1_IRQHandler(void) {
    HAL_UART_IRQHandler(&huart1);
}

/* ---- System Clock ---- */
static void SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

#ifdef STM32F103xB
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState       = RCC_HSE_ON;
    RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    RCC_OscInitStruct.PLL.PLLState   = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource  = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL     = RCC_PLL_MUL9;
    HAL_RCC_OscConfig(&RCC_OscInitStruct);

    RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                       RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2);
#else
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState       = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState   = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource  = RCC_PLLSOURCE_HSE;
  #if defined(STM32F401xC)
    RCC_OscInitStruct.PLL.PLLM       = 25;
    RCC_OscInitStruct.PLL.PLLN       = 168;
    RCC_OscInitStruct.PLL.PLLP       = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ       = 4;
  #else /* STM32F411xE */
    RCC_OscInitStruct.PLL.PLLM       = 25;
    RCC_OscInitStruct.PLL.PLLN       = 192;
    RCC_OscInitStruct.PLL.PLLP       = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ       = 4;
  #endif
    HAL_RCC_OscConfig(&RCC_OscInitStruct);

    RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                       RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2);
#endif
}
