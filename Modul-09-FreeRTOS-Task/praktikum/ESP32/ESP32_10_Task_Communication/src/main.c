/**
 * ============================================================================
 * PROGRAM 10: ESP32 Task Communication (Unsafe Shared Variables)
 * ============================================================================
 * Demonstrasi komunikasi TIDAK AMAN antar task menggunakan global variable
 * 
 * TUJUAN: Menunjukkan MENGAPA mutex/semaphore dibutuhkan
 *   - Race condition pada shared data
 *   - Torn reads/writes (data terbaca sebagian saat ditulis)
 *   - Data corruption karena akses tidak tersinkronisasi
 * 
 * Program ini SENGAJA dibuat tanpa proteksi (no mutex, no queue) untuk
 * mendemonstrasikan masalah yang akan diselesaikan di Modul 10.
 * 
 * Platform: ESP32 DevKit V1
 * Framework: ESP-IDF
 * ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "esp_system.h"
#include "config.h"

/* Tag untuk logging */
static const char *TAG = "UNSAFE_COMM";

/* ======================== STRUKTUR DATA ================================== */

/**
 * Struktur data yang di-share antar task TANPA proteksi
 * 
 * MASALAH: Ketika producer menulis field secara berurutan dan
 * consumer membaca di tengah-tengah penulisan, data yang terbaca
 * bisa inconsistent (sebagian baru, sebagian lama).
 */
typedef struct {
    uint32_t counter;               /* Counter utama */
    uint32_t counter_copy;          /* Salinan counter (harus sama) */
    int64_t timestamp;              /* Timestamp saat penulisan */
    uint32_t magic_start;           /* Sentinel awal (harus MAGIC_VALUE) */
    uint32_t data[SHARED_ARRAY_SIZE]; /* Array data */
    uint32_t checksum;              /* Checksum dari array data */
    uint32_t magic_end;             /* Sentinel akhir (harus MAGIC_VALUE) */
    uint32_t sequence;              /* Nomor urut penulisan */
} shared_data_t;

/**
 * Statistik korupsi data
 */
typedef struct {
    uint32_t total_reads;           /* Total pembacaan */
    uint32_t counter_mismatch;      /* counter != counter_copy */
    uint32_t magic_start_corrupt;   /* magic_start != MAGIC_VALUE */
    uint32_t magic_end_corrupt;     /* magic_end != MAGIC_VALUE */
    uint32_t checksum_error;        /* Checksum tidak cocok */
    uint32_t sequence_error;        /* Sequence number tidak berurutan */
    uint32_t total_corruptions;     /* Total semua korupsi */
    float corruption_rate;          /* Persentase korupsi */
} corruption_stats_t;

/* ======================== VARIABEL GLOBAL ================================ */

/**
 * Shared data - diakses oleh producer dan consumer TANPA proteksi
 * INI SENGAJA TIDAK AMAN untuk demonstrasi race condition
 */
static volatile shared_data_t shared_data = {0};

/* Buffer shared untuk demo tambahan */
static volatile shared_data_t shared_buffer[SHARED_BUFFER_SIZE] = {{0}};
static volatile int write_index = 0;
static volatile int read_index = 0;

/* Statistik */
static corruption_stats_t corr_stats = {0};
static uint32_t producer_count = 0;
static uint32_t consumer_count = 0;
static uint32_t last_sequence_seen = 0;

/* Handle task */
static TaskHandle_t producer_handle = NULL;
static TaskHandle_t consumer_handle = NULL;
static TaskHandle_t fast_producer_handle = NULL;
static TaskHandle_t fast_consumer_handle = NULL;
static TaskHandle_t monitor_handle = NULL;

/* ======================== FUNGSI UTILITAS ================================ */

/**
 * Inisialisasi GPIO
 */
