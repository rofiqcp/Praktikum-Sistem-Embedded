/* Multi_02_Serial_ADC_DAC_Comm - ESP32 sebagai Penerima dan DAC Output */
/* Program ESP32 dengan FreeRTOS untuk menerima data ADC via UART dan output ke DAC */

/* Include header utama ESP32 */
#include "freertos/FreeRTOS.h"
/* Include header task FreeRTOS */
#include "freertos/task.h"
/* Include header queue FreeRTOS */
#include "freertos/queue.h"
/* Include header semaphore FreeRTOS */
#include "freertos/semphr.h"
/* Include driver UART ESP32 */
#include "driver/uart.h"
/* Include driver DAC ESP32 */
// #include "driver/dac.h"
/* Include driver GPIO */
#include "driver/gpio.h"
/* Include header untuk logging */
#include "esp_log.h"

/* Definisi tag untuk logging */
static const char *TAG = "Multi_02_ESP32";

/* Definisi pin UART */
#define UART_NUM UART_NUM_1
/* Definisi pin TX UART */
#define UART_TX_PIN GPIO_NUM_17
/* Definisi pin RX UART */
#define UART_RX_PIN GPIO_NUM_16
/* Definisi baud rate UART */
#define UART_BAUD_RATE 115200

/* Definisi channel DAC ESP32 */
#define DAC_CHANNEL DAC_CHANNEL_1  // GPIO25
/* Definisi channel DAC kedua (opsional) */
#define DAC_CHANNEL_2 DAC_CHANNEL_2  // GPIO26

/* Struktur data ADC yang diterima */
typedef struct {
    /* Nilai ADC yang diterima (0-4095) */
    uint16_t adc_value;
    /* Nilai tegangan dalam mV */
    uint16_t voltage_mv;
    /* Nomor sequence pengiriman */
    uint16_t sequence;
    /* Checksum untuk validasi */
    uint8_t checksum;
} ADC_Data_t;

/* Handle untuk queue data ADC */
QueueHandle_t adcDataQueue;
/* Handle untuk mutex UART */
SemaphoreHandle_t uartMutex;

/* Deklarasi task FreeRTOS */
void UART_Receive_Task(void *arg);
void DAC_Output_Task(void *arg);
void UART_Ack_Task(void *arg);

/* Deklarasi fungsi utility */
uint8_t calculate_checksum(ADC_Data_t *data);

/* Variabel untuk menyimpan data ADC terakhir */
static ADC_Data_t last_adc_data = {0};

