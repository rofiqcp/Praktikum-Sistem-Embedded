/**
 * @file    main.c
 * @brief   ESP32_05 – UART Command Parser (LED ON/OFF, BEEP, STATUS)
 *
 * @details
 * Modul       : Modul-03 Serial UART
 * Board       : ESP32 DevKit / Lolin S2 Mini / ESP32-S3 DevKitC
 * Framework   : ESP-IDF
 *
 * Koneksi     :
 *   - UART0  : pin default (TX=GPIO1, RX=GPIO3 pada ESP32 DevKit)
 *   - LED    : GPIO2 (built-in)
 *   - Buzzer : GPIO4 (aktif HIGH, opsional)
 *   - Hubungkan USB ke PC, gunakan serial monitor 115200 baud
 *
 * Cara Kerja  :
 *   1. Karakter dari UART dikumpulkan hingga menerima '\n' atau '\r'.
 *   2. String perintah di-tokenize (spasi sebagai delimiter).
 *   3. Token pertama dicocokkan dengan tabel perintah:
 *        "LED ON"   → Nyalakan LED
 *        "LED OFF"  → Matikan LED
 *        "BEEP"     → Buzzer ON 200 ms lalu OFF
 *        "STATUS"   → Tampilkan status LED & buzzer
 *        "HELP"     → Tampilkan daftar perintah
 *   4. Respon dikirim kembali via UART.
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "config.h"

static const char *TAG = "CMD_PARSER";

static bool led_state    = false;
static bool buzzer_state = false;

/* ════════════════════════════════════════════════════
 * Helper Functions
 * ════════════════════════════════════════════════════ */

/**
 * @brief Kirim string response ke UART
 */
static void uart_send_str(const char *str)
{
    uart_write_bytes(UART_PORT, str, strlen(str));
}

/**
 * @brief Ubah string ke huruf besar (in-place)
 */
static void str_to_upper(char *s)
{
    for (; *s; s++) {
        *s = toupper((unsigned char)*s);
    }
}

/**
 * @brief Trim whitespace di awal dan akhir string (in-place)
 * @return Pointer ke karakter pertama yang bukan whitespace
 */
static char *str_trim(char *s)
{
    while (*s && isspace((unsigned char)*s)) s++;
    char *end = s + strlen(s) - 1;
    while (end > s && isspace((unsigned char)*end)) *end-- = '\0';
    return s;
}

/* ════════════════════════════════════════════════════
 * Command Handlers
 * ════════════════════════════════════════════════════ */

static void cmd_led_on(void)
{
    led_state = true;
    gpio_set_level(LED_PIN, 1);
    uart_send_str("[OK] LED ON\r\n");
    ESP_LOGI(TAG, "LED ON");
}

static void cmd_led_off(void)
{
    led_state = false;
    gpio_set_level(LED_PIN, 0);
    uart_send_str("[OK] LED OFF\r\n");
    ESP_LOGI(TAG, "LED OFF");
}

static void cmd_beep(void)
{
    buzzer_state = true;
    gpio_set_level(BUZZER_PIN, 1);
    uart_send_str("[OK] BEEP!\r\n");
    ESP_LOGI(TAG, "Buzzer ON (200ms)");
    vTaskDelay(pdMS_TO_TICKS(200));
    gpio_set_level(BUZZER_PIN, 0);
    buzzer_state = false;
}

static void cmd_status(void)
{
    char buf[128];
    snprintf(buf, sizeof(buf),
             "[STATUS] LED=%s | BUZZER=%s\r\n",
             led_state ? "ON" : "OFF",
             buzzer_state ? "ON" : "OFF");
    uart_send_str(buf);
    ESP_LOGI(TAG, "STATUS: LED=%s, BUZZER=%s",
             led_state ? "ON" : "OFF",
             buzzer_state ? "ON" : "OFF");
}

