// src/main.c untuk ESP32_07_I2C_RTOS_Sensor
// Program FreeRTOS dengan I2C sensor dan I2C mutex

// Menginclude header FreeRTOS untuk task creation dan delay
#include "freertos/FreeRTOS.h"
// Menginclude header FreeRTOS untuk mutex
#include "freertos/semphr.h"
// Menginclude header FreeRTOS untuk task management
#include "freertos/task.h"
// Menginclude header I2C ESP32
#include "driver/i2c.h"
// Menginclude header UART ESP32
#include "driver/uart.h"
// Menginclude header untuk konfigurasi aplikasi
#include "config.h"
// Menginclude header standard untuk printf
#include "stdio.h"
// Menginclude header standard untuk string manipulation
#include "string.h"

// Mendefinisikan handle untuk I2C mutex
SemaphoreHandle_t xI2CMutex = NULL;
// Mendefinisikan handle untuk task read sensor
TaskHandle_t xReadTaskHandle = NULL;
// Mendefinisikan handle untuk task process data
TaskHandle_t xProcessTaskHandle = NULL;

// Struktur untuk data sensor BME280 (simulasi)
typedef struct {
    float temperature;   // Suhu dalam Celsius
    float humidity;      // Kelembaban dalam %
    float pressure;      // Tekanan dalam hPa
    uint32_t timestamp;  // Timestamp pembacaan
} sensor_data_t;

// Variabel global untuk data sensor terakhir
sensor_data_t last_sensor_data = {0.0f, 0.0f, 0.0f, 0};

// Fungsi untuk inisialisasi I2C master
void init_i2c_master(void) {
    // Mengatur konfigurasi I2C master
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,               // Mode master
        .sda_io_num = I2C_SDA_PIN,             // Pin SDA
        .scl_io_num = I2C_SCL_PIN,             // Pin SCL
        .sda_pullup_en = GPIO_PULLUP_ENABLE,   // Enable pullup SDA
        .scl_pullup_en = GPIO_PULLUP_ENABLE,   // Enable pullup SCL
        .master.clk_speed = I2C_FREQ_HZ,       // Frekuensi clock 100kHz
    };
    // Mengatur konfigurasi I2C
    i2c_param_config(I2C_PORT, &conf);
    // Install driver I2C master
    i2c_driver_install(I2C_PORT, conf.mode, 0, 0, 0);
}

// Fungsi untuk membaca sensor BME280 (simulasi - dalam praktik nyata baca register asli)
esp_err_t read_bme280_sensor(float *temp, float *hum, float *press) {
    // Mengambil mutex I2C sebelum menggunakan bus
    if (xSemaphoreTake(xI2CMutex, I2C_TIMEOUT) == pdTRUE) {
        // Simulasi membaca data dari BME280
        // Dalam implementasi nyata: baca register BME280 via I2C
        
        // Simulasi data suhu (25.0 + random kecil)
        *temp = 25.0f + ((float)(xTaskGetTickCount() % 100)) / 100.0f;
        // Simulasi data kelembaban (60.0 + random kecil)
        *hum = 60.0f + ((float)(xTaskGetTickCount() % 50)) / 100.0f;
        // Simulasi data tekanan (1013.25 + random kecil)
        *press = 1013.25f + ((float)(xTaskGetTickCount() % 20));
        
        // Memberikan kembali mutex I2C
        xSemaphoreGive(xI2CMutex);
        
        // Mengembalikan sukses
        return ESP_OK;
    }
    // Mengembalikan error jika tidak dapat mutex
    return ESP_ERR_TIMEOUT;
}

