/**
 * ============================================================================
 * File        : main.c
 * Program     : STM32_09_Flash_Key_Value
 * Description : Simple key-value store in internal flash (NVS-like for STM32)
 *
 * Board       : STM32F103C8 (Blue Pill)
 * Framework   : STM32Cube HAL
 *
 * Wiring:
 *   PA9  -> USB-TTL RX  (USART1 TX)
 *   PA10 <- USB-TTL TX  (USART1 RX)
 *   PC13 -> Onboard LED
 *
 * Flash KV Store Layout:
 *   Page 62 (0x0800F800): Active KV page
 *   Page 63 (0x0800FC00): Backup page for compaction
 *
 * Features:
 *   - Key-value pairs stored in internal flash
 *   - Supports int, float, string, and blob types
 *   - Wear leveling via page compaction
 *   - Boot counter, device config, calibration values
 * ============================================================================
 */

#include "stm32f1xx_hal.h"
#include "config.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

/* ==================== Global Handles ==================== */
UART_HandleTypeDef huart1;

/* ==================== KV Store State ==================== */
typedef struct {
    uint8_t  valid;
    char     key[KV_KEY_MAX_LEN];
    uint8_t  type;
    uint8_t  len;
    uint8_t  value[KV_VALUE_MAX_SIZE];
} kv_ram_entry_t;

static kv_ram_entry_t kv_cache[KV_MAX_KEYS];
static uint32_t kv_active_page  = KV_PAGE_A_ADDR;
static uint32_t kv_backup_page  = KV_PAGE_B_ADDR;
static uint32_t kv_write_offset = 0;
static uint32_t kv_entry_count  = 0;
static uint32_t kv_compact_count = 0;

/* ==================== Function Prototypes ==================== */
void SystemClock_Config(void);
static void GPIO_Init(void);
static void UART1_Init(void);
void Error_Handler(void);

/* KV Store API */
int  kv_init(void);
int  kv_set_int(const char *key, int32_t value);
int  kv_get_int(const char *key, int32_t *value);
int  kv_set_float(const char *key, float value);
int  kv_get_float(const char *key, float *value);
int  kv_set_string(const char *key, const char *str);
int  kv_get_string(const char *key, char *buf, uint16_t buflen);
int  kv_delete(const char *key);
int  kv_compact(void);
void kv_list_all(void);
void kv_print_stats(void);

/* Internal helpers */
static int  flash_write_entry(uint32_t addr, const kv_flash_entry_t *entry);
static void flash_read_entry(uint32_t addr, kv_flash_entry_t *entry);
static int  flash_erase_page(uint32_t page_addr);
static int  find_cache_index(const char *key);
static int  find_free_cache_slot(void);
static int  kv_write_to_flash(const kv_flash_entry_t *entry);

/* Demo functions */
static void demo_boot_counter(void);
static void demo_device_config(void);
static void demo_calibration(void);
static void demo_list_and_delete(void);

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

    /* LED on PC13 (active low) */
    GPIO_InitStruct.Pin   = LED_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_PORT, &GPIO_InitStruct);
    HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET); /* LED off */
}

/* ==================== UART1: PA9/PA10, 115200 8N1 ==================== */
static void UART1_Init(void)
{
    __HAL_RCC_USART1_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* TX PA9 - AF push-pull */
    GPIO_InitStruct.Pin   = UART_TX_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(UART_PORT, &GPIO_InitStruct);

    /* RX PA10 - input floating */
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
        printf("[KV] Flash erase failed at 0x%08lX\r\n", page_addr);
        return -2;
    }
    return 0;
}

/* ==================== Flash Write Entry (halfword-by-halfword) ==================== */
static int flash_write_entry(uint32_t addr, const kv_flash_entry_t *entry)
{
    HAL_StatusTypeDef status;
    const uint16_t *src = (const uint16_t *)entry;
    uint32_t words = (sizeof(kv_flash_entry_t) + 1) / 2; /* Round up to halfwords */

    status = HAL_FLASH_Unlock();
    if (status != HAL_OK) return -1;

    for (uint32_t i = 0; i < words; i++) {
        status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD,
                                   addr + (i * 2), src[i]);
        if (status != HAL_OK) {
            HAL_FLASH_Lock();
            printf("[KV] Flash write failed at 0x%08lX\r\n", addr + (i * 2));
            return -2;
        }
    }

    HAL_FLASH_Lock();
    return 0;
}

