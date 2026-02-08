/**
 * @file main.c
 * @brief UART Line Editor dengan Command History
 *
 * @details
 * Modul       : Modul 03 - Serial UART
 * Board       : ESP32 DevKit / ESP32-S2 / ESP32-S3
 * Framework   : ESP-IDF
 *
 * Koneksi:
 *   - USB Serial (UART0) untuk komunikasi terminal
 *
 * Cara Kerja:
 *   1. Membaca karakter satu per satu dari UART0
 *   2. Mendukung backspace (0x08 dan 0x7F) untuk menghapus karakter
 *   3. Setiap karakter di-echo kembali ke terminal
 *   4. Ketika Enter diterima (CR/LF), baris lengkap diproses
 *   5. Menyimpan 5 command terakhir dalam history array
 *   6. Panah atas (ESC[A) untuk recall command dari history
 *   7. Menampilkan prompt "> " sebelum setiap input
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "config.h"

static const char *TAG = "LINE_EDITOR";

/* Line buffer */
static char line_buf[MAX_LINE_LEN];
static int  line_pos = 0;

/* Command history */
static char history[HISTORY_SIZE][MAX_LINE_LEN];
static int  history_count = 0;
static int  history_index = 0;

/* ESC sequence state machine */
typedef enum {
    ESC_NONE,
    ESC_GOT_ESC,
    ESC_GOT_BRACKET
} esc_state_t;

static esc_state_t esc_state = ESC_NONE;

/**
 * @brief Inisialisasi UART0
 */
