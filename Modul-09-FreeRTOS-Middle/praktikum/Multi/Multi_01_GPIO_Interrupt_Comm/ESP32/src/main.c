/* Multi_01_GPIO_Interrupt_Comm - ESP32 sebagai Slave */
/* Program ESP32 dengan FreeRTOS untuk menerima data SPI dari STM32 dan kontrol LED */

/* Include header utama ESP32 */
#include "freertos/FreeRTOS.h"
/* Include header task FreeRTOS */
#include "freertos/task.h"
/* Include header queue FreeRTOS */
#include "freertos/queue.h"
/* Include header semaphore FreeRTOS */
#include "freertos/semphr.h"
/* Include driver GPIO untuk kontrol pin */
#include "driver/gpio.h"
/* Include header untuk logging/debug */
#include "esp_log.h"

/* Definisi tag untuk logging */
static const char *TAG = "Multi_01_ESP32";

/* Definisi pin LED untuk indikator */
#define LED_BUTTON_PIN GPIO_NUM_2
/* Definisi pin LED untuk encoder naik */
#define LED_ENCODER_UP GPIO_NUM_4
/* Definisi pin LED untuk encoder turun */
#define LED_ENCODER_DOWN GPIO_NUM_16

/* Struktur data yang diterima via SPI */
typedef struct {
    /* Status tombol button dari STM32 */
    uint8_t button_status;
    /* Nilai counter encoder dari STM32 (byte tinggi) */
    int16_t encoder_count;
    /* Checksum untuk validasi data */
    uint8_t checksum;
} SPI_Data_t;

/* Handle untuk queue data SPI */
QueueHandle_t spiDataQueue;
/* Handle untuk mutex akses LED */
SemaphoreHandle_t ledMutex;

/* Deklarasi task untuk menerima SPI */
void SPI_Receive_Task(void *arg);
/* Deklarasi task untuk kontrol LED */
void LED_Control_Task(void *arg);
/* Deklarasi fungsi untuk menghitung checksum */
uint8_t calculate_checksum(SPI_Data_t *data);

/* Variabel untuk menyimpan data SPI terakhir */
static SPI_Data_t received_data = {0};

/* Fungsi utama program ESP32 */
void app_main(void)
{
    /* Print informasi bahwa program dimulai */
    ESP_LOGI(TAG, "Multi_01_GPIO_Interrupt_Comm ESP32 Slave Started");
    
    /* Membuat queue untuk data SPI dengan kapasitas 10 item */
    spiDataQueue = xQueueCreate(10, sizeof(SPI_Data_t));
    /* Membuat mutex untuk akses aman ke LED */
    ledMutex = xSemaphoreCreateMutex();
    
    /* Konfigurasi pin LED sebagai output */
    gpio_config_t io_conf = {};
    /* Disable interrupt */
    io_conf.intr_type = GPIO_INTR_DISABLE;
    /* Set mode output */
    io_conf.mode = GPIO_MODE_OUTPUT;
    /* Bit mask untuk pin LED button */
    io_conf.pin_bit_mask = (1ULL<<LED_BUTTON_PIN) | (1ULL<<LED_ENCODER_UP) | (1ULL<<LED_ENCODER_DOWN);
    /* Disable pull-down mode */
    io_conf.pull_down_en = 0;
    /* Disable pull-up mode */
    io_conf.pull_up_en = 0;
    /* Konfigurasi GPIO */
    gpio_config(&io_conf);
    
    /* Matikan semua LED di awal */
    gpio_set_level(LED_BUTTON_PIN, 0);
    gpio_set_level(LED_ENCODER_UP, 0);
    gpio_set_level(LED_ENCODER_DOWN, 0);
    
    /* Membuat task untuk menerima data SPI */
    xTaskCreate(SPI_Receive_Task, "SPI_Recv_Task", 2048, NULL, 5, NULL);
    /* Membuat task untuk kontrol LED */
    xTaskCreate(LED_Control_Task, "LED_Ctrl_Task", 2048, NULL, 4, NULL);
    
    /* Print bahwa task telah dibuat */
    ESP_LOGI(TAG, "Tasks created successfully");
}

