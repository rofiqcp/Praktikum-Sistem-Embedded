/**
 * ============================================================================
 * File        : main.c
 * Program     : STM32_12_Storage_Data_Logger
 * Description : Data logger to internal flash with circular page management
 *
 * Board       : STM32F103C8 (Blue Pill)
 * Framework   : STM32Cube HAL
 *
 * Wiring:
 *   PA9  -> USB-TTL RX  (USART1 TX)
 *   PA10 <- USB-TTL TX  (USART1 RX)
 *   PC13 -> Onboard LED
 *
 * Flash Layout:
 *   Pages 0-55  : Application code
 *   Pages 56-63 : Data logging area (8KB, 680 entries max)
 *
 * Features:
 *   - Circular flash page management
 *   - RAM buffer with periodic flush to flash
 *   - Simulated sensor data (temperature, humidity, ADC)
 *   - CSV output for analysis
 *   - Statistics and flash usage reporting
 *   - Bare-metal (no FreeRTOS), SysTick timing
 * ============================================================================
 */

#include "stm32f1xx_hal.h"
#include "config.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

/* ==================== Global Handles ==================== */
UART_HandleTypeDef huart1;

/* ==================== Logger State ==================== */
static log_entry_t ram_buffer[RAM_BUFFER_SIZE];
static uint32_t    ram_count = 0;                   /* Entries in RAM buffer */

static uint32_t    flash_write_page  = 0;           /* Current write page index (0..LOG_NUM_PAGES-1) */
static uint32_t    flash_write_slot  = 0;           /* Current slot within page */
static uint32_t    total_entries     = 0;            /* Total entries written to flash */
static uint32_t    total_erases      = 0;            /* Total page erases */
static uint32_t    oldest_page       = 0;            /* Oldest data page index */
static uint8_t     flash_full_wrap   = 0;            /* Has the log wrapped around? */

/* Simulation state */
static uint32_t    sim_sample_index  = 0;
static uint16_t    sim_adc_value     = 0;

/* ==================== Function Prototypes ==================== */
void SystemClock_Config(void);
static void GPIO_Init(void);
static void UART1_Init(void);
void Error_Handler(void);

/* Flash helpers */
static int  flash_erase_page(uint32_t page_addr);
static int  flash_write_data(uint32_t addr, const uint8_t *data, uint32_t len);

/* Logger API */
int  logger_init(void);
int  logger_add_entry(const log_entry_t *entry);
int  logger_read_entry(uint32_t index, log_entry_t *entry);
int  logger_read_last_n(uint32_t n, log_entry_t *entries);
uint32_t logger_get_count(void);
int  logger_erase_all(void);
void logger_flush_buffer(void);
void logger_print_stats(void);

/* Page management */
static uint32_t page_index_to_addr(uint32_t page_idx);
static void     advance_write_position(void);
static int      erase_next_page_if_needed(void);

/* Sensor simulation */
static void simulate_sensor_data(log_entry_t *entry);

/* Display */
static void print_entry_csv(const log_entry_t *entry);
static void print_last_entries(uint32_t n);
static void print_flash_usage(void);

/* ==================== Printf Retarget ==================== */
int _write(int file, char *ptr, int len)
{
    HAL_UART_Transmit(&huart1, (uint8_t *)ptr, len, HAL_MAX_DELAY);
    return len;
}

/* ==================== System Clock: HSE 8MHz -> PLL x9 -> 72MHz ==================== */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState       = RCC_HSE_ON;
    RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    RCC_OscInitStruct.PLL.PLLState   = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource  = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL     = RCC_PLL_MUL9;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
        Error_Handler();

    RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                       RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
        Error_Handler();
}

/* ==================== GPIO: LED PC13 ==================== */
static void GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    GPIO_InitStruct.Pin   = LED_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_PORT, &GPIO_InitStruct);
    HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET);
}

