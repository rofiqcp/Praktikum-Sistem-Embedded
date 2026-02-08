/**
 * STM32_03_UART_Ring_Buffer
 * -------------------------
 * Custom ring buffer for USART1 interrupt-driven RX.
 * ISR pushes received bytes into ring buffer.
 * Main loop pops bytes and echoes them back.
 */

#ifdef STM32F1
#include "stm32f1xx_hal.h"
#elif defined(STM32F4)
#include "stm32f4xx_hal.h"
#endif
#include "config.h"
#include <string.h>
#include <stdbool.h>

/* ================================================================
 *  Ring Buffer
 * ================================================================ */
typedef struct {
    uint8_t  buf[RING_BUF_SIZE];
    volatile uint16_t head;     /* write index  */
    volatile uint16_t tail;     /* read index   */
    volatile uint16_t count;    /* bytes stored */
    uint16_t size;              /* capacity     */
} ring_buffer_t;

static ring_buffer_t rx_rb;

static void rb_init(ring_buffer_t *rb, uint16_t size)
{
    rb->head  = 0;
    rb->tail  = 0;
    rb->count = 0;
    rb->size  = size;
}

static bool rb_is_full(const ring_buffer_t *rb)
{
    return rb->count >= rb->size;
}

static bool rb_is_empty(const ring_buffer_t *rb)
{
    return rb->count == 0;
}

static bool rb_push(ring_buffer_t *rb, uint8_t data)
{
    if (rb_is_full(rb)) return false;
    rb->buf[rb->head] = data;
    rb->head = (rb->head + 1) % rb->size;
    rb->count++;
    return true;
}

static bool rb_pop(ring_buffer_t *rb, uint8_t *data)
{
    if (rb_is_empty(rb)) return false;
    *data = rb->buf[rb->tail];
    rb->tail = (rb->tail + 1) % rb->size;
    rb->count--;
    return true;
}

/* ================================================================
 *  Global handles
 * ================================================================ */
UART_HandleTypeDef huart1;
static volatile uint8_t isr_rx_byte;   /* single-byte ISR buffer */

/* -------- Prototypes -------- */
static void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART1_UART_Init(void);
void Error_Handler(void);

int _write(int file, char *ptr, int len) { (void)file; (void)ptr; return len; }

/* ================================================================ */
int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USART1_UART_Init();

    rb_init(&rx_rb, RING_BUF_SIZE);

    const char *banner = "UART Ring Buffer Ready\r\n";
    HAL_UART_Transmit(&huart1, (uint8_t *)banner, strlen(banner), UART_TIMEOUT);

    /* Arm first interrupt receive */
    HAL_UART_Receive_IT(&huart1, (uint8_t *)&isr_rx_byte, 1);

    uint8_t ch;

    while (1)
    {
        /* Pop from ring buffer and echo */
        while (rb_pop(&rx_rb, &ch))
        {
            HAL_UART_Transmit(&huart1, &ch, 1, UART_TIMEOUT);
            HAL_GPIO_TogglePin(LED_PORT, LED_PIN);
        }

        HAL_Delay(1);   /* small yield */
    }
}

/* ================================================================
 *  Callbacks & IRQ
 * ================================================================ */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1)
    {
        rb_push(&rx_rb, isr_rx_byte);
        HAL_UART_Receive_IT(&huart1, (uint8_t *)&isr_rx_byte, 1);
    }
}

void USART1_IRQHandler(void)
{
    HAL_UART_IRQHandler(&huart1);
}

/* ================================================================
 *  Clock – F103 72 MHz / F4xx 84 MHz
 * ================================================================ */
static void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInit = {0};
    RCC_ClkInitTypeDef RCC_ClkInit = {0};

#if defined(STM32F103xB)
    RCC_OscInit.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInit.HSEState       = RCC_HSE_ON;
    RCC_OscInit.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    RCC_OscInit.PLL.PLLState   = RCC_PLL_ON;
    RCC_OscInit.PLL.PLLSource  = RCC_PLLSOURCE_HSE;
    RCC_OscInit.PLL.PLLMUL     = RCC_PLL_MUL9;
    if (HAL_RCC_OscConfig(&RCC_OscInit) != HAL_OK) Error_Handler();

    RCC_ClkInit.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                 RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInit.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInit.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInit.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInit.APB2CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&RCC_ClkInit, FLASH_LATENCY_2) != HAL_OK) Error_Handler();

#elif defined(STM32F401xC) || defined(STM32F411xE)
    RCC_OscInit.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInit.HSEState       = RCC_HSE_ON;
    RCC_OscInit.PLL.PLLState   = RCC_PLL_ON;
    RCC_OscInit.PLL.PLLSource  = RCC_PLLSOURCE_HSE;
    RCC_OscInit.PLL.PLLM       = 25;
    RCC_OscInit.PLL.PLLN       = 168;
    RCC_OscInit.PLL.PLLP       = RCC_PLLP_DIV2;
    RCC_OscInit.PLL.PLLQ       = 4;
    if (HAL_RCC_OscConfig(&RCC_OscInit) != HAL_OK) Error_Handler();

    RCC_ClkInit.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                 RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInit.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInit.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInit.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInit.APB2CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&RCC_ClkInit, FLASH_LATENCY_2) != HAL_OK) Error_Handler();
#endif
}

/* ================================================================
 *  GPIO: LED PC13
 * ================================================================ */
static void MX_GPIO_Init(void)
{
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Pin   = LED_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_PORT, &GPIO_InitStruct);

    HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET);
}

/* ================================================================
 *  USART1 Init – PA9 TX, PA10 RX + NVIC
 * ================================================================ */
static void MX_USART1_UART_Init(void)
{
    __HAL_RCC_USART1_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};

#if defined(STM32F103xB)
    GPIO_InitStruct.Pin   = GPIO_PIN_9;
    GPIO_InitStruct.Mode  = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin   = GPIO_PIN_10;
    GPIO_InitStruct.Mode  = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

#elif defined(STM32F401xC) || defined(STM32F411xE)
    GPIO_InitStruct.Pin       = GPIO_PIN_9 | GPIO_PIN_10;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_PULLUP;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
#endif

    huart1.Instance          = USART1;
    huart1.Init.BaudRate     = UART_BAUD;
    huart1.Init.WordLength   = UART_WORDLENGTH_8B;
    huart1.Init.StopBits     = UART_STOPBITS_1;
    huart1.Init.Parity       = UART_PARITY_NONE;
    huart1.Init.Mode         = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart1) != HAL_OK) Error_Handler();

    HAL_NVIC_SetPriority(USART1_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(USART1_IRQn);
}

/* ================================================================ */
void Error_Handler(void)
{
    __disable_irq();
    while (1) { }
}
