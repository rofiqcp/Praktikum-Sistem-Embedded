/**
 * ==========================================================
 *  Modul 06 - STM32_11_I2C_Clock_Speed_Test
 *  Pengujian kecepatan I2C: 100kHz vs 400kHz
 * ==========================================================
 *  Deskripsi:
 *    Menguji performa I2C pada Standard Mode (100kHz) dan
 *    Fast Mode (400kHz). Reinisialisasi I2C dengan ClockSpeed
 *    berbeda. Mengukur waktu transaksi dengan HAL_GetTick().
 *    Membandingkan throughput kedua mode.
 *  Koneksi:
 *    PB6 = SCL, PB7 = SDA (I2C1)
 *    Target: perangkat I2C apapun (default: MPU6050 @ 0x68)
 * ==========================================================
 */

#ifdef STM32F1
#include "stm32f1xx_hal.h"
#else
#include "stm32f4xx_hal.h"
#endif

#include <stdio.h>
#include <string.h>

UART_HandleTypeDef huart1;
I2C_HandleTypeDef hi2c1;

#define TARGET_ADDR       0x68
#define TARGET_ADDR_W     (TARGET_ADDR << 1)

#define TEST_ITERATIONS   100  /* Jumlah iterasi per pengujian */
#define TEST_READ_SIZE    14   /* Jumlah byte per pembacaan (accel+temp+gyro) */

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
    clk.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK|RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
    clk.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    clk.AHBCLKDivider = RCC_SYSCLK_DIV1;
    clk.APB1CLKDivider = RCC_HCLK_DIV2;
    clk.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_2);
#else
    RCC_OscInitTypeDef osc = {0};
    RCC_ClkInitTypeDef clk = {0};
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);
    osc.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    osc.HSEState = RCC_HSE_ON;
    osc.PLL.PLLState = RCC_PLL_ON;
    osc.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    osc.PLL.PLLM = 8;
    osc.PLL.PLLN = 336;
    osc.PLL.PLLP = RCC_PLLP_DIV4;
    osc.PLL.PLLQ = 7;
    HAL_RCC_OscConfig(&osc);
    clk.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK|RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
    clk.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    clk.AHBCLKDivider = RCC_SYSCLK_DIV1;
    clk.APB1CLKDivider = RCC_HCLK_DIV2;
    clk.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_2);
#endif
}

void MX_USART1_Init(void) {
    __HAL_RCC_USART1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    GPIO_InitTypeDef g = {0};
#ifdef STM32F1
    g.Pin = GPIO_PIN_9; g.Mode = GPIO_MODE_AF_PP; g.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &g);
    g.Pin = GPIO_PIN_10; g.Mode = GPIO_MODE_INPUT; g.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &g);
#else
    g.Pin = GPIO_PIN_9|GPIO_PIN_10; g.Mode = GPIO_MODE_AF_PP;
    g.Pull = GPIO_PULLUP; g.Speed = GPIO_SPEED_FREQ_HIGH; g.Alternate = GPIO_AF7_USART1;
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

/**
 * Inisialisasi I2C1 dengan kecepatan yang ditentukan
 * Gunakan HAL_I2C_DeInit() lalu HAL_I2C_Init() untuk reinit
 */
void I2C1_Init_WithSpeed(uint32_t clock_speed) {
    /* DeInit I2C jika sudah diinisialisasi */
    HAL_I2C_DeInit(&hi2c1);

    __HAL_RCC_I2C1_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    GPIO_InitTypeDef g = {0};
#ifdef STM32F1
    g.Pin = GPIO_PIN_6|GPIO_PIN_7;
    g.Mode = GPIO_MODE_AF_OD;
    g.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &g);
#else
    g.Pin = GPIO_PIN_6|GPIO_PIN_7;
    g.Mode = GPIO_MODE_AF_OD;
    g.Pull = GPIO_PULLUP;
    g.Speed = GPIO_SPEED_FREQ_HIGH;
    g.Alternate = GPIO_AF4_I2C1;
    HAL_GPIO_Init(GPIOB, &g);
#endif

    hi2c1.Instance = I2C1;
    hi2c1.Init.ClockSpeed = clock_speed;
    hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
    hi2c1.Init.OwnAddress1 = 0;
    hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;

    if (HAL_I2C_Init(&hi2c1) != HAL_OK) {
        printf("[ERROR] I2C init gagal @ %lu Hz\r\n", clock_speed);
    } else {
        printf("[OK] I2C diinisialisasi @ %lu Hz (%lu kHz)\r\n",
               clock_speed, clock_speed / 1000);
    }
}

