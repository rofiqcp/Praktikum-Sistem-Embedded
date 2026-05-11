/* Multi_04_Encoder_AllPeripheral_Comm - STM32 dengan Encoder, ADC, dan SPI */
/* Program STM32 dengan FreeRTOS untuk baca encoder interrupt, ADC, kirim via SPI */

#ifdef STM32F1
#include "stm32f1xx_hal.h"
#else
#include "stm32f4xx_hal.h"
#endif
#include "config.h"
#include "FreeRTOS.h"
/* Include header FreeRTOS untuk task management */
#include "task.h"
/* Include header FreeRTOS untuk queue management */
#include "queue.h"
/* Include header FreeRTOS untuk semaphore */
#include "semphr.h"

/* Definisi pin encoder A (interrupt) */
#define ENCODER_A_PIN GPIO_PIN_0
#define ENCODER_A_PORT GPIOA
/* Definisi pin encoder B */
#define ENCODER_B_PIN GPIO_PIN_1
#define ENCODER_B_PORT GPIOA
/* Definisi pin ADC */
#define ADC_CHANNEL ADC_CHANNEL_1
/* Definisi pin SPI CS */
#define SPI_CS_PIN GPIO_PIN_3
#define SPI_CS_PORT GPIOA

/* Definisi handle */
ADC_HandleTypeDef hadc1;
SPI_HandleTypeDef hspi1;

/* Struktur data lengkap untuk dikirim via SPI */
typedef struct {
    /* Nilai counter encoder */
    int16_t encoder_count;
    /* Status arah encoder (1=CW, -1=CCW, 0=stop) */
    int8_t encoder_dir;
    /* Nilai ADC yang dibaca */
    uint16_t adc_value;
    /* Tegangan ADC dalam mV */
    uint16_t voltage_mv;
    /* Status button (opsional) */
    uint8_t button_status;
    /* Checksum untuk validasi */
    uint8_t checksum;
} AllData_t;

/* Handle untuk queue encoder */
QueueHandle_t encoderQueue;
/* Handle untuk queue ADC */
QueueHandle_t adcQueue;
/* Handle untuk mutex SPI */
SemaphoreHandle_t spiMutex;

/* Variabel global untuk encoder */
volatile int16_t g_encoder_count = 0;
volatile int8_t g_encoder_dir = 0;
volatile uint8_t g_last_encoder_A = 0;
volatile uint8_t g_button_pressed = 0;

/* Prototipe fungsi inisialisasi */
void MX_ADC1_Init(void);
void MX_SPI1_Init(void);
void MX_GPIO_Init(void);

/* Prototipe task FreeRTOS */
void Encoder_Interrupt_Task(void *argument);
void ADC_Read_Task(void *argument);
void SPI_Send_AllData_Task(void *argument);
void Button_Task(void *argument);

/* Prototipe fungsi utility */
uint8_t calculate_checksum(AllData_t *data);
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin);
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
    /* Inisialisasi ADC1 */
    MX_ADC1_Init();
    /* Inisialisasi SPI1 */
    MX_SPI1_Init();

    /* Membuat queue untuk encoder dengan kapasitas 10 item */
    encoderQueue = xQueueCreate(10, sizeof(int16_t));
    /* Membuat queue untuk ADC dengan kapasitas 10 item */
    adcQueue = xQueueCreate(10, sizeof(uint16_t));
    /* Membuat mutex untuk akses SPI */
    spiMutex = xSemaphoreCreateMutex();

    /* Membuat task untuk interrupt encoder (dipanggil dari callback) */
    xTaskCreate(Encoder_Interrupt_Task, "Enc_Task", 256, NULL, 4, NULL);
    /* Membuat task untuk baca ADC */
    xTaskCreate(ADC_Read_Task, "ADC_Task", 256, NULL, 3, NULL);
    /* Membuat task untuk kirim data SPI */
    xTaskCreate(SPI_Send_AllData_Task, "SPI_Send", 256, NULL, 2, NULL);
    /* Membuat task untuk button */
    xTaskCreate(Button_Task, "Btn_Task", 128, NULL, 2, NULL);

    /* Memulai scheduler FreeRTOS */
    vTaskStartScheduler();

    /* Loop tak terbatas sebagai pengaman */
    while (1)
    {
    }
}