static void cmd_help(void)
{
    uart_send_str("\r\n=== Daftar Perintah ===\r\n");
    uart_send_str("  LED ON   - Nyalakan LED\r\n");
    uart_send_str("  LED OFF  - Matikan LED\r\n");
    uart_send_str("  BEEP     - Bunyikan buzzer 200ms\r\n");
    uart_send_str("  STATUS   - Tampilkan status perangkat\r\n");
    uart_send_str("  HELP     - Tampilkan bantuan ini\r\n");
    uart_send_str("===========================\r\n");
}

/* ════════════════════════════════════════════════════
 * Command Parser
 * ════════════════════════════════════════════════════ */

/**
 * @brief Parse dan eksekusi perintah dari string input
 * @param input String perintah (sudah null-terminated)
 */
static void parse_command(char *input)
{
    char *cmd = str_trim(input);
    if (strlen(cmd) == 0) return;

    str_to_upper(cmd);
    ESP_LOGI(TAG, "Perintah diterima: '%s'", cmd);

    /* Tokenize: ambil kata pertama */
    char *token1 = strtok(cmd, " ");
    char *token2 = strtok(NULL, " ");

    if (token1 == NULL) return;

    if (strcmp(token1, "LED") == 0) {
        if (token2 && strcmp(token2, "ON") == 0) {
            cmd_led_on();
        } else if (token2 && strcmp(token2, "OFF") == 0) {
            cmd_led_off();
        } else {
            uart_send_str("[ERR] Gunakan: LED ON | LED OFF\r\n");
        }
    } else if (strcmp(token1, "BEEP") == 0) {
        cmd_beep();
    } else if (strcmp(token1, "STATUS") == 0) {
        cmd_status();
    } else if (strcmp(token1, "HELP") == 0) {
        cmd_help();
    } else {
        uart_send_str("[ERR] Perintah tidak dikenal. Ketik HELP\r\n");
        ESP_LOGW(TAG, "Unknown command: '%s'", token1);
    }
}

/* ════════════════════════════════════════════════════
 * Initialization & Main
 * ════════════════════════════════════════════════════ */

/**
 * @brief Inisialisasi periferal
 */
static void peripheral_init(void)
{
    /* ── UART ── */
    const uart_config_t uart_cfg = {
        .baud_rate  = UART_BAUD,
        .data_bits  = UART_DATA_8_BITS,
        .parity     = UART_PARITY_DISABLE,
        .stop_bits  = UART_STOP_BITS_1,
        .flow_ctrl  = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    ESP_ERROR_CHECK(uart_param_config(UART_PORT, &uart_cfg));
    ESP_ERROR_CHECK(uart_set_pin(UART_PORT,
                                 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE,
                                 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    ESP_ERROR_CHECK(uart_driver_install(UART_PORT, BUF_SIZE, BUF_SIZE,
                                        0, NULL, 0));

    /* ── GPIO LED ── */
    gpio_config_t led_cfg = {
        .pin_bit_mask = (1ULL << LED_PIN),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&led_cfg));
    gpio_set_level(LED_PIN, 0);

    /* ── GPIO Buzzer ── */
    gpio_config_t buz_cfg = {
        .pin_bit_mask = (1ULL << BUZZER_PIN),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&buz_cfg));
    gpio_set_level(BUZZER_PIN, 0);
}

/**
 * @brief Entry point aplikasi
 */
void app_main(void)
{
    peripheral_init();

    ESP_LOGI(TAG, "UART Command Parser siap");
    cmd_help();

    char cmd_buf[MAX_CMD_LEN];
    int  cmd_idx = 0;

    while (1) {
        uint8_t byte;
        int len = uart_read_bytes(UART_PORT, &byte, 1, pdMS_TO_TICKS(20));
        if (len > 0) {
            /* Echo karakter yang diketik */
            uart_write_bytes(UART_PORT, (const char *)&byte, 1);

            if (byte == '\r' || byte == '\n') {
                uart_send_str("\r\n");
                if (cmd_idx > 0) {
                    cmd_buf[cmd_idx] = '\0';
                    parse_command(cmd_buf);
                    cmd_idx = 0;
                }
            } else if (cmd_idx < MAX_CMD_LEN - 1) {
                cmd_buf[cmd_idx++] = (char)byte;
            }
        }
    }
}
