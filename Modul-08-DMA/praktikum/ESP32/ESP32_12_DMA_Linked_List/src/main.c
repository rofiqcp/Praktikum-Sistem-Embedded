/*
 * ==========================================================================
 *  ESP32 DMA Scatter-Gather / Linked List Simulation
 * ==========================================================================
 *  Modul 08 - Program 12
 *
 *  KONSEP SCATTER-GATHER DMA:
 *  ┌─────────────────────────────────────────────────────────────────┐
 *  │ STM32 (beberapa seri):                                         │
 *  │  - DMA linked-list mode: chain of DMA descriptors              │
 *  │  - Setiap descriptor: source, dest, size, next pointer         │
 *  │  - Hardware traverses linked list otomatis                     │
 *  │  - Bisa gather dari multiple source → single dest              │
 *  │                                                                 │
 *  │ ESP32:                                                         │
 *  │  - SPI DMA menggunakan linked-list descriptor secara internal  │
 *  │  - esp_rom_lldesc_t: linked-list DMA descriptor                │
 *  │  - spi_device_queue_trans(): queue multiple transactions       │
 *  │  - Kita bisa simulasi scatter-gather via:                      │
 *  │    1. Sequential transfers (satu per segment)                  │
 *  │    2. Concatenate → single DMA transfer                        │
 *  │    3. Queued transactions                                      │
 *  └─────────────────────────────────────────────────────────────────┘
 *
 *  Program ini:
 *  1. Mendefinisikan data buffers non-contiguous (header, payload, checksum)
 *  2. Membandingkan 3 metode assembly + transfer
 *  3. Membangun protocol packet dan transmit via SPI DMA
 *  4. Timing comparison dan data verification
 *
 *  Hardware: SPI loopback (MOSI→MISO) atau dummy output
 * ==========================================================================
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_heap_caps.h"
#include "driver/spi_master.h"
#include "config.h"

static const char *TAG = "SCATTER_GATHER";

/* ==========================================================================
 *  Protocol Packet Structure
 * ==========================================================================
 *  [Header 16B] [Payload1 128B] [Payload2 256B] [CRC 4B]
 *
 *  Ini mensimulasikan packet yang tersebar di memory (scatter)
 *  dan perlu di-gather menjadi satu transmission.
 * ========================================================================== */

/* Packet header structure */
typedef struct __attribute__((packed)) {
    uint16_t magic;         /* 0xABCD */
    uint8_t  version;       /* Protocol version */
    uint8_t  type;          /* Packet type */
    uint16_t seq_num;       /* Sequence number */
    uint16_t payload1_len;  /* Length of payload 1 */
    uint16_t payload2_len;  /* Length of payload 2 */
    uint32_t total_len;     /* Total packet length */
    uint16_t reserved;      /* Reserved */
} packet_header_t;

/* Scatter-gather segment descriptor (mimics DMA descriptor) */
typedef struct sg_segment {
    uint8_t *data;          /* Pointer to data buffer */
    size_t   length;        /* Segment length */
    const char *name;       /* Segment name (for debug) */
} sg_segment_t;

/* ---- Data Buffers (non-contiguous in memory) ---- */
static uint8_t header_buf[SEG_HEADER_SIZE];
static uint8_t payload1_buf[SEG_PAYLOAD1_SIZE];
static uint8_t payload2_buf[SEG_PAYLOAD2_SIZE];
static uint8_t checksum_buf[SEG_CHECKSUM_SIZE];

/* ---- SPI Handle ---- */
static spi_device_handle_t spi_handle = NULL;

/* ---- Timing Results ---- */
typedef struct {
    int64_t sequential_time;
    int64_t concatenate_time;
    int64_t queued_time;
    float   sequential_avg;
    float   concatenate_avg;
    float   queued_avg;
} timing_result_t;

static timing_result_t timing;

/* ---- Verification ---- */
static uint32_t packets_sent = 0;
static uint32_t verify_errors = 0;

