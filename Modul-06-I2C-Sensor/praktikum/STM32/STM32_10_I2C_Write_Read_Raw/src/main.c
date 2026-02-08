/**
 * ==========================================================
 *  Modul 06 - STM32_10_I2C_Write_Read_Raw
 *  Operasi baca/tulis register I2C secara mentah
 * ==========================================================
 *  Deskripsi:
 *    Demonstrasi HAL_I2C_Master_Transmit() dan HAL_I2C_Master_Receive()
 *    untuk membaca dan menulis register perangkat I2C secara manual.
 *    Output verbose menunjukkan setiap langkah operasi I2C.
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

/* Target default: MPU6050 */
#define TARGET_ADDR       0x68
#define TARGET_ADDR_W     (TARGET_ADDR << 1)
#define TARGET_ADDR_R     (TARGET_ADDR_W | 1)

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

void MX_I2C1_Init(void) {
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
    hi2c1.Init.ClockSpeed = 100000;
    hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
    hi2c1.Init.OwnAddress1 = 0;
    hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    HAL_I2C_Init(&hi2c1);
}

/* =================== Fungsi I2C Mentah =================== */

/**
 * Cetak status HAL sebagai string
 */
const char* HAL_StatusStr(HAL_StatusTypeDef status) {
    switch (status) {
        case HAL_OK:      return "OK";
        case HAL_ERROR:   return "ERROR";
        case HAL_BUSY:    return "BUSY";
        case HAL_TIMEOUT: return "TIMEOUT";
        default:          return "UNKNOWN";
    }
}

/**
 * Baca satu register secara mentah menggunakan Master Transmit + Receive
 * Langkah:
 *   1. Transmit: kirim alamat register (1 byte)
 *   2. Receive: baca data dari register (1 byte)
 */
uint8_t I2C_RawReadReg(uint8_t dev_addr, uint8_t reg_addr) {
    HAL_StatusTypeDef status;
    uint8_t reg = reg_addr;
    uint8_t data = 0;

    printf("\r\n  [STEP 1] Master Transmit: Kirim alamat register\r\n");
    printf("    - Device addr: 0x%02X (7-bit) -> 0x%02X (write)\r\n",
           dev_addr, dev_addr << 1);
    printf("    - Register: 0x%02X\r\n", reg_addr);

    status = HAL_I2C_Master_Transmit(&hi2c1, (dev_addr << 1), &reg, 1, 100);
    printf("    - Status: %s\r\n", HAL_StatusStr(status));

    if (status != HAL_OK) {
        printf("    [GAGAL] Transmit gagal!\r\n");
        return 0;
    }

    printf("  [STEP 2] Master Receive: Baca data dari register\r\n");
    printf("    - Device addr: 0x%02X (7-bit) -> 0x%02X (read)\r\n",
           dev_addr, (dev_addr << 1) | 1);

    status = HAL_I2C_Master_Receive(&hi2c1, (dev_addr << 1) | 1, &data, 1, 100);
    printf("    - Status: %s\r\n", HAL_StatusStr(status));
    printf("    - Data diterima: 0x%02X (%d)\r\n", data, data);

    return data;
}

/**
 * Tulis satu register secara mentah menggunakan Master Transmit
 * Langkah:
 *   1. Transmit: kirim alamat register + data (2 bytes)
 */
HAL_StatusTypeDef I2C_RawWriteReg(uint8_t dev_addr, uint8_t reg_addr, uint8_t value) {
    uint8_t buf[2] = {reg_addr, value};

    printf("\r\n  [STEP 1] Master Transmit: Kirim register + data\r\n");
    printf("    - Device addr: 0x%02X (7-bit) -> 0x%02X (write)\r\n",
           dev_addr, dev_addr << 1);
    printf("    - Register: 0x%02X\r\n", reg_addr);
    printf("    - Data: 0x%02X (%d)\r\n", value, value);
    printf("    - Payload: [0x%02X, 0x%02X]\r\n", buf[0], buf[1]);

    HAL_StatusTypeDef status = HAL_I2C_Master_Transmit(&hi2c1, (dev_addr << 1),
                                                        buf, 2, 100);
    printf("    - Status: %s\r\n", HAL_StatusStr(status));

    return status;
}

/**
 * Baca beberapa register berurutan secara mentah
 */
void I2C_RawReadMulti(uint8_t dev_addr, uint8_t start_reg, uint8_t *buf, uint8_t len) {
    HAL_StatusTypeDef status;

    printf("\r\n  [STEP 1] Master Transmit: Set alamat register awal\r\n");
    printf("    - Start register: 0x%02X, Jumlah: %d byte\r\n", start_reg, len);

    status = HAL_I2C_Master_Transmit(&hi2c1, (dev_addr << 1), &start_reg, 1, 100);
    printf("    - Status: %s\r\n", HAL_StatusStr(status));

    printf("  [STEP 2] Master Receive: Baca %d byte data\r\n", len);
    status = HAL_I2C_Master_Receive(&hi2c1, (dev_addr << 1) | 1, buf, len, 100);
    printf("    - Status: %s\r\n", HAL_StatusStr(status));

    printf("    - Data: ");
    for (int i = 0; i < len; i++) {
        printf("0x%02X ", buf[i]);
    }
    printf("\r\n");
}

