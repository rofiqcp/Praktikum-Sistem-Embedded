/**
 * STM32_10_PSRAM_External_RAM
 * 
 * Demonstrates memory region concepts on STM32F103 BluePill.
 * Since F103C8 has no FSMC/PSRAM, this program:
 * - Declares a large static buffer as "simulated external memory"
 * - Shows memory region comparison: stack vs heap vs static (.bss) vs .data
 * - Prints addresses of variables in different memory regions
 * - Compares allocation speed: heap (pvPortMalloc) vs static array
 * - Explains FSMC external SRAM concept for STM32F4xx+
 * 
 * Platform: STM32F103C8 BluePill
 * UART1: PA9(TX), PA10(RX) @ 115200
 * LED: PC13 (active low)
 */

#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>
#include <string.h>

/* ---- UART handle ---- */
UART_HandleTypeDef huart1;

/* ---- Printf redirect ---- */
int _write(int file, char *ptr, int len)
{
    HAL_UART_Transmit(&huart1, (uint8_t *)ptr, len, HAL_MAX_DELAY);
    return len;
}

/* ---- Clock Configuration: 72 MHz ---- */
static void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

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
}

/* ---- GPIO Init: PC13 LED ---- */
static void GPIO_Init(void)
{
    __HAL_RCC_GPIOC_CLK_ENABLE();
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin   = GPIO_PIN_13;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
}