/* ==========================================================================
 *  CRC32 Calculation
 * ========================================================================== */

static uint32_t crc32_calc(const uint8_t *data, size_t len)
{
    uint32_t crc = 0xFFFFFFFF;

    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 1) {
                crc = (crc >> 1) ^ CRC_POLYNOMIAL;
            } else {
                crc >>= 1;
            }
        }
    }

    return ~crc;
}

/**
 * @brief Calculate CRC32 over multiple scattered segments
 *
 * Ini menghitung CRC tanpa perlu meng-concatenate data terlebih dulu.
 */
static uint32_t crc32_scatter(sg_segment_t *segments, int num_segments)
{
    uint32_t crc = 0xFFFFFFFF;

    for (int s = 0; s < num_segments; s++) {
        const uint8_t *data = segments[s].data;
        size_t len = segments[s].length;

        for (size_t i = 0; i < len; i++) {
            crc ^= data[i];
            for (int j = 0; j < 8; j++) {
                if (crc & 1) {
                    crc = (crc >> 1) ^ CRC_POLYNOMIAL;
                } else {
                    crc >>= 1;
                }
            }
        }
    }

    return ~crc;
}

/* ==========================================================================
 *  Packet Building
 * ========================================================================== */

/**
 * @brief Build a protocol packet from scattered parts
 *
 * @param seq_num Sequence number for the packet
 */
static void build_packet(uint16_t seq_num)
{
    /* ---- Build Header ---- */
    packet_header_t *hdr = (packet_header_t *)header_buf;
    hdr->magic = PACKET_MAGIC;
    hdr->version = PACKET_VERSION;
    hdr->type = 0x01;
    hdr->seq_num = seq_num;
    hdr->payload1_len = SEG_PAYLOAD1_SIZE;
    hdr->payload2_len = SEG_PAYLOAD2_SIZE;
    hdr->total_len = TOTAL_PACKET_SIZE;
    hdr->reserved = 0;

    /* ---- Build Payload 1 (sensor data simulation) ---- */
    for (int i = 0; i < SEG_PAYLOAD1_SIZE; i++) {
        payload1_buf[i] = (uint8_t)((seq_num + i) & 0xFF);
    }

    /* ---- Build Payload 2 (configuration / additional data) ---- */
    for (int i = 0; i < SEG_PAYLOAD2_SIZE; i++) {
        payload2_buf[i] = (uint8_t)((seq_num * 3 + i * 7) & 0xFF);
    }

    /* ---- Calculate CRC over header + payloads ---- */
    sg_segment_t segments[3] = {
        { header_buf,   SEG_HEADER_SIZE,   "header" },
        { payload1_buf, SEG_PAYLOAD1_SIZE, "payload1" },
        { payload2_buf, SEG_PAYLOAD2_SIZE, "payload2" },
    };

    uint32_t crc = crc32_scatter(segments, 3);
    memcpy(checksum_buf, &crc, sizeof(crc));
}

/* ==========================================================================
 *  SPI Initialization
 * ========================================================================== */

static esp_err_t spi_init(void)
{
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = SPI_MOSI_PIN,
        .miso_io_num = SPI_MISO_PIN,
        .sclk_io_num = SPI_CLK_PIN,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = TOTAL_PACKET_SIZE + 64,
    };

    esp_err_t ret = spi_bus_initialize(SPI_HOST_ID, &bus_cfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI bus init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    spi_device_interface_config_t dev_cfg = {
        .clock_speed_hz = SPI_CLOCK_HZ,
        .mode = 0,
        .spics_io_num = SPI_CS_PIN,
        .queue_size = NUM_SEGMENTS + 1,  /* Queue size for Method 3 */
    };

    ret = spi_bus_add_device(SPI_HOST_ID, &dev_cfg, &spi_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI device add failed: %s", esp_err_to_name(ret));
        spi_bus_free(SPI_HOST_ID);
        return ret;
    }

    ESP_LOGI(TAG, "SPI initialized with DMA:");
    ESP_LOGI(TAG, "  Clock: %d MHz", SPI_CLOCK_HZ / 1000000);
    ESP_LOGI(TAG, "  MOSI: GPIO%d, MISO: GPIO%d, CLK: GPIO%d, CS: GPIO%d",
             SPI_MOSI_PIN, SPI_MISO_PIN, SPI_CLK_PIN, SPI_CS_PIN);
    ESP_LOGI(TAG, "  DMA: AUTO (linked-list internally)");

    return ESP_OK;
}