/* Callback untuk interrupt GPIO (encoder dan button) */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    /* Cek apakah interrupt dari pin encoder A */
    if (GPIO_Pin == ENCODER_A_PIN)
    {
        /* Baca status pin encoder A */
        uint8_t encoder_A = HAL_GPIO_ReadPin(ENCODER_A_PORT, ENCODER_A_PIN);
        /* Baca status pin encoder B */
        uint8_t encoder_B = HAL_GPIO_ReadPin(ENCODER_B_PORT, ENCODER_B_PIN);
        
        /* Cek rising edge pada encoder A */
        if (encoder_A == 1 && g_last_encoder_A == 0)
        {
            /* Tentukan arah berdasarkan encoder B */
            if (encoder_B == 0)
            {
                /* Rotasi clockwise - increment */
                g_encoder_count++;
                g_encoder_dir = 1;
            }
            else
            {
                /* Rotasi counterclockwise - decrement */
                g_encoder_count--;
                g_encoder_dir = -1;
            }
        }
        
        /* Simpan status encoder A */
        g_last_encoder_A = encoder_A;
    }
    
    /* Cek apakah interrupt dari button */
    if (GPIO_Pin == GPIO_PIN_4)
    {
        /* Toggle status button */
        g_button_pressed = !g_button_pressed;
    }
}