/* ==================== UART1: PA9/PA10, 115200 8N1 ==================== */
static void UART1_Init(void)
{
    __HAL_RCC_USART1_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Pin   = UART_TX_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(UART_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin   = UART_RX_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    HAL_GPIO_Init(UART_PORT, &GPIO_InitStruct);

    huart1.Instance          = USART1;
    huart1.Init.BaudRate     = UART_BAUDRATE;
    huart1.Init.WordLength   = UART_WORDLENGTH_8B;
    huart1.Init.StopBits     = UART_STOPBITS_1;
    huart1.Init.Parity       = UART_PARITY_NONE;
    huart1.Init.Mode         = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart1) != HAL_OK)
        Error_Handler();
}

/* ==================== Flash Page Erase ==================== */
static int flash_erase_page(uint32_t page_addr)
{
    HAL_StatusTypeDef status;
    FLASH_EraseInitTypeDef erase_init;
    uint32_t page_error = 0;

    erase_init.TypeErase   = FLASH_TYPEERASE_PAGES;
    erase_init.PageAddress = page_addr;
    erase_init.NbPages     = 1;

    status = HAL_FLASH_Unlock();
    if (status != HAL_OK) return -1;

    status = HAL_FLASHEx_Erase(&erase_init, &page_error);
    HAL_FLASH_Lock();

    if (status != HAL_OK) {
        printf("[LOG] Flash erase failed at 0x%08lX\r\n", page_addr);
        return -2;
    }
    total_erases++;
    return 0;
}

/* ==================== Flash Write Data (halfword aligned) ==================== */
static int flash_write_data(uint32_t addr, const uint8_t *data, uint32_t len)
{
    HAL_StatusTypeDef status;
    const uint16_t *src = (const uint16_t *)data;
    uint32_t halfwords  = (len + 1) / 2;

    status = HAL_FLASH_Unlock();
    if (status != HAL_OK) return -1;

    for (uint32_t i = 0; i < halfwords; i++) {
        status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD,
                                   addr + (i * 2), src[i]);
        if (status != HAL_OK) {
            HAL_FLASH_Lock();
            printf("[LOG] Flash write failed at 0x%08lX\r\n", addr + (i * 2));
            return -2;
        }
    }

    HAL_FLASH_Lock();
    return 0;
}

/* ==================== Page Index to Address ==================== */
static uint32_t page_index_to_addr(uint32_t page_idx)
{
    return LOG_START_ADDR + (page_idx * FLASH_PAGE_SIZE);
}

/* ==================== Advance Write Position ==================== */
static void advance_write_position(void)
{
    flash_write_slot++;

    if (flash_write_slot >= LOG_ENTRIES_PER_PAGE) {
        /* Move to next page */
        flash_write_slot = 0;
        flash_write_page = (flash_write_page + 1) % LOG_NUM_PAGES;

        if (flash_write_page == oldest_page) {
            /* Wrapped around - need to erase oldest page */
            flash_full_wrap = 1;
            oldest_page = (oldest_page + 1) % LOG_NUM_PAGES;
        }

        /* Erase the new page before writing */
        erase_next_page_if_needed();
    }
}

/* ==================== Erase Next Page If Needed ==================== */
static int erase_next_page_if_needed(void)
{
    uint32_t page_addr = page_index_to_addr(flash_write_page);

    /* Check if page is already erased (first halfword == 0xFFFF) */
    uint16_t first = *(volatile uint16_t *)page_addr;
    if (first == 0xFFFF) return 0; /* Already erased */

    printf("[LOG] Erasing page %lu (0x%08lX) for new data\r\n",
           flash_write_page + LOG_START_PAGE, page_addr);
    return flash_erase_page(page_addr);
}

