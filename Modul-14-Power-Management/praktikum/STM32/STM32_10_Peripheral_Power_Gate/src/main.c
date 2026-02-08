/* ============================================================================
 * STM32_10_Peripheral_Power_Gate - Peripheral Clock Gating
 * ============================================================================
 * Demonstrates disabling unused peripheral clocks to reduce power consumption.
 *
 * - Show all RCC peripheral clock enables
 * - Systematically disable unused peripherals (GPIO ports, SPI, I2C, TIM, ADC)
 * - Print which clocks are enabled/disabled
 * - Measure estimated power savings
 * ============================================================================ */
#include "config.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
/* ---- Private variables --------------------------------------------------- */
static UART_HandleTypeDef huart1;
/* Peripheral current estimates (mA) - approximate values */
typedef struct {
    const char *name;
    uint32_t    rcc_reg_offset; /* offset in RCC registers */
    uint32_t    rcc_bit;
    float       current_mA;
    uint8_t     can_disable;    /* 1 = safe to disable in this demo */
} PeripheralInfo_t;
/* APB2 peripherals */
static PeripheralInfo_t apb2_periphs[] = {
    {"GPIOA",  0, RCC_APB2ENR_IOPAEN,   0.5f, 0},  /* Keep: UART pins */
    {"GPIOB",  0, RCC_APB2ENR_IOPBEN,   0.5f, 1},
    {"GPIOC",  0, RCC_APB2ENR_IOPCEN,   0.5f, 0},  /* Keep: LED */
    {"GPIOD",  0, RCC_APB2ENR_IOPDEN,   0.5f, 1},
    {"ADC1",   0, RCC_APB2ENR_ADC1EN,   1.0f, 1},
    {"ADC2",   0, RCC_APB2ENR_ADC2EN,   1.0f, 1},
    {"SPI1",   0, RCC_APB2ENR_SPI1EN,   0.8f, 1},
    {"TIM1",   0, RCC_APB2ENR_TIM1EN,   0.3f, 1},
    {"USART1", 0, RCC_APB2ENR_USART1EN, 0.5f, 0},  /* Keep: debug output */
    {"AFIO",   0, RCC_APB2ENR_AFIOEN,   0.1f, 0},  /* Keep: AF config */
};
#define APB2_PERIPH_COUNT (sizeof(apb2_periphs)/sizeof(apb2_periphs[0]))
/* APB1 peripherals */
static PeripheralInfo_t apb1_periphs[] = {
    {"TIM2",   0, RCC_APB1ENR_TIM2EN,   0.3f, 1},
    {"TIM3",   0, RCC_APB1ENR_TIM3EN,   0.3f, 1},
    {"TIM4",   0, RCC_APB1ENR_TIM4EN,   0.3f, 1},
    {"WWDG",   0, RCC_APB1ENR_WWDGEN,   0.1f, 1},
    {"SPI2",   0, RCC_APB1ENR_SPI2EN,   0.8f, 1},
    {"USART2", 0, RCC_APB1ENR_USART2EN, 0.5f, 1},
    {"USART3", 0, RCC_APB1ENR_USART3EN, 0.5f, 1},
    {"I2C1",   0, RCC_APB1ENR_I2C1EN,   0.6f, 1},
    {"I2C2",   0, RCC_APB1ENR_I2C2EN,   0.6f, 1},
    {"USB",    0, RCC_APB1ENR_USBEN,    1.5f, 1},
    {"PWR",    0, RCC_APB1ENR_PWREN,    0.1f, 0},  /* Keep: power control */
    {"BKP",    0, RCC_APB1ENR_BKPEN,    0.1f, 0},  /* Keep: backup regs */
};
#define APB1_PERIPH_COUNT (sizeof(apb1_periphs)/sizeof(apb1_periphs[0]))
/* ---- UART Debug (PA9 TX, PA10 RX) ---- */
static void UART_Init(void)
{
    __HAL_RCC_USART1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    GPIO_InitTypeDef g = {0};
#ifdef STM32F103xB
    g.Pin   = GPIO_PIN_9;
    g.Mode  = GPIO_MODE_AF_PP;
    g.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &g);
    g.Pin   = GPIO_PIN_10;
    g.Mode  = GPIO_MODE_INPUT;
    g.Pull  = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &g);
