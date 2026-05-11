/* Multi_04_Encoder_AllPeripheral_Comm - ESP32 sebagai Penerima dan Output */
/* Program ESP32 dengan FreeRTOS untuk terima data dan kontrol output */

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
static const char *TAG = "Multi_04_ESP32";

/* Definisi pin LED untuk indikator */
#define LED_BUTTON_PIN GPIO_NUM_2
#define LED_ENCODER_UP GPIO_NUM_4
#define LED_ENCODER_DOWN GPIO_NUM_16
#define LED_ADC_PIN    GPIO_NUM_17

/* Struktur data yang diterima */
typedef struct {
    /* Status tombol */
    uint8_t button_status;
    /* Nilai encoder */
    int16_t encoder_count;
    /* Nilai ADC */
    uint16_t adc_value;
    /* Tegangan */
    uint32_t voltage_mv;
    /* Checksum */
    uint8_t checksum;
} All_Data_t;

/* Handle untuk queue data */
QueueHandle_t allDataQueue;
/* Handle untuk mutex LED */
SemaphoreHandle_t ledMutex;

/* Deklarasi task */
void Data_Receive_Task(void *arg);
void LED_Control_Task(void *arg);

/* Deklarasi fungsi utility */
uint8_t calculate_checksum(All_Data_t *data);

/* Variabel untuk menyimpan data terakhir */
static All_Data_t last_data = {0};

/* Fungsi utama program ESP32 */
void app_main(void)
{
    /* Print informasi bahwa program dimulai */
    ESP_LOGI(TAG, "Multi_04_Encoder_AllPeripheral_Comm ESP32 Started");
    
    /* Membuat queue untuk data dengan kapasitas 10 item */
    allDataQueue = xQueueCreate(10, sizeof(All_Data_t));
    /* Membuat mutex untuk akses LED */
    ledMutex = xSemaphoreCreateMutex();
    
    /* Konfigurasi pin LED sebagai output */
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL<<LED_BUTTON_PIN) | (1ULL<<LED_ENCODER_UP) | 
                          (1ULL<<LED_ENCODER_DOWN) | (1ULL<<LED_ADC_PIN);
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);
    
    /* Matikan semua LED di awal */
    gpio_set_level(LED_BUTTON_PIN, 0);
    gpio_set_level(LED_ENCODER_UP, 0);
    gpio_set_level(LED_ENCODER_DOWN, 0);
    gpio_set_level(LED_ADC_PIN, 0);
    
    /* Membuat task untuk menerima data */
    xTaskCreate(Data_Receive_Task, "Data_Recv", 2048, NULL, 5, NULL);
    /* Membuat task untuk kontrol LED */
    xTaskCreate(LED_Control_Task, "LED_Ctrl", 2048, NULL, 4, NULL);
    
    /* Print bahwa task telah dibuat */
    ESP_LOGI(TAG, "Tasks created successfully");
}

/* Task untuk menerima data */
void Data_Receive_Task(void *arg)
{
    /* Variabel untuk data */
    All_Data_t data;
    /* Variabel untuk checksum */
    uint8_t calc_checksum;
    
    /* Loop tak terbatas untuk task */
    for (;;)
    {
        /* Simulasi menerima data - dalam implementasi nyata gunakan SPI slave */
        vTaskDelay(pdMS_TO_TICKS(500));
        
        /* Generate dummy data */
        data.button_status = (gpio_get_level(LED_BUTTON_PIN) == 1) ? 1 : 0;
        data.encoder_count++;
        data.adc_value = 2048;
        data.voltage_mv = 1650;
        data.checksum = calculate_checksum(&data);
        
        /* Kirim data ke queue */
        xQueueSend(allDataQueue, &data, 0);
        
        /* Print info */
        ESP_LOGI(TAG, "Data received - Btn: %d, Enc: %d, ADC: %d", 
                 data.button_status, data.encoder_count, data.adc_value);
    }
}

/* Task untuk mengontrol LED */
void LED_Control_Task(void *arg)
{
    /* Variabel untuk data */
    All_Data_t current_data;
    /* Variabel untuk data sebelumnya */
    All_Data_t last_data_local = {0};
    
    /* Loop tak terbatas untuk task */
    for (;;)
    {
        /* Cek apakah ada data di queue */
        if (xQueueReceive(allDataQueue, &current_data, portMAX_DELAY) == pdPASS)
        {
            /* Ambil mutex untuk akses LED */
            if (xSemaphoreTake(ledMutex, pdMS_TO_TICKS(50)) == pdTRUE)
            {
                /* Kontrol LED button */
                gpio_set_level(LED_BUTTON_PIN, current_data.button_status);
                
                /* Kontrol LED encoder (blink berdasarkan nilai) */
                if (current_data.encoder_count % 2 == 0)
                {
                    gpio_set_level(LED_ENCODER_UP, 1);
                    gpio_set_level(LED_ENCODER_DOWN, 0);
                }
                else
                {
                    gpio_set_level(LED_ENCODER_UP, 0);
                    gpio_set_level(LED_ENCODER_DOWN, 1);
                }
                
                /* Kontrol LED ADC (brightness berdasarkan nilai) */
                if (current_data.adc_value > 2048)
                {
                    gpio_set_level(LED_ADC_PIN, 1);
                }
                else
                {
                    gpio_set_level(LED_ADC_PIN, 0);
                }
                
                /* Lepas mutex */
                xSemaphoreGive(ledMutex);
            }
            
            /* Update data terakhir */
            last_data = current_data;
        }
        
        /* Delay task */
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

/* Fungsi untuk menghitung checksum */
uint8_t calculate_checksum(All_Data_t *data)
{
    /* Variabel untuk menyimpan hasil checksum */
    uint8_t sum = 0;
    
    /* Tambahkan button status ke checksum */
    sum += data->button_status;
    /* Tambahkan byte tinggi encoder ke checksum */
    sum += (data->encoder_count >> 8) & 0xFF;
    /* Tambahkan byte rendah encoder ke checksum */
    sum += data->encoder_count & 0xFF;
    /* Tambahkan byte tinggi ADC ke checksum */
    sum += (data->adc_value >> 8) & 0xFF;
    /* Tambahkan byte rendah ADC ke checksum */
    sum += data->adc_value & 0xFF;
    /* Tambahkan byte tinggi voltage ke checksum */
    sum += (data->voltage_mv >> 24) & 0xFF;
    sum += (data->voltage_mv >> 16) & 0xFF;
    sum += (data->voltage_mv >> 8) & 0xFF;
    sum += data->voltage_mv & 0xFF;
    
    /* Kembalikan nilai checksum */
    return sum;
}