/* Fungsi utama program ESP32 */
void app_main(void)
{
    /* Print informasi bahwa program dimulai */
    ESP_LOGI(TAG, "Multi_02_Serial_ADC_DAC_Comm ESP32 Started");
    
    /* Membuat queue untuk data ADC dengan kapasitas 10 item */
    adcDataQueue = xQueueCreate(10, sizeof(ADC_Data_t));
    /* Membuat mutex untuk akses UART */
    uartMutex = xSemaphoreCreateMutex();
    
    /* Konfigurasi UART */
    uart_config_t uart_config = {
        /* Set baud rate */
        .baud_rate = UART_BAUD_RATE,
        /* Set data bits 8 */
        .data_bits = UART_DATA_8_BITS,
        /* Set parity none */
        .parity    = UART_PARITY_DISABLE,
        /* Set stop bits 1 */
        .stop_bits = UART_STOP_BITS_1,
        /* Set flow control none */
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    
    /* Install driver UART */
    uart_param_config(UART_NUM, &uart_config);
    /* Set pin UART */
    uart_set_pin(UART_NUM, UART_TX_PIN, UART_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    /* Install UART driver dengan buffer RX 1024 bytes */
    uart_driver_install(UART_NUM, 1024, 0, 0, NULL, 0);
    
    /* Inisialisasi DAC */
    // dac_output_enable(DAC_CHANNEL);
    /* Set DAC channel 2 juga (opsional) */
    // dac_output_enable(DAC_CHANNEL_2);
    
    /* Matikan DAC di awal */
    // dac_output_voltage(DAC_CHANNEL, 0);
    // dac_output_voltage(DAC_CHANNEL_2, 0);
    
    /* Membuat task untuk menerima UART */
    xTaskCreate(UART_Receive_Task, "UART_Recv", 4096, NULL, 5, NULL);
    /* Membuat task untuk output DAC */
    xTaskCreate(DAC_Output_Task, "DAC_Out", 2048, NULL, 4, NULL);
    /* Membuat task untuk mengirim ACK */
    xTaskCreate(UART_Ack_Task, "UART_Ack", 2048, NULL, 3, NULL);
    
    /* Print bahwa task telah dibuat */
    ESP_LOGI(TAG, "All tasks created successfully");
}

/* Task untuk menerima data ADC via UART dari STM32 */
void UART_Receive_Task(void *arg)
{
    /* Buffer untuk menerima data UART */
    uint8_t rx_buffer[7];
    /* Variabel untuk menyimpan panjang data yang diterima */
    int len;
    /* Struktur data ADC */
    ADC_Data_t adc_data;
    /* Variabel untuk checksum yang dihitung */
    uint8_t calc_checksum;
    /* Loop tak terbatas task */
    for (;;)
    {
        /* Baca data dari UART dengan timeout */
        len = uart_read_bytes(UART_NUM, rx_buffer, 7, pdMS_TO_TICKS(1000));
        
        /* Cek apakah data diterima (7 bytes) */
        if (len == 7)
        {
            /* Cek start delimiter */
            if (rx_buffer[0] == 0xAA)
            {
                /* Ambil data ADC dari buffer */
                adc_data.adc_value = rx_buffer[1] | (rx_buffer[2] << 8);
                /* Ambil data tegangan */
                adc_data.voltage_mv = rx_buffer[3] | (rx_buffer[4] << 8);
                /* Ambil sequence number */
                adc_data.sequence = rx_buffer[5];
                /* Ambil checksum */
                adc_data.checksum = rx_buffer[6];
                
                /* Hitung checksum untuk validasi */
                calc_checksum = calculate_checksum(&adc_data);
                
                /* Validasi checksum */
                if (calc_checksum == adc_data.checksum)
                {
                    /* Kirim data ke queue */
                    xQueueSend(adcDataQueue, &adc_data, 0);
                    
                    /* Print info data diterima */
                    ESP_LOGI(TAG, "ADC Received - Value: %d, Voltage: %dmV, Seq: %d", 
                             adc_data.adc_value, adc_data.voltage_mv, adc_data.sequence);
                    
                    /* Simpan data terakhir */
                    last_adc_data = adc_data;
                }
                else
                {
                    /* Print error checksum */
                    ESP_LOGE(TAG, "Checksum mismatch! Recv: %d, Calc: %d", adc_data.checksum, calc_checksum);
                }
            }
        }
        
        /* Delay task sebentar */
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

/* Task untuk mengoutputkan nilai ADC ke DAC */
void DAC_Output_Task(void *arg)
{
    /* Variabel untuk data ADC */
    ADC_Data_t adc_data;
    /* Variabel untuk nilai DAC (0-255) */
    uint8_t dac_value;
    /* Loop tak terbatas task */
    for (;;)
    {
        /* Ambil data dari queue dengan timeout */
        if (xQueueReceive(adcDataQueue, &adc_data, portMAX_DELAY) == pdPASS)
        {
            /* Konversi nilai ADC (0-4095) ke DAC (0-255) */
            /* DAC ESP32 adalah 8-bit */
            dac_value = (adc_data.adc_value * 255) / 4095;
            
            /* Output ke DAC channel 1 */
            // dac_output_voltage(DAC_CHANNEL, dac_value);
            
            /* Output juga ke DAC channel 2 berdasarkan tegangan */
            /* Tegangan 0-3300mV dikonversi ke 0-255 */
            uint8_t dac_voltage = (adc_data.voltage_mv * 255) / 3300;
            // dac_output_voltage(DAC_CHANNEL_2, dac_voltage);
            
            /* Print info DAC output */
            ESP_LOGI(TAG, "DAC Output - Raw: %d, Voltage DAC: %d", dac_value, dac_voltage);
        }
    }
}

/* Task untuk mengirim konfirmasi (ACK) ke STM32 */
void UART_Ack_Task(void *arg)
{
    /* Buffer untuk mengirim ACK */
    uint8_t ack_buffer[4] = {0xAA, 0x55, 0xAA, 0x55};
    /* Loop tak terbatas task */
    for (;;)
    {
        /* Cek apakah ada data baru yang diterima */
        if (last_adc_data.sequence > 0)
        {
            /* Ambil mutex UART */
            if (xSemaphoreTake(uartMutex, pdMS_TO_TICKS(50)) == pdTRUE)
            {
                /* Kirim ACK ke STM32 */
                uart_write_bytes(UART_NUM, (const char *)ack_buffer, 4);
                
                /* Print info ACK sent */
                ESP_LOGI(TAG, "ACK sent to STM32 for seq %d", last_adc_data.sequence);
                
                /* Lepas mutex */
                xSemaphoreGive(uartMutex);
            }
        }
        
        /* Delay task selama 500ms */
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

/* Fungsi untuk menghitung checksum */
uint8_t calculate_checksum(ADC_Data_t *data)
{
    /* Variabel untuk menyimpan hasil checksum */
    uint8_t sum = 0;
    
    /* Tambahkan ADC value low byte */
    sum += data->adc_value & 0xFF;
    /* Tambahkan ADC value high byte */
    sum += (data->adc_value >> 8) & 0xFF;
    /* Tambahkan voltage low byte */
    sum += data->voltage_mv & 0xFF;
    /* Tambahkan voltage high byte */
    sum += (data->voltage_mv >> 8) & 0xFF;
    /* Tambahkan sequence low byte */
    sum += data->sequence & 0xFF;
    /* Tambahkan sequence high byte */
    sum += (data->sequence >> 8) & 0xFF;
    
    /* Kembalikan nilai checksum */
    return sum;
}