/* Task untuk menerima data SPI dari STM32 */
void SPI_Receive_Task(void *arg)
{
    /* Variabel untuk menyimpan data yang diterima */
    SPI_Data_t data;
    /* Variabel untuk checksum yang dihitung */
    uint8_t calc_checksum;
    
    /* Loop tak terbatas untuk task */
    for (;;)
    {
        /* Simulasi menerima data - dalam implementasi nyata gunakan SPI slave */
        vTaskDelay(pdMS_TO_TICKS(500));
        
        /* Generate dummy data untuk testing */
        data.button_status = (gpio_get_level(LED_BUTTON_PIN) == 1) ? 1 : 0;
        data.encoder_count++;
        data.checksum = calculate_checksum(&data);
        
        /* Kirim data ke queue */
        xQueueSend(spiDataQueue, &data, 0);
        
        /* Print info */
        ESP_LOGI(TAG, "SPI Receive Task - data sent");
    }
}

/* Task untuk mengontrol LED berdasarkan data dari STM32 */
void LED_Control_Task(void *arg)
{
    /* Variabel lokal untuk menyimpan data sebelumnya */
    uint8_t last_button_status = 0;
    /* Variabel untuk menyimpan encoder count sebelumnya */
    int16_t last_encoder_count = 0;
    /* Variabel untuk data saat ini */
    SPI_Data_t current_data;
    
    /* Loop tak terbatas untuk task */
    for (;;)
    {
        /* Ambil data terbaru dari variabel global */
        current_data = received_data;
        
        /* Cek status button */
        if (current_data.button_status != last_button_status)
        {
            /* Ambil mutex untuk akses LED */
            if (xSemaphoreTake(ledMutex, pdMS_TO_TICKS(50)) == pdTRUE)
            {
                /* Kontrol LED button berdasarkan status */
                gpio_set_level(LED_BUTTON_PIN, current_data.button_status);
                
                /* Lepas mutex */
                xSemaphoreGive(ledMutex);
            }
            
            /* Update status button terakhir */
            last_button_status = current_data.button_status;
        }
        
        /* Cek perubahan encoder */
        if (current_data.encoder_count != last_encoder_count)
        {
            /* Ambil mutex untuk akses LED */
            if (xSemaphoreTake(ledMutex, pdMS_TO_TICKS(50)) == pdTRUE)
            {
                /* Cek arah putaran encoder */
                if (current_data.encoder_count > last_encoder_count)
                {
                    /* Encoder naik - nyalakan LED up */
                    gpio_set_level(LED_ENCODER_UP, 1);
                    /* Matikan LED down */
                    gpio_set_level(LED_ENCODER_DOWN, 0);
                    /* Print info */
                    ESP_LOGI(TAG, "Encoder UP - Count: %d", current_data.encoder_count);
                }
                else if (current_data.encoder_count < last_encoder_count)
                {
                    /* Encoder turun - nyalakan LED down */
                    gpio_set_level(LED_ENCODER_DOWN, 1);
                    /* Matikan LED up */
                    gpio_set_level(LED_ENCODER_UP, 0);
                    /* Print info */
                    ESP_LOGI(TAG, "Encoder DOWN - Count: %d", current_data.encoder_count);
                }
                
                /* Lepas mutex */
                xSemaphoreGive(ledMutex);
            }
            
            /* Update encoder count terakhir */
            last_encoder_count = current_data.encoder_count;
        }
        
        /* Delay task selama 100ms */
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

/* Fungsi untuk menghitung checksum */
uint8_t calculate_checksum(SPI_Data_t *data)
{
    /* Variabel untuk menyimpan hasil checksum */
    uint8_t sum = 0;
    
    /* Tambahkan status button ke checksum */
    sum += data->button_status;
    /* Tambahkan byte tinggi encoder ke checksum */
    sum += (data->encoder_count >> 8) & 0xFF;
    /* Tambahkan byte rendah encoder ke checksum */
    sum += data->encoder_count & 0xFF;
    
    /* Kembalikan nilai checksum */
    return sum;
}