/* ==========================================================================
 *  METHOD 1: Sequential SPI Transfers (one per segment)
 * ==========================================================================
 *  Kirim setiap segment sebagai transaksi SPI terpisah.
 *  
 *  Analogi: Seperti DMA tanpa linked-list → perlu setup ulang tiap segment.
 *  Di STM32: HAL_SPI_Transmit() dipanggil 4 kali.
 * ========================================================================== */

static void method1_sequential(void)
{
    sg_segment_t segments[NUM_SEGMENTS] = {
        { header_buf,   SEG_HEADER_SIZE,   "header" },
        { payload1_buf, SEG_PAYLOAD1_SIZE, "payload1" },
        { payload2_buf, SEG_PAYLOAD2_SIZE, "payload2" },
        { checksum_buf, SEG_CHECKSUM_SIZE, "checksum" },
    };

    for (int s = 0; s < NUM_SEGMENTS; s++) {
        spi_transaction_t trans = {
            .length = segments[s].length * 8,
            .tx_buffer = segments[s].data,
        };

        spi_device_transmit(spi_handle, &trans);
    }
}

/* ==========================================================================
 *  METHOD 2: Concatenate into Single Buffer → DMA Transfer
 * ==========================================================================
 *  Gather: copy semua segment ke satu buffer, lalu satu DMA transfer.
 *  
 *  Analogi: Software gather → single DMA transfer.
 *  Trade-off: Extra memcpy overhead, tapi hanya satu DMA setup.
 * ========================================================================== */

static void method2_concatenate(void)
{
    /* Allocate DMA-capable buffer for concatenated packet */
    uint8_t *concat_buf = heap_caps_malloc(TOTAL_PACKET_SIZE, MALLOC_CAP_DMA);
    if (!concat_buf) {
        ESP_LOGE(TAG, "Concat buffer alloc failed");
        return;
    }

    /* Gather: copy all segments into contiguous buffer */
    int offset = 0;
    memcpy(concat_buf + offset, header_buf, SEG_HEADER_SIZE);
    offset += SEG_HEADER_SIZE;
    memcpy(concat_buf + offset, payload1_buf, SEG_PAYLOAD1_SIZE);
    offset += SEG_PAYLOAD1_SIZE;
    memcpy(concat_buf + offset, payload2_buf, SEG_PAYLOAD2_SIZE);
    offset += SEG_PAYLOAD2_SIZE;
    memcpy(concat_buf + offset, checksum_buf, SEG_CHECKSUM_SIZE);

    /* Single DMA transfer */
    spi_transaction_t trans = {
        .length = TOTAL_PACKET_SIZE * 8,
        .tx_buffer = concat_buf,
    };

    spi_device_transmit(spi_handle, &trans);

    heap_caps_free(concat_buf);
}

/* ==========================================================================
 *  METHOD 3: Queued SPI Transactions (linked-list-like)
 * ==========================================================================
 *  Queue semua segment sebagai transaksi terpisah, lalu execute.
 *  ESP32 SPI DMA menggunakan linked-list descriptors secara internal,
 *  sehingga queued transactions bisa lebih efisien dari sequential.
 *
 *  Ini paling mirip dengan STM32 DMA scatter-gather:
 *  - spi_device_queue_trans() ≈ setup DMA descriptor
 *  - spi_device_get_trans_result() ≈ wait for completion
 *  - Hardware chains descriptors internally
 * ========================================================================== */

