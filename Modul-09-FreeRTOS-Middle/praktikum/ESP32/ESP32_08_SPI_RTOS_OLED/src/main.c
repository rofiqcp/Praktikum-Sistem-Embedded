// src/main.c untuk ESP32_08_SPI_RTOS_OLED
// Program FreeRTOS dengan SPI OLED dan SPI mutex

// Menginclude header FreeRTOS untuk task creation dan delay
#include "freertos/FreeRTOS.h"
// Menginclude header FreeRTOS untuk mutex
#include "freertos/semphr.h"
// Menginclude header FreeRTOS untuk queue
#include "freertos/queue.h"
// Menginclude header FreeRTOS untuk task management
#include "freertos/task.h"
// Menginclude header SPI ESP32
#include "driver/spi_master.h"
// Menginclude header GPIO ESP32
#include "driver/gpio.h"
// Menginclude header UART ESP32
#include "driver/uart.h"
// Menginclude header untuk konfigurasi aplikasi
#include "config.h"
// Menginclude header standard untuk printf
#include "stdio.h"
// Menginclude header standard untuk string manipulation
#include "string.h"

// Mendefinisikan handle untuk SPI mutex
SemaphoreHandle_t xSPIMutex = NULL;
// Mendefinisikan handle untuk task prepare display
TaskHandle_t xPrepTaskHandle = NULL;
// Mendefinisikan handle untuk task update OLED
TaskHandle_t xUpdateTaskHandle = NULL;
// Mendefinisikan handle untuk device SPI
spi_device_handle_t spi_device = NULL;

// Struktur untuk data display OLED
typedef struct {
    uint8_t line_num;      // Nomor baris (0-7 untuk 128x64)
    char text[32];          // Teks yang akan ditampilkan
    uint32_t timestamp;     // Timestamp data
} display_data_t;

// Variabel global untuk data display terakhir
display_data_t last_display_data = {0, "", 0};

// Fungsi untuk inisialisasi GPIO OLED (DC dan RST)
void init_oled_gpio(void) {
    // Mengatur mode GPIO DC sebagai output
    gpio_set_direction(OLED_DC_PIN, GPIO_MODE_OUTPUT);
    // Mengatur level awal DC ke LOW (command mode)
    gpio_set_level(OLED_DC_PIN, 0);
    
    // Mengatur mode GPIO RST sebagai output
    gpio_set_direction(OLED_RST_PIN, GPIO_MODE_OUTPUT);
    // Mengatur level awal RST ke HIGH (tidak reset)
    gpio_set_level(OLED_RST_PIN, 1);
}

// Fungsi untuk inisialisasi SPI master
void init_spi_master(void) {
    // Mengatur konfigurasi bus SPI
    spi_bus_config_t buscfg = {
        .mosi_io_num = SPI_MOSI_PIN,   // Pin MOSI
        .miso_io_num = SPI_MISO_PIN,   // Pin MISO
        .sclk_io_num = SPI_SCLK_PIN,   // Pin SCLK
        .quadwp_io_num = -1,           // WP tidak digunakan
        .quadhd_io_num = -1,           // HD tidak digunakan
        .max_transfer_sz = 4096        // Ukuran transfer maksimum
    };
    
    // Menginisialisasi bus SPI
    spi_bus_initialize(SPI_HOST, &buscfg, SPI_DMA_CH_AUTO);
    
    // Mengatur konfigurasi device SPI
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = SPI_CLOCK_SPEED,  // Clock speed 10 MHz
        .mode = 0,                         // SPI mode 0
        .spics_io_num = SPI_CS_PIN,        // Pin CS
        .queue_size = 7,                   // Queue size
    };
    
    // Menambahkan device SPI
    spi_bus_add_device(SPI_HOST, &devcfg, &spi_device);
}

// Fungsi untuk mengirim command ke OLED (simulasi)
void oled_send_command(uint8_t cmd) {
    // Mengatur pin DC ke LOW (command mode)
    gpio_set_level(OLED_DC_PIN, 0);
    
    // Mengatur transaksi SPI
    spi_transaction_t t;
    // Mengosongkan struktur transaksi
    memset(&t, 0, sizeof(t));
    // Mengatur panjang command
    t.length = 8;
    // Mengatur data command
    t.tx_buffer = &cmd;
    
    // Melakukan transaksi SPI
    spi_device_transmit(spi_device, &t);
}

// Fungsi untuk mengirim data ke OLED (simulasi)
void oled_send_data(uint8_t *data, uint16_t len) {
    // Mengatur pin DC ke HIGH (data mode)
    gpio_set_level(OLED_DC_PIN, 1);
    
    // Mengatur transaksi SPI
    spi_transaction_t t;
    // Mengosongkan struktur transaksi
    memset(&t, 0, sizeof(t));
    // Mengatur panjang data
    t.length = len * 8;
    // Mengatur buffer data
    t.tx_buffer = data;
    
    // Melakukan transaksi SPI
    spi_device_transmit(spi_device, &t);
}

