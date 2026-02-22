/**
 * @file    main.c
 * @brief   ESP32_06 – UART JSON Protocol (kirim/terima JSON sederhana)
 *
 * @details
 * Modul       : Modul-03 Serial UART
 * Board       : ESP32 DevKit / Lolin S2 Mini / ESP32-S3 DevKitC
 * Framework   : ESP-IDF
 *
 * Koneksi     :
 *   - UART0  : pin default (TX=GPIO1, RX=GPIO3 pada ESP32 DevKit)
 *   - LED    : GPIO2 (built-in, untuk demo set pin)
 *   - Hubungkan USB ke PC, gunakan serial monitor 115200 baud
 *
 * Cara Kerja  :
 *   1. Karakter dari UART dikumpulkan hingga menerima '}' (akhir JSON).
 *   2. String JSON di-parse secara manual (tanpa library eksternal).
 *   3. Format yang didukung:
 *        Masuk  : {"cmd":"set","pin":2,"val":1}
 *        Keluar : {"status":"ok","pin":2,"val":1}
 *      Perintah "get" mengembalikan level pin saat ini.
 *      Perintah "set" mengatur level pin dan mengonfirmasi.
 *   4. Error dikirim sebagai: {"status":"error","msg":"..."}
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "config.h"

static const char *TAG = "JSON_PROTO";

/* ════════════════════════════════════════════════════
 * Simple JSON Parser (no external library)
 * ════════════════════════════════════════════════════ */

/**
 * @brief Cari nilai string untuk key tertentu dalam JSON
 * @param json String JSON input
 * @param key  Nama key yang dicari (tanpa tanda kutip)
 * @param[out] out Buffer untuk menyimpan nilai
 * @param out_len Ukuran buffer out
 * @return true jika key ditemukan
 *
 * Contoh: json_get_string(buf, "cmd", out, 32) dari {"cmd":"set"} → out="set"
 */
static bool json_get_string(const char *json, const char *key,
                            char *out, size_t out_len)
{
    /* Cari "key" dalam JSON */
    char search[64];
    snprintf(search, sizeof(search), "\"%s\"", key);
    const char *pos = strstr(json, search);
    if (!pos) return false;

    /* Maju ke setelah ':' */
    pos = strchr(pos, ':');
    if (!pos) return false;
    pos++;

    /* Lewati spasi */
    while (*pos && isspace((unsigned char)*pos)) pos++;

    /* Harus dimulai dengan '"' */
    if (*pos != '"') return false;
    pos++;

    /* Salin hingga '"' penutup */
    size_t i = 0;
    while (*pos && *pos != '"' && i < out_len - 1) {
        out[i++] = *pos++;
    }
    out[i] = '\0';
    return true;
}

/**
 * @brief Cari nilai integer untuk key tertentu dalam JSON
 * @param json String JSON input
 * @param key  Nama key yang dicari
 * @param[out] value Pointer untuk menyimpan nilai integer
 * @return true jika key ditemukan dan berhasil di-parse
 *
 * Contoh: json_get_int(buf, "pin", &val) dari {"pin":2} → val=2
 */
static bool json_get_int(const char *json, const char *key, int *value)
{
    char search[64];
    snprintf(search, sizeof(search), "\"%s\"", key);
    const char *pos = strstr(json, search);
    if (!pos) return false;

    pos = strchr(pos, ':');
    if (!pos) return false;
    pos++;

    while (*pos && isspace((unsigned char)*pos)) pos++;

    /* Parse angka (termasuk negatif) */
    if (!isdigit((unsigned char)*pos) && *pos != '-') return false;
    *value = atoi(pos);
    return true;
}

/* ════════════════════════════════════════════════════
 * UART Helper
 * ════════════════════════════════════════════════════ */

/**
 * @brief Kirim string ke UART
 */
static void uart_send_str(const char *str)
{
    uart_write_bytes(UART_PORT, str, strlen(str));
}

/**
 * @brief Kirim JSON response OK
 */
static void send_json_ok(int pin, int val)
{
    char buf[MAX_JSON_LEN];
    snprintf(buf, sizeof(buf),
             "{\"status\":\"ok\",\"pin\":%d,\"val\":%d}\r\n", pin, val);
    uart_send_str(buf);
    ESP_LOGI(TAG, "TX: %s", buf);
}

/**
 * @brief Kirim JSON response error
 */
static void send_json_error(const char *msg)
{
    char buf[MAX_JSON_LEN];
    snprintf(buf, sizeof(buf),
             "{\"status\":\"error\",\"msg\":\"%s\"}\r\n", msg);
    uart_send_str(buf);
    ESP_LOGW(TAG, "TX error: %s", msg);
}

/* ════════════════════════════════════════════════════
 * GPIO Helper
 * ════════════════════════════════════════════════════ */