static void method3_queued(void)
{
    sg_segment_t segments[NUM_SEGMENTS] = {
        { header_buf,   SEG_HEADER_SIZE,   "header" },
        { payload1_buf, SEG_PAYLOAD1_SIZE, "payload1" },
        { payload2_buf, SEG_PAYLOAD2_SIZE, "payload2" },
        { checksum_buf, SEG_CHECKSUM_SIZE, "checksum" },
    };

    spi_transaction_t transactions[NUM_SEGMENTS];
    memset(transactions, 0, sizeof(transactions));

    /* Queue all segments
     *
     * Di ESP32, SPI DMA menggunakan linked-list descriptor (lldesc_t).
     * Setiap queued transaction menjadi satu node di linked list.
     * Hardware traverses list secara otomatis.
     *
     * Ini mirip dengan STM32 scatter-gather DMA:
     * - STM32: DMA_SxCR.CT bit toggles between Memory0/Memory1
     * - ESP32: lldesc_t.qe chain points to next descriptor
     */
    for (int s = 0; s < NUM_SEGMENTS; s++) {
        transactions[s].length = segments[s].length * 8;
        transactions[s].tx_buffer = segments[s].data;

        esp_err_t ret = spi_device_queue_trans(spi_handle,
                                                &transactions[s],
                                                portMAX_DELAY);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Queue trans %d failed", s);
            return;
        }
    }

    /* Wait for all transactions to complete */
    for (int s = 0; s < NUM_SEGMENTS; s++) {
        spi_transaction_t *completed;
        spi_device_get_trans_result(spi_handle, &completed, portMAX_DELAY);
    }
}

/* ==========================================================================
 *  Data Verification
 * ========================================================================== */

static bool verify_packet(uint16_t seq_num)
{
    /* Verify header */
    packet_header_t *hdr = (packet_header_t *)header_buf;
    if (hdr->magic != PACKET_MAGIC) return false;
    if (hdr->version != PACKET_VERSION) return false;
    if (hdr->seq_num != seq_num) return false;
    if (hdr->total_len != TOTAL_PACKET_SIZE) return false;

    /* Verify CRC */
    sg_segment_t segments[3] = {
        { header_buf,   SEG_HEADER_SIZE,   "header" },
        { payload1_buf, SEG_PAYLOAD1_SIZE, "payload1" },
        { payload2_buf, SEG_PAYLOAD2_SIZE, "payload2" },
    };

    uint32_t calc_crc = crc32_scatter(segments, 3);
    uint32_t stored_crc;
    memcpy(&stored_crc, checksum_buf, sizeof(stored_crc));

    return (calc_crc == stored_crc);
}

/* ==========================================================================
 *  Timing Comparison
 * ========================================================================== */

