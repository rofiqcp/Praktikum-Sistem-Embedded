/**
 * STM32_08_UART_Framing_STX_ETX
 *
 * Binary framing protocol over USART1.
 * Frame format: STX | escaped_data | ETX
 * Byte stuffing: if data contains STX/ETX/DLE, prefix with DLE.
 * frame_encode() and frame_decode() functions.
 * Sends test frames periodically, receives and validates.
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

/* Receiver state machine */
typedef enum {
    FRAME_IDLE,
    FRAME_RECEIVING,
    FRAME_ESCAPE
} FrameState_t;

static FrameState_t frame_state = FRAME_IDLE;
static uint8_t  frame_rx_buf[MAX_FRAME];
static uint16_t frame_rx_len = 0;
static volatile uint8_t frame_received = 0;

static uint32_t frames_sent     = 0;
static uint32_t frames_received = 0;
static uint32_t frame_errors    = 0;

/* ---- Forward Declarations ---- */
static void SystemClock_Config(void);
static void GPIO_Init(void);
static void UART1_Init(void);
static void UART_SendString(const char *str);
static void UART_SendByte(uint8_t b);

static int  frame_encode(const uint8_t *data, uint16_t len,
                         uint8_t *out, uint16_t out_max);
static int  frame_decode(const uint8_t *frame, uint16_t frame_len,
                         uint8_t *out, uint16_t out_max);
static void frame_send(const uint8_t *data, uint16_t len);
static void process_received_frame(const uint8_t *data, uint16_t len);
static void print_hex(const uint8_t *data, uint16_t len);

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

    UART_SendString("\r\n=== UART Framing STX/ETX Protocol ===\r\n");
    UART_SendString("STX=0x02, ETX=0x03, DLE=0x10\r\n");
    UART_SendString("Sending test frames every 3 seconds...\r\n\r\n");

    HAL_UART_Receive_IT(&huart1, &rx_byte, 1);

    uint32_t last_send = 0;
    uint8_t  test_counter = 0;

    while (1) {
        /* Process received frames */
        if (frame_received) {
            frame_received = 0;

            uint8_t decoded[MAX_FRAME];
            int decoded_len = frame_decode(frame_rx_buf, frame_rx_len,
                                           decoded, sizeof(decoded));
            if (decoded_len > 0) {
                frames_received++;
                process_received_frame(decoded, (uint16_t)decoded_len);
            } else {
                frame_errors++;
                UART_SendString("[ERROR] Frame decode failed\r\n");
            }
        }

        /* Send test frames periodically */
        uint32_t now = HAL_GetTick();
        if (now - last_send >= FRAME_SEND_INTERVAL) {
            last_send = now;
            test_counter++;

            /* Test 1: Simple text data */
            if (test_counter % 3 == 1) {
                const char *msg = "Hello STX/ETX!";
                UART_SendString("[TX] Sending text: \"");
                UART_SendString(msg);
                UART_SendString("\"\r\n");
                frame_send((const uint8_t *)msg, (uint16_t)strlen(msg));
            }
            /* Test 2: Data containing special bytes (STX, ETX, DLE) */
            else if (test_counter % 3 == 2) {
                uint8_t special_data[] = {
                    0x41, STX, 0x42, ETX, 0x43, DLE, 0x44
                };
                UART_SendString("[TX] Sending data with special bytes: ");
                print_hex(special_data, sizeof(special_data));
                UART_SendString("\r\n");
                frame_send(special_data, sizeof(special_data));
            }
            /* Test 3: Binary counter data */
            else {
                uint8_t counter_data[8];
                for (int i = 0; i < 8; i++) {
                    counter_data[i] = (uint8_t)(test_counter + i);
                }
                UART_SendString("[TX] Sending counter data: ");
                print_hex(counter_data, sizeof(counter_data));
                UART_SendString("\r\n");
                frame_send(counter_data, sizeof(counter_data));
            }

            /* Status report */
            char buf[80];
            snprintf(buf, sizeof(buf),
                     "[STAT] Sent:%lu  Rcvd:%lu  Errors:%lu\r\n\r\n",
                     (unsigned long)frames_sent,
                     (unsigned long)frames_received,
                     (unsigned long)frame_errors);
            UART_SendString(buf);

            HAL_GPIO_TogglePin(LED_PORT, LED_PIN);
        }
    }
}

/* ---- Frame Encode ---- */
/**
 * Encode data into a framed packet: STX | escaped_data | ETX
 * Returns total encoded length, or -1 on error.
 */