// Task 1: Read sensor (BME280)
void task_read_sensor(void *pvParameters) {
    // Variabel untuk menyimpan data sensor
    sensor_data_t sensor_data;
    // Variabel untuk status error
    esp_err_t err;
    
    // Loop tak terhingga untuk task ini
    while (1) {
        // Membaca data suhu
        err = read_bme280_sensor(&sensor_data.temperature, &sensor_data.humidity, &sensor_data.pressure);
        
        // Mengecek apakah pembacaan berhasil
        if (err == ESP_OK) {
            // Mengisi timestamp
            sensor_data.timestamp = xTaskGetTickCount();
            
            // Menyimpan ke variabel global (dalam praktik nyata gunakan queue)
            last_sensor_data = sensor_data;
            
            // Mencetak data sensor
            printf("Read: Temp=%.2fC, Hum=%.2f%%, Press=%.2fhPa\n", 
                   sensor_data.temperature, sensor_data.humidity, sensor_data.pressure);
        } else {
            // Mencetak pesan error
            printf("Read: Failed to read sensor!\n");
        }
        
        // Delay task selama 1000ms
        vTaskDelay(TASK_READ_DELAY);
    }
}

// Task 2: Process data
void task_process_data(void *pvParameters) {
    // Variabel untuk menyimpan data yang diproses
    sensor_data_t processed_data;
    // Counter untuk cetak status
    uint32_t print_counter = 0;
    
    // Loop tak terhingga untuk task ini
    while (1) {
        // Mengambil mutex I2C sebelum mengakses data global
        if (xSemaphoreTake(xI2CMutex, I2C_TIMEOUT) == pdTRUE) {
            // Copy data sensor global ke lokal
            processed_data = last_sensor_data;
            // Memberikan kembali mutex
            xSemaphoreGive(xI2CMutex);
            
            // Mencetak status setiap 2 iterasi (sekitar 1 detik dengan delay 500ms)
            if (print_counter++ >= 2) {
                // Mencetak data yang diproses
                printf("Process: Received data at t=%lu\n", processed_data.timestamp);
                // Reset counter
                print_counter = 0;
            }
            
            // Proses data (simulasi - misal: deteksi suhu tinggi)
            if (processed_data.temperature > 30.0f) {
                // Peringatan suhu tinggi
                printf("Process: WARNING - High temperature!\n");
            }
        }
        
        // Delay task selama 500ms
        vTaskDelay(TASK_PROC_DELAY);
    }
}

// Fungsi utama program (app_main di ESP-IDF)
void app_main(void) {
    // Mencetak pesan bahwa program dimulai
    printf("ESP32 I2C RTOS Sensor Demo Started\n");
    // Mencetak informasi tentang penggunaan mutex
    printf("Using I2C mutex for bus sharing\n");
    
    // Inisialisasi I2C master
    init_i2c_master();
    
    // Membuat mutex untuk I2C bus
    xI2CMutex = xSemaphoreCreateMutex();
    
    // Mengecek apakah mutex berhasil dibuat
    if (xI2CMutex != NULL) {
        // Mencetak pesan bahwa mutex berhasil dibuat
        printf("I2C mutex created successfully\n");
        
        // Membuat task read sensor dengan stack 4096 bytes
        xTaskCreate(task_read_sensor, "SensorRd", TASK_READ_STACK_SIZE, NULL, TASK_READ_PRIORITY, &xReadTaskHandle);
        // Membuat task process data dengan stack 3072 bytes
        xTaskCreate(task_process_data, "DataProc", TASK_PROC_STACK_SIZE, NULL, TASK_PROC_PRIORITY, &xProcessTaskHandle);
        
        // Mencetak pesan bahwa task telah dibuat
        printf("Sensor read and data process tasks created\n");
        // Mencetak petunjuk penggunaan
        printf("Connect BME280 to I2C pins (SDA=21, SCL=22)\n");
    } else {
        // Mencetak pesan error jika mutex gagal dibuat
        printf("Failed to create I2C mutex!\n");
    }
    
    // app_main harus mengembalikan void atau memanggil vTaskDelete
    // Task tidak perlu dihapus karena akan berjalan terus menerus
}