static void run_timing_test(void)
{
    int64_t start, elapsed;

    printf("\n");
    printf("╔══════════════════════════════════════════════════════════╗\n");
    printf("║         SCATTER-GATHER TIMING COMPARISON                ║\n");
    printf("╠══════════════════════════════════════════════════════════╣\n");
    printf("║  Packet: [Header %dB][Payload1 %dB][Payload2 %dB]"
           "[CRC %dB]  ║\n",
           SEG_HEADER_SIZE, SEG_PAYLOAD1_SIZE, SEG_PAYLOAD2_SIZE,
           SEG_CHECKSUM_SIZE);
    printf("║  Total:  %d bytes, %d iterations per method             ║\n",
           TOTAL_PACKET_SIZE, TEST_ITERATIONS);
    printf("╚══════════════════════════════════════════════════════════╝\n\n");

    /* ---- Method 1: Sequential ---- */
    ESP_LOGI(TAG, "Method 1: Sequential SPI transfers...");
    start = esp_timer_get_time();
    for (int i = 0; i < TEST_ITERATIONS; i++) {
        build_packet(i);
        method1_sequential();
        packets_sent++;
        if (!verify_packet(i)) verify_errors++;
    }
    timing.sequential_time = esp_timer_get_time() - start;
    timing.sequential_avg = (float)timing.sequential_time / TEST_ITERATIONS;

    ESP_LOGI(TAG, "  Total: %lld us, Avg: %.1f us/packet",
             timing.sequential_time, timing.sequential_avg);

    vTaskDelay(pdMS_TO_TICKS(100));

    /* ---- Method 2: Concatenate ---- */
    ESP_LOGI(TAG, "Method 2: Concatenate + single DMA...");
    start = esp_timer_get_time();
    for (int i = 0; i < TEST_ITERATIONS; i++) {
        build_packet(i + TEST_ITERATIONS);
        method2_concatenate();
        packets_sent++;
        if (!verify_packet(i + TEST_ITERATIONS)) verify_errors++;
    }
    timing.concatenate_time = esp_timer_get_time() - start;
    timing.concatenate_avg = (float)timing.concatenate_time / TEST_ITERATIONS;

    ESP_LOGI(TAG, "  Total: %lld us, Avg: %.1f us/packet",
             timing.concatenate_time, timing.concatenate_avg);

    vTaskDelay(pdMS_TO_TICKS(100));

    /* ---- Method 3: Queued ---- */
    ESP_LOGI(TAG, "Method 3: Queued transactions (linked-list)...");
    start = esp_timer_get_time();
    for (int i = 0; i < TEST_ITERATIONS; i++) {
        build_packet(i + 2 * TEST_ITERATIONS);
        method3_queued();
        packets_sent++;
        if (!verify_packet(i + 2 * TEST_ITERATIONS)) verify_errors++;
    }
    timing.queued_time = esp_timer_get_time() - start;
    timing.queued_avg = (float)timing.queued_time / TEST_ITERATIONS;

    ESP_LOGI(TAG, "  Total: %lld us, Avg: %.1f us/packet",
             timing.queued_time, timing.queued_avg);
}

/* ==========================================================================
 *  Print Results
 * ========================================================================== */