/* ==================== Logger Init ==================== */
int logger_init(void)
{
    printf("[LOG] Initializing data logger...\r\n");
    printf("[LOG] Flash area: 0x%08lX - 0x%08lX (%lu bytes)\r\n",
           (uint32_t)LOG_START_ADDR, (uint32_t)LOG_END_ADDR, (uint32_t)LOG_TOTAL_SIZE);
    printf("[LOG] Entry size: %u bytes\r\n", (unsigned)LOG_ENTRY_SIZE);
    printf("[LOG] Entries/page: %u\r\n", (unsigned)LOG_ENTRIES_PER_PAGE);
    printf("[LOG] Max entries: %u\r\n", (unsigned)LOG_MAX_ENTRIES);

    ram_count        = 0;
    flash_write_page = 0;
    flash_write_slot = 0;
    total_entries    = 0;
    oldest_page      = 0;
    flash_full_wrap  = 0;

    /* Scan flash to find existing entries and write position */
    for (uint32_t page = 0; page < LOG_NUM_PAGES; page++) {
        uint32_t page_addr = page_index_to_addr(page);

        for (uint32_t slot = 0; slot < LOG_ENTRIES_PER_PAGE; slot++) {
            uint32_t entry_addr = page_addr + (slot * LOG_ENTRY_SIZE);
            const log_entry_t *e = (const log_entry_t *)entry_addr;

            if (e->magic == LOG_ENTRY_MAGIC) {
                total_entries++;
            } else if (e->magic == 0xFFFF) {
                /* Found empty slot - this is write position */
                flash_write_page = page;
                flash_write_slot = slot;
                printf("[LOG] Found write position: page %lu, slot %lu\r\n",
                       page, slot);
                printf("[LOG] Existing entries: %lu\r\n", total_entries);
                return 0;
            }
        }
    }

    /* If we get here, all pages are full - start from page 0 */
    printf("[LOG] Flash full, wrapping to page 0\r\n");
    flash_write_page = 0;
    flash_write_slot = 0;
    flash_full_wrap  = 1;
    oldest_page      = 1;
    erase_next_page_if_needed();

    return 0;
}

/* ==================== Logger Add Entry ==================== */
int logger_add_entry(const log_entry_t *entry)
{
    /* Add to RAM buffer */
    memcpy(&ram_buffer[ram_count], entry, sizeof(log_entry_t));
    ram_count++;

    /* Flush when buffer is full */
    if (ram_count >= RAM_BUFFER_SIZE) {
        logger_flush_buffer();
    }

    return 0;
}

/* ==================== Logger Flush RAM Buffer to Flash ==================== */
void logger_flush_buffer(void)
{
    if (ram_count == 0) return;

    printf("[LOG] Flushing %lu entries to flash (page %lu, slot %lu)\r\n",
           ram_count, flash_write_page + LOG_START_PAGE, flash_write_slot);

    for (uint32_t i = 0; i < ram_count; i++) {
        uint32_t addr = page_index_to_addr(flash_write_page)
                      + (flash_write_slot * LOG_ENTRY_SIZE);

        int ret = flash_write_data(addr, (const uint8_t *)&ram_buffer[i], LOG_ENTRY_SIZE);
        if (ret != 0) {
            printf("[LOG] ERROR: Failed to write entry %lu\r\n", i);
            continue;
        }

        total_entries++;
        advance_write_position();
    }

    ram_count = 0;
    HAL_GPIO_TogglePin(LED_PORT, LED_PIN);
}

/* ==================== Logger Read Entry by Index ==================== */
int logger_read_entry(uint32_t index, log_entry_t *entry)
{
    if (index >= LOG_MAX_ENTRIES) return -1;

    /* Calculate which page and slot */
    uint32_t page = index / LOG_ENTRIES_PER_PAGE;
    uint32_t slot = index % LOG_ENTRIES_PER_PAGE;

    if (page >= LOG_NUM_PAGES) return -2;

    uint32_t addr = page_index_to_addr(page) + (slot * LOG_ENTRY_SIZE);
    memcpy(entry, (const void *)addr, sizeof(log_entry_t));

    if (entry->magic != LOG_ENTRY_MAGIC) return -3;
    return 0;
}

