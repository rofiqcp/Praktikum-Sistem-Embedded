/**
 * Multi_05_I2C_RTOS_Weather_Station - Sisi STM32
 * 
 * Deskripsi:
 *   STM32 menggunakan FreeRTOS dengan multiple tasks untuk data processing.
 *   Mengolah data sensor dan mengirimkan ke ESP32 untuk ditampilkan.
 * 
 * Hardware:
 *   - STM32F103C8 (Blue Pill) / STM32F401CC / STM32F411CE
 *   - ESP32 sebagai gateway/display
 *   - Sensor BME280, BH1750, DS3231 (opsional)
 * 
 * Koneksi Pin:
 *   STM32 PB7 (SDA) <-> ESP32 GPIO21
 *   STM32 PB6 (SCL) <-> ESP32 GPIO22
 *   GND STM32 <-> GND ESP32
 *   Pull-up 4.7k pada SDA dan SCL ke 3V3
 * 
 * Instruksi:
 *   1. Upload kode ke STM32
 *   2. Pastikan ESP32 sudah diupload dengan kode weather station
 *   3. Buka serial monitor 115200 baud
 *   4. Observer pemrosesan data sensor dengan RTOS
 * 
 * Variabel yang bisa dicoba:
 *   - I2C_OWN_ADDR: Alamat I2C slave STM32 (default 0x42)
 *   - READ_INTERVAL_MS: Interval baca sensor (default 1000ms)
 *   - DISPLAY_UPDATE_MS: Interval update display (default 2000ms)
 *   - REG_COUNT: Jumlah register virtual
 */
#ifdef STM32F1
#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#else
#include "stm32f4xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#endif
#include <stdio.h>
#include "config.h"

UART_HandleTypeDef huart1;
I2C_HandleTypeDef hi2c1;
static uint8_t regs[REG_COUNT];
QueueHandle_t weather_queue;    // Queue data cuaca
SemaphoreHandle_t i2c_semaphore; // Semaphore untuk koordinasi I2C

// Redirect stdout ke UART1
int _write(int file, char *ptr, int len) {
    HAL_UART_Transmit(&huart1, (uint8_t *)ptr, len, HAL_MAX_DELAY);
    return len;
}

// Konfigurasi clock sistem
void SystemClock_Config(void) {
#ifdef STM32F1
    RCC_OscInitTypeDef osc = {0};
    RCC_ClkInitTypeDef clk = {0};
    osc.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    osc.HSEState = RCC_HSE_ON;
    osc.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    osc.PLL.PLLState = RCC_PLL_ON;
    osc.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    osc.PLL.PLLMUL = RCC_PLL_MUL9;
    HAL_RCC_OscConfig(&osc);
    clk.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    clk.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    clk.APB1CLKDivider = RCC_HCLK_DIV2;
    HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_2);
#endif
}

// Inisialisasi UART1
static void uart_init(void) {
    __HAL_RCC_USART1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    GPIO_InitTypeDef g = {0};
    g.Pin = GPIO_PIN_9;
    g.Mode = GPIO_MODE_AF_PP;
    g.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &g);
    huart1.Instance = USART1;
    huart1.Init.BaudRate = 115200;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    HAL_UART_Init(&huart1);
}

// Inisialisasi I2C slave
static void i2c_slave_init(void) {
    __HAL_RCC_I2C1_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    GPIO_InitTypeDef g = {0};
    g.Pin = GPIO_PIN_6 | GPIO_PIN_7;
    g.Mode = GPIO_MODE_AF_OD;
    g.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &g);
    hi2c1.Instance = I2C1;
    hi2c1.Init.ClockSpeed = 100000;
    hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
    hi2c1.Init.OwnAddress1 = I2C_OWN_ADDR << 1;
    hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    HAL_I2C_Init(&hi2c1);
}

// Task 1: Terima data dari ESP32 master via I2C slave
static void i2c_receive_task(void *arg) {
    uint8_t pkt[12];
    while (1) {
        if (xSemaphoreTake(i2c_semaphore, portMAX_DELAY) == pdTRUE) {
            if (HAL_I2C_Slave_Receive(&hi2c1, pkt, sizeof(pkt), 1000) == HAL_OK) {
                // Parse data cuaca dari ESP32
                uint16_t temp = (pkt[2] << 8) | pkt[3];
                uint16_t hum = (pkt[4] << 8) | pkt[5];
                uint16_t lux = (pkt[6] << 8) | pkt[7];
                
                // Simpan ke register
                for (int i = 0; i < sizeof(pkt); i++) {
                    if (pkt[0] + i < REG_COUNT) {
                        regs[pkt[0] + i] = pkt[i];
                    }
                }
                
                // Kirim ke queue untuk task processing
                xQueueSend(weather_queue, pkt, 0);
                printf("I2C RX: temp=%.1fC, hum=%.1f%%, lux=%u\r\n", 
                       temp / 10.0, hum / 10.0, lux);
            }
            xSemaphoreGive(i2c_semaphore);
        }
        vTaskDelay(10);
    }
}

// Task 2: Data processing (filtering/averaging)
static void data_processing_task(void *arg) {
    uint8_t pkt[12];
    static float temp_avg = 0;
    static uint8_t count = 0;
    
    while (1) {
        if (xQueueReceive(weather_queue, pkt, portMAX_DELAY) == pdPASS) {
            uint16_t temp = (pkt[2] << 8) | pkt[3];
            temp_avg += temp / 10.0;
            count++;
            
            if (count >= 5) {  // Average setiap 5 sampel
                printf("Processing: avg_temp=%.1fC\r\n", temp_avg / count);
                temp_avg = 0;
                count = 0;
            }
        }
    }
}

// Task 3: Display update (simulasi OLED)
static void display_task(void *arg) {
    while (1) {
        uint16_t temp = (regs[2] << 8) | regs[3];
        uint16_t hum = (regs[4] << 8) | regs[5];
        uint8_t hour = regs[8];
        uint8_t minute = regs[9];
        
        printf("DISPLAY: %.1fC, %.1f%%, %02u:%02u\r\n", 
               temp / 10.0, hum / 10.0, hour, minute);
        
        vTaskDelay(pdMS_TO_TICKS(DISPLAY_UPDATE_MS));
    }
}

// Main dengan FreeRTOS
int main(void) {
    HAL_Init();
    SystemClock_Config();
    uart_init();
    i2c_slave_init();
    
    weather_queue = xQueueCreate(5, 12);  // Queue untuk 5 paket data (12 bytes)
    i2c_semaphore = xSemaphoreCreateMutex();
    
    // Buat multiple RTOS tasks
    xTaskCreate(i2c_receive_task, "i2c_rx", 256, NULL, 3, NULL);
    xTaskCreate(data_processing_task, "processing", 256, NULL, 2, NULL);
    xTaskCreate(display_task, "display", 256, NULL, 1, NULL);
    
    printf("Multi_05 STM32 RTOS Weather Station started\r\n");
    vTaskStartScheduler();
    
    while (1) {}  // Seharusnya tidak sampai sini
}
