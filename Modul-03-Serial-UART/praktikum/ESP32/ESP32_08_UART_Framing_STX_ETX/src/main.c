/**
 * @file main.c
 * @brief UART Binary Framing Protocol dengan STX/ETX dan Byte Stuffing
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
 *   1. Menggunakan framing protocol: STX | DATA | ETX
 *   2. Byte stuffing dengan DLE jika data mengandung STX/ETX/DLE
 *   3. Encoding: STX/ETX/DLE dalam data di-escape dengan prefix DLE
 *      - STX dalam data -> DLE STX
 *      - ETX dalam data -> DLE ETX
 *      - DLE dalam data -> DLE DLE
 *   4. frame_encode() membungkus data mentah menjadi frame
 *   5. frame_decode() mengekstrak data dari frame yang diterima
 *   6. Mengirim frame test secara periodik
 *   7. Menerima dan memvalidasi frame yang masuk
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "config.h"

static const char *TAG = "FRAMING";

/**
 * @brief Encode data menjadi frame dengan byte stuffing
 *
 * @param data    Data mentah yang akan di-frame
 * @param len     Panjang data
 * @param out     Buffer output untuk frame
 * @param out_len Pointer ke panjang output (diisi oleh fungsi)
 * @return        0 jika sukses, -1 jika buffer penuh
 */
static int frame_encode(const uint8_t *data, int len, uint8_t *out, int *out_len)
{
    int pos = 0;

    if (len == 0 || len > MAX_DATA_SIZE) {
        ESP_LOGW(TAG, "Data kosong atau terlalu besar: %d", len);
        return -1;
    }

    /* Start of frame */
    out[pos++] = STX;

    /* Encode data with byte stuffing */
    for (int i = 0; i < len; i++) {
        if (pos >= MAX_FRAME_SIZE - 2) {
            ESP_LOGW(TAG, "Frame buffer penuh saat encoding");
            return -1;
        }

        if (data[i] == STX || data[i] == ETX || data[i] == DLE) {
            out[pos++] = DLE;    /* Escape prefix */
            out[pos++] = data[i]; /* Original byte */
        } else {
            out[pos++] = data[i];
        }
    }

    /* End of frame */
    if (pos >= MAX_FRAME_SIZE) {
        return -1;
    }
    out[pos++] = ETX;

    *out_len = pos;
    return 0;
}

/**
 * @brief Decode frame, ekstrak data dengan menghapus byte stuffing
 *
 * @param in       Frame input (termasuk STX dan ETX)
 * @param in_len   Panjang frame input
 * @param data     Buffer output untuk data yang diekstrak
 * @param data_len Pointer ke panjang data (diisi oleh fungsi)
 * @return         0 jika sukses, -1 jika frame tidak valid
 */
static int frame_decode(const uint8_t *in, int in_len, uint8_t *data, int *data_len)
{
    int pos = 0;

    if (in_len < 2) {
        ESP_LOGW(TAG, "Frame terlalu pendek: %d bytes", in_len);
        return -1;
    }

    /* Validate STX */
    if (in[0] != STX) {
        ESP_LOGW(TAG, "Frame tidak dimulai dengan STX: 0x%02X", in[0]);
        return -1;
    }

    /* Validate ETX */
    if (in[in_len - 1] != ETX) {
        ESP_LOGW(TAG, "Frame tidak diakhiri dengan ETX: 0x%02X", in[in_len - 1]);
        return -1;
    }

    /* Decode data between STX and ETX */
    int i = 1;  /* Skip STX */
    while (i < in_len - 1) {  /* Stop before ETX */
        if (pos >= MAX_DATA_SIZE) {
            ESP_LOGW(TAG, "Data buffer penuh saat decoding");
            return -1;
        }

        if (in[i] == DLE) {
            /* Next byte is escaped */
            i++;
            if (i >= in_len - 1) {
                ESP_LOGW(TAG, "DLE tanpa byte berikutnya");
                return -1;
            }
            data[pos++] = in[i];
        } else {
            data[pos++] = in[i];
        }
        i++;
    }

    *data_len = pos;
    return 0;
}

/**
 * @brief Print hex dump dari buffer
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
 * @brief Task kirim frame test secara periodik
 */