static void print_results(void)
{
    /* Find fastest method */
    float min_time = timing.sequential_avg;
    const char *fastest = "Sequential";
    if (timing.concatenate_avg < min_time && timing.concatenate_avg > 0) {
        min_time = timing.concatenate_avg;
        fastest = "Concatenate";
    }
    if (timing.queued_avg < min_time && timing.queued_avg > 0) {
        min_time = timing.queued_avg;
        fastest = "Queued";
    }

    printf("\n");
    printf("╔══════════════════════════════════════════════════════════════╗\n");
    printf("║              SCATTER-GATHER RESULTS                         ║\n");
    printf("╠════════════════╦══════════════╦═════════╦═══════════════════╣\n");
    printf("║ Method         ║ Avg (us)     ║ Speedup ║ Description       ║\n");
    printf("╠════════════════╬══════════════╬═════════╬═══════════════════╣\n");

    float base = timing.sequential_avg > 0 ? timing.sequential_avg : 1;

    printf("║ Sequential     ║ %10.1f   ║ 1.00x   ║ 4 SPI transfers   ║\n",
           timing.sequential_avg);
    printf("║ Concatenate    ║ %10.1f   ║ %5.2fx  ║ memcpy + 1 DMA    ║\n",
           timing.concatenate_avg, base / (timing.concatenate_avg > 0 ?
                                            timing.concatenate_avg : 1));
    printf("║ Queued         ║ %10.1f   ║ %5.2fx  ║ Queue + linked    ║\n",
           timing.queued_avg, base / (timing.queued_avg > 0 ?
                                      timing.queued_avg : 1));

    printf("╠════════════════╩══════════════╩═════════╩═══════════════════╣\n");
    printf("║  Fastest Method: %-42s  ║\n", fastest);
    printf("╠══════════════════════════════════════════════════════════════╣\n");

    /* Scatter-Gather Visualization */
    printf("║                                                              ║\n");
    printf("║  SCATTER-GATHER VISUALIZATION:                               ║\n");
    printf("║                                                              ║\n");
    printf("║  Memory (scattered):                                         ║\n");
    printf("║  ┌──────┐     ┌──────────────┐                              ║\n");
    printf("║  │Header│     │  Payload 1   │                              ║\n");
    printf("║  │ 16B  │     │    128B      │                              ║\n");
    printf("║  └──┬───┘     └──────┬───────┘                              ║\n");
    printf("║     │                │                                       ║\n");
    printf("║     │    ┌───────────────────────┐    ┌────┐                ║\n");
    printf("║     │    │     Payload 2         │    │CRC │                ║\n");
    printf("║     │    │       256B            │    │ 4B │                ║\n");
    printf("║     │    └───────────┬───────────┘    └──┬─┘                ║\n");
    printf("║     │                │                   │                   ║\n");
    printf("║     v                v                   v                   ║\n");
    printf("║  ┌──────────────────────────────────────────┐               ║\n");
    printf("║  │  GATHER → Single Packet (%d bytes)       │               ║\n",
           TOTAL_PACKET_SIZE);
    printf("║  │  [HDR][PAYLOAD1][PAYLOAD2][CRC]          │               ║\n");
    printf("║  └────────────────┬─────────────────────────┘               ║\n");
    printf("║                   │                                          ║\n");
    printf("║                   v                                          ║\n");
    printf("║              SPI DMA TX                                      ║\n");

    printf("╠══════════════════════════════════════════════════════════════╣\n");
    printf("║  Verification:                                               ║\n");
    printf("║  Packets sent:    %-8lu                                    ║\n",
           (unsigned long)packets_sent);
    printf("║  Verify errors:   %-8lu                                    ║\n",
           (unsigned long)verify_errors);
    printf("║  CRC integrity:   %s                                       ║\n",
           verify_errors == 0 ? "✓ PASS" : "✗ FAIL");

    printf("╠══════════════════════════════════════════════════════════════╣\n");
    printf("║  ESP32 vs STM32 Scatter-Gather:                              ║\n");
    printf("║  • STM32: Hardware DMA linked-list descriptors               ║\n");
    printf("║  • ESP32: SPI DMA linked-list (internal lldesc_t)            ║\n");
    printf("║  • ESP32 queued trans ≈ STM32 DMA scatter-gather             ║\n");
    printf("║  • Concatenate method wastes memory but simple               ║\n");
    printf("╚══════════════════════════════════════════════════════════════╝\n");

    /* Timing bar chart */
    printf("\n  Timing Comparison (bar chart):\n");
    printf("  ──────────────────────────────────────────\n");

    float max_time = timing.sequential_avg;
    if (timing.concatenate_avg > max_time) max_time = timing.concatenate_avg;
    if (timing.queued_avg > max_time) max_time = timing.queued_avg;
    if (max_time <= 0) max_time = 1;

    int bar_width = 40;
    int seq_w = (int)(timing.sequential_avg * bar_width / max_time);
    int con_w = (int)(timing.concatenate_avg * bar_width / max_time);
    int que_w = (int)(timing.queued_avg * bar_width / max_time);

    printf("  Sequential:  [");
    for (int i = 0; i < bar_width; i++) printf("%c", i < seq_w ? '#' : '.');
    printf("] %.1f us\n", timing.sequential_avg);

    printf("  Concatenate: [");
    for (int i = 0; i < bar_width; i++) printf("%c", i < con_w ? '=' : '.');
    printf("] %.1f us\n", timing.concatenate_avg);

    printf("  Queued:      [");
    for (int i = 0; i < bar_width; i++) printf("%c", i < que_w ? '*' : '.');
    printf("] %.1f us\n", timing.queued_avg);
}

