/**
 * Multi_03_I2C_Role_Swap_Command_Response - Sisi STM32
 * 
 * Deskripsi:
 *   STM32 dan ESP32 bergantian peran sebagai master/slave.
 *   Handshake GPIO BUS_OWNER menentukan siapa yang memegang kendali bus.
 * 
 * Hardware:
 *   - STM32F103C8 (Blue Pill) / STM32F401CC / STM32F411CE
 *   - ESP32 sebagai peer di bus I2C
 * 
 * Koneksi Pin:
 *   STM32 PB7 (SDA) <-> ESP32 GPIO21
 *   STM32 PB6 (SCL) <-> ESP32 GPIO22
 *   STM32 PA0 (BUS_OWNER) <-> ESP32 GPIO4
 *   GND STM32 <-> GND ESP32
 *   Pull-up 4.7k pada SDA dan SCL ke 3V3
 *   Pull-down 10k pada BUS_OWNER
 * 
 * Instruksi:
 *   1. Upload kode ke STM32 dan ESP32
 *   2. Buka serial monitor kedua device (115200 baud)
 *   3. Amati pergantian peran master/slave setiap 3 detik
 * 
 * Variabel yang bisa dicoba:
 *   - ESP32_ADDR: Alamat I2C ESP32 (default 0x32)
 *   - STM32_ADDR: Alamat I2C STM32 (default 0x42)
 *   - BUS_OWNER_PIN: GPIO untuk handshake (PA0)
 *   - I2C_PEER_ADDR: Alamat peer untuk komunikasi
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
static uint8_t command_reg;

int _write(int file, char *ptr, int len) {
    HAL_UART_Transmit(&huart1, (uint8_t*)ptr, len, HAL_MAX_DELAY);
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

static void gpio_init(void) {
    __HAL_RCC_GPIOA_CLK_ENABLE();
    GPIO_InitTypeDef g = {0};
    g.Pin = GPIO_PIN_0;
    g.Mode = GPIO_MODE_INPUT;
    g.Pull = GPIO_PULLDOWN;
    HAL_GPIO_Init(GPIOA, &g);
}

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

static void i2c_init(uint8_t slave) {
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
    hi2c1.Init.OwnAddress1 = slave ? (I2C_OWN_ADDR << 1) : 0;
    hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    HAL_I2C_Init(&hi2c1);
}

int main(void) {
    HAL_Init();
    SystemClock_Config();
    gpio_init();
    uart_init();
    i2c_init(1);
    
    while (1) {
        if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_SET) {
            HAL_I2C_Slave_Receive(&hi2c1, &command_reg, 1, 100);
            printf("slave window cmd=%u\r\n", command_reg);
        } else {
            HAL_I2C_DeInit(&hi2c1);
            i2c_init(0);
            uint8_t reg = 0;
            uint8_t data = 0;
            HAL_I2C_Master_Transmit(&hi2c1, I2C_PEER_ADDR << 1, &reg, 1, 100);
            HAL_I2C_Master_Receive(&hi2c1, I2C_PEER_ADDR << 1, &data, 1, 100);
            printf("master window read ESP32=%u\r\n", data);
            HAL_I2C_DeInit(&hi2c1);
            i2c_init(1);
            HAL_Delay(1000);
        }
        HAL_Delay(100);
    }
}