#elif defined(STM32F401xC) || defined(STM32F411xE)
    g.Pin       = GPIO_PIN_9 | GPIO_PIN_10;
    g.Mode      = GPIO_MODE_AF_PP;
    g.Pull      = GPIO_PULLUP;
    g.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
    g.Alternate = GPIO_AF7_USART1;
    HAL_GPIO_Init(GPIOA, &g);
#endif
    huart1.Instance          = USART1;
    huart1.Init.BaudRate     = 115200;
    huart1.Init.WordLength   = UART_WORDLENGTH_8B;
    huart1.Init.StopBits     = UART_STOPBITS_1;
    huart1.Init.Parity       = UART_PARITY_NONE;
    huart1.Init.Mode         = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(&huart1);
}
static void UART_Printf(const char *fmt, ...)
{
    char buf[256];
    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    if (len > 0) HAL_UART_Transmit(&huart1, (uint8_t *)buf, len, HAL_MAX_DELAY);
}
/* ============================================================
 *  System Clock Configuration (Multi-platform)
 * ============================================================ */
#ifdef STM32F103xB
/* F103: 8 MHz HSE -> PLL x9 -> 72 MHz SYSCLK */
static void SystemClock_Config(void)
{
    RCC_OscInitTypeDef osc = {0};
    RCC_ClkInitTypeDef clk = {0};
    osc.OscillatorType      = RCC_OSCILLATORTYPE_HSE;
    osc.HSEState            = RCC_HSE_ON;
    osc.HSEPredivValue      = RCC_HSE_PREDIV_DIV1;
    osc.PLL.PLLState        = RCC_PLL_ON;
    osc.PLL.PLLSource       = RCC_PLLSOURCE_HSE;
    osc.PLL.PLLMUL          = RCC_PLL_MUL9;
    HAL_RCC_OscConfig(&osc);
    clk.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                       | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    clk.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    clk.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    clk.APB1CLKDivider = RCC_HCLK_DIV2;
    clk.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_2);
}
#elif defined(STM32F401xC) || defined(STM32F411xE)
/* F4xx: 25 MHz HSE -> PLL -> 84 MHz SYSCLK */
static void SystemClock_Config(void)
{
    RCC_OscInitTypeDef osc = {0};
    RCC_ClkInitTypeDef clk = {0};
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);
    osc.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    osc.HSEState       = RCC_HSE_ON;
    osc.PLL.PLLState   = RCC_PLL_ON;
    osc.PLL.PLLSource  = RCC_PLLSOURCE_HSE;
    osc.PLL.PLLM       = 25;
    osc.PLL.PLLN       = 336;
    osc.PLL.PLLP       = RCC_PLLP_DIV4;   /* 336/4 = 84 MHz */
    osc.PLL.PLLQ       = 7;
    HAL_RCC_OscConfig(&osc);
    clk.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                       | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    clk.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    clk.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    clk.APB1CLKDivider = RCC_HCLK_DIV2;
    clk.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_2);
}
#endif
/* ---- LED ----------------------------------------------------------------- */
static void LED_Init(void)
{
    __HAL_RCC_GPIOC_CLK_ENABLE();
    GPIO_InitTypeDef gpio = {0};
    gpio.Pin   = LED_PIN;
    gpio.Mode  = GPIO_MODE_OUTPUT_PP;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_PORT, &gpio);
}
/* ---- Enable all peripheral clocks for baseline --------------------------- */
static void Enable_All_Peripheral_Clocks(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_ADC1_CLK_ENABLE();
    __HAL_RCC_ADC2_CLK_ENABLE();
    __HAL_RCC_SPI1_CLK_ENABLE();
    __HAL_RCC_TIM1_CLK_ENABLE();
    __HAL_RCC_AFIO_CLK_ENABLE();
    __HAL_RCC_TIM2_CLK_ENABLE();
    __HAL_RCC_TIM3_CLK_ENABLE();
    __HAL_RCC_TIM4_CLK_ENABLE();
    __HAL_RCC_WWDG_CLK_ENABLE();
    __HAL_RCC_SPI2_CLK_ENABLE();
    __HAL_RCC_USART2_CLK_ENABLE();
    __HAL_RCC_USART3_CLK_ENABLE();
    __HAL_RCC_I2C1_CLK_ENABLE();
    __HAL_RCC_I2C2_CLK_ENABLE();
    __HAL_RCC_USB_CLK_ENABLE();
    __HAL_RCC_PWR_CLK_ENABLE();
    #ifdef STM32F103xB
    __HAL_RCC_BKP_CLK_ENABLE();
    #endif
}
/* ---- Peripheral Power Gate Task ------------------------------------------ */
static void vPowerGateTask(void *pvParameters)
{
    (void)pvParameters;
    UART_Printf("\r\n========================================\r\n");
    UART_Printf("  STM32 Peripheral Clock Gating Demo\r\n");
    UART_Printf("========================================\r\n\r\n");
    for (;;) {
        /* Phase 1: Enable ALL clocks */
        UART_Printf("[PHASE 1] Enabling ALL peripheral clocks...\r\n");
        Enable_All_Peripheral_Clocks();
        vTaskDelay(pdMS_TO_TICKS(500));
        /* Read and display APB2 register */
        uint32_t apb2enr = RCC->APB2ENR;
        uint32_t apb1enr = RCC->APB1ENR;
        uint32_t ahbenr  = RCC->AHBENR;
        UART_Printf("[REG] RCC_APB2ENR = 0x%08lX\r\n", apb2enr);
        UART_Printf("[REG] RCC_APB1ENR = 0x%08lX\r\n", apb1enr);
        UART_Printf("[REG] RCC_AHBENR  = 0x%08lX\r\n\r\n", ahbenr);
        float total_all = 0;
        UART_Printf("APB2 Peripherals (all enabled):\r\n");
        for (uint32_t i = 0; i < APB2_PERIPH_COUNT; i++) {
            uint8_t en = (apb2enr & apb2_periphs[i].rcc_bit) ? 1 : 0;
            UART_Printf("  %-8s: %s  (~%.1f mA)\r\n",
                        apb2_periphs[i].name,
                        en ? "ON " : "OFF",
                        en ? apb2_periphs[i].current_mA : 0.0f);
            if (en) total_all += apb2_periphs[i].current_mA;
        }
        UART_Printf("\r\nAPB1 Peripherals (all enabled):\r\n");
        for (uint32_t i = 0; i < APB1_PERIPH_COUNT; i++) {
            uint8_t en = (apb1enr & apb1_periphs[i].rcc_bit) ? 1 : 0;
            UART_Printf("  %-8s: %s  (~%.1f mA)\r\n",
                        apb1_periphs[i].name,
                        en ? "ON " : "OFF",
                        en ? apb1_periphs[i].current_mA : 0.0f);
            if (en) total_all += apb1_periphs[i].current_mA;
        }
        UART_Printf("\r\n[TOTAL] All peripherals current: ~%.1f mA (peripheral only)\r\n", total_all);
        vTaskDelay(pdMS_TO_TICKS(2000));
        /* Phase 2: Disable unused clocks */
        UART_Printf("\r\n[PHASE 2] Disabling UNUSED peripheral clocks...\r\n");
        __HAL_RCC_GPIOB_CLK_DISABLE();
        __HAL_RCC_GPIOD_CLK_DISABLE();
        __HAL_RCC_ADC1_CLK_DISABLE();
        __HAL_RCC_ADC2_CLK_DISABLE();
        __HAL_RCC_SPI1_CLK_DISABLE();
        __HAL_RCC_TIM1_CLK_DISABLE();
        __HAL_RCC_TIM2_CLK_DISABLE();
        __HAL_RCC_TIM3_CLK_DISABLE();
        __HAL_RCC_TIM4_CLK_DISABLE();
        __HAL_RCC_WWDG_CLK_DISABLE();
        __HAL_RCC_SPI2_CLK_DISABLE();
        __HAL_RCC_USART2_CLK_DISABLE();
        __HAL_RCC_USART3_CLK_DISABLE();
        __HAL_RCC_I2C1_CLK_DISABLE();
        __HAL_RCC_I2C2_CLK_DISABLE();
        __HAL_RCC_USB_CLK_DISABLE();
        vTaskDelay(pdMS_TO_TICKS(200));
        apb2enr = RCC->APB2ENR;
        apb1enr = RCC->APB1ENR;
        UART_Printf("[REG] RCC_APB2ENR = 0x%08lX\r\n", apb2enr);
        UART_Printf("[REG] RCC_APB1ENR = 0x%08lX\r\n\r\n", apb1enr);
        float total_min = 0, saved = 0;
        UART_Printf("APB2 Peripherals (optimized):\r\n");
        for (uint32_t i = 0; i < APB2_PERIPH_COUNT; i++) {
            uint8_t en = (apb2enr & apb2_periphs[i].rcc_bit) ? 1 : 0;
            UART_Printf("  %-8s: %s  (~%.1f mA) %s\r\n",
                        apb2_periphs[i].name,
                        en ? "ON " : "OFF",
                        en ? apb2_periphs[i].current_mA : 0.0f,
                        (!en && apb2_periphs[i].can_disable) ? "<-- SAVED" : "");
            if (en) total_min += apb2_periphs[i].current_mA;
            if (!en && apb2_periphs[i].can_disable) saved += apb2_periphs[i].current_mA;
        }
        UART_Printf("\r\nAPB1 Peripherals (optimized):\r\n");
        for (uint32_t i = 0; i < APB1_PERIPH_COUNT; i++) {
            uint8_t en = (apb1enr & apb1_periphs[i].rcc_bit) ? 1 : 0;
            UART_Printf("  %-8s: %s  (~%.1f mA) %s\r\n",
                        apb1_periphs[i].name,
                        en ? "ON " : "OFF",
                        en ? apb1_periphs[i].current_mA : 0.0f,
                        (!en && apb1_periphs[i].can_disable) ? "<-- SAVED" : "");
            if (en) total_min += apb1_periphs[i].current_mA;
            if (!en && apb1_periphs[i].can_disable) saved += apb1_periphs[i].current_mA;
        }
        UART_Printf("\r\n========================================\r\n");
        UART_Printf("  Power Budget Summary\r\n");
        UART_Printf("========================================\r\n");
        UART_Printf("  All clocks ON:       ~%.1f mA (periph)\r\n", total_all);
        UART_Printf("  Optimized:           ~%.1f mA (periph)\r\n", total_min);
        UART_Printf("  Current saved:       ~%.1f mA\r\n", saved);
        UART_Printf("  Saving percentage:   ~%.1f%%\r\n",
                    total_all > 0 ? (saved / total_all * 100.0f) : 0);
        UART_Printf("========================================\r\n\r\n");
        HAL_GPIO_TogglePin(LED_PORT, LED_PIN);
        UART_Printf("Next cycle in 15 seconds...\r\n\r\n");
        vTaskDelay(pdMS_TO_TICKS(15000));
    }
}
/* ---- FreeRTOS Hooks ------------------------------------------------------ */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask; UART_Printf("[FATAL] Stack overflow: %s\r\n", pcTaskName); for (;;);
}
void vApplicationMallocFailedHook(void)
{
    UART_Printf("[FATAL] Malloc failed!\r\n"); for (;;);
}
static StaticTask_t xIdleTaskTCB;
static StackType_t  uxIdleTaskStack[configMINIMAL_STACK_SIZE];
void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer,
                                   StackType_t **ppxIdleTaskStackBuffer,
                                   uint32_t *pulIdleTaskStackSize)
{
    *ppxIdleTaskTCBBuffer   = &xIdleTaskTCB;
    *ppxIdleTaskStackBuffer = uxIdleTaskStack;
    *pulIdleTaskStackSize   = configMINIMAL_STACK_SIZE;
}
static StaticTask_t xTimerTaskTCB;
static StackType_t  uxTimerTaskStack[configTIMER_TASK_STACK_DEPTH];
void vApplicationGetTimerTaskMemory(StaticTask_t **ppxTimerTaskTCBBuffer,
                                    StackType_t **ppxTimerTaskStackBuffer,
                                    uint32_t *pulTimerTaskStackSize)
{
    *ppxTimerTaskTCBBuffer   = &xTimerTaskTCB;
    *ppxTimerTaskStackBuffer = uxTimerTaskStack;
    *pulTimerTaskStackSize   = configTIMER_TASK_STACK_DEPTH;
}
/* ---- Main ---------------------------------------------------------------- */
int main(void)
{
    HAL_Init();
    SystemClock_Config();
    LED_Init();
    UART_Init();
    xTaskCreate(vPowerGateTask, "PWR_GATE", MAIN_TASK_STACK_SIZE * 2, NULL,
                MAIN_TASK_PRIORITY, NULL);
    vTaskStartScheduler();
    for (;;);
}