static void uart_init(void)
{
    uart_config_t uart_config = {
        .baud_rate  = UART_BAUD_RATE,
        .data_bits  = UART_DATA_8_BITS,
        .parity     = UART_PARITY_DISABLE,
        .stop_bits  = UART_STOP_BITS_1,
        .flow_ctrl  = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    uart_param_config(UART_PORT, &uart_config);
    uart_driver_install(UART_PORT, UART_BUF_SIZE, UART_BUF_SIZE, 0, NULL, 0);

    ESP_LOGI(TAG, "UART%d diinisialisasi pada %d baud", UART_PORT, UART_BAUD_RATE);
}

/**
 * @brief Kirim string ke UART
 */
static void uart_send_str(const char *str)
{
    uart_write_bytes(UART_PORT, str, strlen(str));
}

/**
 * @brief Kirim satu karakter ke UART
 */
static void uart_send_char(char c)
{
    uart_write_bytes(UART_PORT, &c, 1);
}

/**
 * @brief Hapus baris saat ini dari terminal
 */
static void clear_current_line(void)
{
    /* Move cursor to beginning of line, clear line */
    uart_send_char('\r');
    uart_send_str(PROMPT);
    for (int i = 0; i < MAX_LINE_LEN; i++) {
        uart_send_char(' ');
    }
    uart_send_char('\r');
    uart_send_str(PROMPT);
}

/**
 * @brief Simpan command ke history
 */
static void history_add(const char *cmd)
{
    if (strlen(cmd) == 0) return;

    /* Shift history up */
    if (history_count < HISTORY_SIZE) {
        history_count++;
    }
    for (int i = history_count - 1; i > 0; i--) {
        strncpy(history[i], history[i - 1], MAX_LINE_LEN - 1);
        history[i][MAX_LINE_LEN - 1] = '\0';
    }
    strncpy(history[0], cmd, MAX_LINE_LEN - 1);
    history[0][MAX_LINE_LEN - 1] = '\0';

    history_index = 0;
}

/**
 * @brief Recall command dari history (panah atas)
 */
static void history_recall(void)
{
    if (history_count == 0 || history_index >= history_count) return;

    /* Clear current line */
    clear_current_line();

    /* Copy history entry to line buffer */
    strncpy(line_buf, history[history_index], MAX_LINE_LEN - 1);
    line_buf[MAX_LINE_LEN - 1] = '\0';
    line_pos = strlen(line_buf);

    /* Display recalled command */
    uart_send_str(line_buf);

    /* Advance history index for next recall */
    if (history_index < history_count - 1) {
        history_index++;
    }
}

/**
 * @brief Proses command yang sudah lengkap
 */
static void process_command(const char *cmd)
{
    ESP_LOGI(TAG, "Command diterima: \"%s\" (len=%d)", cmd, strlen(cmd));

    if (strcmp(cmd, "help") == 0) {
        uart_send_str("\r\n--- Bantuan ---\r\n");
        uart_send_str("  help     - Tampilkan bantuan\r\n");
        uart_send_str("  history  - Tampilkan command history\r\n");
        uart_send_str("  clear    - Bersihkan layar\r\n");
        uart_send_str("  info     - Info sistem\r\n");
    } else if (strcmp(cmd, "history") == 0) {
        uart_send_str("\r\n--- Command History ---\r\n");
        for (int i = 0; i < history_count; i++) {
            char buf[MAX_LINE_LEN + 16];
            snprintf(buf, sizeof(buf), "  [%d] %s\r\n", i, history[i]);
            uart_send_str(buf);
        }
    } else if (strcmp(cmd, "clear") == 0) {
        uart_send_str("\033[2J\033[H");
    } else if (strcmp(cmd, "info") == 0) {
        uart_send_str("\r\nESP32 UART Line Editor\r\n");
        char buf[64];
        snprintf(buf, sizeof(buf), "  Max line length : %d\r\n", MAX_LINE_LEN);
        uart_send_str(buf);
        snprintf(buf, sizeof(buf), "  History size    : %d\r\n", HISTORY_SIZE);
        uart_send_str(buf);
        snprintf(buf, sizeof(buf), "  Free heap       : %lu bytes\r\n", (unsigned long)esp_get_free_heap_size());
        uart_send_str(buf);
    } else if (strlen(cmd) > 0) {
        uart_send_str("\r\nCommand tidak dikenal: ");
        uart_send_str(cmd);
        uart_send_str("\r\nKetik 'help' untuk bantuan.\r\n");
    }
}

/**
 * @brief Task utama line editor
 */
static void line_editor_task(void *pvParameters)
{
    uint8_t byte;

    uart_send_str("\r\n=== ESP32 UART Line Editor ===\r\n");
    uart_send_str("Ketik 'help' untuk bantuan. Panah atas untuk history.\r\n");
    uart_send_str(PROMPT);

    while (1) {
        int len = uart_read_bytes(UART_PORT, &byte, 1, pdMS_TO_TICKS(100));
        if (len <= 0) continue;

        /* ESC sequence state machine for arrow keys */
        if (esc_state == ESC_GOT_BRACKET) {
            esc_state = ESC_NONE;
            if (byte == 0x41) {  /* 'A' = Up arrow */
                history_recall();
            }
            /* Ignore other arrow keys */
            continue;
        }

        if (esc_state == ESC_GOT_ESC) {
            if (byte == 0x5B) {  /* '[' */
                esc_state = ESC_GOT_BRACKET;
            } else {
                esc_state = ESC_NONE;
            }
            continue;
        }

        if (byte == CHAR_ESC) {
            esc_state = ESC_GOT_ESC;
            continue;
        }

        /* Backspace handling */
        if (byte == CHAR_BACKSPACE_1 || byte == CHAR_BACKSPACE_2) {
            if (line_pos > 0) {
                line_pos--;
                line_buf[line_pos] = '\0';
                /* Echo: backspace, space, backspace to erase char */
                uart_send_str("\b \b");
            }
            continue;
        }

        /* Enter handling */
        if (byte == CHAR_CR || byte == CHAR_LF) {
            line_buf[line_pos] = '\0';
            uart_send_str("\r\n");

            /* Save to history and process */
            if (line_pos > 0) {
                history_add(line_buf);
                process_command(line_buf);
            }

            /* Reset line buffer and history browse index */
            line_pos = 0;
            memset(line_buf, 0, sizeof(line_buf));
            history_index = 0;

            uart_send_str(PROMPT);
            continue;
        }

        /* Normal printable character */
        if (byte >= 0x20 && byte < 0x7F && line_pos < MAX_LINE_LEN - 1) {
            line_buf[line_pos++] = (char)byte;
            line_buf[line_pos] = '\0';
            uart_send_char((char)byte);  /* Echo */
        }
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "=== Modul 03: UART Line Editor ===");

    uart_init();

    xTaskCreate(line_editor_task, "line_editor", 4096, NULL, 5, NULL);
}