static void init_gpio(void)
{
    gpio_config_t conf = {
        .pin_bit_mask = (1ULL << LED_CORRUPTION_PIN) | (1ULL << LED_STATUS_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&conf);
    gpio_set_level(LED_CORRUPTION_PIN, 0);
    gpio_set_level(LED_STATUS_PIN, 0);
}

/**
 * Hitung checksum dari array data
 * @param data Array data
 * @param len Panjang array
 * @return Checksum (XOR semua elemen)
 */
static uint32_t calculate_checksum(volatile uint32_t *data, int len)
{
    uint32_t sum = 0;
    for (int i = 0; i < len; i++) {
        sum ^= data[i];
        sum += data[i];
    }
    return sum;
}

/* ======================== TASK FUNCTIONS ================================= */

/**
 * Task Producer - menulis data ke shared struct
 * 
 * Producer menulis field secara berurutan. Karena penulisan tidak atomik,
 * consumer bisa membaca struct saat masih dalam proses penulisan, 
 * menyebabkan data inconsistent.
 * 
 * Contoh masalah:
 *   1. Producer tulis counter = 100
 *   2. Consumer baca counter = 100, counter_copy = 99 (belum diupdate)
 *   3. Ini adalah "torn write" - data sebagian baru, sebagian lama
 * 
 * @param pvParameters Tidak digunakan
 */
static void producer_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Producer dimulai pada Core %d", xPortGetCoreID());

    uint32_t seq = 0;

    while (1) {
        seq++;

        /**
         * PENULISAN TIDAK ATOMIK - Setiap field ditulis satu per satu
         * Ada jeda antar penulisan di mana consumer bisa membaca
         * data yang belum lengkap diupdate
         */

        /* Tulis magic start */
        shared_data.magic_start = MAGIC_VALUE;

        /* Tulis counter (dua salinan yang harus identik) */
        shared_data.counter = seq;

        /* === TITIK RAWAN: Jeda antara counter dan counter_copy === */
        /* Di sini consumer bisa membaca counter yang sudah baru */
        /* tapi counter_copy yang masih lama */

        shared_data.counter_copy = seq;

        /* Tulis timestamp */
        shared_data.timestamp = esp_timer_get_time();

        /* Tulis array data - setiap elemen = counter + index */
        for (int i = 0; i < SHARED_ARRAY_SIZE; i++) {
            shared_data.data[i] = seq + i;
            /* === TITIK RAWAN: Jeda antar elemen array === */
        }

        /* Hitung dan tulis checksum */
        shared_data.checksum = calculate_checksum(
            (volatile uint32_t *)shared_data.data, SHARED_ARRAY_SIZE);

        /* Tulis sequence number */
        shared_data.sequence = seq;

        /* Tulis magic end */
        shared_data.magic_end = MAGIC_VALUE;

        producer_count++;

        /* Cetak setiap 100 iterasi */
        if (seq % 100 == 0) {
            printf("[DATA]PRODUCER,seq=%lu,count=%lu,core=%d\n",
                   seq, producer_count, xPortGetCoreID());
        }

        /* Delay minimal atau tanpa delay untuk meningkatkan kemungkinan race */
        if (PRODUCER_DELAY_MS > 0) {
            vTaskDelay(pdMS_TO_TICKS(PRODUCER_DELAY_MS));
        } else {
            taskYIELD();  /* Yield tanpa delay */
        }
    }
}

/**
 * Task Consumer - membaca data dari shared struct
 * 
 * Consumer membaca semua field dan memvalidasi konsistensi:
 *   - counter == counter_copy?
 *   - magic_start == MAGIC_VALUE?
 *   - magic_end == MAGIC_VALUE?
 *   - checksum cocok dengan data?
 *   - sequence berurutan?
 * 
 * @param pvParameters Tidak digunakan
 */
