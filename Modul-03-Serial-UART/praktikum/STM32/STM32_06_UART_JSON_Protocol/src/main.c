/**
 * STM32_06_UART_JSON_Protocol
 *
 * Simple JSON parser for UART commands.
 * Receive: {"cmd":"set","pin":13,"val":1}
 * Respond: {"status":"ok","pin":13,"val":1}
 * Manual JSON parsing (find key between quotes, find value).
 * Functions: json_get_string(), json_get_int()
 * Toggle LED PC13 based on JSON commands.
 */

#include "config.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

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
static char     rx_buffer[MAX_JSON_LEN];
static uint16_t rx_index = 0;
static volatile uint8_t msg_ready = 0;

/* ---- Forward Declarations ---- */
static void SystemClock_Config(void);
static void GPIO_Init(void);
static void UART1_Init(void);
static void UART_SendString(const char *str);

static int  json_get_string(const char *json, const char *key, char *out, int out_len);
static int  json_get_int(const char *json, const char *key, int *out);
static void process_json(char *json);

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

    UART_SendString("\r\n=== UART JSON Protocol ===\r\n");
    UART_SendString("Send: {\"cmd\":\"set\",\"pin\":13,\"val\":1}\r\n");
    UART_SendString("Send: {\"cmd\":\"get\",\"pin\":13}\r\n");
    UART_SendString("Send: {\"cmd\":\"toggle\",\"pin\":13}\r\n\r\n");

    HAL_UART_Receive_IT(&huart1, &rx_byte, 1);

    while (1) {
        if (msg_ready) {
            msg_ready = 0;
            process_json(rx_buffer);
        }
    }
}

/* ---- JSON Parser Functions ---- */

/**
 * json_get_string - Extract string value for a given key.
 * Searches for "key":"value" pattern.
 * Returns 0 on success, -1 on failure.
 */
static int json_get_string(const char *json, const char *key, char *out, int out_len) {
    char search[64];
    snprintf(search, sizeof(search), "\"%s\"", key);

    const char *pos = strstr(json, search);
    if (!pos) return -1;

    pos += strlen(search);

    /* Skip whitespace and colon */
    while (*pos == ' ' || *pos == ':') pos++;

    if (*pos != '"') return -1;
    pos++; /* skip opening quote */

    int i = 0;
    while (*pos && *pos != '"' && i < out_len - 1) {
        out[i++] = *pos++;
    }
    out[i] = '\0';
    return 0;
}

/**
 * json_get_int - Extract integer value for a given key.
 * Searches for "key":number pattern.
 * Returns 0 on success, -1 on failure.
 */
static int json_get_int(const char *json, const char *key, int *out) {
    char search[64];
    snprintf(search, sizeof(search), "\"%s\"", key);

    const char *pos = strstr(json, search);
    if (!pos) return -1;

    pos += strlen(search);

    /* Skip whitespace and colon */
    while (*pos == ' ' || *pos == ':') pos++;

    /* Parse integer (handle negative) */
    if (*pos == '-' || (*pos >= '0' && *pos <= '9')) {
        *out = atoi(pos);
        return 0;
    }
    return -1;
}

/* ---- Process JSON Command ---- */
static void process_json(char *json) {
    char cmd[32] = {0};
    int pin = 0;
    int val = 0;
    char response[MAX_JSON_LEN];

    if (json_get_string(json, "cmd", cmd, sizeof(cmd)) != 0) {
        UART_SendString("{\"status\":\"error\",\"msg\":\"missing cmd\"}\r\n");
        return;
    }

    if (strcmp(cmd, "set") == 0) {
        if (json_get_int(json, "pin", &pin) != 0) {
            UART_SendString("{\"status\":\"error\",\"msg\":\"missing pin\"}\r\n");
            return;
        }
        if (json_get_int(json, "val", &val) != 0) {
            UART_SendString("{\"status\":\"error\",\"msg\":\"missing val\"}\r\n");
            return;
        }

        if (pin == 13) {
            /* Active low: val=1 means LED ON = pin RESET */
            HAL_GPIO_WritePin(LED_PORT, LED_PIN,
                              val ? GPIO_PIN_RESET : GPIO_PIN_SET);
            snprintf(response, sizeof(response),
                     "{\"status\":\"ok\",\"pin\":%d,\"val\":%d}\r\n", pin, val);
            UART_SendString(response);
        } else {
            snprintf(response, sizeof(response),
                     "{\"status\":\"error\",\"msg\":\"unsupported pin %d\"}\r\n", pin);
            UART_SendString(response);
        }

    } else if (strcmp(cmd, "get") == 0) {
        if (json_get_int(json, "pin", &pin) != 0) {
            UART_SendString("{\"status\":\"error\",\"msg\":\"missing pin\"}\r\n");
            return;
        }

        if (pin == 13) {
            GPIO_PinState state = HAL_GPIO_ReadPin(LED_PORT, LED_PIN);
            val = (state == GPIO_PIN_RESET) ? 1 : 0; /* Active low */
            snprintf(response, sizeof(response),
                     "{\"status\":\"ok\",\"pin\":%d,\"val\":%d}\r\n", pin, val);
            UART_SendString(response);
        } else {
            snprintf(response, sizeof(response),
                     "{\"status\":\"error\",\"msg\":\"unsupported pin %d\"}\r\n", pin);
            UART_SendString(response);
        }

    } else if (strcmp(cmd, "toggle") == 0) {
        if (json_get_int(json, "pin", &pin) != 0) {
            UART_SendString("{\"status\":\"error\",\"msg\":\"missing pin\"}\r\n");
            return;
        }

        if (pin == 13) {
            HAL_GPIO_TogglePin(LED_PORT, LED_PIN);
            GPIO_PinState state = HAL_GPIO_ReadPin(LED_PORT, LED_PIN);
            val = (state == GPIO_PIN_RESET) ? 1 : 0;
            snprintf(response, sizeof(response),
                     "{\"status\":\"ok\",\"pin\":%d,\"val\":%d}\r\n", pin, val);
            UART_SendString(response);
        } else {
            snprintf(response, sizeof(response),
                     "{\"status\":\"error\",\"msg\":\"unsupported pin %d\"}\r\n", pin);
            UART_SendString(response);
        }

    } else {
        snprintf(response, sizeof(response),
                 "{\"status\":\"error\",\"msg\":\"unknown cmd: %s\"}\r\n", cmd);
        UART_SendString(response);
    }
}

/* ---- UART RX Callback ---- */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART1) {
        if (rx_byte == '\r' || rx_byte == '\n') {
            if (rx_index > 0) {
                rx_buffer[rx_index] = '\0';
                rx_index = 0;
                msg_ready = 1;
            }
        } else {
            if (rx_index < MAX_JSON_LEN - 1) {
                rx_buffer[rx_index++] = (char)rx_byte;
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
    __HAL_RCC_GPIOC_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* LED PC13 */
    GPIO_InitStruct.Pin   = LED_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    HAL_GPIO_Init(LED_PORT, &GPIO_InitStruct);
    HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET); /* OFF */
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