/* ---- UART1 Init ---- */
static void UART1_Init(void)
{
    __HAL_RCC_USART1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin   = GPIO_PIN_9;
    GPIO_InitStruct.Mode  = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin  = GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

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

/* ---- Memory regions ---- */

/* .data section (initialized global) */
static uint32_t data_section_var = 0xDEADBEEF;

/* .bss section (uninitialized global) */
static uint8_t bss_buffer[256];

/* Simulated "External SRAM" - large static buffer in .bss */
#define SIMULATED_EXTRAM_SIZE  2048
static uint8_t simulated_ext_ram[SIMULATED_EXTRAM_SIZE];

/* Linker symbols (extern) */
extern uint32_t _estack;
extern uint32_t _sdata;
extern uint32_t _edata;
extern uint32_t _sbss;
extern uint32_t _ebss;

/* ---- Memory region identification ---- */
static const char *identify_region(uint32_t addr)
{
    /* STM32F103C8 memory map:
     * Flash:  0x08000000 - 0x0800FFFF (64KB)
     * SRAM:   0x20000000 - 0x20004FFF (20KB)
     * Periph: 0x40000000 - 0x5FFFFFFF
     */
    if (addr >= 0x08000000 && addr < 0x08010000) {
        return "FLASH (.text/.rodata)";
    } else if (addr >= 0x20000000 && addr < 0x20005000) {
        return "SRAM";
    } else if (addr >= 0x40000000 && addr < 0x60000000) {
        return "PERIPHERAL";
    } else if (addr >= 0x60000000 && addr < 0x80000000) {
        return "FSMC/External (NOT available on F103C8)";
    }
    return "UNKNOWN";
}

/* ---- Speed benchmark ---- */
static uint32_t benchmark_static_access(uint8_t *buf, size_t size, uint32_t iterations)
{
    uint32_t start = HAL_GetTick();
    for (uint32_t iter = 0; iter < iterations; iter++) {
        for (size_t i = 0; i < size; i++) {
            buf[i] = (uint8_t)(i + iter);
        }
        uint32_t sum = 0;
        for (size_t i = 0; i < size; i++) {
            sum += buf[i];
        }
        (void)sum;
    }
    return HAL_GetTick() - start;
}

/* ---- Memory Explorer Task ---- */
static void vMemoryExplorerTask(void *pvParameters)
{
    (void)pvParameters;

    /* Stack variable */
    volatile uint32_t stack_var = 0x12345678;
    uint8_t stack_buffer[64];
    memset(stack_buffer, 0xAA, sizeof(stack_buffer));

    printf("\r\n================================================\r\n");
    printf("  STM32_10: Memory Region Explorer\r\n");
    printf("  (PSRAM/External SRAM Concept Demo)\r\n");
    printf("  MCU: STM32F103C8 (20KB SRAM, 64KB Flash)\r\n");
    printf("================================================\r\n\r\n");

    /* Section 1: STM32F103 Memory Map */
    printf("[MEMMAP] STM32F103C8 Memory Map:\r\n");
    printf("[MEMMAP]   Flash : 0x08000000 - 0x0800FFFF (64 KB)\r\n");
    printf("[MEMMAP]   SRAM  : 0x20000000 - 0x20004FFF (20 KB)\r\n");
    printf("[MEMMAP]   Periph: 0x40000000 - 0x5FFFFFFF\r\n");
    printf("[MEMMAP]   FSMC  : 0x60000000 - 0x7FFFFFFF (NOT on F103C8)\r\n\r\n");

    /* Section 2: Variable addresses in different regions */
    printf("[REGION] --- Variable Address Analysis ---\r\n");
    printf("[REGION] .data section (initialized):\r\n");
    printf("[REGION]   data_section_var : addr=0x%08lX region=%s\r\n",
           (uint32_t)&data_section_var, identify_region((uint32_t)&data_section_var));

    printf("[REGION] .bss section (uninitialized):\r\n");
    printf("[REGION]   bss_buffer       : addr=0x%08lX region=%s\r\n",
           (uint32_t)bss_buffer, identify_region((uint32_t)bss_buffer));
    printf("[REGION]   simulated_ext_ram: addr=0x%08lX region=%s size=%u\r\n",
           (uint32_t)simulated_ext_ram, identify_region((uint32_t)simulated_ext_ram),
           SIMULATED_EXTRAM_SIZE);

    printf("[REGION] Stack variables:\r\n");
    printf("[REGION]   stack_var        : addr=0x%08lX region=%s\r\n",
           (uint32_t)&stack_var, identify_region((uint32_t)&stack_var));
    printf("[REGION]   stack_buffer     : addr=0x%08lX region=%s\r\n",
           (uint32_t)stack_buffer, identify_region((uint32_t)stack_buffer));

    /* Heap allocation */
    void *heap_ptr = pvPortMalloc(128);
    if (heap_ptr) {
        printf("[REGION] Heap (pvPortMalloc):\r\n");
        printf("[REGION]   heap_ptr         : addr=0x%08lX region=%s\r\n",
               (uint32_t)heap_ptr, identify_region((uint32_t)heap_ptr));
        vPortFree(heap_ptr);
    }

    /* Function address (Flash) */
    extern int main(void);
    printf("[REGION] Code (.text):\r\n");
    printf("[REGION]   main()           : addr=0x%08lX region=%s\r\n",
           (uint32_t)(uintptr_t)main, identify_region((uint32_t)(uintptr_t)main));
    printf("[REGION]   this function    : addr=0x%08lX region=%s\r\n",
           (uint32_t)(uintptr_t)vMemoryExplorerTask, identify_region((uint32_t)(uintptr_t)vMemoryExplorerTask));

    printf("\r\n");

    /* Section 3: Memory region sizes */
    printf("[SIZE] --- Memory Region Usage ---\r\n");
    printf("[SIZE] Stack top (_estack)  : 0x%08lX\r\n", (uint32_t)&_estack);
    printf("[SIZE] Current SP (approx)  : 0x%08lX\r\n", (uint32_t)&stack_var);
    printf("[SIZE] .data start          : 0x%08lX\r\n", (uint32_t)&_sdata);
    printf("[SIZE] .data end            : 0x%08lX\r\n", (uint32_t)&_edata);
    printf("[SIZE] .bss start           : 0x%08lX\r\n", (uint32_t)&_sbss);
    printf("[SIZE] .bss end             : 0x%08lX\r\n", (uint32_t)&_ebss);
    printf("[SIZE] .data size           : %lu bytes\r\n",
           (uint32_t)&_edata - (uint32_t)&_sdata);
    printf("[SIZE] .bss size            : %lu bytes\r\n",
           (uint32_t)&_ebss - (uint32_t)&_sbss);
    printf("\r\n");

    /* Section 4: Simulated External RAM operations */
    printf("[EXTRAM] --- Simulated External SRAM Test ---\r\n");
    printf("[EXTRAM] Buffer at: 0x%08lX, Size: %u bytes\r\n",
           (uint32_t)simulated_ext_ram, SIMULATED_EXTRAM_SIZE);

    /* Write pattern */
    printf("[EXTRAM] Writing test pattern...\r\n");
    for (int i = 0; i < SIMULATED_EXTRAM_SIZE; i++) {
        simulated_ext_ram[i] = (uint8_t)(i & 0xFF);
    }

    /* Verify */
    int errors = 0;
    for (int i = 0; i < SIMULATED_EXTRAM_SIZE; i++) {
        if (simulated_ext_ram[i] != (uint8_t)(i & 0xFF)) {
            errors++;
        }
    }
    printf("[EXTRAM] Verification: %s (%d errors)\r\n",
           errors == 0 ? "PASS" : "FAIL", errors);
    printf("\r\n");

    /* Section 5: Speed comparison */
    printf("[SPEED] --- Memory Access Speed Comparison ---\r\n");
    uint32_t iterations = 100;

    /* Static .bss buffer */
    uint32_t time_static = benchmark_static_access(bss_buffer, 256, iterations);
    printf("[SPEED] Static (.bss) 256B x %lu iters : %lu ms\r\n",
           iterations, time_static);

    /* Simulated external RAM buffer */
    uint32_t time_ext = benchmark_static_access(simulated_ext_ram, 256, iterations);
    printf("[SPEED] Sim ExtRAM 256B x %lu iters    : %lu ms\r\n",
           iterations, time_ext);

    /* Heap allocated buffer */
    uint8_t *heap_buf = pvPortMalloc(256);
    if (heap_buf) {
        uint32_t time_heap = benchmark_static_access(heap_buf, 256, iterations);
        printf("[SPEED] Heap 256B x %lu iters          : %lu ms\r\n",
               iterations, time_heap);
        vPortFree(heap_buf);
    }

    /* Stack buffer */
    uint32_t time_stack = benchmark_static_access(stack_buffer, 64, iterations);
    printf("[SPEED] Stack 64B x %lu iters           : %lu ms\r\n",
           iterations, time_stack);

    printf("\r\n[SPEED] Note: All buffers are in internal SRAM on F103C8\r\n");
    printf("[SPEED] Real external SRAM (via FSMC) would be slower due to bus latency\r\n");
    printf("\r\n");

    /* Section 6: Heap stats */
    HeapStats_t stats;
    vPortGetHeapStats(&stats);
    printf("[HEAP] --- FreeRTOS Heap Status ---\r\n");
    printf("[HEAP] Total heap size     : %u bytes\r\n", (unsigned)configTOTAL_HEAP_SIZE);
    printf("[HEAP] Available           : %u bytes\r\n", (unsigned)stats.xAvailableHeapSpaceInBytes);
    printf("[HEAP] Largest free block  : %u bytes\r\n", (unsigned)stats.xSizeOfLargestFreeBlockInBytes);
    printf("[HEAP] Min ever free       : %u bytes\r\n", (unsigned)stats.xMinimumEverFreeBytesRemaining);
    printf("\r\n");

    /* Section 7: FSMC concept explanation */
    printf("[CONCEPT] === External SRAM via FSMC (STM32F4xx+) ===\r\n");
    printf("[CONCEPT] On STM32F4/F7 with FSMC/FMC peripheral:\r\n");
    printf("[CONCEPT] - External SRAM mapped at 0x60000000-0x6FFFFFFF\r\n");
    printf("[CONCEPT] - Accessible like normal memory (pointer dereference)\r\n");
    printf("[CONCEPT] - Typical: IS62WV51216 (1MB SRAM)\r\n");
    printf("[CONCEPT] - PSRAM (Pseudo-SRAM): higher density, DRAM-like refresh\r\n");
    printf("[CONCEPT] - Configure FSMC timing for access/setup/hold cycles\r\n");
    printf("[CONCEPT] - Can extend FreeRTOS heap into external RAM\r\n");
    printf("[CONCEPT] - Use heap_5 for multiple non-contiguous memory regions\r\n");
    printf("[CONCEPT] - STM32F103C8 BluePill: NO FSMC pins available\r\n");
    printf("[CONCEPT] - STM32F103VE/ZE: Has FSMC but 100/144 pin packages\r\n\r\n");

    printf("[DONE] Memory region exploration complete\r\n\r\n");

    /* Blink LED */
    while (1) {
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

/* ---- FreeRTOS hooks ---- */
void vApplicationMallocFailedHook(void)
{
    printf("[ERROR] Malloc failed!\r\n");
    while (1) {
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
        HAL_Delay(100);
    }
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    printf("[ERROR] Stack overflow in task: %s\r\n", pcTaskName);
    while (1) {
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
        HAL_Delay(100);
    }
}

/* ---- Main ---- */
int main(void)
{
    HAL_Init();
    SystemClock_Config();
    GPIO_Init();
    UART1_Init();

    printf("\r\n\r\n--- System Boot ---\r\n");

    xTaskCreate(vMemoryExplorerTask, "MemExplore", 512, NULL, 2, NULL);

    vTaskStartScheduler();

    while (1) {
    }
}
