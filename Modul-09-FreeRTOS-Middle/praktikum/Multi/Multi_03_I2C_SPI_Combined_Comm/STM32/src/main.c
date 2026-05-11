/* Multi_03_I2C_SPI_Combined_Comm - STM32 sebagai Pembaca Sensor I2C */
/* Program STM32 dengan FreeRTOS untuk baca sensor I2C dan kirim via SPI ke ESP32 */

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

/* Definisi alamat I2C sensor (contoh: BMP280 = 0x76) */
#define I2C_SENSOR_ADDR 0x76
/* Definisi I2C handle */
I2C_HandleTypeDef hi2c1;
/* Definisi SPI handle */
SPI_HandleTypeDef hspi1;

/* Struktur data sensor untuk dikirim via SPI */
typedef struct {
    /* ID sensor */
    uint8_t sensor_id;
    /* Data sensor byte 0 (temperature low) */
    uint16_t sensor_data1;
    /* Data sensor byte 1 (temperature high) */
    uint16_t sensor_data2;
    /* Data sensor byte 2 (pressure low) */
    uint16_t sensor_data3;
    /* Checksum untuk validasi */
    uint8_t checksum;
} Sensor_Data_t;

/* Handle untuk queue data sensor */
QueueHandle_t sensorQueue;
/* Handle untuk mutex I2C */
SemaphoreHandle_t i2cMutex;
/* Handle untuk mutex SPI */
SemaphoreHandle_t spiMutex;

/* Prototipe fungsi inisialisasi */
void MX_I2C1_Init(void);
void MX_SPI1_Init(void);
void MX_GPIO_Init(void);

/* Prototipe task FreeRTOS */
void I2C_Sensor_Read_Task(void *argument);
void SPI_Send_Task(void *argument);

/* Prototipe fungsi utility */
uint8_t calculate_checksum(Sensor_Data_t *data);
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
    /* Inisialisasi I2C1 */
    MX_I2C1_Init();
    /* Inisialisasi SPI1 */
    MX_SPI1_Init();

    /* Membuat queue untuk data sensor dengan kapasitas 5 item */
    sensorQueue = xQueueCreate(5, sizeof(Sensor_Data_t));
    /* Membuat mutex untuk akses I2C */
    i2cMutex = xSemaphoreCreateMutex();
    /* Membuat mutex untuk akses SPI */
    spiMutex = xSemaphoreCreateMutex();

    /* Membuat task untuk baca sensor I2C */
    xTaskCreate(I2C_Sensor_Read_Task, "I2C_Read", 256, NULL, 3, NULL);
    /* Membuat task untuk kirim data SPI */
    xTaskCreate(SPI_Send_Task, "SPI_Send", 256, NULL, 2, NULL);

    /* Memulai scheduler FreeRTOS */
    vTaskStartScheduler();

    /* Loop tak terbatas sebagai pengaman */
    while (1)
    {
    }
}