/* ==================== Flash Read Entry ==================== */
static void flash_read_entry(uint32_t addr, kv_flash_entry_t *entry)
{
    memcpy(entry, (const void *)addr, sizeof(kv_flash_entry_t));
}

/* ==================== Find Key in Cache ==================== */
static int find_cache_index(const char *key)
{
    for (int i = 0; i < KV_MAX_KEYS; i++) {
        if (kv_cache[i].valid && strncmp(kv_cache[i].key, key, KV_KEY_MAX_LEN) == 0)
            return i;
    }
    return -1;
}

/* ==================== Find Free Cache Slot ==================== */
static int find_free_cache_slot(void)
{
    for (int i = 0; i < KV_MAX_KEYS; i++) {
        if (!kv_cache[i].valid)
            return i;
    }
    return -1;
}

/* ==================== KV Init: Scan Active Page ==================== */
int kv_init(void)
{
    kv_flash_entry_t entry;

    /* Clear RAM cache */
    memset(kv_cache, 0, sizeof(kv_cache));
    kv_entry_count  = 0;
    kv_write_offset = 0;

    printf("[KV] Initializing key-value store...\r\n");
    printf("[KV] Active page: 0x%08lX\r\n", kv_active_page);
    printf("[KV] Backup page: 0x%08lX\r\n", kv_backup_page);
    printf("[KV] Entry size : %u bytes\r\n", (unsigned)sizeof(kv_flash_entry_t));
    printf("[KV] Entries/page: %u\r\n", (unsigned)KV_ENTRIES_PER_PAGE);

    /* Scan active page for valid entries */
    for (uint32_t i = 0; i < KV_ENTRIES_PER_PAGE; i++) {
        uint32_t addr = kv_active_page + (i * sizeof(kv_flash_entry_t));
        flash_read_entry(addr, &entry);

        if (entry.magic == KV_ENTRY_EMPTY) {
            /* Reached empty area — this is our write offset */
            kv_write_offset = i;
            break;
        }

        if (entry.magic == KV_ENTRY_MAGIC) {
            /* Valid entry: update or add to cache */
            int idx = find_cache_index(entry.key);
            if (idx < 0) {
                idx = find_free_cache_slot();
                if (idx < 0) {
                    printf("[KV] WARNING: Cache full, skipping key '%s'\r\n", entry.key);
                    continue;
                }
            }
            kv_cache[idx].valid = 1;
            strncpy(kv_cache[idx].key, entry.key, KV_KEY_MAX_LEN);
            kv_cache[idx].type = entry.type;
            kv_cache[idx].len  = entry.len;
            memcpy(kv_cache[idx].value, entry.value, entry.len);
            kv_entry_count++;
        }
        /* KV_ENTRY_DELETED entries are skipped */
        kv_write_offset = i + 1;
    }

    printf("[KV] Found %lu valid entries, write offset=%lu\r\n",
           kv_entry_count, kv_write_offset);
    return 0;
}

/* ==================== Write Entry to Flash ==================== */
static int kv_write_to_flash(const kv_flash_entry_t *entry)
{
    /* Check if page is full */
    if (kv_write_offset >= KV_ENTRIES_PER_PAGE) {
        printf("[KV] Active page full, compacting...\r\n");
        if (kv_compact() != 0) {
            printf("[KV] ERROR: Compaction failed!\r\n");
            return -1;
        }
    }

    uint32_t addr = kv_active_page + (kv_write_offset * sizeof(kv_flash_entry_t));
    int ret = flash_write_entry(addr, entry);
    if (ret == 0) {
        kv_write_offset++;
    }
    return ret;
}

