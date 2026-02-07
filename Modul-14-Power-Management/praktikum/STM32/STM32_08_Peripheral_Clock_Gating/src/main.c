/**
 * ================================================================================
 * PROGRAM 8: PERIPHERAL CLOCK GATING - STM32F103C8T6
 * ================================================================================
 * Deskripsi:
 *   Demonstrasi penghematan daya dengan menonaktifkan clock peripheral
 *   yang tidak sedang digunakan. STM32 HAL menyediakan macro:
 *   - __HAL_RCC_XXX_CLK_ENABLE()  : Aktifkan clock peripheral
 *   - __HAL_RCC_XXX_CLK_DISABLE() : Matikan clock peripheral
 *
 *   Setiap peripheral yang aktif menambah konsumsi arus ~0.5-2mA.
 *   Dengan mematikan peripheral yang tidak dipakai, total penghematan
 *   bisa mencapai 5-10mA.
 *
 * Hardware:
 *   - STM32F103C8T6 (Blue Pill)
 *   - LED PC13 / UART1 (PA9/PA10)
 *
 * ================================================================================
 */

#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include <string.h>
#include <stdio.h>

UART_HandleTypeDef huart1;

static void UART_SendString(const char *str)
{
    HAL_UART_Transmit(&huart1, (uint8_t *)str, strlen(str), HAL_MAX_DELAY);
}

static void UART_Printf(const char *fmt, ...)
{
    char buf[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    UART_SendString(buf);
}

/* ================================================================================
 * KONFIGURASI HARDWARE
 * ================================================================================ */

static void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
    HAL_RCC_OscConfig(&RCC_OscInitStruct);

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                  RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2);
}

static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    GPIO_InitStruct.Pin = GPIO_PIN_13;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
}

static void MX_USART1_UART_Init(void)
{
    __HAL_RCC_USART1_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    huart1.Instance = USART1;
    huart1.Init.BaudRate = 115200;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    HAL_UART_Init(&huart1);
}

/* ================================================================================
 * CLOCK GATING HELPERS
 * ================================================================================ */

typedef struct {
    const char *name;
    uint8_t bus;        /* 0=AHB, 1=APB1, 2=APB2 */
    uint32_t bit_mask;  /* RCC enable bit */
} peripheral_info_t;

/* Daftar peripheral yang bisa di-gate */
static const peripheral_info_t peripherals[] = {
    {"SPI1",    2, RCC_APB2ENR_SPI1EN},
    {"SPI2",    1, RCC_APB1ENR_SPI2EN},
    {"I2C1",    1, RCC_APB1ENR_I2C1EN},
    {"I2C2",    1, RCC_APB1ENR_I2C2EN},
    {"USART2",  1, RCC_APB1ENR_USART2EN},
    {"USART3",  1, RCC_APB1ENR_USART3EN},
    {"TIM2",    1, RCC_APB1ENR_TIM2EN},
    {"TIM3",    1, RCC_APB1ENR_TIM3EN},
    {"TIM4",    1, RCC_APB1ENR_TIM4EN},
    {"ADC1",    2, RCC_APB2ENR_ADC1EN},
    {"ADC2",    2, RCC_APB2ENR_ADC2EN},
};

#define NUM_PERIPHERALS (sizeof(peripherals) / sizeof(peripherals[0]))

static uint8_t is_peripheral_enabled(const peripheral_info_t *p)
{
    switch (p->bus) {
        case 0: return (RCC->AHBENR & p->bit_mask) ? 1 : 0;
        case 1: return (RCC->APB1ENR & p->bit_mask) ? 1 : 0;
        case 2: return (RCC->APB2ENR & p->bit_mask) ? 1 : 0;
    }
    return 0;
}

static void enable_peripheral_clock(const peripheral_info_t *p)
{
    switch (p->bus) {
        case 0: RCC->AHBENR |= p->bit_mask; break;
        case 1: RCC->APB1ENR |= p->bit_mask; break;
        case 2: RCC->APB2ENR |= p->bit_mask; break;
    }
}

static void disable_peripheral_clock(const peripheral_info_t *p)
{
    switch (p->bus) {
        case 0: RCC->AHBENR &= ~p->bit_mask; break;
        case 1: RCC->APB1ENR &= ~p->bit_mask; break;
        case 2: RCC->APB2ENR &= ~p->bit_mask; break;
    }
}