static void consumer_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Consumer dimulai pada Core %d", xPortGetCoreID());

    while (1) {
        /**
         * PEMBACAAN TIDAK ATOMIK - Setiap field dibaca satu per satu
         * Producer bisa mengubah data di tengah pembacaan
         */
        uint32_t r_magic_start = shared_data.magic_start;
        uint32_t r_counter = shared_data.counter;
        uint32_t r_counter_copy = shared_data.counter_copy;
        int64_t r_timestamp = shared_data.timestamp;
        uint32_t r_data[SHARED_ARRAY_SIZE];
        for (int i = 0; i < SHARED_ARRAY_SIZE; i++) {
            r_data[i] = shared_data.data[i];
        }
        uint32_t r_checksum = shared_data.checksum;
        uint32_t r_sequence = shared_data.sequence;
        uint32_t r_magic_end = shared_data.magic_end;

        consumer_count++;
        corr_stats.total_reads++;
        bool corruption_found = false;

        /* === CEK 1: Counter harus sama dengan salinannya === */
        if (r_counter != r_counter_copy) {
            corr_stats.counter_mismatch++;
            corruption_found = true;
            printf("[DATA]CORRUPT,type=counter_mismatch,counter=%lu,copy=%lu,read=%lu\n",
                   r_counter, r_counter_copy, corr_stats.total_reads);
        }

        /* === CEK 2: Magic start sentinel === */
        if (r_magic_start != MAGIC_VALUE) {
            corr_stats.magic_start_corrupt++;
            corruption_found = true;
            printf("[DATA]CORRUPT,type=magic_start,got=0x%08lX,expected=0x%08lX\n",
                   r_magic_start, (uint32_t)MAGIC_VALUE);
        }

        /* === CEK 3: Magic end sentinel === */
        if (r_magic_end != MAGIC_VALUE) {
            corr_stats.magic_end_corrupt++;
            corruption_found = true;
            printf("[DATA]CORRUPT,type=magic_end,got=0x%08lX,expected=0x%08lX\n",
                   r_magic_end, (uint32_t)MAGIC_VALUE);
        }

        /* === CEK 4: Checksum === */
        uint32_t calc_checksum = calculate_checksum(
            (volatile uint32_t *)r_data, SHARED_ARRAY_SIZE);
        if (calc_checksum != r_checksum) {
            corr_stats.checksum_error++;
            corruption_found = true;
            printf("[DATA]CORRUPT,type=checksum,calc=0x%08lX,stored=0x%08lX\n",
                   calc_checksum, r_checksum);
        }

        /* === CEK 5: Sequence number monoton naik === */
        if (r_sequence > 0 && last_sequence_seen > 0) {
            if (r_sequence < last_sequence_seen) {
                corr_stats.sequence_error++;
                corruption_found = true;
                printf("[DATA]CORRUPT,type=sequence,current=%lu,last=%lu\n",
                       r_sequence, last_sequence_seen);
            }
        }
        last_sequence_seen = r_sequence;

        /* Update total korupsi */
        if (corruption_found) {
            corr_stats.total_corruptions++;

            /* LED berkedip saat korupsi terdeteksi */
            gpio_set_level(LED_CORRUPTION_PIN, 1);
            /* Delay kecil untuk visualisasi LED */
            for (volatile int i = 0; i < 10000; i++) {}
            gpio_set_level(LED_CORRUPTION_PIN, 0);
        }

        /* Hitung corruption rate */
        if (corr_stats.total_reads > 0) {
            corr_stats.corruption_rate =
                (float)corr_stats.total_corruptions / corr_stats.total_reads * 100.0f;
        }

        /* Cetak setiap 100 iterasi */
        if (consumer_count % 100 == 0) {
            printf("[DATA]CONSUMER,reads=%lu,seq=%lu,corruptions=%lu,rate=%.2f%%\n",
                   corr_stats.total_reads, r_sequence,
                   corr_stats.total_corruptions, corr_stats.corruption_rate);
        }

        /* Delay minimal untuk meningkatkan race condition */
        if (CONSUMER_DELAY_MS > 0) {
            vTaskDelay(pdMS_TO_TICKS(CONSUMER_DELAY_MS));
        } else {
            taskYIELD();
        }
    }
}

/**
 * Task Fast Producer - versi agresif untuk memaksimalkan race condition
 * Berjalan di core berbeda dari fast consumer untuk memaksimalkan
 * kemungkinan concurrent access
 * 
 * @param pvParameters Tidak digunakan
 */