/* ==================== KV Set Integer ==================== */
int kv_set_int(const char *key, int32_t value)
{
    if (strlen(key) >= KV_KEY_MAX_LEN) return -1;

    printf("[KV] SET int '%s' = %ld\r\n", key, value);

    /* Update cache */
    int idx = find_cache_index(key);
    if (idx < 0) {
        idx = find_free_cache_slot();
        if (idx < 0) {
            printf("[KV] ERROR: No free slots\r\n");
            return -2;
        }
        kv_entry_count++;
    }
    kv_cache[idx].valid = 1;
    strncpy(kv_cache[idx].key, key, KV_KEY_MAX_LEN);
    kv_cache[idx].type = KV_TYPE_INT;
    kv_cache[idx].len  = sizeof(int32_t);
    memcpy(kv_cache[idx].value, &value, sizeof(int32_t));

    /* Write to flash */
    kv_flash_entry_t entry;
    memset(&entry, 0xFF, sizeof(entry));
    entry.magic = KV_ENTRY_MAGIC;
    strncpy(entry.key, key, KV_KEY_MAX_LEN);
    entry.type = KV_TYPE_INT;
    entry.len  = sizeof(int32_t);
    memcpy(entry.value, &value, sizeof(int32_t));

    return kv_write_to_flash(&entry);
}

/* ==================== KV Get Integer ==================== */
int kv_get_int(const char *key, int32_t *value)
{
    int idx = find_cache_index(key);
    if (idx < 0) return -1;
    if (kv_cache[idx].type != KV_TYPE_INT) return -2;

    memcpy(value, kv_cache[idx].value, sizeof(int32_t));
    printf("[KV] GET int '%s' = %ld\r\n", key, *value);
    return 0;
}

/* ==================== KV Set Float ==================== */
int kv_set_float(const char *key, float value)
{
    if (strlen(key) >= KV_KEY_MAX_LEN) return -1;

    printf("[KV] SET float '%s' = %.4f\r\n", key, (double)value);

    int idx = find_cache_index(key);
    if (idx < 0) {
        idx = find_free_cache_slot();
        if (idx < 0) return -2;
        kv_entry_count++;
    }
    kv_cache[idx].valid = 1;
    strncpy(kv_cache[idx].key, key, KV_KEY_MAX_LEN);
    kv_cache[idx].type = KV_TYPE_FLOAT;
    kv_cache[idx].len  = sizeof(float);
    memcpy(kv_cache[idx].value, &value, sizeof(float));

    kv_flash_entry_t entry;
    memset(&entry, 0xFF, sizeof(entry));
    entry.magic = KV_ENTRY_MAGIC;
    strncpy(entry.key, key, KV_KEY_MAX_LEN);
    entry.type = KV_TYPE_FLOAT;
    entry.len  = sizeof(float);
    memcpy(entry.value, &value, sizeof(float));

    return kv_write_to_flash(&entry);
}

/* ==================== KV Get Float ==================== */
int kv_get_float(const char *key, float *value)
{
    int idx = find_cache_index(key);
    if (idx < 0) return -1;
    if (kv_cache[idx].type != KV_TYPE_FLOAT) return -2;

    memcpy(value, kv_cache[idx].value, sizeof(float));
    printf("[KV] GET float '%s' = %.4f\r\n", key, (double)*value);
    return 0;
}