/* ==================== Logger Read Last N Entries ==================== */
int logger_read_last_n(uint32_t n, log_entry_t *entries)
{
    /* Work backwards from current write position */
    uint32_t read_page = flash_write_page;
    uint32_t read_slot = flash_write_slot;
    uint32_t count = 0;

    /* First check RAM buffer (most recent, not yet flushed) */
    for (int32_t i = (int32_t)ram_count - 1; i >= 0 && count < n; i--) {
        memcpy(&entries[count], &ram_buffer[i], sizeof(log_entry_t));
        count++;
    }

    /* Then read from flash backwards */
    while (count < n) {
        if (read_slot == 0) {
            if (read_page == 0) read_page = LOG_NUM_PAGES - 1;
            else read_page--;

            if (read_page == flash_write_page && count > 0) break; /* Wrapped around */
            read_slot = LOG_ENTRIES_PER_PAGE;
        }
        read_slot--;

        uint32_t addr = page_index_to_addr(read_page) + (read_slot * LOG_ENTRY_SIZE);
        const log_entry_t *e = (const log_entry_t *)addr;

        if (e->magic != LOG_ENTRY_MAGIC) break;

        memcpy(&entries[count], e, sizeof(log_entry_t));
        count++;
    }

    return (int)count;
}

/* ==================== Logger Get Count ==================== */
uint32_t logger_get_count(void)
{
    return total_entries + ram_count;
}

/* ==================== Logger Erase All ==================== */
int logger_erase_all(void)
{
    printf("[LOG] Erasing all data pages...\r\n");

    for (uint32_t page = 0; page < LOG_NUM_PAGES; page++) {
        uint32_t page_addr = page_index_to_addr(page);
        if (flash_erase_page(page_addr) != 0) {
            printf("[LOG] ERROR: Failed to erase page %lu\r\n", page + LOG_START_PAGE);
            return -1;
        }
    }

    ram_count        = 0;
    flash_write_page = 0;
    flash_write_slot = 0;
    total_entries    = 0;
    oldest_page      = 0;
    flash_full_wrap  = 0;

    printf("[LOG] All data erased. Logger reset.\r\n");
    return 0;
}

/* ==================== Simulate Sensor Data ==================== */
static void simulate_sensor_data(log_entry_t *entry)
{
    entry->timestamp_ms = HAL_GetTick();

    /* Temperature: sine wave */
    double angle = (double)sim_sample_index * 0.05; /* Slow sine */
    entry->temperature = (int16_t)(TEMP_BASE + (int16_t)(TEMP_AMPLITUDE * sin(angle)));

    /* Humidity: base + noise */
    int16_t noise = (int16_t)((HAL_GetTick() * 1103515245 + 12345) % (HUMIDITY_NOISE * 2))
                    - HUMIDITY_NOISE;
    entry->humidity = (int16_t)(HUMIDITY_BASE + noise);

    /* ADC: sawtooth wave */
    sim_adc_value = (sim_adc_value + ADC_SAWTOOTH_STEP) % (ADC_SAWTOOTH_MAX + 1);
    entry->adc_raw = sim_adc_value;

    /* Magic marker */
    entry->magic = LOG_ENTRY_MAGIC;

    sim_sample_index++;
}

/* ==================== Print Entry as CSV ==================== */
static void print_entry_csv(const log_entry_t *entry)
{
    printf("%lu,%.2f,%.2f,%u\r\n",
           entry->timestamp_ms,
           entry->temperature / 100.0,
           entry->humidity / 100.0,
           entry->adc_raw);
}

/* ==================== Print Last N Entries ==================== */
static void print_last_entries(uint32_t n)
{
    log_entry_t entries[32];
    if (n > 32) n = 32;

    int count = logger_read_last_n(n, entries);
    if (count <= 0) {
        printf("[LOG] No entries to display\r\n");
        return;
    }

    printf("\r\n[CSV_START]\r\n");
    printf("timestamp_ms,temperature_c,humidity_pct,adc_raw\r\n");

    /* Print in chronological order (entries are in reverse) */
    for (int i = count - 1; i >= 0; i--) {
        print_entry_csv(&entries[i]);
    }
    printf("[CSV_END]\r\n");
}

