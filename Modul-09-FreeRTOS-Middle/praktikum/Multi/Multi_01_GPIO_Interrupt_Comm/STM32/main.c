/* Multi_01_GPIO_Interrupt_Comm - STM32 sebagai Master */
/* Program STM32 dengan FreeRTOS untuk komunikasi SPI dengan ESP32 */

/* Include header file utama STM32 HAL */
#include "main.h"
/* Include header FreeRTOS untuk task creation dan scheduling */
#include "FreeRTOS.h"
/* Include header FreeRTOS untuk task management */
#include "task.h"
/* Include header FreeRTOS untuk queue management */
#include "queue.h"
/* Include header FreeRTOS untuk semaphore management */
#include "semphr.h"

/* Definisi pin untuk tombol button */
#define BUTTON_PIN GPIO_PIN_0
/* Definisi port untuk tombol button */
#define BUTTON_PORT GPIOA
/* Definisi pin encoder A */
#define ENCODER_A_PIN GPIO_PIN_1
/* Definisi pin encoder B */
#define ENCODER_B_PIN GPIO_PIN_2
/* Definisi port encoder */
#define ENCODER_PORT GPIOA
/* Definisi pin SPI CS (Chip Select) */
#define SPI_CS_PIN GPIO_PIN_3
/* Definisi port SPI CS */
#define SPI_CS_PORT GPIOA

/* Struktur data yang akan dikirim via SPI */
typedef struct {
    /* Status tombol button (0 = tidak ditekan, 1 = ditekan) */
    uint8_t button_status;
    /* Nilai counter encoder */
    int16_t encoder_count;
    /* Checksum untuk validasi data */
    uint8_t checksum;
} SPI_Data_t;

/* Deklarasi handle untuk queue button */
QueueHandle_t buttonQueue;
/* Deklarasi handle untuk queue encoder */
QueueHandle_t encoderQueue;
/* Deklarasi handle untuk semaphore SPI */
SemaphoreHandle_t spiMutex;

/* Deklarasi handle SPI */
SPI_HandleTypeDef hspi1;
/* Deklarasi handle GPIO */
GPIO_InitTypeDef GPIO_InitStruct = {0};

/* Variabel global untuk menyimpan status button */
volatile uint8_t button_pressed = 0;
/* Variabel global untuk menyimpan count encoder */
volatile int16_t encoder_counter = 0;
/* Variabel untuk menyimpan status pin encoder A sebelumnya */
volatile uint8_t last_encoder_A = 0;

/* Prototipe fungsi untuk inisialisasi SPI */
void MX_SPI1_Init(void);
/* Prototipe fungsi untuk inisialisasi GPIO */
void MX_GPIO_Init(void);
/* Prototipe task untuk membaca button */
void Button_Task(void *argument);
/* Prototipe task untuk membaca encoder */
void Encoder_Task(void *argument);
/* Prototipe task untuk mengirim data via SPI */
void SPI_Send_Task(void *argument);
/* Prototipe fungsi untuk menghitung checksum */
uint8_t calculate_checksum(SPI_Data_t *data);

/* Fungsi utama program */
int main(void)
{
    /* Inisialisasi HAL (Hardware Abstraction Layer) */
    HAL_Init();
    /* Konfigurasi clock sistem */
    SystemClock_Config();
    /* Inisialisasi GPIO */
    MX_GPIO_Init();
    /* Inisialisasi SPI1 */
    MX_SPI1_Init();

    /* Membuat queue untuk data button dengan kapasitas 10 item */
    buttonQueue = xQueueCreate(10, sizeof(uint8_t));
    /* Membuat queue untuk data encoder dengan kapasitas 10 item */
    encoderQueue = xQueueCreate(10, sizeof(int16_t));
    /* Membuat mutex untuk akses SPI yang aman */
    spiMutex = xSemaphoreCreateMutex();

    /* Membuat task untuk membaca button dengan prioritas normal */
    xTaskCreate(Button_Task, "ButtonTask", 128, NULL, 2, NULL);
    /* Membuat task untuk membaca encoder dengan prioritas normal */
    xTaskCreate(Encoder_Task, "EncoderTask", 128, NULL, 2, NULL);
    /* Membuat task untuk mengirim data SPI dengan prioritas tinggi */
    xTaskCreate(SPI_Send_Task, "SPISendTask", 256, NULL, 3, NULL);

    /* Memulai scheduler FreeRTOS */
    vTaskStartScheduler();

    /* Loop tak terbatas sebagai pengaman jika scheduler gagal */
    while (1)
    {
    }
}

