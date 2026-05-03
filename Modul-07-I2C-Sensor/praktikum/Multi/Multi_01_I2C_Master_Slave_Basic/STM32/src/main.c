/**
 * Multi_01_I2C_Master_Slave_Basic - Sisi STM32
 * 
 * Deskripsi:
 *   STM32 berperan sebagai slave I2C yang mengekspos virtual register bagi master ESP32.
 *   Data sensor disimulasikan melalui register status, counter, dan suhu.
 * 
 * Hardware:
 *   - STM32F103C8 (Blue Pill) / STM32F401CC / STM32F411CE
 *   - ESP32 sebagai master di bus I2C
 * 
 * Koneksi Pin:
 *   STM32 PB7 (SDA) <-> ESP32 GPIO21
 *   STM32 PB6 (SCL) <-> ESP32 GPIO22
 *   GND STM32 <-> GND ESP32
 *   Pull-up 4.7k pada SDA dan SCL ke 3V3
 * 
 * Instruksi:
 *   1. Upload kode ke STM32
 *   2. Pastikan ESP32 sudah diupload dengan kode master
 *   3. Buka serial monitor 115200 baud
 *   4. Observer respon STM32 saat di-polling oleh ESP32
 * 
 * Variabel yang bisa dicoba:
 *   - STM32_ADDR: Alamat I2C slave (default 0x42)
 *   - REG_STATUS, REG_COUNTER, REG_TEMP_C: Register virtual
 *   - REG_LED_CMD: Register perintah LED
 */
#ifdef STM32F1
#include "stm32f1xx_hal.h"
#else
#include "stm32f4xx_hal.h"
#endif
#include <stdio.h>
#include <string.h>
#include "config.h"

UART_HandleTypeDef huart1;
I2C_HandleTypeDef hi2c1;
static uint8_t regs[REG_COUNT];
static uint8_t selected_reg;

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
    clk.AHBCLKDivider = RCC_SYSCLK_DIV1;
    clk.APB1CLKDivider = RCC_HCLK_DIV2;
    clk.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_2);
#endif
}

static void uart_init(void) {
    __HAL_RCC_USART1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    GPIO_InitTypeDef g = {0};
#ifdef STM32F1
    g.Pin = GPIO_PIN_9;
    g.Mode = GPIO_MODE_AF_PP;
    g.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &g);
    g.Pin = GPIO_PIN_10;
    g.Mode = GPIO_MODE_INPUT;
    g.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &g);
#endif
    huart1.Instance = USART1;
    huart1.Init.BaudRate = 115200;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    HAL_UART_Init(&huart1);
}

static void i2c_slave_init(void) {
    __HAL_RCC_I2C1_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    GPIO_InitTypeDef g = {0};
#ifdef STM32F1
    g.Pin = GPIO_PIN_6 | GPIO_PIN_7;
    g.Mode = GPIO_MODE_AF_OD;
    g.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &g);
#endif
    hi2c1.Instance = I2C1;
    hi2c1.Init.ClockSpeed = 100000;
    hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
    hi2c1.Init.OwnAddress1 = I2C_OWN_ADDR << 1;
    hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.OwnAddress2 = 0;
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    HAL_I2C_Init(&hi2c1);
}

static void update_regs(void) {
    regs[0] = 0xA5;
    regs[1]++;
    regs[2] = 27;
}

int main(void) {
    HAL_Init();
    SystemClock_Config();
    uart_init();
    i2c_slave_init();
    printf("Multi 01 STM32 slave addr=0x%02X\r\n", I2C_OWN_ADDR);
    while (1) {
        update_regs();
        if (HAL_I2C_Slave_Receive(&hi2c1, &selected_reg, 1, 10) == HAL_OK) {
            uint8_t value;
            if (HAL_I2C_Slave_Receive(&hi2c1, &value, 1, 10) == HAL_OK && selected_reg < REG_COUNT) {
                regs[selected_reg] = value;
            }
        }
        if (selected_reg < REG_COUNT) {
            HAL_I2C_Slave_Transmit(&hi2c1, &regs[selected_reg], REG_COUNT - selected_reg, 10);
        }
        HAL_Delay(100);
    }
}
