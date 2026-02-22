/**
 * @file main.c
 * @brief UART CRC-8 Checksum Calculation dan Verification
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
 *   1. Menghitung CRC-8 menggunakan polynomial 0x07 (x^8+x^2+x+1)
 *   2. Format pengiriman: [LENGTH][DATA...][CRC8]
 *      - LENGTH: 1 byte, jumlah byte data (tidak termasuk length dan CRC)
 *      - DATA: 1-128 bytes data
 *      - CRC8: 1 byte CRC dihitung dari LENGTH+DATA
 *   3. Penerima memvalidasi CRC dan melaporkan pass/fail
 *   4. Mengirim data test secara periodik dengan CRC
 *   5. Menerima dan memverifikasi data yang masuk
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "config.h"

static const char *TAG = "CRC8";

/* Statistics */
static uint32_t tx_count = 0;
static uint32_t rx_count = 0;
static uint32_t crc_pass = 0;
static uint32_t crc_fail = 0;

/**
 * @brief Hitung CRC-8
 *
 * @param data  Pointer ke data
 * @param len   Panjang data
 * @return      Nilai CRC-8
 */
static uint8_t crc8_calc(const uint8_t *data, int len)
{
    uint8_t crc = CRC_INIT;

    for (int i = 0; i < len; i++) {
        crc ^= data[i];
        for (int bit = 0; bit < 8; bit++) {
            if (crc & 0x80) {
                crc = (crc << 1) ^ CRC_POLY;
            } else {
                crc <<= 1;
            }
        }
    }

    return crc;
}

/**
 * @brief Kirim data dengan CRC-8
 *
 * Format: [LENGTH][DATA...][CRC8]
 *
 * @param data  Data yang akan dikirim
 * @param len   Panjang data
 * @return      0 jika sukses, -1 jika gagal
 */
static int send_with_crc(const uint8_t *data, int len)
{
    if (len <= 0 || len > MAX_DATA_SIZE) {
        ESP_LOGW(TAG, "Panjang data tidak valid: %d", len);
        return -1;
    }

    uint8_t packet[MAX_PACKET_SIZE];
    int pos = 0;

    /* LENGTH byte */
    packet[pos++] = (uint8_t)len;

    /* DATA bytes */
    memcpy(&packet[pos], data, len);
    pos += len;

    /* Calculate CRC over LENGTH + DATA */
    uint8_t crc = crc8_calc(packet, pos);
    packet[pos++] = crc;

    /* Send packet */
    uart_write_bytes(UART_PORT, packet, pos);
    tx_count++;

    ESP_LOGI(TAG, "TX: len=%d, CRC=0x%02X", len, crc);

    /* Hex dump */
    printf("  TX Packet: ");
    for (int i = 0; i < pos; i++) {
        printf("%02X ", packet[i]);
    }
    printf("\r\n");

    return 0;
}

/**
 * @brief Verifikasi CRC-8 dari data yang diterima
 *
 * @param packet  Packet lengkap [LENGTH][DATA...][CRC8]
 * @param pkt_len Panjang total packet
 * @return        0 jika CRC valid, -1 jika invalid
 */
static int verify_crc(const uint8_t *packet, int pkt_len)
{
    if (pkt_len < 3) {
        ESP_LOGW(TAG, "Packet terlalu pendek: %d bytes", pkt_len);
        return -1;
    }

    uint8_t data_len = packet[0];

    /* Validate length field */
    if (data_len + 2 != pkt_len) {
        ESP_LOGW(TAG, "Length mismatch: field=%d, actual=%d", data_len, pkt_len - 2);
        return -1;
    }

    /* CRC dihitung atas LENGTH + DATA (semua kecuali byte CRC terakhir) */
    uint8_t calc_crc = crc8_calc(packet, pkt_len - 1);
    uint8_t recv_crc = packet[pkt_len - 1];

    if (calc_crc == recv_crc) {
        ESP_LOGI(TAG, "  ✓ CRC PASS (0x%02X)", calc_crc);
        return 0;
    } else {
        ESP_LOGW(TAG, "  ✗ CRC FAIL! calc=0x%02X, recv=0x%02X", calc_crc, recv_crc);
        return -1;
    }
}

/**
 * @brief Print hex dump
 */
static void hex_dump(const char *label, const uint8_t *data, int len)
{
    printf("%s (%d bytes): ", label, len);
    for (int i = 0; i < len; i++) {
        printf("%02X ", data[i]);
    }
    printf("\r\n");
}

/**
 * @brief Inisialisasi UART
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
 * @brief Task pengirim data dengan CRC
 */