/* Task untuk membaca status button */
void Button_Task(void *argument)
{
    /* Variabel lokal untuk menyimpan status button */
    uint8_t button_state = 0;
    /* Variabel untuk membaca status pin */
    uint8_t pin_state;

    /* Loop tak terbatas untuk task */
    for (;;)
    {
        /* Membaca status pin button */
        pin_state = HAL_GPIO_ReadPin(BUTTON_PORT, BUTTON_PIN);
        
        /* Cek apakah button ditekan (active low) */
        if (pin_state == GPIO_PIN_RESET)
        {
            /* Debouncing sederhana dengan delay 50ms */
            vTaskDelay(pdMS_TO_TICKS(50));
            /* Baca lagi untuk konfirmasi */
            pin_state = HAL_GPIO_ReadPin(BUTTON_PORT, BUTTON_PIN);
            /* Jika masih ditekan */
            if (pin_state == GPIO_PIN_RESET)
            {
                /* Set status button ke 1 (ditekan) */
                button_state = 1;
                /* Kirim data button ke queue */
                xQueueSend(buttonQueue, &button_state, 0);
                /* Tunggu hingga button dilepas */
                while (HAL_GPIO_ReadPin(BUTTON_PORT, BUTTON_PIN) == GPIO_PIN_RESET)
                {
                    vTaskDelay(pdMS_TO_TICKS(10));
                }
            }
        }
        else
        {
            /* Set status button ke 0 (tidak ditekan) */
            button_state = 0;
            /* Kirim data button ke queue */
            xQueueSend(buttonQueue, &button_state, 0);
        }
        
        /* Delay task selama 100ms */
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

/* Task untuk membaca encoder */
void Encoder_Task(void *argument)
{
    /* Variabel lokal untuk menyimpan count encoder */
    int16_t current_count;
    /* Variabel untuk status encoder A saat ini */
    uint8_t encoder_A;
    /* Variabel untuk status encoder B saat ini */
    uint8_t encoder_B;

    /* Loop tak terbatas untuk task */
    for (;;)
    {
        /* Membaca status pin encoder A */
        encoder_A = HAL_GPIO_ReadPin(ENCODER_PORT, ENCODER_A_PIN);
        /* Membaca status pin encoder B */
        encoder_B = HAL_GPIO_ReadPin(ENCODER_PORT, ENCODER_B_PIN);

        /* Cek apakah ada perubahan pada encoder A (rising edge) */
        if (encoder_A != last_encoder_A && encoder_A == 1)
        {
            /* Jika encoder B = 0, maka putar clockwise */
            if (encoder_B == 0)
            {
                /* Increment counter encoder */
                encoder_counter++;
            }
            /* Jika encoder B = 1, maka putar counterclockwise */
            else
            {
                /* Decrement counter encoder */
                encoder_counter--;
            }
        }

        /* Simpan status encoder A untuk perbandingan berikutnya */
        last_encoder_A = encoder_A;
        /* Salin nilai counter saat ini */
        current_count = encoder_counter;
        /* Kirim data encoder ke queue */
        xQueueSend(encoderQueue, &current_count, 0);
        
        /* Delay task selama 10ms untuk pembacaan yang cukup cepat */
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

/* Task untuk mengirim data via SPI ke ESP32 */
void SPI_Send_Task(void *argument)
{
    /* Variabel untuk data button */
    uint8_t button_data;
    /* Variabel untuk data encoder */
    int16_t encoder_data;
    /* Struktur data SPI yang akan dikirim */
    SPI_Data_t spi_data;
    /* Buffer untuk mengirim data via SPI */
    uint8_t tx_buffer[4];
    /* Variabel untuk status pengiriman */
    HAL_StatusTypeDef status;

    /* Loop tak terbatas untuk task */
    for (;;)
    {
        /* Cek apakah ada data button di queue */
        if (xQueueReceive(buttonQueue, &button_data, 0) == pdPASS)
        {
            /* Isi data button ke struktur SPI */
            spi_data.button_status = button_data;
        }
        
        /* Cek apakah ada data encoder di queue */
        if (xQueueReceive(encoderQueue, &encoder_data, 0) == pdPASS)
        {
            /* Isi data encoder ke struktur SPI */
            spi_data.encoder_count = encoder_data;
        }

        /* Hitung checksum untuk validasi data */
        spi_data.checksum = calculate_checksum(&spi_data);

        /* Ambil mutex SPI untuk akses eksklusif */
        if (xSemaphoreTake(spiMutex, pdMS_TO_TICKS(100)) == pdTRUE)
        {
            /* Isi buffer transmisi dengan data */
            tx_buffer[0] = spi_data.button_status;
            tx_buffer[1] = (spi_data.encoder_count >> 8) & 0xFF;
            tx_buffer[2] = spi_data.encoder_count & 0xFF;
            tx_buffer[3] = spi_data.checksum;

            /* Set pin CS ke LOW untuk memulai transmisi */
            HAL_GPIO_WritePin(SPI_CS_PORT, SPI_CS_PIN, GPIO_PIN_RESET);
            
            /* Delay singkat untuk stabilitas */
            vTaskDelay(pdMS_TO_TICKS(1));
            
            /* Kirim data via SPI */
            status = HAL_SPI_Transmit(&hspi1, tx_buffer, 4, 100);
            
            /* Delay singkat untuk stabilitas */
            vTaskDelay(pdMS_TO_TICKS(1));
            
            /* Set pin CS ke HIGH untuk mengakhiri transmisi */
            HAL_GPIO_WritePin(SPI_CS_PORT, SPI_CS_PIN, GPIO_PIN_SET);
            
            /* Lepas mutex SPI */
            xSemaphoreGive(spiMutex);
        }

        /* Delay task selama 500ms sebelum mengirim data berikutnya */
        vTaskDelay(pdMS_TO_TICKS(500));
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

/* Fungsi inisialisasi SPI1 */
void MX_SPI1_Init(void)
{
    /* Enable clock untuk SPI1 */
    __HAL_RCC_SPI1_CLK_ENABLE();
    
    /* Konfigurasi SPI1 sebagai master */
    hspi1.Instance = SPI1;
    /* Set mode master */
    hspi1.Init.Mode = SPI_MODE_MASTER;
    /* Set ukuran data 8 bit */
    hspi1.Init.Direction = SPI_DIRECTION_2LINES;
    /* Set clock prescaler */
    hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
    /* Set clock polarity low */
    hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
    /* Set clock phase 1 edge */
    hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
    /* Set NSS software */
    hspi1.Init.NSS = SPI_NSS_SOFT;
    /* Set baud rate prescaler */
    hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
    /* Set MSB first */
    hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
    /* Set TI mode disabled */
    hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
    /* Set CRC disabled */
    hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    
    /* Inisialisasi SPI1 dengan konfigurasi */
    HAL_SPI_Init(&hspi1);
}

/* Fungsi inisialisasi GPIO */
void MX_GPIO_Init(void)
{
    /* Enable clock untuk GPIOA */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    
    /* Konfigurasi pin button sebagai input dengan pull-up */
    GPIO_InitStruct.Pin = BUTTON_PIN;
    /* Set mode input */
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    /* Set pull-up */
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    /* Set speed */
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    /* Inisialisasi pin button */
    HAL_GPIO_Init(BUTTON_PORT, &GPIO_InitStruct);
    
    /* Konfigurasi pin encoder A sebagai input dengan pull-up */
    GPIO_InitStruct.Pin = ENCODER_A_PIN;
    /* Set mode input */
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    /* Set pull-up */
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    /* Inisialisasi pin encoder A */
    HAL_GPIO_Init(ENCODER_PORT, &GPIO_InitStruct);
    
    /* Konfigurasi pin encoder B sebagai input dengan pull-up */
    GPIO_InitStruct.Pin = ENCODER_B_PIN;
    /* Set mode input */
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    /* Set pull-up */
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    /* Inisialisasi pin encoder B */
    HAL_GPIO_Init(ENCODER_PORT, &GPIO_InitStruct);
    
    /* Konfigurasi pin SPI CS sebagai output */
    GPIO_InitStruct.Pin = SPI_CS_PIN;
    /* Set mode output */
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    /* Set pull-up */
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    /* Set speed tinggi */
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    /* Inisialisasi pin CS */
    HAL_GPIO_Init(SPI_CS_PORT, &GPIO_InitStruct);
    
    /* Set pin CS ke HIGH secara default (tidak aktif) */
    HAL_GPIO_WritePin(SPI_CS_PORT, SPI_CS_PIN, GPIO_PIN_SET);
}

/* Fungsi konfigurasi clock sistem */
void SystemClock_Config(void)
{
    /* Variabel untuk konfigurasi clock */
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    /* Variabel untuk konfigurasi clock bus */
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    
    /* Enable HSI oscillator */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    /* Set HSI state enable */
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    /* Set HSI calibration */
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    /* Set PLL state disable */
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
    /* Inisialisasi oscillator */
    HAL_RCC_OscConfig(&RCC_OscInitStruct);
    
    /* Konfigurasi clock bus */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
    /* Set source clock SYSCLK */
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
    /* Set divisi untuk HCLK */
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    /* Set divisi untuk PCLK1 */
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    /* Set divisi untuk PCLK2 */
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    /* Inisialisasi clock bus */
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0);
}
