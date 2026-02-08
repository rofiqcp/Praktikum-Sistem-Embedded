/**
 * @file    main.c
 * @brief   ESP32_04 – UART Printf Redirect & Formatted Output
 *
 * @details
 * Modul       : Modul-03 Serial UART
 * Board       : ESP32 DevKit / Lolin S2 Mini / ESP32-S3 DevKitC
 * Framework   : ESP-IDF
 *
 * Koneksi     :
 *   - UART0 menggunakan pin default (TX=GPIO1, RX=GPIO3 pada ESP32 DevKit)
 *   - LED pada GPIO2 (built-in pada sebagian besar board ESP32)
 *   - Hubungkan USB ke PC, gunakan serial monitor 115200 baud
 *
 * Cara Kerja  :
 *   1. Mendemonstrasikan bahwa printf() & ESP_LOGI() otomatis terkirim ke
 *      UART0 pada ESP-IDF.
 *   2. Menampilkan data sensor simulasi dalam format tabel di serial monitor.
 *   3. Menerima perintah sederhana ("ON"/"OFF") dari serial untuk toggle LED.
 *   4. Setiap 2 detik, data sensor dummy (suhu, kelembaban, tekanan)
 *      ditampilkan dengan format rapi.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_random.h"
#include "config.h"

static const char *TAG = "UART_PRINTF";

static bool led_state = false;

/**
 * @brief Inisialisasi UART dan GPIO LED
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

    /* ── LED GPIO ── */
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << LED_PIN),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&io_conf));
    gpio_set_level(LED_PIN, 0);
}

/**
 * @brief Cetak header tabel sensor
 */
static void print_table_header(void)
{
    printf("\r\n");
    printf("+------+----------+----------+----------+\r\n");
    printf("|  No  | Suhu(°C) |  RH(%%)   | Pres(hPa)|\r\n");
    printf("+------+----------+----------+----------+\r\n");
}

/**
 * @brief Cetak satu baris data sensor simulasi
 * @param idx Nomor urut data
 */
static void print_sensor_row(int idx)
{
    float temp     = 20.0f + (float)(esp_random() % 200) / 10.0f;   /* 20–40 °C */
    float humidity = 40.0f + (float)(esp_random() % 400) / 10.0f;   /* 40–80 %  */
    float pressure = 1000.0f + (float)(esp_random() % 300) / 10.0f; /* 1000–1030 hPa */

    printf("| %4d | %7.1f  | %7.1f  | %8.1f |\r\n",
           idx, temp, humidity, pressure);
    printf("+------+----------+----------+----------+\r\n");
}

/**
 * @brief Task untuk membaca perintah dari serial & toggle LED
 */
static void cmd_task(void *pvParameters)
{
    uint8_t data[64];

    while (1) {
        int len = uart_read_bytes(UART_PORT, data, sizeof(data) - 1,
                                  pdMS_TO_TICKS(100));
        if (len > 0) {
            data[len] = '\0';
            /* Trim CR/LF */
            char *p = (char *)data;
            while (*p && (*p == '\r' || *p == '\n')) p++;
            char *end = p + strlen(p) - 1;
            while (end > p && (*end == '\r' || *end == '\n')) *end-- = '\0';

            if (strcasecmp(p, "ON") == 0) {
                led_state = true;
                gpio_set_level(LED_PIN, 1);
                ESP_LOGI(TAG, "LED ON");
                printf("[CMD] LED dinyalakan\r\n");
            } else if (strcasecmp(p, "OFF") == 0) {
                led_state = false;
                gpio_set_level(LED_PIN, 0);
                ESP_LOGI(TAG, "LED OFF");
                printf("[CMD] LED dimatikan\r\n");
            } else if (strlen(p) > 0) {
                ESP_LOGW(TAG, "Perintah tidak dikenal: '%s'", p);
                printf("[CMD] Perintah valid: ON | OFF\r\n");
            }
        }
    }
}

/**
 * @brief Entry point aplikasi
 */
void app_main(void)
{
    peripheral_init();

    /* ── Demonstrasi printf & ESP_LOG* ── */
    printf("\r\n========================================\r\n");
    printf("  ESP32 UART Printf Redirect Demo\r\n");
    printf("========================================\r\n");
    ESP_LOGI(TAG, "printf() dan ESP_LOGI() keduanya ke UART0");
    ESP_LOGW(TAG, "Ini contoh warning (ESP_LOGW)");
    printf("Ketik 'ON' atau 'OFF' untuk toggle LED (GPIO%d)\r\n", LED_PIN);

    /* ── Buat task untuk baca perintah ── */
    xTaskCreate(cmd_task, "cmd_task", 4096, NULL, 10, NULL);

    /* ── Loop utama: cetak data sensor tiap 2 detik ── */
    int sample = 0;
    while (1) {
        sample++;
        if (sample % 5 == 1) {
            print_table_header();
        }
        print_sensor_row(sample);

        ESP_LOGI(TAG, "LED=%s | Sample #%d",
                 led_state ? "ON" : "OFF", sample);

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}