// Task 1: Prepare display
void task_prepare_display(void *pvParameters) {
    // Counter untuk nomor baris
    uint8_t line = 0;
    // Buffer untuk teks
    char text_buffer[32];
    
    // Loop tak terhingga untuk task ini
    while (1) {
        // Mengambil mutex SPI sebelum menggunakan bus
        if (xSemaphoreTake(xSPIMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            // Format teks untuk ditampilkan
            snprintf(text_buffer, sizeof(text_buffer), "Line %d: %lu", line, xTaskGetTickCount());
            
            // Menyimpan ke data global
            last_display_data.line_num = line;
            // Mengcopy teks ke data global
            strncpy(last_display_data.text, text_buffer, sizeof(last_display_data.text) - 1);
            // Mengakhiri string dengan null
            last_display_data.text[sizeof(last_display_data.text) - 1] = '\0';
            // Mengisi timestamp
            last_display_data.timestamp = xTaskGetTickCount();
            
            // Memberikan kembali mutex SPI
            xSemaphoreGive(xSPIMutex);
            
            // Mencetak info persiapan display
            printf("Prepare: %s\n", text_buffer);
            
            // Pindah ke baris berikutnya (0-7)
            line = (line + 1) % 8;
        } else {
            // Mencetak pesan jika tidak dapat mutex
            printf("Prepare: Failed to get SPI mutex!\n");
        }
        
        // Delay task selama 1000ms
        vTaskDelay(TASK_PREP_DELAY);
    }
}

// Task 2: Update OLED
void task_update_oled(void *pvParameters) {
    // Variabel untuk data display lokal
    display_data_t local_data;
    // Counter untuk cetak status
    uint32_t print_counter = 0;
    
    // Loop tak terhingga untuk task ini
    while (1) {
        // Mengambil mutex SPI sebelum menggunakan bus
        if (xSemaphoreTake(xSPIMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            // Copy data display global ke lokal
            local_data = last_display_data;
            // Memberikan kembali mutex SPI
            xSemaphoreGive(xSPIMutex);
            
            // Mencetak status setiap 4 iterasi (sekitar 2 detik dengan delay 500ms)
            if (print_counter++ >= 4) {
                // Mencetak info update OLED
                printf("Update: OLED Line %d: %s\n", local_data.line_num, local_data.text);
                // Reset counter
                print_counter = 0;
            }
            
            // Simulasi update OLED (dalam praktik nyata: kirim data ke OLED via SPI)
            oled_send_command(0xB0 + local_data.line_num);  // Set page address
            // Simulasi kirim data (dummy)
            uint8_t dummy_data[16] = {0};
            // Mengirim data dummy ke OLED
            oled_send_data(dummy_data, 16);
        } else {
            // Mencetak pesan jika tidak dapat mutex
            printf("Update: Failed to get SPI mutex!\n");
        }
        
        // Delay task selama 500ms
        vTaskDelay(TASK_UPD_DELAY);
    }
}

// Fungsi utama program (app_main di ESP-IDF)
void app_main(void) {
    // Mencetak pesan bahwa program dimulai
    printf("ESP32 SPI RTOS OLED Demo Started\n");
    // Mencetak informasi tentang penggunaan mutex
    printf("Using SPI mutex for bus sharing\n");
    
    // Inisialisasi GPIO OLED
    init_oled_gpio();
    // Inisialisasi SPI master
    init_spi_master();
    
    // Membuat mutex untuk SPI bus
    xSPIMutex = xSemaphoreCreateMutex();
    
    // Mengecek apakah mutex berhasil dibuat
    if (xSPIMutex != NULL) {
        // Mencetak pesan bahwa mutex berhasil dibuat
        printf("SPI mutex created successfully\n");
        
        // Membuat task prepare display dengan stack 4096 bytes
        xTaskCreate(task_prepare_display, "DispPrep", TASK_PREP_STACK_SIZE, NULL, TASK_PREP_PRIORITY, &xPrepTaskHandle);
        // Membuat task update OLED dengan stack 3072 bytes
        xTaskCreate(task_update_oled, "OLEDUpd", TASK_UPD_STACK_SIZE, NULL, TASK_UPD_PRIORITY, &xUpdateTaskHandle);
        
        // Mencetak pesan bahwa task telah dibuat
        printf("Display prepare and OLED update tasks created\n");
        // Mencetak petunjuk penggunaan
        printf("Connect OLED to SPI pins (MOSI=23, SCLK=18, CS=5)\n");
    } else {
        // Mencetak pesan error jika mutex gagal dibuat
        printf("Failed to create SPI mutex!\n");
    }
    
    // app_main harus mengembalikan void atau memanggil vTaskDelete
    // Task tidak perlu dihapus karena akan berjalan terus menerus
}