/* ==================== KV Set String ==================== */
int kv_set_string(const char *key, const char *str)
{
    if (strlen(key) >= KV_KEY_MAX_LEN) return -1;
    uint8_t slen = (uint8_t)(strlen(str) + 1); /* Include null terminator */
    if (slen > KV_VALUE_MAX_SIZE) return -3;

    printf("[KV] SET string '%s' = \"%s\"\r\n", key, str);

    int idx = find_cache_index(key);
    if (idx < 0) {
        idx = find_free_cache_slot();
        if (idx < 0) return -2;
        kv_entry_count++;
    }
    kv_cache[idx].valid = 1;
    strncpy(kv_cache[idx].key, key, KV_KEY_MAX_LEN);
    kv_cache[idx].type = KV_TYPE_STRING;
    kv_cache[idx].len  = slen;
    memset(kv_cache[idx].value, 0, KV_VALUE_MAX_SIZE);
    memcpy(kv_cache[idx].value, str, slen);

    kv_flash_entry_t entry;
    memset(&entry, 0xFF, sizeof(entry));
    entry.magic = KV_ENTRY_MAGIC;
    strncpy(entry.key, key, KV_KEY_MAX_LEN);
    entry.type = KV_TYPE_STRING;
    entry.len  = slen;
    memset(entry.value, 0, KV_VALUE_MAX_SIZE);
    memcpy(entry.value, str, slen);

    return kv_write_to_flash(&entry);
}

/* ==================== KV Get String ==================== */
int kv_get_string(const char *key, char *buf, uint16_t buflen)
{
    int idx = find_cache_index(key);
    if (idx < 0) return -1;
    if (kv_cache[idx].type != KV_TYPE_STRING) return -2;

    uint16_t copylen = kv_cache[idx].len;
    if (copylen > buflen) copylen = buflen - 1;
    memcpy(buf, kv_cache[idx].value, copylen);
    buf[copylen] = '\0';
    printf("[KV] GET string '%s' = \"%s\"\r\n", key, buf);
    return 0;
}

/* ==================== KV Delete ==================== */
int kv_delete(const char *key)
{
    int idx = find_cache_index(key);
    if (idx < 0) {
        printf("[KV] DELETE '%s': key not found\r\n", key);
        return -1;
    }

    printf("[KV] DELETE '%s'\r\n", key);

    /* Mark as deleted in cache */
    kv_cache[idx].valid = 0;
    kv_entry_count--;

    /* Write a deleted marker to flash */
    kv_flash_entry_t entry;
    memset(&entry, 0xFF, sizeof(entry));
    entry.magic = KV_ENTRY_DELETED;
    strncpy(entry.key, key, KV_KEY_MAX_LEN);
    entry.type = 0;
    entry.len  = 0;

    return kv_write_to_flash(&entry);
}

/* ==================== KV Compact (Wear Leveling) ==================== */
int kv_compact(void)
{
    printf("[KV] --- Compaction Start ---\r\n");
    printf("[KV] Copying %lu valid entries to backup page 0x%08lX\r\n",
           kv_entry_count, kv_backup_page);

    /* Erase backup page */
    if (flash_erase_page(kv_backup_page) != 0)
        return -1;

    /* Write all valid cache entries to backup page */
    uint32_t write_idx = 0;
    for (int i = 0; i < KV_MAX_KEYS; i++) {
        if (!kv_cache[i].valid) continue;

        kv_flash_entry_t entry;
        memset(&entry, 0xFF, sizeof(entry));
        entry.magic = KV_ENTRY_MAGIC;
        strncpy(entry.key, kv_cache[i].key, KV_KEY_MAX_LEN);
        entry.type = kv_cache[i].type;
        entry.len  = kv_cache[i].len;
        memcpy(entry.value, kv_cache[i].value, kv_cache[i].len);

        uint32_t addr = kv_backup_page + (write_idx * sizeof(kv_flash_entry_t));
        if (flash_write_entry(addr, &entry) != 0) {
            printf("[KV] ERROR: Compact write failed at index %lu\r\n", write_idx);
            return -2;
        }
        write_idx++;
    }

    /* Erase old active page */
    if (flash_erase_page(kv_active_page) != 0)
        return -3;

    /* Swap pages */
    uint32_t temp     = kv_active_page;
    kv_active_page    = kv_backup_page;
    kv_backup_page    = temp;
    kv_write_offset   = write_idx;
    kv_compact_count++;

    printf("[KV] Compaction done. New active page: 0x%08lX, entries: %lu\r\n",
           kv_active_page, write_idx);
    printf("[KV] --- Compaction End (total compactions: %lu) ---\r\n", kv_compact_count);
    return 0;
}

