/* Multi_05_FullSystem_RTOS_Comm - STM32 Sistem Lengkap dengan RTOS */
/* Program STM32 dengan FreeRTOS untuk semua peripheral dan komunikasi SPI */

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
/* Include header FreeRTOS untuk event group */
#include "event_groups.h"

/* Definisi pin GPIO untuk LED indikator */
#define LED1_PIN GPIO_PIN_12
#define LED1_PORT GPIOB
#define LED2_PIN GPIO_PIN_13
#define LED2_PORT GPIOB

/* Definisi pin button dengan interrupt */
#define BUTTON_PIN GPIO_PIN_0
#define BUTTON_PORT GPIOA

/* Definisi pin encoder A dan B dengan interrupt */
#define ENCODER_A_PIN GPIO_PIN_1
#define ENCODER_A_PORT GPIOA
#define ENCODER_B_PIN GPIO_PIN_2
#define ENCODER_B_PORT GPIOA

/* Definisi pin ADC */
#define ADC_CHANNEL ADC_CHANNEL_3

/* Definisi alamat I2C sensor BMP280 */
#define I2C_SENSOR_ADDR 0x76

/* Definisi pin SPI CS */
#define SPI_CS_PIN GPIO_PIN_3
#define SPI_CS_PORT GPIOA

/* Definisi handle peripheral */
ADC_HandleTypeDef hadc1;
I2C_HandleTypeDef hi2c1;
SPI_HandleTypeDef hspi1;

/* Struktur data lengkap sistem untuk dikirim via SPI */
typedef struct {
    /* Header paket data */
    uint8_t header;
    /* Status button */
    uint8_t button_status;
    /* Counter encoder */
    int16_t encoder_count;
    /* Arah encoder */
    int8_t encoder_dir;
    /* Nilai ADC */
    uint16_t adc_value;
    /* Tegangan ADC mV */
    uint16_t voltage_mv;
    /* Data sensor I2C - suhu */
    int16_t sensor_temp;
    /* Data sensor I2C - tekanan */
    uint32_t sensor_press;
    /* Counter sequence */
    uint16_t sequence;
    /* Checksum */
    uint8_t checksum;
} FullSystem_Data_t;

/* Handle untuk queue data */
QueueHandle_t buttonQueue;
QueueHandle_t encoderQueue;
QueueHandle_t adcQueue;
QueueHandle_t sensorQueue;
QueueHandle_t spiTxQueue;

/* Handle untuk semaphore */
SemaphoreHandle_t spiMutex;
SemaphoreHandle_t i2cMutex;

/* Handle untuk event group */
EventGroupHandle_t systemEventGroup;

/* Definisi bit event */
#define BUTTON_EVENT_BIT   (1 << 0)
#define ENCODER_EVENT_BIT  (1 << 1)
#define ADC_EVENT_BIT      (1 << 2)
#define SENSOR_EVENT_BIT   (1 << 3)
#define SPI_TX_EVENT_BIT   (1 << 4)

/* Variabel global untuk encoder */
volatile int16_t g_encoder_count = 0;
volatile int8_t g_encoder_dir = 0;
volatile uint8_t g_last_encoder_A = 0;

/* Variabel global untuk button */
volatile uint8_t g_button_pressed = 0;

/* Variabel global sequence */
volatile uint16_t g_sequence = 0;

/* Prototipe fungsi inisialisasi */
void MX_ADC1_Init(void);
void MX_I2C1_Init(void);
void MX_SPI1_Init(void);
void MX_GPIO_Init(void);

/* Prototipe task FreeRTOS */
void Button_Task(void *argument);
void Encoder_Task(void *argument);
void ADC_Read_Task(void *argument);
void I2C_Sensor_Task(void *argument);
void SPI_Send_Task(void *argument);
void System_Monitor_Task(void *argument);

/* Prototipe fungsi callback interrupt */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin);

