/**
 * STM32_07_UART_Line_Editor
 *
 * Interactive line editor via USART1.
 * - Support backspace (0x08, 0x7F), echo characters
 * - Prompt "> " displayed
 * - Process complete line on Enter (0x0D)
 * - Store last 5 commands, recall with up/down arrow (ESC sequences)
 */

#include "config.h"
#include <string.h>
#include <stdio.h>

#ifdef STM32F103xB
  #include "stm32f1xx_hal.h"
#elif defined(STM32F401xC)
  #include "stm32f4xx_hal.h"
#elif defined(STM32F411xE)
  #include "stm32f4xx_hal.h"
#endif

/* ---- Global Variables ---- */
UART_HandleTypeDef huart1;

static uint8_t rx_byte;

/* Line buffer */
static char    line_buf[MAX_LINE];
static uint16_t line_pos = 0;
static volatile uint8_t line_ready = 0;

/* History */
static char    history[HISTORY_SIZE][MAX_LINE];
static int     history_count = 0;
static int     history_index = -1;  /* -1 = not browsing history */

/* ESC sequence state machine */
typedef enum {
    ESC_NONE,
    ESC_GOT_ESC,    /* Received 0x1B */
    ESC_GOT_BRACKET /* Received 0x1B 0x5B */
} EscState_t;

static EscState_t esc_state = ESC_NONE;

/* ---- Forward Declarations ---- */
static void SystemClock_Config(void);
static void GPIO_Init(void);
static void UART1_Init(void);
static void UART_SendString(const char *str);
static void UART_SendChar(char c);
static void process_line(const char *line);
static void history_add(const char *line);
static void history_recall(int direction);
static void clear_current_line(void);

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

    UART_SendString("\r\n=== UART Line Editor ===\r\n");
    UART_SendString("Features: echo, backspace, command history (Up/Down arrows)\r\n");
    UART_SendString("> ");

    HAL_UART_Receive_IT(&huart1, &rx_byte, 1);

    while (1) {
        if (line_ready) {
            line_ready = 0;
            process_line(line_buf);
            UART_SendString("> ");
        }
    }
}

/* ---- Process Line ---- */
static void process_line(const char *line) {
    if (strlen(line) == 0) return;

    /* Add to history */
    history_add(line);
    history_index = -1;

    /* Echo the command back as confirmation */
    UART_SendString("Executed: ");
    UART_SendString(line);
    UART_SendString("\r\n");

    /* Built-in commands */
    if (strcmp(line, "history") == 0) {
        UART_SendString("Command history:\r\n");
        int start = (history_count > HISTORY_SIZE) ? history_count - HISTORY_SIZE : 0;
        for (int i = start; i < history_count; i++) {
            char buf[MAX_LINE + 16];
            int idx = i % HISTORY_SIZE;
            snprintf(buf, sizeof(buf), "  [%d] %s\r\n", i + 1, history[idx]);
            UART_SendString(buf);
        }
    } else if (strcmp(line, "clear") == 0) {
        /* Send ANSI clear screen */
        UART_SendString("\033[2J\033[H");
    } else if (strcmp(line, "help") == 0) {
        UART_SendString("Available commands:\r\n");
        UART_SendString("  history - Show command history\r\n");
        UART_SendString("  clear   - Clear screen\r\n");
        UART_SendString("  help    - Show this help\r\n");
        UART_SendString("  Up/Down - Browse command history\r\n");
    }
}

/* ---- History Management ---- */
static void history_add(const char *line) {
    int idx = history_count % HISTORY_SIZE;
    strncpy(history[idx], line, MAX_LINE - 1);
    history[idx][MAX_LINE - 1] = '\0';
    history_count++;
}

static void clear_current_line(void) {
    /* Erase current line on terminal */
    while (line_pos > 0) {
        UART_SendString("\b \b");
        line_pos--;
    }
    line_buf[0] = '\0';
}

static void history_recall(int direction) {
    int total = (history_count < HISTORY_SIZE) ? history_count : HISTORY_SIZE;
    if (total == 0) return;

    if (direction == -1) {
        /* Up arrow - go back in history */
        if (history_index == -1) {
            history_index = history_count - 1;
        } else if (history_index > 0 &&
                   history_index > history_count - total) {
            history_index--;
        } else {
            return; /* Already at oldest */
        }
    } else {
        /* Down arrow - go forward in history */
        if (history_index == -1) return;
        history_index++;
        if (history_index >= history_count) {
            /* Past most recent - clear line */
            history_index = -1;
            clear_current_line();
            return;
        }
    }

    /* Display recalled command */
    clear_current_line();
    int idx = history_index % HISTORY_SIZE;
    strcpy(line_buf, history[idx]);
    line_pos = (uint16_t)strlen(line_buf);
    UART_SendString(line_buf);
}

/* ---- UART RX Callback ---- */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART1) {
        switch (esc_state) {
            case ESC_NONE:
                if (rx_byte == 0x1B) {
                    /* ESC received */
                    esc_state = ESC_GOT_ESC;
                } else if (rx_byte == '\r' || rx_byte == '\n') {
                    /* Enter */
                    line_buf[line_pos] = '\0';
                    UART_SendString("\r\n");
                    if (line_pos > 0) {
                        line_ready = 1;
                    } else {
                        /* Empty line, just reprint prompt from main loop */
                        line_ready = 1;
                        line_buf[0] = '\0';
                    }
                    line_pos = 0;
                } else if (rx_byte == 0x08 || rx_byte == 0x7F) {
                    /* Backspace */
                    if (line_pos > 0) {
                        line_pos--;
                        line_buf[line_pos] = '\0';
                        UART_SendString("\b \b");
                    }
                } else {
                    /* Normal character */
                    if (line_pos < MAX_LINE - 1) {
                        line_buf[line_pos++] = (char)rx_byte;
                        UART_SendChar((char)rx_byte);
                    }
                }
                break;

            case ESC_GOT_ESC:
                if (rx_byte == '[') {
                    esc_state = ESC_GOT_BRACKET;
                } else {
                    esc_state = ESC_NONE;
                }
                break;

            case ESC_GOT_BRACKET:
                esc_state = ESC_NONE;
                if (rx_byte == 'A') {
                    /* Up arrow */
                    history_recall(-1);
                } else if (rx_byte == 'B') {
                    /* Down arrow */
                    history_recall(1);
                }
                /* Ignore other escape sequences */
                break;
        }

        HAL_UART_Receive_IT(&huart1, &rx_byte, 1);
    }
}

/* ---- UART Send ---- */
static void UART_SendString(const char *str) {
    HAL_UART_Transmit(&huart1, (uint8_t *)str, (uint16_t)strlen(str), HAL_MAX_DELAY);
}

static void UART_SendChar(char c) {
    HAL_UART_Transmit(&huart1, (uint8_t *)&c, 1, HAL_MAX_DELAY);
}

/* ---- Peripheral Init ---- */
static void GPIO_Init(void) {
    __HAL_RCC_GPIOA_CLK_ENABLE();
}

static void UART1_Init(void) {
    __HAL_RCC_USART1_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};

#ifdef STM32F103xB
    GPIO_InitStruct.Pin   = USART1_TX_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(USART1_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin  = USART1_RX_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(USART1_PORT, &GPIO_InitStruct);
#else
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
  #else
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