/* ==================== Print Flash Usage ==================== */
static void print_flash_usage(void)
{
    printf("\r\n[LOG] === Flash Page Usage ===\r\n");
    printf("Page │ Address    │ Entries │ Status\r\n");
    printf("─────┼────────────┼─────────┼──────────\r\n");

    for (uint32_t page = 0; page < LOG_NUM_PAGES; page++) {
        uint32_t page_addr = page_index_to_addr(page);
        uint32_t entries = 0;

        for (uint32_t slot = 0; slot < LOG_ENTRIES_PER_PAGE; slot++) {
            uint32_t addr = page_addr + (slot * LOG_ENTRY_SIZE);
            const log_entry_t *e = (const log_entry_t *)addr;
            if (e->magic == LOG_ENTRY_MAGIC) entries++;
        }

        const char *status;
        if (page == flash_write_page)    status = "WRITING";
        else if (page == oldest_page && flash_full_wrap) status = "OLDEST";
        else if (entries == 0)           status = "EMPTY";
        else if (entries >= LOG_ENTRIES_PER_PAGE) status = "FULL";
        else                             status = "PARTIAL";

        printf("  %2lu │ 0x%08lX │ %3lu/%2u  │ %s\r\n",
               page + LOG_START_PAGE, page_addr,
               entries, (unsigned)LOG_ENTRIES_PER_PAGE, status);
    }
    printf("\r\n");
}

/* ==================== Logger Print Statistics ==================== */
void logger_print_stats(void)
{
    uint32_t total = logger_get_count();
    uint32_t flash_used = total_entries * LOG_ENTRY_SIZE;
    uint32_t usage_pct = (flash_used * 100) / LOG_TOTAL_SIZE;

    printf("\r\n[LOG] === Data Logger Statistics ===\r\n");
    printf("[LOG] Total entries    : %lu (flash: %lu, RAM buffer: %lu)\r\n",
           total, total_entries, ram_count);
    printf("[LOG] Flash usage      : %lu / %lu bytes (%lu%%)\r\n",
           flash_used, (uint32_t)LOG_TOTAL_SIZE, usage_pct);
    printf("[LOG] Current page     : %lu (page %lu)\r\n",
           flash_write_page, flash_write_page + LOG_START_PAGE);
    printf("[LOG] Current slot     : %lu / %u\r\n",
           flash_write_slot, (unsigned)LOG_ENTRIES_PER_PAGE);
    printf("[LOG] Page erases      : %lu\r\n", total_erases);
    printf("[LOG] Wrapped          : %s\r\n", flash_full_wrap ? "YES" : "NO");
    printf("[LOG] Uptime           : %lu sec\r\n", HAL_GetTick() / 1000);
    printf("[LOG] Log interval     : %u ms\r\n", LOG_INTERVAL_MS);

    /* Estimated capacity */
    uint32_t remaining = 0;
    if (!flash_full_wrap) {
        remaining = LOG_MAX_ENTRIES - total_entries;
    }
    uint32_t est_time_sec = (remaining * LOG_INTERVAL_MS) / 1000;
    printf("[LOG] Est. remaining   : %lu entries (~%lu sec)\r\n",
           remaining, est_time_sec);
}