/** @brief Daftar pin GPIO yang diperbolehkan untuk kontrol */
static const int allowed_pins[] = { 2, 4, 5, 12, 13, 14, 15 };
static const int allowed_pins_count = sizeof(allowed_pins) / sizeof(allowed_pins[0]);

/**
 * @brief Cek apakah pin ada di daftar yang diperbolehkan
 */
static bool is_pin_allowed(int pin)
{
    for (int i = 0; i < allowed_pins_count; i++) {
        if (allowed_pins[i] == pin) return true;
    }
    return false;
}

/**
 * @brief Konfigurasi pin sebagai output jika belum
 */
static bool pin_configured[GPIO_NUM_MAX] = { false };

static void ensure_pin_output(int pin)
{
    if (!pin_configured[pin]) {
        gpio_config_t io_cfg = {
            .pin_bit_mask = (1ULL << pin),
            .mode         = GPIO_MODE_OUTPUT,
            .pull_up_en   = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type    = GPIO_INTR_DISABLE,
        };
        gpio_config(&io_cfg);
        pin_configured[pin] = true;
        ESP_LOGI(TAG, "GPIO%d dikonfigurasi sebagai output", pin);
    }
}

/* ════════════════════════════════════════════════════
 * JSON Command Processor
 * ════════════════════════════════════════════════════ */

/**
 * @brief Proses perintah JSON yang diterima
 * @param json String JSON (null-terminated)
 */
static void process_json(const char *json)
{
    ESP_LOGI(TAG, "RX: %s", json);

    /* Parse field "cmd" */
    char cmd[16];
    if (!json_get_string(json, "cmd", cmd, sizeof(cmd))) {
        send_json_error("missing 'cmd' field");
        return;
    }

    /* Parse field "pin" */
    int pin;
    if (!json_get_int(json, "pin", &pin)) {
        send_json_error("missing 'pin' field");
        return;
    }

    /* Validasi pin */
    if (!is_pin_allowed(pin)) {
        send_json_error("pin not allowed");
        return;
    }

    /* ── Perintah SET ── */
    if (strcmp(cmd, "set") == 0) {
        int val;
        if (!json_get_int(json, "val", &val)) {
            send_json_error("missing 'val' field for set");
            return;
        }
        val = val ? 1 : 0;   /* Normalisasi ke 0/1 */

        ensure_pin_output(pin);
        gpio_set_level((gpio_num_t)pin, val);
        send_json_ok(pin, val);

    /* ── Perintah GET ── */
    } else if (strcmp(cmd, "get") == 0) {
        int level = gpio_get_level((gpio_num_t)pin);
        send_json_ok(pin, level);

    } else {
        send_json_error("unknown cmd (use 'set' or 'get')");
    }
}

/* ════════════════════════════════════════════════════
 * Initialization & Main
 * ════════════════════════════════════════════════════ */

/**
 * @brief Inisialisasi UART
 */
static void uart_init(void)
{
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
}

/**
 * @brief Entry point aplikasi
 */
void app_main(void)
{
    uart_init();

    /* Konfigurasi LED pin default */
    ensure_pin_output(LED_PIN);
    gpio_set_level(LED_PIN, 0);

    ESP_LOGI(TAG, "UART JSON Protocol siap");
    uart_send_str("\r\n=== ESP32 JSON Protocol ===\r\n");
    uart_send_str("Format: {\"cmd\":\"set\",\"pin\":2,\"val\":1}\r\n");
    uart_send_str("        {\"cmd\":\"get\",\"pin\":2}\r\n\r\n");

    char json_buf[MAX_JSON_LEN];
    int  json_idx   = 0;
    bool in_json    = false;   /* Sedang di dalam object JSON */
    int  brace_depth = 0;      /* Kedalaman kurung kurawal    */

    while (1) {
        uint8_t byte;
        int len = uart_read_bytes(UART_PORT, &byte, 1, pdMS_TO_TICKS(20));
        if (len <= 0) continue;

        /* Echo karakter */
        uart_write_bytes(UART_PORT, (const char *)&byte, 1);

        if (byte == '{') {
            in_json = true;
            brace_depth = 1;
            json_idx = 0;
            json_buf[json_idx++] = (char)byte;
        } else if (in_json) {
            if (json_idx < MAX_JSON_LEN - 1) {
                json_buf[json_idx++] = (char)byte;
            }

            if (byte == '{') {
                brace_depth++;
            } else if (byte == '}') {
                brace_depth--;
                if (brace_depth == 0) {
                    /* JSON lengkap diterima */
                    json_buf[json_idx] = '\0';
                    uart_send_str("\r\n");
                    process_json(json_buf);
                    in_json  = false;
                    json_idx = 0;
                }
            }
        }
        /* Abaikan karakter di luar JSON object */
    }
}