/* Prototipe fungsi utility */
uint8_t calculate_checksum(FullSystem_Data_t *data);
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
    /* Inisialisasi ADC */
    MX_ADC1_Init();
    /* Inisialisasi I2C */
    MX_I2C1_Init();
    /* Inisialisasi SPI */
    MX_SPI1_Init();

    /* Membuat queue untuk button */
    buttonQueue = xQueueCreate(10, sizeof(uint8_t));
    /* Membuat queue untuk encoder */
    encoderQueue = xQueueCreate(10, sizeof(int16_t));
    /* Membuat queue untuk ADC */
    adcQueue = xQueueCreate(10, sizeof(uint16_t));
    /* Membuat queue untuk sensor I2C */
    sensorQueue = xQueueCreate(5, sizeof(uint32_t) * 2);
    /* Membuat queue untuk SPI transmission */
    spiTxQueue = xQueueCreate(5, sizeof(FullSystem_Data_t));
    
    /* Membuat mutex untuk SPI */
    spiMutex = xSemaphoreCreateMutex();
    /* Membuat mutex untuk I2C */
    i2cMutex = xSemaphoreCreateMutex();
    
    /* Membuat event group sistem */
    systemEventGroup = xEventGroupCreate();

    /* Membuat task button */
    xTaskCreate(Button_Task, "BtnTask", 128, NULL, 2, NULL);
    /* Membuat task encoder */
    xTaskCreate(Encoder_Task, "EncTask", 256, NULL, 3, NULL);
    /* Membuat task ADC */
    xTaskCreate(ADC_Read_Task, "ADCTask", 256, NULL, 3, NULL);
    /* Membuat task sensor I2C */
    xTaskCreate(I2C_Sensor_Task, "I2CTask", 256, NULL, 3, NULL);
    /* Membuat task SPI send */
    xTaskCreate(SPI_Send_Task, "SPISend", 512, NULL, 4, NULL);
    /* Membuat task system monitor */
    xTaskCreate(System_Monitor_Task, "SysMon", 256, NULL, 1, NULL);

    /* Memulai scheduler FreeRTOS */
    vTaskStartScheduler();

    /* Loop tak terbatas sebagai pengaman */
    while (1)
    {
    }
}

/* Callback interrupt GPIO */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    /* Cek interrupt dari encoder A */
    if (GPIO_Pin == ENCODER_A_PIN)
    {
        /* Baca status encoder A dan B */
        uint8_t enc_A = HAL_GPIO_ReadPin(ENCODER_A_PORT, ENCODER_A_PIN);
        uint8_t enc_B = HAL_GPIO_ReadPin(ENCODER_B_PORT, ENCODER_B_PIN);
        
        /* Cek rising edge */
        if (enc_A == 1 && g_last_encoder_A == 0)
        {
            /* Tentukan arah */
            if (enc_B == 0)
            {
                /* Clockwise */
                g_encoder_count++;
                g_encoder_dir = 1;
            }
            else
            {
                /* Counter-clockwise */
                g_encoder_count--;
                g_encoder_dir = -1;
            }
            
            /* Set event bit encoder */
            BaseType_t xHigherPriorityTaskWoken = pdFALSE;
            xEventGroupSetBitsFromISR(systemEventGroup, ENCODER_EVENT_BIT, &xHigherPriorityTaskWoken);
        }
        
        /* Simpan status A */
        g_last_encoder_A = enc_A;
    }
    
    /* Cek interrupt dari button */
    if (GPIO_Pin == BUTTON_PIN)
    {
        /* Toggle button status */
        g_button_pressed = !g_button_pressed;
        
        /* Set event bit button */
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        xEventGroupSetBitsFromISR(systemEventGroup, BUTTON_EVENT_BIT, &xHigherPriorityTaskWoken);
    }
}

/* Task untuk button processing */
void Button_Task(void *argument)
{
    /* Variabel status button */
    uint8_t btn_status;
    
    /* Loop tak terbatas */
    for (;;)
    {
        /* Tunggu event button */
        EventBits_t bits = xEventGroupWaitBits(systemEventGroup,
                                               BUTTON_EVENT_BIT,
                                               pdTRUE,
                                               pdFALSE,
                                               portMAX_DELAY);
        
        /* Cek apakah button event terjadi */
        if (bits & BUTTON_EVENT_BIT)
        {
            /* Ambil status button */
            btn_status = g_button_pressed;
            
            /* Kirim ke queue */
            xQueueSend(buttonQueue, &btn_status, 0);
            
            /* Toggle LED1 sebagai indikator */
            HAL_GPIO_TogglePin(LED1_PORT, LED1_PIN);
            
            /* Print info */
// printf("Button: %d\n", btn_status);
        }
    }
}