/* Task untuk menangani data encoder dari interrupt */
void Encoder_Interrupt_Task(void *argument)
{
    /* Variabel lokal untuk encoder count */
    int16_t current_count;
    /* Loop tak terbatas task */
    for (;;)
    {
        /* Ambil nilai encoder global */
        current_count = g_encoder_count;
        
        /* Kirim ke queue */
        xQueueSend(encoderQueue, &current_count, 0);
        
        /* Delay untuk debouncing dan pengiriman periodik */
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

/* Task untuk membaca ADC */
void ADC_Read_Task(void *argument)
{
    /* Variabel untuk nilai ADC */
    uint16_t adc_value;
    /* Variabel untuk tegangan */
    uint32_t voltage;
    /* Status ADC */
    HAL_StatusTypeDef status;
    
    /* Loop tak terbatas task */
    for (;;)
    {
        /* Mulai konversi ADC */
        status = HAL_ADC_Start(&hadc1);
        
        /* Cek apakah start berhasil */
        if (status == HAL_OK)
        {
            /* Tunggu konversi selesai */
            status = HAL_ADC_PollForConversion(&hadc1, 100);
            
            /* Cek apakah konversi selesai */
            if (status == HAL_OK)
            {
                /* Baca nilai ADC */
                adc_value = HAL_ADC_GetValue(&hadc1);
                
                /* Kirim ke queue */
                xQueueSend(adcQueue, &adc_value, 0);
                
                /* Print info (opsional) */
// printf("ADC: %d\n", adc_value);
            }
            
            /* Stop ADC */
            HAL_ADC_Stop(&hadc1);
        }
        
        /* Delay task selama 500ms */
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

/* Task untuk mengirim semua data via SPI ke ESP32 */
void SPI_Send_AllData_Task(void *argument)
{
    /* Variabel untuk data encoder */
    int16_t encoder_data;
    /* Variabel untuk data ADC */
    uint16_t adc_data;
    /* Struktur data lengkap */
    AllData_t all_data;
    /* Buffer transmisi SPI */
    uint8_t tx_buffer[10];
    /* Status SPI */
    HAL_StatusTypeDef status;
    
    /* Loop tak terbatas task */
    for (;;)
    {
        /* Ambil data encoder dari queue */
        if (xQueueReceive(encoderQueue, &encoder_data, 0) == pdPASS)
        {
            /* Isi data encoder */
            all_data.encoder_count = encoder_data;
            all_data.encoder_dir = g_encoder_dir;
        }
        
        /* Ambil data ADC dari queue */
        if (xQueueReceive(adcQueue, &adc_data, 0) == pdPASS)
        {
            /* Isi data ADC */
            all_data.adc_value = adc_data;
            /* Hitung tegangan */
            all_data.voltage_mv = (adc_data * 3300) / 4095;
        }
        
        /* Isi status button */
        all_data.button_status = g_button_pressed;
        
        /* Hitung checksum */
        all_data.checksum = calculate_checksum(&all_data);
        
        /* Ambil mutex SPI */
        if (xSemaphoreTake(spiMutex, pdMS_TO_TICKS(100)) == pdTRUE)
        {
            /* Susun buffer transmisi */
            /* Byte 0-1: Encoder count */
            tx_buffer[0] = (all_data.encoder_count >> 8) & 0xFF;
            tx_buffer[1] = all_data.encoder_count & 0xFF;
            /* Byte 2: Encoder direction */
            tx_buffer[2] = (uint8_t)all_data.encoder_dir;
            /* Byte 3-4: ADC value */
            tx_buffer[3] = (all_data.adc_value >> 8) & 0xFF;
            tx_buffer[4] = all_data.adc_value & 0xFF;
            /* Byte 5-6: Voltage */
            tx_buffer[5] = (all_data.voltage_mv >> 8) & 0xFF;
            tx_buffer[6] = all_data.voltage_mv & 0xFF;
            /* Byte 7: Button status */
            tx_buffer[7] = all_data.button_status;
            /* Byte 8: Checksum */
            tx_buffer[8] = all_data.checksum;
            
            /* Set CS LOW */
            HAL_GPIO_WritePin(SPI_CS_PORT, SPI_CS_PIN, GPIO_PIN_RESET);
            /* Delay singkat */
            vTaskDelay(pdMS_TO_TICKS(1));
            /* Kirim via SPI */
            status = HAL_SPI_Transmit(&hspi1, tx_buffer, 9, 100);
            /* Delay singkat */
            vTaskDelay(pdMS_TO_TICKS(1));
            /* Set CS HIGH */
            HAL_GPIO_WritePin(SPI_CS_PORT, SPI_CS_PIN, GPIO_PIN_SET);
            
            /* Cek status */
            if (status == HAL_OK)
            {
                /* Print info - commented out */
            }
            
            /* Lepas mutex */
            xSemaphoreGive(spiMutex);
        }
        
        /* Delay task selama 500ms */
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

/* Task untuk button (opsional) */
void Button_Task(void *argument)
{
    /* Loop tak terbatas task */
    for (;;)
    {
        /* Baca status button (jika menggunakan polling selain interrupt) */
        /* Dalam kasus ini kita sudah menggunakan interrupt */
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

/* Fungsi untuk menghitung checksum */
uint8_t calculate_checksum(AllData_t *data)
{
    /* Variabel untuk checksum */
    uint8_t sum = 0;
    
    /* Tambahkan encoder count */
    sum += (data->encoder_count >> 8) & 0xFF;
    sum += data->encoder_count & 0xFF;
    /* Tambahkan encoder dir */
    sum += (uint8_t)data->encoder_dir;
    /* Tambahkan ADC value */
    sum += (data->adc_value >> 8) & 0xFF;
    sum += data->adc_value & 0xFF;
    /* Tambahkan voltage */
    sum += (data->voltage_mv >> 8) & 0xFF;
    sum += data->voltage_mv & 0xFF;
    /* Tambahkan button status */
    sum += data->button_status;
    
    /* Kembalikan checksum */
    return sum;
}

/* Inisialisasi ADC1 */
void MX_ADC1_Init(void)
{
    /* Enable clock ADC1 */
    __HAL_RCC_ADC1_CLK_ENABLE();
    
    /* Konfigurasi ADC1 */
    hadc1.Instance = ADC1;
    /* Set alignment right */
    hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    /* Set scan mode disable */
    hadc1.Init.ScanConvMode = DISABLE;
    /* Set continuous convert disable */
    hadc1.Init.ContinuousConvMode = DISABLE;
    /* Set external trigger software */
    hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    
#if defined(STM32F401xC) || defined(STM32F411xE)
    /* F4-specific: Set resolusi 12-bit */
    hadc1.Init.Resolution = ADC_RESOLUTION_12B;
    /* F4-specific: Set EOC selection */
    hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
#endif
    
    /* Inisialisasi ADC1 */
    HAL_ADC_Init(&hadc1);
    
    /* Konfigurasi channel ADC */
    ADC_ChannelConfTypeDef sConfig = {0};
    /* Set channel */
    sConfig.Channel = ADC_CHANNEL_1;
    /* Set rank */
    sConfig.Rank = 1;
    /* Set sampling time */
#if defined(STM32F401xC) || defined(STM32F411xE)
    /* F4: 84 cycles */
    sConfig.SamplingTime = ADC_SAMPLETIME_84CYCLES;
#else
    /* F1: 55.5 cycles */
    sConfig.SamplingTime = ADC_SAMPLETIME_55CYCLES_5;
#endif
    /* Konfigurasi channel */
    HAL_ADC_ConfigChannel(&hadc1, &sConfig);
}

/* Inisialisasi SPI1 */
void MX_SPI1_Init(void)
{
    /* Enable clock SPI1 */
    __HAL_RCC_SPI1_CLK_ENABLE();
    
    /* Konfigurasi SPI1 */
    hspi1.Instance = SPI1;
    /* Set mode master */
    hspi1.Init.Mode = SPI_MODE_MASTER;
    /* Set direction 2 lines */
    hspi1.Init.Direction = SPI_DIRECTION_2LINES;
    /* Set data size 8-bit */
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
    /* Set TI mode disable */
    hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
    /* Set CRC disable */
    hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    
    /* Inisialisasi SPI1 */
    HAL_SPI_Init(&hspi1);
}

/* Inisialisasi GPIO */
void MX_GPIO_Init(void)
{
    /* Enable clock GPIO */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    
    /* Konfigurasi pin encoder A sebagai interrupt falling/rising */
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    /* Set pin encoder A */
    GPIO_InitStruct.Pin = ENCODER_A_PIN;
    /* Set mode interrupt rising falling */
    GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
    /* Set pull-up */
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    /* Inisialisasi pin */
    HAL_GPIO_Init(ENCODER_A_PORT, &GPIO_InitStruct);
    
    /* Konfigurasi pin encoder B sebagai input */
    GPIO_InitStruct.Pin = ENCODER_B_PIN;
    /* Set mode input */
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    /* Set pull-up */
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    /* Inisialisasi pin */
    HAL_GPIO_Init(ENCODER_B_PORT, &GPIO_InitStruct);
    
    /* Konfigurasi pin CS sebagai output */
    GPIO_InitStruct.Pin = SPI_CS_PIN;
    /* Set mode output */
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    /* Set no pull */
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    /* Set speed high */
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    /* Inisialisasi pin */
    HAL_GPIO_Init(SPI_CS_PORT, &GPIO_InitStruct);
    
    /* Set CS HIGH (tidak aktif) */
    HAL_GPIO_WritePin(SPI_CS_PORT, SPI_CS_PIN, GPIO_PIN_SET);
    
    /* Enable interrupt EXTI untuk encoder A */
    HAL_NVIC_SetPriority(EXTI0_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(EXTI0_IRQn);
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
    /* Set kalibrasi default */
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    /* Disable PLL */
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
    /* Inisialisasi oscillator */
    HAL_RCC_OscConfig(&RCC_OscInitStruct);
    
    /* Konfigurasi clock bus */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
    /* Set sumber clock */
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