static void frame_sender_task(void *pvParameters)
{
    uint8_t frame_buf[MAX_FRAME_SIZE];
    int frame_len;
    int test_num = 0;

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(SEND_INTERVAL_MS));
        test_num++;

        ESP_LOGI(TAG, "--- Test Frame #%d ---", test_num);

        /* Test 1: Data biasa tanpa karakter khusus */
        uint8_t data1[] = "Hello ESP32!";
        if (frame_encode(data1, strlen((char *)data1), frame_buf, &frame_len) == 0) {
            hex_dump("  TX Frame (teks biasa)", frame_buf, frame_len);
            uart_write_bytes(UART_PORT, frame_buf, frame_len);

            /* Self-decode untuk verifikasi */
            uint8_t decoded[MAX_DATA_SIZE];
            int decoded_len;
            if (frame_decode(frame_buf, frame_len, decoded, &decoded_len) == 0) {
                decoded[decoded_len] = '\0';
                ESP_LOGI(TAG, "  Decoded: \"%s\" (%d bytes)", decoded, decoded_len);
            }
        }

        /* Test 2: Data yang mengandung karakter khusus STX, ETX, DLE */
        uint8_t data2[] = {0x41, STX, 0x42, ETX, 0x43, DLE, 0x44};
        if (frame_encode(data2, sizeof(data2), frame_buf, &frame_len) == 0) {
            hex_dump("  TX Frame (dgn special chars)", frame_buf, frame_len);

            uint8_t decoded[MAX_DATA_SIZE];
            int decoded_len;
            if (frame_decode(frame_buf, frame_len, decoded, &decoded_len) == 0) {
                hex_dump("  Decoded data", decoded, decoded_len);
                if (decoded_len == sizeof(data2) &&
                    memcmp(decoded, data2, decoded_len) == 0) {
                    ESP_LOGI(TAG, "  ✓ Encode/Decode MATCH");
                } else {
                    ESP_LOGW(TAG, "  ✗ Encode/Decode MISMATCH!");
                }
            }
        }

        /* Test 3: Data berisi counter yang berubah */
        uint8_t data3[8];
        for (int i = 0; i < 8; i++) {
            data3[i] = (uint8_t)(test_num * 10 + i);
        }
        if (frame_encode(data3, sizeof(data3), frame_buf, &frame_len) == 0) {
            hex_dump("  TX Frame (counter)", frame_buf, frame_len);

            uint8_t decoded[MAX_DATA_SIZE];
            int decoded_len;
            if (frame_decode(frame_buf, frame_len, decoded, &decoded_len) == 0) {
                hex_dump("  Decoded data", decoded, decoded_len);
            }
        }
    }
}

/**
 * @brief Task penerima frame
 */
static void frame_receiver_task(void *pvParameters)
{
    uint8_t rx_buf[MAX_FRAME_SIZE];
    int rx_pos = 0;
    bool in_frame = false;
    bool dle_pending = false;

    while (1) {
        uint8_t byte;
        int len = uart_read_bytes(UART_PORT, &byte, 1, pdMS_TO_TICKS(100));
        if (len <= 0) continue;

        if (!in_frame) {
            if (byte == STX) {
                in_frame = true;
                rx_pos = 0;
                rx_buf[rx_pos++] = STX;
            }
            continue;
        }

        /* Inside frame */
        if (rx_pos >= MAX_FRAME_SIZE) {
            ESP_LOGW(TAG, "RX frame overflow, reset");
            in_frame = false;
            rx_pos = 0;
            continue;
        }

        rx_buf[rx_pos++] = byte;

        if (dle_pending) {
            dle_pending = false;
            continue;
        }

        if (byte == DLE) {
            dle_pending = true;
            continue;
        }

        if (byte == ETX) {
            /* Frame complete */
            ESP_LOGI(TAG, "RX frame diterima (%d bytes)", rx_pos);
            hex_dump("  RX Frame", rx_buf, rx_pos);

            uint8_t decoded[MAX_DATA_SIZE];
            int decoded_len;
            if (frame_decode(rx_buf, rx_pos, decoded, &decoded_len) == 0) {
                hex_dump("  RX Data", decoded, decoded_len);
                ESP_LOGI(TAG, "  ✓ Frame valid, data %d bytes", decoded_len);
            } else {
                ESP_LOGW(TAG, "  ✗ Frame decode gagal!");
            }

            in_frame = false;
            rx_pos = 0;
        }
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "=== Modul 03: UART Framing STX/ETX ===");
    ESP_LOGI(TAG, "STX=0x%02X, ETX=0x%02X, DLE=0x%02X", STX, ETX, DLE);

    uart_init();

    xTaskCreate(frame_sender_task, "frame_tx", 4096, NULL, 5, NULL);
    xTaskCreate(frame_receiver_task, "frame_rx", 4096, NULL, 5, NULL);
}
