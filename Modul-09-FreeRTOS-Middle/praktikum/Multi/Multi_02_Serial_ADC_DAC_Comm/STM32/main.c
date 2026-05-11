/* Multi_02_Serial_ADC_DAC_Comm - STM32 sebagai Pengirim ADC */
/* Program STM32 dengan FreeRTOS untuk membaca ADC dan kirim via UART ke ESP32 */

/* Include header file utama STM32 HAL */
#include "main.h"
/* Include header FreeRTOS untuk task creation */
#include "FreeRTOS.h"
/* Include header FreeRTOS untuk task management */
#include "task.h"
/* Include header FreeRTOS untuk queue management */
#include "queue.h"
/* Include header FreeRTOS untuk semaphore */
#include "semphr.h"

/* Definisi pin ADC */
#define ADC_CHANNEL ADC_CHANNEL_0
/* Definisi UART handle */
UART_HandleTypeDef huart2;
/* Definisi ADC handle */
ADC_HandleTypeDef hadc1;

/* Struktur data ADC untuk dikirim via UART */
typedef struct {
    /* Nilai ADC yang dibaca (0-4095 untuk 12-bit) */
    uint16_t adc_value;
    /* Nilai tegangan dalam mV */
    uint16_t voltage_mv;
    /* Counter sequence pengiriman */
    uint16_t sequence;
    /* Checksum untuk validasi */
    uint8_t checksum;
} ADC_Data_t;

/* Handle untuk queue data ADC */
QueueHandle_t adcQueue;
/* Handle untuk mutex UART */
SemaphoreHandle_t uartMutex;
/* Handle untuk semaphore konfirmasi dari ESP32 */
SemaphoreHandle_t ackSemaphore;

/* Variabel global untuk sequence counter */
volatile uint16_t seq_counter = 0;

/* Prototipe fungsi inisialisasi */
void MX_USART2_UART_Init(void);
void MX_ADC1_Init(void);
void MX_GPIO_Init(void);

/* Prototipe task FreeRTOS */
void ADC_Read_Task(void *argument);
void UART_Send_Task(void *argument);
void UART_Receive_Task(void *argument);

/* Prototipe fungsi utility */
uint8_t calculate_checksum(ADC_Data_t *data);
void SystemClock_Config(void);

/* Fungsi utama program */
int main(void)
{
    /* Inisialisasi HAL */
    HAL_Init();
    /* Konfigurasi clock sistem */
    SystemClock_Config();
    /* Inisialisasi GPIO */
    MX_GPIO_Init();
    /* Inisialisasi UART2 */
    MX_USART2_UART_Init();
    /* Inisialisasi ADC1 */
    MX_ADC1_Init();

    /* Membuat queue untuk data ADC dengan kapasitas 10 item */
    adcQueue = xQueueCreate(10, sizeof(ADC_Data_t));
    /* Membuat mutex untuk akses UART */
    uartMutex = xSemaphoreCreateMutex();
    /* Membuat semaphore biner untuk konfirmasi */
    ackSemaphore = xSemaphoreCreateBinary();

    /* Membuat task untuk membaca ADC */
    xTaskCreate(ADC_Read_Task, "ADC_Read", 256, NULL, 3, NULL);
    /* Membuat task untuk mengirim data UART */
    xTaskCreate(UART_Send_Task, "UART_Send", 256, NULL, 2, NULL);
    /* Membuat task untuk menerima konfirmasi */
    xTaskCreate(UART_Receive_Task, "UART_Recv", 256, NULL, 2, NULL);

    /* Memulai scheduler FreeRTOS */
    vTaskStartScheduler();

    /* Loop tak terbatas sebagai pengaman */
    while (1)
    {
    }
}

