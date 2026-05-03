/**
 * Multi_04_I2C_RTOS_Gateway - Sisi STM32
 * 
 * Deskripsi:
 *   STM32 menggunakan FreeRTOS dengan 2 task (sensor read, I2C slave respond).
 *   Data sensor dibaca dan dikirim ke ESP32 melalui queue I2C.
 * 
 * Hardware:
 *   - STM32F103C8 (Blue Pill) / STM32F401CC / STM32F411CE
 *   - ESP32 sebagai master I2C
 *   - Sensor BH1750 (opsional)
 * 
 * Koneksi Pin:
 *   STM32 PB7 (SDA) <-> ESP32 GPIO21
 *   STM32 PB6 (SCL) <-> ESP32 GPIO22
 *   GND STM32 <-> GND ESP32
 *   Pull-up 4.7k pada SDA dan SCL ke 3V3
 * 
 * Instruksi:
 *   1. Upload kode ke STM32
 *   2. Pastikan ESP32 sudah diupload dengan kode gateway
 *   3. Buka serial monitor 115200 baud
 *   4. Observer data sensor yang dikirim ke ESP32
 * 
 * Variabel yang bisa dicoba:
 *   - I2C_OWN_ADDR: Alamat I2C slave STM32 (default 0x42)
 *   - SENSOR_POLL_MS: Interval poll sensor (default 500ms)
 *   - SLAVE_RESP_QUEUE_LEN: Panjang queue response
 *   - REG_COUNT: Jumlah register virtual
 */
#ifdef STM32F1
#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#else
#include "stm32f4xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#endif
#include <stdio.h>
#include "config.h"

UART_HandleTypeDef huart1;
I2C_HandleTypeDef hi2c1;
static uint8_t regs[REG_COUNT];
QueueHandle_t sensor_queue;  // Queue untuk data sensor

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

// Task 1: Simulasi baca sensor (dalam praktik nyata baca BME280/BH1750)
static void sensor_read_task(void *arg) {
    uint16_t lux = 0;
    while (1) {
        lux += 10;  // Simulasi pembacaan sensor
        if (lux > 1000) lux = 100;
        xQueueSend(sensor_queue, &lux, 0);
        printf("Sensor task: lux=%u\r\n", lux);
        vTaskDelay(pdMS_TO_TICKS(SENSOR_POLL_MS));
    }
}

// Task 2: Response slave I2C ke ESP32 master
static void i2c_slave_task(void *arg) {
    uint8_t frame[4];
    while (1) {
        uint16_t lux;
        if (xQueueReceive(sensor_queue, &lux, portMAX_DELAY) == pdPASS) {
            frame[0] = 0x00;           // Register status
            frame[1] = 0xA4;           // Status value
            frame[2] = lux >> 8;       // Lux MSB
            frame[3] = lux & 0xFF;     // Lux LSB
            
            // Update register lokal
            regs[1] = frame[2];
            regs[2] = frame[3];
        }
        
        // Listen untuk request dari ESP32 master
        if (HAL_I2C_Slave_Receive(&hi2c1, frame, sizeof(frame), 500) == HAL_OK) {
            printf("Slave task: data diterima dari ESP32\r\n");
        }
        vTaskDelay(10);
    }
}

// Main dengan FreeRTOS
int main(void) {
    HAL_Init();
    SystemClock_Config();
    uart_init();
    i2c_slave_init();
    
    sensor_queue = xQueueCreate(SLAVE_RESP_QUEUE_LEN, sizeof(uint16_t));
    
    // Buat 2 FreeRTOS tasks
    xTaskCreate(sensor_read_task, "sensor", 256, NULL, 2, NULL);
    xTaskCreate(i2c_slave_task, "i2c_slave", 256, NULL, 1, NULL);
    
    printf("Multi_04 STM32 RTOS Gateway started\r\n");
    vTaskStartScheduler();  // Mulai FreeRTOS scheduler
    
    while (1) {}  // Seharusnya tidak sampai sini
}