/* Task untuk encoder processing */
void Encoder_Task(void *argument)
{
    /* Variabel untuk data encoder */
    int16_t enc_count;
    /* Loop tak terbatas */
    for (;;)
    {
        /* Tunggu event encoder */
        EventBits_t bits = xEventGroupWaitBits(systemEventGroup,
                                               ENCODER_EVENT_BIT,
                                               pdTRUE,
                                               pdFALSE,
                                               portMAX_DELAY);
        
        /* Cek apakah encoder event terjadi */
        if (bits & ENCODER_EVENT_BIT)
        {
            /* Ambil data encoder */
            enc_count = g_encoder_count;
            
            /* Kirim ke queue */
            xQueueSend(encoderQueue, &enc_count, 0);
            
            /* Print info */
// printf("Encoder: %d, Dir: %d\n", enc_count, g_encoder_dir);
        }
        
        /* Delay sebentar */
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

/* Task untuk baca ADC */
void ADC_Read_Task(void *argument)
{
    /* Variabel ADC */
    uint16_t adc_value;
    /* Variabel tegangan */
    uint32_t voltage;
    /* Status ADC */
    HAL_StatusTypeDef status;
    
    /* Loop tak terbatas */
    for (;;)
    {
        /* Mulai ADC */
        status = HAL_ADC_Start(&hadc1);
        
        /* Cek status */
        if (status == HAL_OK)
        {
            /* Tunggu konversi */
            status = HAL_ADC_PollForConversion(&hadc1, 100);
            
            /* Cek konversi selesai */
            if (status == HAL_OK)
            {
                /* Baca ADC */
                adc_value = HAL_ADC_GetValue(&hadc1);
                
                /* Hitung tegangan */
                voltage = (adc_value * 3300) / 4095;
                
                /* Kirim ke queue */
                xQueueSend(adcQueue, &adc_value, 0);
                
                /* Print info */
// printf("ADC: %d, Voltage: %lu mV\n", adc_value, voltage);
            }
            
            /* Stop ADC */
            HAL_ADC_Stop(&hadc1);
        }
        
        /* Delay 500ms */
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

/* Task untuk baca sensor I2C */
void I2C_Sensor_Task(void *argument)
{
    /* Buffer I2C */
    uint8_t i2c_buffer[6];
    /* Status I2C */
    HAL_StatusTypeDef status;
    /* Data sensor */
    int16_t temperature;
    uint32_t pressure;
    
    /* Loop tak terbatas */
    for (;;)
    {
        /* Ambil mutex I2C */
        if (xSemaphoreTake(i2cMutex, pdMS_TO_TICKS(100)) == pdTRUE)
        {
            /* Baca register sensor BMP280 (0xF7-0xFC) */
            status = HAL_I2C_Mem_Read(&hi2c1, I2C_SENSOR_ADDR << 1, 0xF7, 1, i2c_buffer, 6, 100);
            
            /* Cek status */
            if (status == HAL_OK)
            {
                /* Parse tekanan (20-bit) */
                pressure = ((uint32_t)i2c_buffer[0] << 12) | ((uint32_t)i2c_buffer[1] << 4) | (i2c_buffer[2] >> 4);
                
                /* Parse suhu (20-bit) */
                temperature = ((uint32_t)i2c_buffer[3] << 12) | ((uint32_t)i2c_buffer[4] << 4) | (i2c_buffer[5] >> 4);
                
                /* Kirim ke queue (menggunakan struktur sederhana) */
                /* Dalam praktiknya, gunakan struktur yang tepat */
// printf("Sensor - Temp: %d, Press: %lu\n", temperature, pressure);
            }
            else
            {
                /* Print error */
// printf("I2C Error: %d\n", status);
            }
            
            /* Lepas mutex */
            xSemaphoreGive(i2cMutex);
        }
        
        /* Delay 1 detik */
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/* Task untuk kirim data lengkap via SPI */
void SPI_Send_Task(void *argument)
{
    /* Variabel data */
    FullSystem_Data_t sys_data;
    /* Buffer SPI */
    uint8_t tx_buffer[20];
    /* Status SPI */
    HAL_StatusTypeDef status;
    
    /* Loop tak terbatas */
    for (;;)
    {
        /* Susun data lengkap */
        /* Set header */
        sys_data.header = 0xAA;
        /* Set button status */
        sys_data.button_status = g_button_pressed;
        /* Set encoder data */
        sys_data.encoder_count = g_encoder_count;
        sys_data.encoder_dir = g_encoder_dir;
        
        /* Baca ADC terbaru dari queue (non-blocking) */
        uint16_t adc_val = 0;
        if (xQueueReceive(adcQueue, &adc_val, 0) == pdPASS)
        {
            /* Isi data ADC */
            sys_data.adc_value = adc_val;
            sys_data.voltage_mv = (adc_val * 3300) / 4095;
        }
        
        /* Increment sequence */
        g_sequence++;
        sys_data.sequence = g_sequence;
        
        /* Hitung checksum */
        sys_data.checksum = calculate_checksum(&sys_data);
        
        /* Ambil mutex SPI */
        if (xSemaphoreTake(spiMutex, pdMS_TO_TICKS(100)) == pdTRUE)
        {
            /* Susun buffer transmisi */
            /* Header */
            tx_buffer[0] = sys_data.header;
            /* Button */
            tx_buffer[1] = sys_data.button_status;
            /* Encoder count */
            tx_buffer[2] = (sys_data.encoder_count >> 8) & 0xFF;
            tx_buffer[3] = sys_data.encoder_count & 0xFF;
            /* Encoder dir */
            tx_buffer[4] = (uint8_t)sys_data.encoder_dir;
            /* ADC value */
            tx_buffer[5] = (sys_data.adc_value >> 8) & 0xFF;
            tx_buffer[6] = sys_data.adc_value & 0xFF;
            /* Voltage */
            tx_buffer[7] = (sys_data.voltage_mv >> 8) & 0xFF;
            tx_buffer[8] = sys_data.voltage_mv & 0xFF;
            /* Sequence */
            tx_buffer[9] = (sys_data.sequence >> 8) & 0xFF;
            tx_buffer[10] = sys_data.sequence & 0xFF;
            /* Checksum */
            tx_buffer[11] = sys_data.checksum;
            
            /* Set CS LOW */
            HAL_GPIO_WritePin(SPI_CS_PORT, SPI_CS_PIN, GPIO_PIN_RESET);
            /* Delay */
            vTaskDelay(pdMS_TO_TICKS(1));
            /* Kirim SPI */
            status = HAL_SPI_Transmit(&hspi1, tx_buffer, 12, 100);
            /* Delay */
            vTaskDelay(pdMS_TO_TICKS(1));
            /* Set CS HIGH */
            HAL_GPIO_WritePin(SPI_CS_PORT, SPI_CS_PIN, GPIO_PIN_SET);
            
            /* Cek status */
            if (status == HAL_OK)
            {
                /* Print info */
// printf("SPI Sent seq %d\n", g_sequence);
            }
            
            /* Lepas mutex */
            xSemaphoreGive(spiMutex);
        }
        
        /* Delay 500ms */
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

/* Task untuk monitor sistem */
void System_Monitor_Task(void *argument)
{
    /* Loop tak terbatas */
    for (;;)
    {
        /* Toggle LED2 sebagai indikator sistem hidup */
        HAL_GPIO_TogglePin(LED2_PORT, LED2_PIN);
        
        /* Print info sistem - commented out */
        
        /* Delay 1 detik */
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/* Fungsi hitung checksum */
uint8_t calculate_checksum(FullSystem_Data_t *data)
{
    /* Variabel checksum */
    uint8_t sum = 0;
    
    /* Tambahkan semua field */
    sum += data->header;
    sum += data->button_status;
    sum += (data->encoder_count >> 8) & 0xFF;
    sum += data->encoder_count & 0xFF;
    sum += (uint8_t)data->encoder_dir;
    sum += (data->adc_value >> 8) & 0xFF;
    sum += data->adc_value & 0xFF;
    sum += (data->voltage_mv >> 8) & 0xFF;
    sum += data->voltage_mv & 0xFF;
    sum += (data->sequence >> 8) & 0xFF;
    sum += data->sequence & 0xFF;
    
    /* Kembalikan checksum */
    return sum;
}

/* Inisialisasi ADC1 */
void MX_ADC1_Init(void)
{
    /* Enable clock ADC1 */
    __HAL_RCC_ADC1_CLK_ENABLE();
    
    /* Konfigurasi ADC */
    hadc1.Instance = ADC1;
    hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc1.Init.ScanConvMode = DISABLE;
    hadc1.Init.ContinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    
#if defined(STM32F401xC) || defined(STM32F411xE)
    /* F4-specific */
    hadc1.Init.Resolution = ADC_RESOLUTION_12B;
    hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
#endif
    
    HAL_ADC_Init(&hadc1);
    
    /* Konfigurasi channel */
    ADC_ChannelConfTypeDef sConfig = {0};
    sConfig.Channel = ADC_CHANNEL_3;
    sConfig.Rank = 1;
#if defined(STM32F401xC) || defined(STM32F411xE)
    /* F4: 84 cycles */
    sConfig.SamplingTime = ADC_SAMPLETIME_84CYCLES;
#else
    /* F1: 55.5 cycles */
    sConfig.SamplingTime = ADC_SAMPLETIME_55CYCLES_5;
#endif
    HAL_ADC_ConfigChannel(&hadc1, &sConfig);
}

/* Inisialisasi I2C1 */
void MX_I2C1_Init(void)
{
    /* Enable clock I2C1 */
    __HAL_RCC_I2C1_CLK_ENABLE();
    
    /* Konfigurasi I2C */
    hi2c1.Instance = I2C1;
    hi2c1.Init.ClockSpeed = 100000;
    hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
    hi2c1.Init.OwnAddress1 = 0;
    hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    HAL_I2C_Init(&hi2c1);
}

/* Inisialisasi SPI1 */
void MX_SPI1_Init(void)
{
    /* Enable clock SPI1 */
    __HAL_RCC_SPI1_CLK_ENABLE();
    
    /* Konfigurasi SPI */
    hspi1.Instance = SPI1;
    hspi1.Init.Mode = SPI_MODE_MASTER;
    hspi1.Init.Direction = SPI_DIRECTION_2LINES;
    hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
    hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
    hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
    hspi1.Init.NSS = SPI_NSS_SOFT;
    hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
    hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
    hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
    hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    HAL_SPI_Init(&hspi1);
}

/* Inisialisasi GPIO */
void MX_GPIO_Init(void)
{
    /* Enable clock GPIO */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    
    /* Konfigurasi LED1 sebagai output */
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = LED1_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED1_PORT, &GPIO_InitStruct);
    
    /* Konfigurasi LED2 sebagai output */
    GPIO_InitStruct.Pin = LED2_PIN;
    HAL_GPIO_Init(LED2_PORT, &GPIO_InitStruct);
    
    /* Konfigurasi button sebagai interrupt */
    GPIO_InitStruct.Pin = BUTTON_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(BUTTON_PORT, &GPIO_InitStruct);
    
    /* Konfigurasi encoder A sebagai interrupt */
    GPIO_InitStruct.Pin = ENCODER_A_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(ENCODER_A_PORT, &GPIO_InitStruct);
    
    /* Konfigurasi encoder B sebagai input */
    GPIO_InitStruct.Pin = ENCODER_B_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(ENCODER_B_PORT, &GPIO_InitStruct);
    
    /* Konfigurasi CS sebagai output */
    GPIO_InitStruct.Pin = SPI_CS_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(SPI_CS_PORT, &GPIO_InitStruct);
    HAL_GPIO_WritePin(SPI_CS_PORT, SPI_CS_PIN, GPIO_PIN_SET);
    
    /* Enable interrupt EXTI */
    HAL_NVIC_SetPriority(EXTI0_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(EXTI0_IRQn);
    HAL_NVIC_SetPriority(EXTI1_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(EXTI1_IRQn);
}

/* Konfigurasi clock sistem */
void SystemClock_Config(void)
{
    /* Variabel konfigurasi */
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    
    /* Konfigurasi HSI */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
    HAL_RCC_OscConfig(&RCC_OscInitStruct);
    
    /* Konfigurasi clock bus */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0);
}
