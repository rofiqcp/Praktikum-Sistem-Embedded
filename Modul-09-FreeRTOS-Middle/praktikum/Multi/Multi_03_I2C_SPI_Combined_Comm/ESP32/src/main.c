/* Multi_03_I2C_SPI_Combined_Comm - ESP32 sebagai Penerima dan OLED Display */
/* Program ESP32 dengan FreeRTOS untuk terima data SPI dan tampilkan ke OLED */

/* Include header utama ESP32 */
#include "freertos/FreeRTOS.h"
/* Include header task FreeRTOS */
#include "freertos/task.h"
/* Include header queue FreeRTOS */
#include "freertos/queue.h"
/* Include header semaphore FreeRTOS */
#include "freertos/semphr.h"
/* Include driver GPIO */
#include "driver/gpio.h"
/* Include header untuk logging */
#include "esp_log.h"

/* Definisi tag untuk logging */
static const char *TAG = "Multi_03_ESP32";

/* Definisi pin SPI slave (untuk menerima dari STM32) */
#define SPI_SLAVE_PIN_MISO GPIO_NUM_19
#define SPI_SLAVE_PIN_MOSI GPIO_NUM_23
#define SPI_SLAVE_PIN_SCLK GPIO_NUM_18
#define SPI_SLAVE_PIN_CS   GPIO_NUM_5

/* Definisi pin SPI master (untuk OLED) */
#define OLED_SPI_PIN_MOSI GPIO_NUM_13
#define OLED_SPI_PIN_SCLK GPIO_NUM_14
#define OLED_SPI_PIN_CS   GPIO_NUM_15
#define OLED_SPI_PIN_DC   GPIO_NUM_27
#define OLED_SPI_PIN_RST  GPIO_NUM_26

/* Struktur data sensor yang diterima */
typedef struct {
    /* ID sensor dari STM32 */
    uint8_t sensor_id;
    /* Data sensor 1 (temperature low) */
    uint16_t sensor_data1;
    /* Data sensor 2 (temperature high) */
    uint16_t sensor_data2;
    /* Data sensor 3 (pressure) */
    uint16_t sensor_data3;
    /* Checksum untuk validasi */
    uint8_t checksum;
} Sensor_Data_t;

/* Handle untuk queue data sensor */
QueueHandle_t sensorDataQueue;
/* Handle untuk mutex SPI OLED */
SemaphoreHandle_t oledMutex;

/* Deklarasi task FreeRTOS */
void SPI_Receive_Task(void *arg);
void OLED_Display_Task(void *arg);

/* Deklarasi fungsi utility */
uint8_t calculate_checksum(Sensor_Data_t *data);
void oled_init(void);
void oled_display_data(Sensor_Data_t *data);

/* Variabel untuk menyimpan data sensor terakhir */
static Sensor_Data_t last_sensor_data = {0};

/* Fungsi utama program ESP32 */
void app_main(void)
{
    /* Print informasi bahwa program dimulai */
    ESP_LOGI(TAG, "Multi_03_I2C_SPI_Combined_Comm ESP32 Started");
    
    /* Membuat queue untuk data sensor dengan kapasitas 5 item */
    sensorDataQueue = xQueueCreate(5, sizeof(Sensor_Data_t));
    /* Membuat mutex untuk akses OLED */
    oledMutex = xSemaphoreCreateMutex();
    
    /* Inisialisasi OLED */
    oled_init();
    
    /* Membuat task untuk menerima SPI */
    xTaskCreate(SPI_Receive_Task, "SPI_Recv", 4096, NULL, 5, NULL);
    /* Membuat task untuk display OLED */
    xTaskCreate(OLED_Display_Task, "OLED_Disp", 4096, NULL, 4, NULL);
    
    /* Print bahwa task telah dibuat */
    ESP_LOGI(TAG, "All tasks created successfully");
}

/* Task untuk menerima data SPI dari STM32 */
void SPI_Receive_Task(void *arg)
{
    /* Variabel untuk data sensor */
    Sensor_Data_t sensor_data;
    /* Variabel untuk checksum yang dihitung */
    uint8_t calc_checksum;
    
    /* Loop tak terbatas untuk task */
    for (;;)
    {
        /* Simulasi menerima data - dalam implementasi nyata gunakan SPI slave */
        vTaskDelay(pdMS_TO_TICKS(1000));
        
        /* Print info */
        ESP_LOGI(TAG, "SPI Receive Task running");
    }
}

/* Task untuk menampilkan data ke OLED */
void OLED_Display_Task(void *arg)
{
    /* Variabel untuk data sensor */
    Sensor_Data_t received_data;
    /* Variabel untuk checksum yang dihitung */
    uint8_t calc_checksum;
    
    /* Loop tak terbatas untuk task */
    for (;;)
    {
        /* Cek apakah ada data di queue */
        if (xQueueReceive(sensorDataQueue, &received_data, portMAX_DELAY) == pdPASS)
        {
            /* Hitung checksum */
            calc_checksum = calculate_checksum(&received_data);
            
            /* Validasi checksum */
            if (calc_checksum == received_data.checksum)
            {
                /* Simpan data terakhir */
                last_sensor_data = received_data;
                
                /* Tampilkan ke OLED */
                oled_display_data(&received_data);
                
                /* Print info */
                ESP_LOGI(TAG, "Received - ID: %d, Data1: %d, Data2: %d, Data3: %d", 
                         received_data.sensor_id, received_data.sensor_data1, 
                         received_data.sensor_data2, received_data.sensor_data3);
            }
            else
            {
                /* Print error jika checksum tidak valid */
                ESP_LOGE(TAG, "Checksum mismatch! Received: %d, Calculated: %d", 
                          received_data.checksum, calc_checksum);
            }
        }
    }
}

/* Fungsi untuk menghitung checksum */
uint8_t calculate_checksum(Sensor_Data_t *data)
{
    /* Variabel untuk menyimpan hasil checksum */
    uint8_t sum = 0;
    
    /* Tambahkan ID sensor ke checksum */
    sum += data->sensor_id;
    /* Tambahkan byte tinggi sensor1 ke checksum */
    sum += (data->sensor_data1 >> 8) & 0xFF;
    /* Tambahkan byte rendah sensor1 ke checksum */
    sum += data->sensor_data1 & 0xFF;
    /* Tambahkan byte tinggi sensor2 ke checksum */
    sum += (data->sensor_data2 >> 8) & 0xFF;
    /* Tambahkan byte rendah sensor2 ke checksum */
    sum += data->sensor_data2 & 0xFF;
    /* Tambahkan byte tinggi sensor3 ke checksum */
    sum += (data->sensor_data3 >> 8) & 0xFF;
    /* Tambahkan byte rendah sensor3 ke checksum */
    sum += data->sensor_data3 & 0xFF;
    
    /* Kembalikan nilai checksum */
    return sum;
}

/* Inisialisasi OLED */
void oled_init(void)
{
    /* Placeholder untuk inisialisasi OLED */
    ESP_LOGI(TAG, "OLED init (placeholder)");
}

/* Display data ke OLED */
void oled_display_data(Sensor_Data_t *data)
{
    /* Placeholder untuk display ke OLED */
    ESP_LOGI(TAG, "OLED display (placeholder)");
}