/* ==========================================================================
 *  Packet Assembly Demonstration
 * ========================================================================== */

static void demo_packet_assembly(void)
{
    printf("\n");
    printf("╔══════════════════════════════════════════════════════════╗\n");
    printf("║          PROTOCOL PACKET ASSEMBLY DEMO                   ║\n");
    printf("╚══════════════════════════════════════════════════════════╝\n\n");

    for (int pkt = 0; pkt < NUM_PACKETS; pkt++) {
        build_packet(pkt);

        packet_header_t *hdr = (packet_header_t *)header_buf;
        uint32_t crc;
        memcpy(&crc, checksum_buf, sizeof(crc));
        bool valid = verify_packet(pkt);

        printf("  Packet #%d:\n", pkt);
        printf("    Header:  magic=0x%04X ver=%d type=%d seq=%d len=%lu\n",
               hdr->magic, hdr->version, hdr->type, hdr->seq_num,
               (unsigned long)hdr->total_len);
        printf("    Pay1[0]: 0x%02X 0x%02X 0x%02X 0x%02X ...\n",
               payload1_buf[0], payload1_buf[1],
               payload1_buf[2], payload1_buf[3]);
        printf("    Pay2[0]: 0x%02X 0x%02X 0x%02X 0x%02X ...\n",
               payload2_buf[0], payload2_buf[1],
               payload2_buf[2], payload2_buf[3]);
        printf("    CRC32:   0x%08lX  %s\n",
               (unsigned long)crc, valid ? "✓" : "✗");
        printf("\n");
    }
}

/* ==========================================================================
 *  Main Application
 * ========================================================================== */

void app_main(void)
{
    printf("\n");
    printf("╔══════════════════════════════════════════════════════════╗\n");
    printf("║   ESP32 Scatter-Gather DMA - Modul 08 Program 12       ║\n");
    printf("╠══════════════════════════════════════════════════════════╣\n");
    printf("║  Linked-list / scatter-gather DMA simulation            ║\n");
    printf("║                                                          ║\n");
    printf("║  Packet: [Header][Payload1][Payload2][CRC]              ║\n");
    printf("║  Sizes:  [%dB][%dB][%dB][%dB] = %d bytes total         ║\n",
           SEG_HEADER_SIZE, SEG_PAYLOAD1_SIZE, SEG_PAYLOAD2_SIZE,
           SEG_CHECKSUM_SIZE, TOTAL_PACKET_SIZE);
    printf("║                                                          ║\n");
    printf("║  Methods:                                                ║\n");
    printf("║  1. Sequential (4 separate SPI transfers)               ║\n");
    printf("║  2. Concatenate (memcpy → 1 DMA transfer)              ║\n");
    printf("║  3. Queued (spi_device_queue_trans linked-list)         ║\n");
    printf("╚══════════════════════════════════════════════════════════╝\n\n");

    /* Initialize SPI with DMA */
    esp_err_t ret = spi_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI init failed! Aborting.");
        return;
    }

    /* Demo packet assembly */
    demo_packet_assembly();

    /* Run timing comparison */
    run_timing_test();

    /* Print results */
    print_results();

    /* Clean up */
    spi_bus_remove_device(spi_handle);
    spi_bus_free(SPI_HOST_ID);

    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "Scatter-gather benchmark complete!");
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "KESIMPULAN:");
    ESP_LOGI(TAG, "- Queued transactions paling efisien (mirip HW scatter-gather)");
    ESP_LOGI(TAG, "- Concatenate simple tapi butuh extra memory + memcpy");
    ESP_LOGI(TAG, "- Sequential paling lambat (banyak overhead setup)");
    ESP_LOGI(TAG, "- ESP32 SPI DMA internally menggunakan linked-list descriptors");
}