/**
 * Bandingkan dengan HAL_I2C_Mem_Read (metode convenience)
 */
void I2C_CompareWithMemRead(uint8_t dev_addr, uint8_t reg_addr) {
    uint8_t raw_val, mem_val;

    printf("\r\n--- Perbandingan: Raw vs Mem_Read ---\r\n");

    /* Metode 1: Raw (Transmit + Receive) */
    printf("[Metode 1] HAL_I2C_Master_Transmit + Receive:\r\n");
    raw_val = I2C_RawReadReg(dev_addr, reg_addr);

    /* Metode 2: Mem_Read */
    printf("\r\n[Metode 2] HAL_I2C_Mem_Read:\r\n");
    printf("  - Satu panggilan fungsi untuk transmit + receive\r\n");
    HAL_StatusTypeDef status = HAL_I2C_Mem_Read(&hi2c1, (dev_addr << 1), reg_addr,
                                                 I2C_MEMADD_SIZE_8BIT, &mem_val, 1, 100);
    printf("  - Status: %s\r\n", HAL_StatusStr(status));
    printf("  - Data: 0x%02X\r\n", mem_val);

    /* Bandingkan */
    printf("\r\n[Hasil] Raw=0x%02X, Mem=0x%02X -> %s\r\n",
           raw_val, mem_val,
           (raw_val == mem_val) ? "SAMA (OK)" : "BERBEDA (!)");
}

int main(void) {
    HAL_Init();
    SystemClock_Config();
    MX_USART1_Init();
    MX_I2C1_Init();

    printf("\r\n================================================\r\n");
    printf("  STM32 I2C Raw Read/Write Demo\r\n");
    printf("  Target: MPU6050 @ 0x%02X\r\n", TARGET_ADDR);
    printf("================================================\r\n");

    /* Cek koneksi */
    if (HAL_I2C_IsDeviceReady(&hi2c1, TARGET_ADDR_W, 3, 100) != HAL_OK) {
        printf("[ERROR] Perangkat 0x%02X tidak ditemukan!\r\n", TARGET_ADDR);
        printf("[INFO] Tetap melanjutkan untuk demonstrasi...\r\n");
    } else {
        printf("[OK] Perangkat 0x%02X terdeteksi\r\n", TARGET_ADDR);
    }

    uint32_t iterasi = 0;

    while (1) {
        iterasi++;
        printf("\r\n======== Iterasi #%lu ========\r\n", iterasi);

        /* === Test 1: Baca WHO_AM_I register (0x75) === */
        printf("\r\n--- Test 1: Baca Register WHO_AM_I (0x75) ---\r\n");
        uint8_t who = I2C_RawReadReg(TARGET_ADDR, 0x75);
        printf("  WHO_AM_I = 0x%02X (seharusnya 0x68 untuk MPU6050)\r\n", who);

        /* === Test 2: Tulis register PWR_MGMT_1 (0x6B) === */
        printf("\r\n--- Test 2: Tulis Register PWR_MGMT_1 (0x6B) ---\r\n");
        printf("  Mengirim 0x00 untuk wake up dari sleep mode\r\n");
        I2C_RawWriteReg(TARGET_ADDR, 0x6B, 0x00);

        /* Verifikasi dengan membaca kembali */
        printf("\r\n  Verifikasi: membaca kembali register 0x6B...\r\n");
        uint8_t pwr = I2C_RawReadReg(TARGET_ADDR, 0x6B);
        printf("  PWR_MGMT_1 = 0x%02X (seharusnya 0x00)\r\n", pwr);

        /* === Test 3: Baca multi-byte (akselerometer 6 byte) === */
        printf("\r\n--- Test 3: Baca Multi-Byte (Accel 6 byte) ---\r\n");
        uint8_t accel_buf[6];
        I2C_RawReadMulti(TARGET_ADDR, 0x3B, accel_buf, 6);

        int16_t ax = (int16_t)(accel_buf[0]<<8 | accel_buf[1]);
        int16_t ay = (int16_t)(accel_buf[2]<<8 | accel_buf[3]);
        int16_t az = (int16_t)(accel_buf[4]<<8 | accel_buf[5]);
        printf("  Accel Raw: X=%d Y=%d Z=%d\r\n", ax, ay, az);
        printf("  Accel (g): X=%.3f Y=%.3f Z=%.3f\r\n",
               ax/16384.0f, ay/16384.0f, az/16384.0f);

        /* === Test 4: Perbandingan Raw vs Mem_Read === */
        I2C_CompareWithMemRead(TARGET_ADDR, 0x75);

        printf("\r\nDATA,%lu,%d,%d,%d,0x%02X\r\n", iterasi, ax, ay, az, who);

        HAL_Delay(3000);
    }
}