/* ==================== Main ==================== */
int main(void)
{
    HAL_Init();
    SystemClock_Config();
    GPIO_Init();
    UART1_Init();

    printf("\r\n");
    printf("========================================\r\n");
    printf("  STM32_12: Storage Data Logger\r\n");
    printf("  Board : Blue Pill STM32F103C8\r\n");
    printf("  Clock : 72 MHz (HSE + PLL)\r\n");
    printf("  Flash : Pages %d-%d (%d KB)\r\n",
           LOG_START_PAGE, LOG_END_PAGE, LOG_NUM_PAGES);
    printf("========================================\r\n");

    /* Initialize logger */
    logger_init();
    logger_print_stats();

    /* Erase all if previously used (for demo purposes) */
    printf("\r\n[DEMO] Erasing previous data for clean demo...\r\n");
    logger_erase_all();

    /* === Phase 1: Collect data === */
    printf("\r\n=== Phase 1: Collecting Sensor Data ===\r\n");
    printf("[DEMO] Logging %d entries at %d ms interval\r\n",
           RAM_BUFFER_SIZE * 8, LOG_INTERVAL_MS);

    uint32_t target_entries = RAM_BUFFER_SIZE * 8; /* 128 entries */
    uint32_t last_log_tick  = HAL_GetTick();
    uint32_t logged = 0;

    while (logged < target_entries) {
        uint32_t now = HAL_GetTick();

        if ((now - last_log_tick) >= LOG_INTERVAL_MS) {
            last_log_tick = now;

            log_entry_t entry;
            simulate_sensor_data(&entry);
            logger_add_entry(&entry);
            logged++;

            if (logged % PRINT_INTERVAL_ENTRIES == 0) {
                printf("[DEMO] Logged %lu/%lu entries (%.1f%%)\r\n",
                       logged, target_entries,
                       (double)logged * 100.0 / target_entries);
                logger_print_stats();
            }
        }
    }

    /* Flush remaining entries in RAM buffer */
    logger_flush_buffer();

    /* === Phase 2: Read back and display === */
    printf("\r\n=== Phase 2: Reading Data Back ===\r\n");
    printf("[DEMO] Total entries logged: %lu\r\n", logger_get_count());

    /* Print last 20 entries as CSV */
    printf("\r\n[DEMO] Last 20 entries:\r\n");
    print_last_entries(20);

    /* Print all entries sequentially */
    printf("\r\n[DEMO] All entries (CSV):\r\n");
    printf("[CSV_ALL_START]\r\n");
    printf("timestamp_ms,temperature_c,humidity_pct,adc_raw\r\n");

    for (uint32_t i = 0; i < LOG_NUM_PAGES; i++) {
        uint32_t page_addr = page_index_to_addr(i);
        for (uint32_t slot = 0; slot < LOG_ENTRIES_PER_PAGE; slot++) {
            uint32_t addr = page_addr + (slot * LOG_ENTRY_SIZE);
            const log_entry_t *e = (const log_entry_t *)addr;
            if (e->magic == LOG_ENTRY_MAGIC) {
                print_entry_csv(e);
            }
        }
    }
    printf("[CSV_ALL_END]\r\n");

    /* === Phase 3: Statistics === */
    printf("\r\n=== Phase 3: Final Statistics ===\r\n");
    logger_print_stats();
    print_flash_usage();

    /* === Phase 4: Circular buffer wrap test === */
    printf("\r\n=== Phase 4: Circular Wrap Test ===\r\n");
    printf("[DEMO] Writing entries until flash wraps around...\r\n");

    uint32_t wrap_count = 0;
    while (!flash_full_wrap && wrap_count < LOG_MAX_ENTRIES + 100) {
        log_entry_t entry;
        simulate_sensor_data(&entry);
        logger_add_entry(&entry);
        wrap_count++;

        if (wrap_count % 100 == 0) {
            logger_flush_buffer();
            printf("[DEMO] Written %lu more entries, total=%lu\r\n",
                   wrap_count, logger_get_count());
        }
    }
    logger_flush_buffer();

    if (flash_full_wrap) {
        printf("[DEMO] Flash wrapped! Oldest data overwritten.\r\n");
    }

    /* Final stats */
    logger_print_stats();
    print_flash_usage();

    printf("\r\n========================================\r\n");
    printf("  Data Logger Demo Complete!\r\n");
    printf("========================================\r\n");

    /* Main loop - continue logging */
    uint32_t loop_count = 0;
    while (1) {
        uint32_t now = HAL_GetTick();

        if ((now - last_log_tick) >= LOG_INTERVAL_MS) {
            last_log_tick = now;

            log_entry_t entry;
            simulate_sensor_data(&entry);
            logger_add_entry(&entry);
            loop_count++;

            if (loop_count % PRINT_INTERVAL_ENTRIES == 0) {
                logger_flush_buffer();
                logger_print_stats();
            }
        }

        HAL_GPIO_TogglePin(LED_PORT, LED_PIN);
        HAL_Delay(100);
    }
}

/* ==================== Error Handler ==================== */
void Error_Handler(void)
{
    __disable_irq();
    while (1) {
        HAL_GPIO_TogglePin(LED_PORT, LED_PIN);
        for (volatile uint32_t i = 0; i < 200000; i++);
    }
}