static void crc_sender_task(void *pvParameters)
{
    int test_num = 0;

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(SEND_INTERVAL_MS));
        test_num++;

        ESP_LOGI(TAG, "=== Test CRC #%d ===", test_num);

        /* Test 1: String biasa */
        uint8_t data1[] = "Hello CRC!";
        ESP_LOGI(TAG, "Test 1: Teks biasa");
        send_with_crc(data1, strlen((char *)data1));

        vTaskDelay(pdMS_TO_TICKS(500));

        /* Test 2: Data biner */
        uint8_t data2[] = {0x01, 0x02, 0x03, 0x04, 0xFF, 0xFE};
        ESP_LOGI(TAG, "Test 2: Data biner");
        send_with_crc(data2, sizeof(data2));

        vTaskDelay(pdMS_TO_TICKS(500));

        /* Test 3: Data dengan counter */
        uint8_t data3[4];
        data3[0] = (uint8_t)(test_num >> 8);
        data3[1] = (uint8_t)(test_num & 0xFF);
        data3[2] = (uint8_t)(test_num * 3);
        data3[3] = (uint8_t)(test_num * 7);
        ESP_LOGI(TAG, "Test 3: Counter data");
        send_with_crc(data3, sizeof(data3));

        /* Self-verify: buat packet dan verifikasi sendiri */
        vTaskDelay(pdMS_TO_TICKS(500));
        ESP_LOGI(TAG, "--- Self-Verification ---");

        uint8_t test_pkt[MAX_PACKET_SIZE];
        int pkt_len = 0;
        test_pkt[pkt_len++] = (uint8_t)strlen((char *)data1);
        memcpy(&test_pkt[pkt_len], data1, strlen((char *)data1));
        pkt_len += strlen((char *)data1);
        test_pkt[pkt_len] = crc8_calc(test_pkt, pkt_len);
        pkt_len++;

        hex_dump("  Verify packet", test_pkt, pkt_len);
        rx_count++;
        if (verify_crc(test_pkt, pkt_len) == 0) {
            crc_pass++;
        } else {
            crc_fail++;
        }

        /* Test with corrupted data */
        ESP_LOGI(TAG, "--- Corrupt Test ---");
        test_pkt[2] ^= 0xFF;  /* Corrupt satu byte */
        hex_dump("  Corrupted pkt", test_pkt, pkt_len);
        rx_count++;
        if (verify_crc(test_pkt, pkt_len) == 0) {
            crc_pass++;
        } else {
            crc_fail++;
        }

        /* Statistics */
        ESP_LOGI(TAG, "Stats: TX=%lu, RX=%lu, Pass=%lu, Fail=%lu",
                 (unsigned long)tx_count, (unsigned long)rx_count,
                 (unsigned long)crc_pass, (unsigned long)crc_fail);
    }
}

/**
 * @brief Task penerima data dengan verifikasi CRC
 */
static void crc_receiver_task(void *pvParameters)
{
    uint8_t rx_buf[MAX_PACKET_SIZE];
    int rx_pos = 0;
    int expected_len = -1;

    while (1) {
        uint8_t byte;
        int len = uart_read_bytes(UART_PORT, &byte, 1, pdMS_TO_TICKS(100));
        if (len <= 0) {
            /* Timeout: jika ada data partial, reset */
            if (rx_pos > 0 && expected_len < 0) {
                rx_pos = 0;
            }
            continue;
        }

        if (rx_pos >= MAX_PACKET_SIZE) {
            ESP_LOGW(TAG, "RX buffer overflow, reset");
            rx_pos = 0;
            expected_len = -1;
            continue;
        }

        rx_buf[rx_pos++] = byte;

        /* First byte is LENGTH */
        if (rx_pos == 1) {
            expected_len = byte + 2;  /* LENGTH + data + CRC */
            if (expected_len > MAX_PACKET_SIZE || expected_len < 3) {
                ESP_LOGW(TAG, "Invalid length field: %d", byte);
                rx_pos = 0;
                expected_len = -1;
            }
            continue;
        }

        /* Check if packet complete */
        if (rx_pos == expected_len) {
            ESP_LOGI(TAG, "RX Packet diterima (%d bytes)", rx_pos);
            hex_dump("  RX Packet", rx_buf, rx_pos);

            rx_count++;
            if (verify_crc(rx_buf, rx_pos) == 0) {
                crc_pass++;
                /* Print data content */
                printf("  Data: ");
                for (int i = 1; i < rx_pos - 1; i++) {
                    if (rx_buf[i] >= 0x20 && rx_buf[i] < 0x7F) {
                        printf("%c", rx_buf[i]);
                    } else {
                        printf("[%02X]", rx_buf[i]);
                    }
                }
                printf("\r\n");
            } else {
                crc_fail++;
            }

            rx_pos = 0;
            expected_len = -1;
        }
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "=== Modul 03: UART CRC-8 Checksum ===");
    ESP_LOGI(TAG, "Polynomial: 0x%02X, Init: 0x%02X", CRC_POLY, CRC_INIT);

    uart_init();

    /* CRC lookup table verification */
    uint8_t test[] = {0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39};
    uint8_t result = crc8_calc(test, sizeof(test));
    ESP_LOGI(TAG, "CRC-8 test '123456789' = 0x%02X (expected 0xF4)", result);

    xTaskCreate(crc_sender_task, "crc_tx", 4096, NULL, 5, NULL);
    xTaskCreate(crc_receiver_task, "crc_rx", 4096, NULL, 5, NULL);
}
