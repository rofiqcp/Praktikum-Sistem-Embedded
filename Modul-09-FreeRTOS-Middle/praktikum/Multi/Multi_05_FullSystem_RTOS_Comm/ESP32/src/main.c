/* Multi_05_FullSystem_RTOS_Comm - ESP32 sebagai Penerima dan Display */
/* Program ESP32 dengan FreeRTOS untuk sistem penuh */

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
static const char *TAG = "Multi_05_ESP32";

/* Definisi pin SPI slave (untuk menerima dari STM32) */
#define SPI_SLAVE_PIN_MISO GPIO_NUM_19
#define SPI_SLAVE_PIN_MOSI GPIO_NUM_23
#define SPI_SLAVE_PIN_SCLK GPIO_NUM_18
#define SPI_SLAVE_PIN_CS   GPIO_NUM_5

/* Definisi pin LED untuk indikator */
#define LED_BUTTON_PIN GPIO_NUM_2
#define LED_ENCODER_PIN GPIO_NUM_4
#define LED_ADC_PIN    GPIO_NUM_16
#define LED_SYSTEM_PIN GPIO_NUM_17

/* Struktur data yang diterima */
typedef struct {
    /* Sequence number */
    uint16_t sequence;
    /* Status tombol */
    uint8_t button_status;
    /* Nilai encoder */
    int16_t encoder_count;
    /* Nilai encoder direction */
    int8_t encoder_dir;
    /* Nilai ADC */
    uint16_t adc_value;
    /* Tegangan (mV) */
    uint32_t voltage_mv;
    /* Checksum */
    uint8_t checksum;
} System_Data_t;

/* Handle untuk queue data */
QueueHandle_t systemDataQueue;
/* Handle untuk mutex LED */
SemaphoreHandle_t ledMutex;

/* Deklarasi task */
void SPI_Receive_Task(void *arg);
void LED_Control_Task(void *arg);
void System_Monitor_Task(void *arg);

/* Deklarasi fungsi utility */
uint8_t calculate_checksum(System_Data_t *data);

/* Variabel untuk menyimpan data terakhir */
static System_Data_t last_data = {0};

/* Fungsi utama program ESP32 */
void app_main(void)
{
    /* Print informasi bahwa program dimulai */
    ESP_LOGI(TAG, "Multi_05_FullSystem_RTOS_Comm ESP32 Started");
    
    /* Membuat queue untuk data sistem */
    systemDataQueue = xQueueCreate(10, sizeof(System_Data_t));
    /* Membuat mutex untuk akses LED */
    ledMutex = xSemaphoreCreateMutex();
    
    /* Konfigurasi pin LED sebagai output */
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL<<LED_BUTTON_PIN) | (1ULL<<LED_ENCODER_PIN) | 
                          (1ULL<<LED_ADC_PIN) | (1ULL<<LED_SYSTEM_PIN);
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);
    
    /* Matikan semua LED di awal */
    gpio_set_level(LED_BUTTON_PIN, 0);
    gpio_set_level(LED_ENCODER_PIN, 0);
    gpio_set_level(LED_ADC_PIN, 0);
    gpio_set_level(LED_SYSTEM_PIN, 0);
    
    /* Membuat task untuk menerima SPI */
    xTaskCreate(SPI_Receive_Task, "SPI_Recv", 4096, NULL, 5, NULL);
    /* Membuat task untuk kontrol LED */
    xTaskCreate(LED_Control_Task, "LED_Ctrl", 2048, NULL, 4, NULL);
    /* Membuat task untuk monitoring sistem */
    xTaskCreate(System_Monitor_Task, "Sys_Mon", 2048, NULL, 3, NULL);
    
    /* Print bahwa task telah dibuat */
    ESP_LOGI(TAG, "All tasks created successfully");
}

/* Task untuk menerima data SPI dari STM32 */
void SPI_Receive_Task(void *arg)
{
    /* Variabel untuk data */
    System_Data_t data;
    /* Variabel untuk checksum */
    uint8_t calc_checksum;
    
    /* Loop tak terbatas untuk task */
    for (;;)
    {
        /* Simulasi menerima data - dalam implementasi nyata gunakan SPI slave */
        vTaskDelay(pdMS_TO_TICKS(500));
        
        /* Print info */
        ESP_LOGI(TAG, "SPI Receive Task running");
    }
}

/* Task untuk mengontrol LED */
void LED_Control_Task(void *arg)
{
    /* Variabel untuk data */
    System_Data_t current_data;
    /* Variabel untuk data sebelumnya */
    System_Data_t last_data_local = {0};
    
    /* Loop tak terbatas untuk task */
    for (;;)
    {
        /* Cek apakah ada data di queue */
        if (xQueueReceive(systemDataQueue, &current_data, portMAX_DELAY) == pdPASS)
        {
            /* Ambil mutex untuk akses LED */
            if (xSemaphoreTake(ledMutex, pdMS_TO_TICKS(50)) == pdTRUE)
            {
                /* Kontrol LED button */
                gpio_set_level(LED_BUTTON_PIN, current_data.button_status);
                
                /* Kontrol LED encoder (blink berdasarkan direction) */
                if (current_data.encoder_dir != 0)
                {
                    gpio_set_level(LED_ENCODER_PIN, 1);
                }
                else
                {
                    gpio_set_level(LED_ENCODER_PIN, 0);
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

/* Task untuk monitoring sistem */
void System_Monitor_Task(void *arg)
{
    /* Loop tak terbatas untuk task */
    for (;;)
    {
        /* Print info sistem */
        ESP_LOGI(TAG, "System Running - Seq: %d, Enc: %d, Btn: %d", 
                 last_data.sequence, last_data.encoder_count, last_data.button_status);
        
        /* Toggle LED sistem */
        gpio_set_level(LED_SYSTEM_PIN, !gpio_get_level(LED_SYSTEM_PIN));
        
        /* Delay 1 detik */
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/* Fungsi untuk menghitung checksum */
uint8_t calculate_checksum(System_Data_t *data)
{
    /* Variabel untuk menyimpan hasil checksum */
    uint8_t sum = 0;
    
    /* Tambahkan sequence ke checksum */
    sum += (data->sequence >> 8) & 0xFF;
    sum += data->sequence & 0xFF;
    /* Tambahkan button status ke checksum */
    sum += data->button_status;
    /* Tambahkan encoder count ke checksum */
    sum += (data->encoder_count >> 8) & 0xFF;
    sum += data->encoder_count & 0xFF;
    /* Tambahkan encoder dir ke checksum */
    sum += data->encoder_dir;
    /* Tambahkan ADC value ke checksum */
    sum += (data->adc_value >> 8) & 0xFF;
    sum += data->adc_value & 0xFF;
    /* Tambahkan voltage ke checksum */
    sum += (data->voltage_mv >> 24) & 0xFF;
    sum += (data->voltage_mv >> 16) & 0xFF;
    sum += (data->voltage_mv >> 8) & 0xFF;
    sum += data->voltage_mv & 0xFF;
    
    /* Kembalikan nilai checksum */
    return sum;
}