/**
 * Jalankan pengujian throughput I2C
 * Baca TEST_READ_SIZE byte sebanyak TEST_ITERATIONS kali
 */
uint32_t RunSpeedTest(uint32_t clock_speed) {
    uint8_t buf[TEST_READ_SIZE];
    uint32_t errors = 0;

    /* Reinisialisasi I2C dengan kecepatan baru */
    I2C1_Init_WithSpeed(clock_speed);
    HAL_Delay(10);

    printf("\r\n--- Pengujian %lu kHz ---\r\n", clock_speed / 1000);
    printf("  Iterasi: %d, Byte/read: %d\r\n", TEST_ITERATIONS, TEST_READ_SIZE);

    /* Mulai pengukuran waktu */
    uint32_t start_tick = HAL_GetTick();

    for (int i = 0; i < TEST_ITERATIONS; i++) {
        HAL_StatusTypeDef status = HAL_I2C_Mem_Read(&hi2c1, TARGET_ADDR_W, 0x3B,
                                                     I2C_MEMADD_SIZE_8BIT,
                                                     buf, TEST_READ_SIZE, 50);
        if (status != HAL_OK) {
            errors++;
        }
    }

    uint32_t elapsed = HAL_GetTick() - start_tick;

    /* Hitung statistik */
    uint32_t total_bytes = (uint32_t)TEST_ITERATIONS * TEST_READ_SIZE;
    float throughput_bps = (elapsed > 0) ? (total_bytes * 8000.0f / elapsed) : 0;
    float throughput_kbps = throughput_bps / 1000.0f;
    float avg_time_ms = (float)elapsed / TEST_ITERATIONS;

    printf("  Hasil:\r\n");
    printf("    Total waktu  : %lu ms\r\n", elapsed);
    printf("    Total byte   : %lu\r\n", total_bytes);
    printf("    Rata-rata/txn: %.2f ms\r\n", avg_time_ms);
    printf("    Throughput   : %.1f kbps\r\n", throughput_kbps);
    printf("    Error        : %lu / %d\r\n", errors, TEST_ITERATIONS);

    printf("SPEED,%lu,%lu,%lu,%.2f,%.1f,%lu\r\n",
           clock_speed / 1000, elapsed, total_bytes,
           avg_time_ms, throughput_kbps, errors);

    return elapsed;
}

int main(void) {
    HAL_Init();
    SystemClock_Config();
    MX_USART1_Init();

    printf("\r\n================================================\r\n");
    printf("  STM32 I2C Clock Speed Test\r\n");
    printf("  100kHz (Standard) vs 400kHz (Fast Mode)\r\n");
    printf("================================================\r\n");

    /* Inisialisasi awal dengan 100kHz */
    I2C1_Init_WithSpeed(100000);

    /* Cek koneksi */
    if (HAL_I2C_IsDeviceReady(&hi2c1, TARGET_ADDR_W, 3, 100) != HAL_OK) {
        printf("[ERROR] Perangkat 0x%02X tidak ditemukan!\r\n", TARGET_ADDR);
        while (1) { HAL_Delay(1000); }
    }
    printf("[OK] Perangkat 0x%02X terdeteksi\r\n", TARGET_ADDR);

    /* Wake up MPU6050 */
    uint8_t val = 0x00;
    HAL_I2C_Mem_Write(&hi2c1, TARGET_ADDR_W, 0x6B, I2C_MEMADD_SIZE_8BIT, &val, 1, 100);
    HAL_Delay(100);

    uint32_t test_round = 0;

    while (1) {
        test_round++;
        printf("\r\n============ Round #%lu ============\r\n", test_round);

        /* Test Standard Mode (100kHz) */
        uint32_t time_100k = RunSpeedTest(100000);

        HAL_Delay(500);

        /* Test Fast Mode (400kHz) */
        uint32_t time_400k = RunSpeedTest(400000);

        /* Perbandingan */
        printf("\r\n=== PERBANDINGAN ===\r\n");
        printf("  100kHz: %lu ms\r\n", time_100k);
        printf("  400kHz: %lu ms\r\n", time_400k);
        if (time_400k > 0) {
            float speedup = (float)time_100k / time_400k;
            printf("  Percepatan: %.2fx\r\n", speedup);
            printf("COMPARE,%lu,%lu,%lu,%.2f\r\n", test_round, time_100k, time_400k, speedup);
        }

        /* Kembali ke 100kHz untuk idle */
        I2C1_Init_WithSpeed(100000);

        printf("\r\nTest berikutnya dalam 10 detik...\r\n");
        HAL_Delay(10000);
    }
}