static void print_peripheral_status(void)
{
    UART_SendString("Peripheral Clock Status:\r\n");
    UART_SendString("+----------+------+--------+\r\n");
    UART_SendString("| Name     | Bus  | Status |\r\n");
    UART_SendString("+----------+------+--------+\r\n");

    uint8_t enabled_count = 0;
    for (uint32_t i = 0; i < NUM_PERIPHERALS; i++) {
        const char *bus_name = (peripherals[i].bus == 0) ? "AHB" :
                               (peripherals[i].bus == 1) ? "APB1" : "APB2";
        uint8_t en = is_peripheral_enabled(&peripherals[i]);
        if (en) enabled_count++;

        UART_Printf("| %-8s | %-4s | %-6s |\r\n",
                     peripherals[i].name, bus_name,
                     en ? "ON" : "OFF");
    }
    UART_SendString("+----------+------+--------+\r\n");
    UART_Printf("Enabled: %d/%d peripherals\r\n\r\n", enabled_count, NUM_PERIPHERALS);
}

/* ================================================================================
 * FREERTOS TASK
 * ================================================================================ */

static void ClockGatingTask(void *pvParameters)
{
    UART_SendString("\r\n");
    UART_SendString("==========================================\r\n");
    UART_SendString("  STM32 Peripheral Clock Gating Demo\r\n");
    UART_SendString("  Modul 14: Power Management\r\n");
    UART_SendString("==========================================\r\n");
    UART_SendString("\r\n");

    while (1) {
        /* === FASE 1: Enable semua peripheral === */
        UART_SendString("[PHASE 1] Enabling ALL peripherals...\r\n");
        UART_SendString("Expected: Higher current draw\r\n\r\n");

        for (uint32_t i = 0; i < NUM_PERIPHERALS; i++) {
            enable_peripheral_clock(&peripherals[i]);
        }
        print_peripheral_status();

        /* LED nyala terus (high power) */
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
        vTaskDelay(pdMS_TO_TICKS(5000));

        /* === FASE 2: Disable semua peripheral (kecuali yang dipakai) === */
        UART_SendString("[PHASE 2] Disabling unused peripherals...\r\n");
        UART_SendString("Keeping: GPIOA, GPIOC, USART1 only\r\n\r\n");

        for (uint32_t i = 0; i < NUM_PERIPHERALS; i++) {
            disable_peripheral_clock(&peripherals[i]);
        }
        print_peripheral_status();

        /* Baca register RCC langsung */
        UART_Printf("RCC->APB1ENR = 0x%08lX\r\n", RCC->APB1ENR);
        UART_Printf("RCC->APB2ENR = 0x%08lX\r\n", RCC->APB2ENR);
        UART_Printf("RCC->AHBENR  = 0x%08lX\r\n", RCC->AHBENR);

        /* LED berkedip lambat (low power) */
        for (int i = 0; i < 5; i++) {
            HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);

        UART_SendString("\r\nCycle complete. Repeating in 3s...\r\n\r\n");
        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}

/* ================================================================================
 * MAIN
 * ================================================================================ */

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USART1_UART_Init();

    xTaskCreate(ClockGatingTask, "ClkGate", 512, NULL,
                tskIDLE_PRIORITY + 2, NULL);

    vTaskStartScheduler();
    while (1) {}
}

/* ================================================================================
 * EXCEPTION HANDLERS & FREERTOS HOOKS
 * ================================================================================ */

void NMI_Handler(void) {}
void HardFault_Handler(void) { while (1) {} }
void MemManage_Handler(void) { while (1) {} }
void BusFault_Handler(void) { while (1) {} }
void UsageFault_Handler(void) { while (1) {} }
void DebugMon_Handler(void) {}

void SysTick_Handler(void)
{
    HAL_IncTick();
    if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED) {
        xPortSysTickHandler();
    }
}

void vApplicationMallocFailedHook(void) { while (1) {} }
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask; (void)pcTaskName; while (1) {}
}

static StaticTask_t xIdleTaskTCB;
static StackType_t uxIdleTaskStack[configMINIMAL_STACK_SIZE];
void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer,
                                   StackType_t **ppxIdleTaskStackBuffer,
                                   uint32_t *pulIdleTaskStackSize)
{
    *ppxIdleTaskTCBBuffer = &xIdleTaskTCB;
    *ppxIdleTaskStackBuffer = uxIdleTaskStack;
    *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}

static StaticTask_t xTimerTaskTCB;
static StackType_t uxTimerTaskStack[configTIMER_TASK_STACK_DEPTH];
void vApplicationGetTimerTaskMemory(StaticTask_t **ppxTimerTaskTCBBuffer,
                                    StackType_t **ppxTimerTaskStackBuffer,
                                    uint32_t *pulTimerTaskStackSize)
{
    *ppxTimerTaskTCBBuffer = &xTimerTaskTCB;
    *ppxTimerTaskStackBuffer = uxTimerTaskStack;
    *pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
}