static void fast_producer_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Fast Producer pada Core %d", xPortGetCoreID());
    uint32_t seq = 0;

    while (1) {
        seq++;
        int idx = write_index;

        /* Tulis ke buffer tanpa proteksi */
        shared_buffer[idx].magic_start = MAGIC_VALUE;
        shared_buffer[idx].counter = seq;
        /* Sengaja ada jeda logis */
        shared_buffer[idx].counter_copy = seq;
        for (int i = 0; i < SHARED_ARRAY_SIZE; i++) {
            shared_buffer[idx].data[i] = seq + i;
        }
        shared_buffer[idx].checksum = calculate_checksum(
            (volatile uint32_t *)shared_buffer[idx].data, SHARED_ARRAY_SIZE);
        shared_buffer[idx].sequence = seq;
        shared_buffer[idx].magic_end = MAGIC_VALUE;

        /* Update write index tanpa atomik */
        write_index = (idx + 1) % SHARED_BUFFER_SIZE;

        if (seq % 500 == 0) {
            printf("[DATA]FAST_PROD,seq=%lu,widx=%d,core=%d\n",
                   seq, write_index, xPortGetCoreID());
        }

        taskYIELD();
    }
}

/**
 * Task Fast Consumer - membaca dari buffer tanpa proteksi
 * 
 * @param pvParameters Tidak digunakan
 */
static void fast_consumer_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Fast Consumer pada Core %d", xPortGetCoreID());
    uint32_t read_count = 0;
    uint32_t fast_corruptions = 0;

    while (1) {
        int idx = read_index;

        /* Baca dari buffer tanpa proteksi */
        uint32_t r_cnt = shared_buffer[idx].counter;
        uint32_t r_cpy = shared_buffer[idx].counter_copy;
        uint32_t r_seq = shared_buffer[idx].sequence;
        uint32_t r_magic_s = shared_buffer[idx].magic_start;
        uint32_t r_magic_e = shared_buffer[idx].magic_end;

        read_count++;

        /* Cek korupsi */
        if (r_cnt != r_cpy || r_magic_s != MAGIC_VALUE || r_magic_e != MAGIC_VALUE) {
            fast_corruptions++;
            printf("[DATA]FAST_CORRUPT,cnt=%lu,cpy=%lu,seq=%lu,reads=%lu\n",
                   r_cnt, r_cpy, r_seq, read_count);
        }

        /* Update read index */
        read_index = (idx + 1) % SHARED_BUFFER_SIZE;

        if (read_count % 500 == 0) {
            printf("[DATA]FAST_CONS,reads=%lu,corruptions=%lu,rate=%.2f%%\n",
                   read_count, fast_corruptions,
                   (float)fast_corruptions / read_count * 100.0f);
        }

        taskYIELD();
    }
}

/**
 * Task Monitor - cetak ringkasan statistik korupsi
 * 
 * @param pvParameters Tidak digunakan
 */
