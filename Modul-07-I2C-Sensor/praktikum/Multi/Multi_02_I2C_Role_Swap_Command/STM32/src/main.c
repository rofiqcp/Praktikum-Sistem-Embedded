/**
 * Multi_02_I2C_Master_Slave_STM32_Master - Sisi STM32
 * 
 * Deskripsi:
 *   STM32 berperan sebagai master I2C yang mem-poll register virtual ESP32 slave.
 *   STM32 membaca data sensor dan mengirim perintah LED ke ESP32.
 * 
 * Hardware:
 *   - STM32F103C8 (Blue Pill) / STM32F401CC / STM32F411CE
 *   - ESP32 sebagai slave I2C
 * 
 * Koneksi Pin:
 *   STM32 PB7 (SDA) <-> ESP32 GPIO21
 *   STM32 PB6 (SCL) <-> ESP32 GPIO22
 *   GND STM32 <-> GND ESP32
 *   Pull-up 4.7k pada SDA dan SCL ke 3V3
 * 
 * Instruksi:
 *   1. Upload kode ke STM32
 *   2. Pastikan ESP32 sudah diupload dengan kode slave
 *   3. Buka serial monitor 115200 baud
 *   4. STM32 akan mem-poll data dari ESP32 setiap 1 detik
 * 
 * Variabel yang bisa dicoba:
 *   - ESP32_SLAVE_ADDR: Alamat I2C slave ESP32 (default 0x32)
 *   - I2C_HZ: Kecepatan I2C (default 100000)
 *   - REG_STATUS, REG_COUNTER, REG_TEMP_C: Register yang dibaca
 *   - REG_LED_CMD: Register perintah LED
 */
#ifdef STM32F1
#include "stm32f1xx_hal.h"
#else
#include "stm32f4xx_hal.h"
#endif
#include <stdio.h>
#include "config.h"

UART_HandleTypeDef huart1;
I2C_HandleTypeDef hi2c1;

int _write(int file, char *ptr, int len) {
    HAL_UART_Transmit(&huart1, (uint8_t *)ptr, len, HAL_MAX_DELAY);
    return len;
}

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

static void uart_init(void) {
    __HAL_RCC_USART1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    GPIO_InitTypeDef g = {0};
    g.Pin = GPIO_PIN_9;
    g.Mode = GPIO_MODE_AF_PP;
    g.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &g);
    g.Pin = GPIO_PIN_10;
    g.Mode = GPIO_MODE_INPUT;
    HAL_GPIO_Init(GPIOA, &g);
    huart1.Instance = USART1;
    huart1.Init.BaudRate = 115200;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    HAL_UART_Init(&huart1);
}

static void i2c_master_init(void) {
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
    hi2c1.Init.OwnAddress1 = 0;
    hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    HAL_I2C_Init(&hi2c1);
}

static HAL_StatusTypeDef reg_read(uint8_t reg, uint8_t *data, uint16_t len) {
    return HAL_I2C_Master_Transmit(&hi2c1, I2C_PEER_ADDR << 1, &reg, 1, 100) == HAL_OK
        ? HAL_I2C_Master_Receive(&hi2c1, I2C_PEER_ADDR << 1, data, len, 100)
        : HAL_ERROR;
}

int main(void) {
    HAL_Init();
    SystemClock_Config();
    uart_init();
    i2c_master_init();
    while (1) {
        uint8_t buf[3] = {0};
        if (reg_read(0x00, buf, sizeof(buf)) == HAL_OK) {
            printf("ESP32 regs: status=%u counter=%u temp=%u\r\n", buf[0], buf[1], buf[2]);
        } else {
            printf("ESP32 slave tidak ACK; scaffold slave perlu driver target\r\n");
        }
        HAL_Delay(1000);
    }
}