static int frame_encode(const uint8_t *data, uint16_t len,
                        uint8_t *out, uint16_t out_max) {
    uint16_t pos = 0;

    if (pos >= out_max) return -1;
    out[pos++] = STX;

    for (uint16_t i = 0; i < len; i++) {
        if (data[i] == STX || data[i] == ETX || data[i] == DLE) {
            /* Byte stuffing: prefix with DLE */
            if (pos + 2 > out_max) return -1;
            out[pos++] = DLE;
            out[pos++] = data[i];
        } else {
            if (pos + 1 > out_max) return -1;
            out[pos++] = data[i];
        }
    }

    if (pos >= out_max) return -1;
    out[pos++] = ETX;

    return (int)pos;
}

/* ---- Frame Decode ---- */
/**
 * Decode a received frame buffer (already stripped of STX/ETX by receiver).
 * Handles DLE un-stuffing.
 * Returns decoded data length, or -1 on error.
 */
static int frame_decode(const uint8_t *frame, uint16_t frame_len,
                        uint8_t *out, uint16_t out_max) {
    uint16_t pos = 0;
    uint16_t i = 0;

    while (i < frame_len) {
        if (frame[i] == DLE) {
            i++;
            if (i >= frame_len) return -1; /* DLE at end = error */
            if (pos >= out_max) return -1;
            out[pos++] = frame[i++];
        } else {
            if (pos >= out_max) return -1;
            out[pos++] = frame[i++];
        }
    }

    return (int)pos;
}

/* ---- Frame Send ---- */
static void frame_send(const uint8_t *data, uint16_t len) {
    uint8_t encoded[MAX_FRAME * 2];
    int enc_len = frame_encode(data, len, encoded, sizeof(encoded));

    if (enc_len > 0) {
        HAL_UART_Transmit(&huart1, encoded, (uint16_t)enc_len, HAL_MAX_DELAY);
        frames_sent++;
    }
}

/* ---- Process Received Frame ---- */
static void process_received_frame(const uint8_t *data, uint16_t len) {
    UART_SendString("[RX] Decoded data (");

    char buf[16];
    snprintf(buf, sizeof(buf), "%u bytes): ", len);
    UART_SendString(buf);

    /* Try to print as text if all printable */
    uint8_t printable = 1;
    for (uint16_t i = 0; i < len; i++) {
        if (data[i] < 0x20 || data[i] > 0x7E) {
            printable = 0;
            break;
        }
    }

    if (printable) {
        UART_SendString("\"");
        HAL_UART_Transmit(&huart1, data, len, HAL_MAX_DELAY);
        UART_SendString("\"");
    } else {
        print_hex(data, len);
    }
    UART_SendString("\r\n");
}

/* ---- Print Hex ---- */
static void print_hex(const uint8_t *data, uint16_t len) {
    char hex[4];
    for (uint16_t i = 0; i < len; i++) {
        snprintf(hex, sizeof(hex), "%02X ", data[i]);
        UART_SendString(hex);
    }
}

/* ---- UART RX Callback (Frame Receiver State Machine) ---- */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART1) {
        switch (frame_state) {
            case FRAME_IDLE:
                if (rx_byte == STX) {
                    frame_state  = FRAME_RECEIVING;
                    frame_rx_len = 0;
                }
                /* Ignore bytes outside a frame */
                break;

            case FRAME_RECEIVING:
                if (rx_byte == ETX) {
                    /* End of frame */
                    frame_received = 1;
                    frame_state = FRAME_IDLE;
                } else if (rx_byte == DLE) {
                    /* Next byte is escaped */
                    frame_state = FRAME_ESCAPE;
                } else if (rx_byte == STX) {
                    /* Unexpected STX - restart frame */
                    frame_rx_len = 0;
                    frame_errors++;
                } else {
                    if (frame_rx_len < MAX_FRAME) {
                        frame_rx_buf[frame_rx_len++] = rx_byte;
                    } else {
                        /* Buffer overflow */
                        frame_state = FRAME_IDLE;
                        frame_errors++;
                    }
                }
                break;

            case FRAME_ESCAPE:
                /* Store the escaped byte as-is (with DLE prefix for decode) */
                if (frame_rx_len + 2 <= MAX_FRAME) {
                    frame_rx_buf[frame_rx_len++] = DLE;
                    frame_rx_buf[frame_rx_len++] = rx_byte;
                } else {
                    frame_state = FRAME_IDLE;
                    frame_errors++;
                }
                frame_state = FRAME_RECEIVING;
                break;
        }

        HAL_UART_Receive_IT(&huart1, &rx_byte, 1);
    }
}

/* ---- UART Send ---- */
static void UART_SendString(const char *str) {
    HAL_UART_Transmit(&huart1, (uint8_t *)str, (uint16_t)strlen(str), HAL_MAX_DELAY);
}

static void UART_SendByte(uint8_t b) {
    HAL_UART_Transmit(&huart1, &b, 1, HAL_MAX_DELAY);
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
    HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET);
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