/* ==================== KV List All Entries ==================== */
void kv_list_all(void)
{
    printf("\r\n[KV] === All Key-Value Pairs ===\r\n");
    printf("%-4s %-16s %-8s %-6s %s\r\n", "Idx", "Key", "Type", "Len", "Value");
    printf("---- ---------------- -------- ------ --------------------------------\r\n");

    int count = 0;
    for (int i = 0; i < KV_MAX_KEYS; i++) {
        if (!kv_cache[i].valid) continue;

        const char *type_str;
        char val_buf[80];

        switch (kv_cache[i].type) {
            case KV_TYPE_INT: {
                type_str = "INT";
                int32_t v;
                memcpy(&v, kv_cache[i].value, sizeof(int32_t));
                snprintf(val_buf, sizeof(val_buf), "%ld", v);
                break;
            }
            case KV_TYPE_FLOAT: {
                type_str = "FLOAT";
                float f;
                memcpy(&f, kv_cache[i].value, sizeof(float));
                snprintf(val_buf, sizeof(val_buf), "%.4f", (double)f);
                break;
            }
            case KV_TYPE_STRING:
                type_str = "STRING";
                snprintf(val_buf, sizeof(val_buf), "\"%s\"", (char *)kv_cache[i].value);
                break;
            case KV_TYPE_BLOB:
                type_str = "BLOB";
                snprintf(val_buf, sizeof(val_buf), "[%u bytes]", kv_cache[i].len);
                break;
            default:
                type_str = "???";
                snprintf(val_buf, sizeof(val_buf), "unknown");
                break;
        }

        printf("%-4d %-16s %-8s %-6u %s\r\n",
               i, kv_cache[i].key, type_str, kv_cache[i].len, val_buf);
        count++;
    }
    printf("---- Total: %d entries ----\r\n\r\n", count);
}

/* ==================== KV Print Stats ==================== */
void kv_print_stats(void)
{
    printf("\r\n[KV] === Statistics ===\r\n");
    printf("[KV] Active page    : 0x%08lX\r\n", kv_active_page);
    printf("[KV] Backup page    : 0x%08lX\r\n", kv_backup_page);
    printf("[KV] Entries in use : %lu / %u\r\n", kv_entry_count, KV_MAX_KEYS);
    printf("[KV] Flash writes   : %lu / %u\r\n", kv_write_offset, (unsigned)KV_ENTRIES_PER_PAGE);
    printf("[KV] Page usage     : %lu%%\r\n",
           (kv_write_offset * 100) / KV_ENTRIES_PER_PAGE);
    printf("[KV] Compactions    : %lu\r\n", kv_compact_count);
    printf("[KV] Entry size     : %u bytes\r\n", (unsigned)sizeof(kv_flash_entry_t));
    printf("[KV] Page capacity  : %u entries\r\n\r\n", (unsigned)KV_ENTRIES_PER_PAGE);
}

/* ==================== Demo: Boot Counter ==================== */
static void demo_boot_counter(void)
{
    printf("\r\n=== Demo 1: Boot Counter ===\r\n");

    int32_t boot_count = 0;
    if (kv_get_int("boot_count", &boot_count) == 0) {
        printf("[DEMO] Previous boot count: %ld\r\n", boot_count);
    } else {
        printf("[DEMO] First boot! Initializing counter.\r\n");
    }

    boot_count++;
    kv_set_int("boot_count", boot_count);
    printf("[DEMO] Boot count now: %ld\r\n", boot_count);

    /* Store last boot timestamp */
    kv_set_int("last_boot_ms", (int32_t)HAL_GetTick());
}

/* ==================== Demo: Device Configuration ==================== */
static void demo_device_config(void)
{
    printf("\r\n=== Demo 2: Device Configuration ===\r\n");

    /* Store device name */
    kv_set_string("dev_name", "BluePill-01");

    /* Store WiFi SSID */
    kv_set_string("wifi_ssid", "LabIoT_5G");

    /* Store WiFi password */
    kv_set_string("wifi_pass", "embedded2025");

    /* Read back */
    char buf[64];
    kv_get_string("dev_name", buf, sizeof(buf));
    printf("[DEMO] Device name: %s\r\n", buf);

    kv_get_string("wifi_ssid", buf, sizeof(buf));
    printf("[DEMO] WiFi SSID : %s\r\n", buf);

    kv_get_string("wifi_pass", buf, sizeof(buf));
    printf("[DEMO] WiFi Pass : %s\r\n", buf);
}