static void monitor_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Monitor dimulai pada Core %d", xPortGetCoreID());
    int64_t start_time = esp_timer_get_time();
    uint32_t cycle = 0;

    vTaskDelay(pdMS_TO_TICKS(3000));

    while (1) {
        cycle++;
        int64_t uptime = (esp_timer_get_time() - start_time) / 1000000;

        printf("\n========================================\n");
        printf("  RACE CONDITION MONITOR - Siklus #%lu\n", cycle);
        printf("  Uptime: %lld detik\n", uptime);
        printf("========================================\n");

        /* LED status toggle */
        gpio_set_level(LED_STATUS_PIN, cycle % 2);

        /* Statistik lengkap */
        printf("[DATA]RACE_STATS,reads=%lu,corruptions=%lu,rate=%.4f%%\n",
               corr_stats.total_reads, corr_stats.total_corruptions,
               corr_stats.corruption_rate);

        printf("[DATA]DETAIL,counter_mismatch=%lu,magic_start=%lu,"
               "magic_end=%lu,checksum=%lu,sequence=%lu\n",
               corr_stats.counter_mismatch,
               corr_stats.magic_start_corrupt,
               corr_stats.magic_end_corrupt,
               corr_stats.checksum_error,
               corr_stats.sequence_error);

        printf("[DATA]THROUGHPUT,producer=%lu,consumer=%lu,uptime=%lld\n",
               producer_count, consumer_count, uptime);

        /* Heap info */
        printf("[DATA]HEAP,free=%lu,min=%lu\n",
               (uint32_t)esp_get_free_heap_size(),
               (uint32_t)esp_get_minimum_free_heap_size());

        /* Pesan edukasi */
        if (corr_stats.total_corruptions > 0) {
            printf("[DATA]WARNING,RACE CONDITION TERDETEKSI! "
                   "Gunakan mutex/semaphore untuk proteksi (Modul 10)\n");
        } else if (corr_stats.total_reads > 1000) {
            printf("[DATA]NOTE,Belum ada korupsi terdeteksi. "
                   "Race condition bersifat non-deterministic!\n");
        }

        vTaskDelay(pdMS_TO_TICKS(MONITOR_INTERVAL_MS));
    }
}

/* ======================== ENTRY POINT ==================================== */

void app_main(void)
{
    printf("\n");
    printf("============================================================\n");
    printf("  ESP32 FreeRTOS - Unsafe Task Communication Demo\n");
    printf("  SENGAJA TIDAK AMAN - Menunjukkan Race Condition\n");
    printf("============================================================\n");
    printf("  PERINGATAN: Program ini SENGAJA tidak menggunakan mutex!\n");
    printf("  Tujuan: Menunjukkan mengapa sinkronisasi dibutuhkan.\n");
    printf("  Solusi ada di Modul 10 (Queue & Semaphore).\n");
    printf("  Free Heap: %lu bytes\n", (uint32_t)esp_get_free_heap_size());
    printf("============================================================\n\n");

    /* Inisialisasi GPIO */
    init_gpio();

    /* Inisialisasi shared data */
    memset((void *)&shared_data, 0, sizeof(shared_data));
    memset((void *)shared_buffer, 0, sizeof(shared_buffer));

    printf("[DATA]CONFIG,buffer_size=%d,array_size=%d,magic=0x%08lX\n",
           SHARED_BUFFER_SIZE, SHARED_ARRAY_SIZE, (uint32_t)MAGIC_VALUE);

    /* Buat producer dan consumer di CORE BERBEDA untuk memaksimalkan race */
    ESP_LOGI(TAG, "Membuat task producer di Core 0...");
    xTaskCreatePinnedToCore(producer_task, "Producer", PRODUCER_TASK_STACK,
                            NULL, PRODUCER_TASK_PRIORITY, &producer_handle, 0);

    ESP_LOGI(TAG, "Membuat task consumer di Core 1...");
    xTaskCreatePinnedToCore(consumer_task, "Consumer", CONSUMER_TASK_STACK,
                            NULL, CONSUMER_TASK_PRIORITY, &consumer_handle, 1);

    /* Fast producer/consumer juga di core berbeda */
    ESP_LOGI(TAG, "Membuat fast producer/consumer...");
    xTaskCreatePinnedToCore(fast_producer_task, "FastProd", PRODUCER_TASK_STACK,
                            NULL, FAST_PROD_PRIORITY, &fast_producer_handle, 0);
    xTaskCreatePinnedToCore(fast_consumer_task, "FastCons", CONSUMER_TASK_STACK,
                            NULL, FAST_CONS_PRIORITY, &fast_consumer_handle, 1);

    /* Monitor */
    xTaskCreatePinnedToCore(monitor_task, "Monitor", MONITOR_TASK_STACK,
                            NULL, MONITOR_TASK_PRIORITY, &monitor_handle, 0);

    ESP_LOGI(TAG, "Semua task dimulai. Menunggu race condition...");
    printf("[DATA]STATUS,system=started,tasks=5\n");
}