/* Task untuk membaca nilai ADC */
void ADC_Read_Task(void *argument)
{
    /* Variabel untuk menyimpan data ADC */
    ADC_Data_t adc_data;
    /* Variabel untuk konversi tegangan */
    uint32_t voltage;
    /* Status konversi ADC */
    HAL_StatusTypeDef status;

    /* Loop tak terbatas task */
    for (;;)
    {
        /* Mulai konversi ADC */
        status = HAL_ADC_Start(&hadc1);
        
        /* Cek apakah start ADC berhasil */
        if (status == HAL_OK)
        {
            /* Tunggu konversi selesai dengan timeout 100ms */
            status = HAL_ADC_PollForConversion(&hadc1, 100);
            
            /* Cek apakah konversi selesai */
            if (status == HAL_OK)
            {
                /* Baca nilai ADC hasil konversi */
                adc_data.adc_value = HAL_ADC_GetValue(&hadc1);
                
                /* Hitung tegangan dalam mV (asumsi VREF = 3300mV) */
                voltage = (adc_data.adc_value * 3300) / 4095;
                /* Simpan nilai tegangan */
                adc_data.voltage_mv = (uint16_t)voltage;
                
                /* Increment sequence counter */
                seq_counter++;
                /* Simpan nomor sequence */
                adc_data.sequence = seq_counter;
                
                /* Hitung checksum */
                adc_data.checksum = calculate_checksum(&adc_data);
                
                /* Kirim data ke queue */
                xQueueSend(adcQueue, &adc_data, 0);
                
                /* Print info ke debug (opsional) */
                printf("ADC Read: %d, Voltage: %dmV\n", adc_data.adc_value, adc_data.voltage_mv);
            }
            
            /* Stop ADC conversion */
            HAL_ADC_Stop(&hadc1);
        }
        
        /* Delay task selama 500ms */
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

/* Task untuk mengirim data ADC via UART ke ESP32 */
void UART_Send_Task(void *argument)
{
    /* Variabel untuk data ADC */
    ADC_Data_t adc_data;
    /* Buffer untuk pengiriman UART */
    uint8_t tx_buffer[7];
    /* Status pengiriman UART */
    HAL_StatusTypeDef status;

    /* Loop tak terbatas task */
    for (;;)
    {
        /* Ambil data dari queue dengan timeout */
        if (xQueueReceive(adcQueue, &adc_data, portMAX_DELAY) == pdPASS)
        {
            /* Ambil mutex UART untuk akses eksklusif */
            if (xSemaphoreTake(uartMutex, pdMS_TO_TICKS(100)) == pdTRUE)
            {
                /* Susun data ke buffer transmisi */
                /* Byte 0: Start delimiter */
                tx_buffer[0] = 0xAA;
                /* Byte 1: ADC value low byte */
                tx_buffer[1] = adc_data.adc_value & 0xFF;
                /* Byte 2: ADC value high byte */
                tx_buffer[2] = (adc_data.adc_value >> 8) & 0xFF;
                /* Byte 3: Voltage low byte */
                tx_buffer[3] = adc_data.voltage_mv & 0xFF;
                /* Byte 4: Voltage high byte */
                tx_buffer[4] = (adc_data.voltage_mv >> 8) & 0xFF;
                /* Byte 5: Sequence number low byte */
                tx_buffer[5] = adc_data.sequence & 0xFF;
                /* Byte 6: Checksum */
                tx_buffer[6] = adc_data.checksum;
                
                /* Kirim data via UART */
                status = HAL_UART_Transmit(&huart2, tx_buffer, 7, 1000);
                
                /* Cek status pengiriman */
                if (status == HAL_OK)
                {
                    /* Print info pengiriman */
                    printf("Sent ADC data seq %d\n", adc_data.sequence);
                }
                
                /* Lepas mutex UART */
                xSemaphoreGive(uartMutex);
            }
        }
    }
}

/* Task untuk menerima konfirmasi dari ESP32 */
void UART_Receive_Task(void *argument)
{
    /* Buffer untuk menerima data */
    uint8_t rx_buffer[4];
    /* Status penerimaan UART */
    HAL_StatusTypeDef status;
    /* Variabel untuk menyimpan byte yang diterima */
    uint8_t received_byte;

    /* Loop tak terbatas task */
    for (;;)
    {
        /* Terima data dari ESP32 dengan timeout */
        status = HAL_UART_Receive(&huart2, rx_buffer, 4, 1000);
        
        /* Cek apakah penerimaan berhasil */
        if (status == HAL_OK)
        {
            /* Cek apakah ini konfirmasi yang valid (0xAA 0x55 0xAA 0x55) */
            if (rx_buffer[0] == 0xAA && rx_buffer[1] == 0x55 && 
                rx_buffer[2] == 0xAA && rx_buffer[3] == 0x55)
            {
                /* Print konfirmasi diterima */
                printf("ACK received from ESP32\n");
                
                /* Beri sinyal ke semaphore bahwa ACK diterima */
                xSemaphoreGive(ackSemaphore);
            }
        }
        
        /* Delay task selama 100ms */
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

/* Fungsi untuk menghitung checksum */
uint8_t calculate_checksum(ADC_Data_t *data)
{
    /* Variabel untuk menyimpan hasil checksum */
    uint8_t sum = 0;
    
    /* Tambahkan ADC value low byte ke checksum */
    sum += data->adc_value & 0xFF;
    /* Tambahkan ADC value high byte ke checksum */
    sum += (data->adc_value >> 8) & 0xFF;
    /* Tambahkan voltage low byte ke checksum */
    sum += data->voltage_mv & 0xFF;
    /* Tambahkan voltage high byte ke checksum */
    sum += (data->voltage_mv >> 8) & 0xFF;
    /* Tambahkan sequence low byte ke checksum */
    sum += data->sequence & 0xFF;
    /* Tambahkan sequence high byte ke checksum */
    sum += (data->sequence >> 8) & 0xFF;
    
    /* Kembalikan nilai checksum */
    return sum;
}

/* Inisialisasi UART2 */
void MX_USART2_UART_Init(void)
{
    /* Enable clock untuk USART2 */
    __HAL_RCC_USART2_CLK_ENABLE();
    
    /* Konfigurasi UART2 */
    huart2.Instance = USART2;
    /* Set baud rate 115200 */
    huart2.Init.BaudRate = 115200;
    /* Set word length 8 bit */
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    /* Set stop bits 1 */
    huart2.Init.StopBits = UART_STOPBITS_1;
    /* Set parity none */
    huart2.Init.Parity = UART_PARITY_NONE;
    /* Set mode TX dan RX */
    huart2.Init.Mode = UART_MODE_TX_RX;
    /* Set hardware flow control none */
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    /* Set oversampling 16 */
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    
    /* Inisialisasi UART2 */
    HAL_UART_Init(&huart2);
}

/* Inisialisasi ADC1 */
void MX_ADC1_Init(void)
{
    /* Enable clock untuk ADC1 */
    __HAL_RCC_ADC1_CLK_ENABLE();
    
    /* Konfigurasi ADC1 */
    hadc1.Instance = ADC1;
    /* Set resolusi 12 bit */
    hadc1.Init.Resolution = ADC_RESOLUTION_12B;
    /* Set data alignment right */
    hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    /* Set scan mode disable */
    hadc1.Init.ScanConvMode = DISABLE;
    /* Set continuous conversion disable */
    hadc1.Init.ContinuousConvMode = DISABLE;
    /* Set discontinuous conversion disable */
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    /* Set external trigger none (software trigger) */
    hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    /* Set DMA continuous requests disable */
    hadc1.Init.DMAContinuousRequests = DISABLE;
    /* Set EOC selection end of single conversion */
    hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
    
    /* Inisialisasi ADC1 */
    HAL_ADC_Init(&hadc1);
    
    /* Konfigurasi channel ADC */
    ADC_ChannelConfTypeDef sConfig = {0};
    /* Set channel ADC0 */
    sConfig.Channel = ADC_CHANNEL_0;
    /* Set rank 1 */
    sConfig.Rank = 1;
    /* Set sampling time 84 cycles */
    sConfig.SamplingTime = ADC_SAMPLETIME_84CYCLES;
    
    /* Konfigurasi channel ADC */
    HAL_ADC_ConfigChannel(&hadc1, &sConfig);
}

/* Inisialisasi GPIO */
void MX_GPIO_Init(void)
{
    /* Enable clock GPIO */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
}

/* Konfigurasi clock sistem */
void SystemClock_Config(void)
{
    /* Variabel konfigurasi oscillator */
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    /* Variabel konfigurasi clock */
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    
    /* Konfigurasi HSI */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    /* Enable HSI */
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    /* Set kalibrasi HSI default */
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    /* Disable PLL */
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
    /* Inisialisasi oscillator */
    HAL_RCC_OscConfig(&RCC_OscInitStruct);
    
    /* Konfigurasi clock bus */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
    /* Set sumber clock SYSCLK */
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
    /* Set divider AHB */
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    /* Set divider APB1 */
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    /* Set divider APB2 */
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    /* Inisialisasi clock */
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0);
}