/* ==================== Demo: Calibration Values ==================== */
static void demo_calibration(void)
{
    printf("\r\n=== Demo 3: Calibration Values ===\r\n");

    /* Store calibration offset */
    kv_set_float("cal_offset", -2.35f);

    /* Store calibration gain */
    kv_set_float("cal_gain", 1.0247f);

    /* Store ADC reference */
    kv_set_int("adc_ref_mv", 3300);

    /* Store calibration temperature */
    kv_set_float("cal_temp", 25.0f);

    /* Read back */
    float fval;
    int32_t ival;

    kv_get_float("cal_offset", &fval);
    printf("[DEMO] Cal offset : %.4f\r\n", (double)fval);

    kv_get_float("cal_gain", &fval);
    printf("[DEMO] Cal gain   : %.4f\r\n", (double)fval);

    kv_get_int("adc_ref_mv", &ival);
    printf("[DEMO] ADC ref    : %ld mV\r\n", ival);

    kv_get_float("cal_temp", &fval);
    printf("[DEMO] Cal temp   : %.1f C\r\n", (double)fval);
}

/* ==================== Demo: List All and Delete ==================== */
static void demo_list_and_delete(void)
{
    printf("\r\n=== Demo 4: List and Delete ===\r\n");

    /* List all entries */
    kv_list_all();

    /* Delete WiFi password */
    printf("[DEMO] Deleting 'wifi_pass'...\r\n");
    kv_delete("wifi_pass");

    /* Try to read deleted key */
    char buf[64];
    int ret = kv_get_string("wifi_pass", buf, sizeof(buf));
    if (ret != 0) {
        printf("[DEMO] 'wifi_pass' correctly not found after deletion\r\n");
    }

    /* List again after deletion */
    kv_list_all();

    /* Print stats */
    kv_print_stats();
}

/* ==================== Demo: Force Compaction ==================== */
static void demo_force_compaction(void)
{
    printf("\r\n=== Demo 5: Force Compaction ===\r\n");

    /* Write many updates to fill the page and trigger compaction */
    printf("[DEMO] Writing multiple updates to trigger compaction...\r\n");
    for (int i = 0; i < 15; i++) {
        kv_set_int("counter", (int32_t)i);
        HAL_GPIO_TogglePin(LED_PORT, LED_PIN);
    }

    /* Print final state */
    int32_t val;
    kv_get_int("counter", &val);
    printf("[DEMO] Final counter value: %ld\r\n", val);

    kv_print_stats();
    kv_list_all();
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
    printf("  STM32_09: Flash Key-Value Store\r\n");
    printf("  Board : Blue Pill STM32F103C8\r\n");
    printf("  Clock : 72 MHz (HSE + PLL)\r\n");
    printf("========================================\r\n");

    /* Initialize KV store */
    kv_init();

    /* Run demos */
    demo_boot_counter();
    HAL_Delay(200);

    demo_device_config();
    HAL_Delay(200);

    demo_calibration();
    HAL_Delay(200);

    demo_list_and_delete();
    HAL_Delay(200);

    demo_force_compaction();

    printf("\r\n========================================\r\n");
    printf("  All demos complete!\r\n");
    printf("========================================\r\n");

    /* Main loop */
    uint32_t loop_count = 0;
    while (1) {
        HAL_GPIO_TogglePin(LED_PORT, LED_PIN);
        HAL_Delay(1000);

        if (++loop_count % 30 == 0) {
            /* Periodically update uptime */
            kv_set_int("uptime_sec", (int32_t)(HAL_GetTick() / 1000));
            kv_print_stats();
        }
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