/* Task untuk membaca sensor via I2C */
void I2C_Sensor_Read_Task(void *argument)
{
    /* Variabel untuk data sensor */
    Sensor_Data_t sensor_data;
    /* Buffer untuk baca data I2C */
    uint8_t i2c_buffer[6];
    /* Status komunikasi I2C */
    HAL_StatusTypeDef status;
    /* Variabel untuk menyimpan suhu */
    int16_t temperature;
    /* Variabel untuk menyimpan tekanan */
    uint32_t pressure;

    /* Loop tak terbatas task */
    for (;;)
    {
        /* Ambil mutex I2C untuk akses eksklusif */
        if (xSemaphoreTake(i2cMutex, pdMS_TO_TICKS(100)) == pdTRUE)
        {
            /* Baca register sensor (contoh: BMP280 register 0xF7-0xFC untuk data) */
            status = HAL_I2C_Mem_Read(&hi2c1, I2C_SENSOR_ADDR << 1, 0xF7, 1, i2c_buffer, 6, 100);
            
            /* Cek apakah pembacaan berhasil */
            if (status == HAL_OK)
            {
                /* Parse data tekanan (20-bit) */
                pressure = ((uint32_t)i2c_buffer[0] << 12) | ((uint32_t)i2c_buffer[1] << 4) | (i2c_buffer[2] >> 4);
                
                /* Parse data suhu (20-bit) */
                temperature = ((uint32_t)i2c_buffer[3] << 12) | ((uint32_t)i2c_buffer[4] << 4) | (i2c_buffer[5] >> 4);
                
                /* Isi struktur data sensor */
                /* Set ID sensor */
                sensor_data.sensor_id = I2C_SENSOR_ADDR;
                /* Set data sensor 1 (suhu low) */
                sensor_data.sensor_data1 = temperature & 0xFFFF;
                /* Set data sensor 2 (suhu high) */
                sensor_data.sensor_data2 = (temperature >> 16) & 0xFFFF;
                /* Set data sensor 3 (tekanan) */
                sensor_data.sensor_data3 = pressure & 0xFFFF;
                
                /* Hitung checksum */
                sensor_data.checksum = calculate_checksum(&sensor_data);
                
                /* Kirim data ke queue */
                xQueueSend(sensorQueue, &sensor_data, 0);
                
                /* Print info pembacaan */
// printf("I2C Read - Temp: %d, Press: %lu\n", temperature, pressure);
            }
            else
            {
                /* Print error jika gagal baca I2C */
// printf("I2C Read Error: %d\n", status);
            }
            
            /* Lepas mutex I2C */
            xSemaphoreGive(i2cMutex);
        }
        
        /* Delay task selama 1 detik */
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/* Task untuk mengirim data sensor via SPI ke ESP32 */
void SPI_Send_Task(void *argument)
{
    /* Variabel untuk data sensor */
    Sensor_Data_t sensor_data;
    /* Buffer untuk transmisi SPI */
    uint8_t tx_buffer[8];
    /* Status transmisi SPI */
    HAL_StatusTypeDef status;
    /* Pin CS (Chip Select) */
    GPIO_PinState cs_pin = GPIO_PIN_SET;

    /* Loop tak terbatas task */
    for (;;)
    {
        /* Ambil data dari queue dengan timeout */
        if (xQueueReceive(sensorQueue, &sensor_data, portMAX_DELAY) == pdPASS)
        {
            /* Ambil mutex SPI untuk akses eksklusif */
            if (xSemaphoreTake(spiMutex, pdMS_TO_TICKS(100)) == pdTRUE)
            {
                /* Susun data ke buffer transmisi */
                /* Byte 0: Sensor ID */
                tx_buffer[0] = sensor_data.sensor_id;
                /* Byte 1: Sensor data1 low */
                tx_buffer[1] = sensor_data.sensor_data1 & 0xFF;
                /* Byte 2: Sensor data1 high */
                tx_buffer[2] = (sensor_data.sensor_data1 >> 8) & 0xFF;
                /* Byte 3: Sensor data2 low */
                tx_buffer[3] = sensor_data.sensor_data2 & 0xFF;
                /* Byte 4: Sensor data2 high */
                tx_buffer[4] = (sensor_data.sensor_data2 >> 8) & 0xFF;
                /* Byte 5: Sensor data3 low */
                tx_buffer[5] = sensor_data.sensor_data3 & 0xFF;
                /* Byte 6: Sensor data3 high */
                tx_buffer[6] = (sensor_data.sensor_data3 >> 8) & 0xFF;
                /* Byte 7: Checksum */
                tx_buffer[7] = sensor_data.checksum;
                
                /* Set pin CS ke LOW (aktif) */
                HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_RESET);
                
                /* Delay singkat untuk stabilitas */
                vTaskDelay(pdMS_TO_TICKS(1));
                
                /* Kirim data via SPI */
                status = HAL_SPI_Transmit(&hspi1, tx_buffer, 8, 100);
                
                /* Delay singkat */
                vTaskDelay(pdMS_TO_TICKS(1));
                
                /* Set pin CS ke HIGH (tidak aktif) */
                HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_SET);
                
                /* Cek status pengiriman */
                if (status == HAL_OK)
                {
                    /* Print info pengiriman */
// printf("SPI Sent sensor data\n");
                }
                
                /* Lepas mutex SPI */
                xSemaphoreGive(spiMutex);
            }
        }
    }
}

/* Fungsi untuk menghitung checksum */
uint8_t calculate_checksum(Sensor_Data_t *data)
{
    /* Variabel untuk menyimpan hasil checksum */
    uint8_t sum = 0;
    
    /* Tambahkan sensor_id ke checksum */
    sum += data->sensor_id;
    /* Tambahkan sensor_data1 low byte */
    sum += data->sensor_data1 & 0xFF;
    /* Tambahkan sensor_data1 high byte */
    sum += (data->sensor_data1 >> 8) & 0xFF;
    /* Tambahkan sensor_data2 low byte */
    sum += data->sensor_data2 & 0xFF;
    /* Tambahkan sensor_data2 high byte */
    sum += (data->sensor_data2 >> 8) & 0xFF;
    /* Tambahkan sensor_data3 low byte */
    sum += data->sensor_data3 & 0xFF;
    /* Tambahkan sensor_data3 high byte */
    sum += (data->sensor_data3 >> 8) & 0xFF;
    
    /* Kembalikan nilai checksum */
    return sum;
}

/* Inisialisasi I2C1 */
void MX_I2C1_Init(void)
{
    /* Enable clock untuk I2C1 */
    __HAL_RCC_I2C1_CLK_ENABLE();
    
    /* Konfigurasi I2C1 */
    hi2c1.Instance = I2C1;
    /* Set clock speed 100kHz (standard mode) */
    hi2c1.Init.ClockSpeed = 100000;
    /* Set duty cycle 2 */
    hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
    /* Set addressing mode 7-bit */
    hi2c1.Init.OwnAddress1 = 0;
    /* Set addressing mode 7-bit */
    hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    /* Set dual addressing disable */
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    /* Set general call disable */
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    /* Set no stretch mode disable */
    hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    
    /* Inisialisasi I2C1 */
    HAL_I2C_Init(&hi2c1);
}

/* Inisialisasi SPI1 sebagai master */
void MX_SPI1_Init(void)
{
    /* Enable clock untuk SPI1 */
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
    /* Set baud rate prescaler 16 */
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
    /* Enable clock GPIOA */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    /* Enable clock GPIOB */
    __HAL_RCC_GPIOB_CLK_ENABLE();
    
    /* Konfigurasi pin SPI CS sebagai output */
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    /* Set pin CS */
    GPIO_InitStruct.Pin = GPIO_PIN_3;
    /* Set mode output */
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    /* Set no pull */
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    /* Set speed high */
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    /* Inisialisasi pin CS */
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    /* Set CS ke HIGH (tidak aktif) */
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_SET);
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
