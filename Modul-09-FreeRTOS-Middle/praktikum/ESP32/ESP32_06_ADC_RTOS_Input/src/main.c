// src/main.c untuk ESP32_06_ADC_RTOS_Input
// Program FreeRTOS dengan ADC dan RTOS queue menggunakan ADC Oneshot Driver

// Menginclude header FreeRTOS untuk task creation dan delay
#include "freertos/FreeRTOS.h"
// Menginclude header FreeRTOS untuk queue
#include "freertos/queue.h"
// Menginclude header FreeRTOS untuk task management
#include "freertos/task.h"
// Menginclude header ADC ESP32 (new oneshot driver)
#include "esp_adc/adc_oneshot.h"
// Menginclude header GPIO ESP32
#include "driver/gpio.h"
// Menginclude header UART ESP32
#include "driver/uart.h"
// Menginclude header untuk konfigurasi aplikasi
#include "config.h"
// Menginclude header standard untuk printf
#include "stdio.h"

// Mendefinisikan struktur untuk data ADC
typedef struct {
    uint32_t raw_value;      // Nilai mentah ADC (0-4095)
    float filtered_value;    // Nilai yang sudah difilter
    uint32_t timestamp;      // Timestamp pembacaan
} adc_data_t;

// Mendefinisikan handle untuk queue ADC
QueueHandle_t xADCQueue = NULL;
// Mendefinisikan handle untuk task read ADC
TaskHandle_t xReadTaskHandle = NULL;
// Mendefinisikan handle untuk task filter dan process
TaskHandle_t xProcessTaskHandle = NULL;
// Mendefinisikan nilai filtered sebelumnya (untuk exponential moving average)
float last_filtered_value = 0.0f;
// Mendefinisikan handle ADC oneshot unit
adc_oneshot_unit_handle_t adc_handle = NULL;

// Fungsi untuk inisialisasi ADC
void init_adc(void) {
    // Konfigurasi ADC oneshot unit
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,
        .ulp_mode = false,
    };
    // Membuat ADC oneshot unit
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc_handle));
    
    // Konfigurasi channel ADC
    adc_oneshot_chan_cfg_t chan_config = {
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_12,
    };
    // Mengatur konfigurasi channel
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, ADC_CHANNEL, &chan_config));
}

// Fungsi untuk inisialisasi LED
void init_led(void) {
    // Mengatur mode GPIO LED sebagai output
    gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);
    // Mengatur level awal LED ke LOW (mati)
    gpio_set_level(LED_GPIO, 0);
}

// Fungsi filter exponential moving average
float filter_adc_value(uint32_t raw_value) {
    // Menghitung nilai filter dengan formula: alpha * new + (1-alpha) * old
    float filtered = (FILTER_ALPHA * raw_value) + ((1.0f - FILTER_ALPHA) * last_filtered_value);
    // Menyimpan nilai filter untuk perhitungan berikutnya
    last_filtered_value = filtered;
    // Mengembalikan nilai yang sudah difilter
    return filtered;
}

// Task 1: Read ADC
void task_read_adc(void *pvParameters) {
    // Variabel untuk menyimpan nilai ADC mentah
    int adc_raw;
    // Variabel untuk data ADC yang akan dikirim ke queue
    adc_data_t adc_data;
    
    // Loop tak terhingga untuk task ini
    while (1) {
        // Membaca nilai ADC dari channel yang ditentukan
        adc_oneshot_read(adc_handle, ADC_CHANNEL, &adc_raw);
        
        // Mengisi struktur data ADC
        adc_data.raw_value = (uint32_t)adc_raw;
        // Mengisi timestamp
        adc_data.timestamp = xTaskGetTickCount();
        
        // Mengirim data ke queue (non-blocking)
        if (xQueueSend(xADCQueue, &adc_data, 0) != pdTRUE) {
            // Mencetak pesan jika queue penuh
            printf("Read ADC: Queue full!\n");
        }
        
        // Delay task selama 100ms
        vTaskDelay(TASK_READ_DELAY);
    }
}

// Task 2: Filter and process
void task_filter_process(void *pvParameters) {
    // Variabel untuk menyimpan data ADC dari queue
    adc_data_t received_data;
    // Variabel untuk LED state
    // Counter untuk cetak status
    uint32_t print_counter = 0;
    
    // Loop tak terhingga untuk task ini
    while (1) {
        // Menerima data dari queue (blocking dengan timeout portMAX_DELAY)
        if (xQueueReceive(xADCQueue, &received_data, portMAX_DELAY) == pdTRUE) {
            // Memfilter nilai ADC
            received_data.filtered_value = filter_adc_value(received_data.raw_value);
            
            // Mencetak status setiap 10 pembacaan
            if (print_counter++ >= 10) {
                // Mencetak nilai ADC mentah dan yang sudah difilter
                printf("Process: Raw=%lu, Filtered=%.2f\n", received_data.raw_value, received_data.filtered_value);
                // Reset counter cetak
                print_counter = 0;
            }
            
            // Menyalakan LED jika nilai ADC di atas 2048 (setengah dari 4095)
            if (received_data.raw_value > 2048) {
                // Mengatur LED nyala
                gpio_set_level(LED_GPIO, 1);
            } else {
                // Mengatur LED mati
                gpio_set_level(LED_GPIO, 0);
            }
        }
    }
}

// Fungsi utama program (app_main di ESP-IDF)
void app_main(void) {
    // Mencetak pesan bahwa program dimulai
    printf("ESP32 ADC RTOS Input Demo Started\n");
    // Mencetak informasi tentang penggunaan queue
    printf("ADC readings will be queued and processed\n");
    
    // Inisialisasi ADC
    init_adc();
    // Inisialisasi LED
    init_led();
    
    // Membuat queue untuk ADC values dengan ukuran 10
    xADCQueue = xQueueCreate(ADC_QUEUE_SIZE, sizeof(adc_data_t));
    
    // Mengecek apakah queue berhasil dibuat
    if (xADCQueue != NULL) {
        // Mencetak pesan bahwa queue berhasil dibuat
        printf("ADC queue created successfully\n");
        
        // Membuat task read ADC dengan stack 2048 bytes
        xTaskCreate(task_read_adc, "ADCRead", TASK_READ_STACK_SIZE, NULL, TASK_READ_PRIORITY, &xReadTaskHandle);
        // Membuat task filter dan process dengan stack 3072 bytes
        xTaskCreate(task_filter_process, "ADCFilt", TASK_PROC_STACK_SIZE, NULL, TASK_PROC_PRIORITY, &xProcessTaskHandle);
        
        // Mencetak pesan bahwa task telah dibuat
        printf("ADC read and filter tasks created\n");
        // Mencetak petunjuk penggunaan
        printf("Connect potentiometer to GPIO36 (ADC1_CH0)\n");
    } else {
        // Mencetak pesan error jika queue gagal dibuat
        printf("Failed to create ADC queue!\n");
    }
    
    // app_main harus mengembalikan void atau memanggil vTaskDelete
    // Task tidak perlu dihapus karena akan berjalan terus menerus
}
