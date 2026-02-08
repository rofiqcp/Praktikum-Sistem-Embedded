/**
 * ==========================================================
 *  Modul 06 - STM32_12_I2C_Error_Recovery
 *  Penanganan error dan pemulihan bus I2C
 * ==========================================================
 *  Deskripsi:
 *    Mendeteksi error I2C menggunakan HAL_I2C_GetError().
 *    Pemulihan bus: HAL_I2C_DeInit(), toggle SCL via GPIO,
 *    kemudian HAL_I2C_Init() ulang.
 *    Logika timeout dan retry otomatis.
 *  Koneksi:
 *    PB6 = SCL, PB7 = SDA (I2C1)
 *    Target: perangkat I2C apapun (default: 0x68)
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

#define MAX_RETRY         3    /* Maksimum percobaan ulang */
#define RETRY_DELAY_MS    100  /* Delay antar percobaan */

/* Statistik error */
typedef struct {
    uint32_t total_transaksi;
    uint32_t sukses;
    uint32_t error_af;       /* Acknowledge failure */
    uint32_t error_berr;     /* Bus error */
    uint32_t error_arlo;     /* Arbitration lost */
    uint32_t error_ovr;      /* Overrun/Underrun */
    uint32_t error_timeout;  /* Timeout */
    uint32_t error_lainnya;
    uint32_t recovery_count; /* Jumlah pemulihan bus */
    uint32_t retry_count;    /* Total percobaan ulang */
} I2C_ErrorStats;

static I2C_ErrorStats stats = {0};

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

/* =================== Error Handling =================== */

/**
 * Analisis dan cetak detail error I2C
 */
const char* I2C_AnalyzeError(uint32_t error) {
    if (error == HAL_I2C_ERROR_NONE) return "Tidak ada error";
    if (error & HAL_I2C_ERROR_BERR) {
        stats.error_berr++;
        return "Bus Error (BERR) - Sinyal START/STOP tidak valid";
    }
    if (error & HAL_I2C_ERROR_ARLO) {
        stats.error_arlo++;
        return "Arbitration Lost (ARLO) - Kehilangan prioritas bus";
    }
    if (error & HAL_I2C_ERROR_AF) {
        stats.error_af++;
        return "Acknowledge Failure (AF) - Slave tidak merespon";
    }
    if (error & HAL_I2C_ERROR_OVR) {
        stats.error_ovr++;
        return "Overrun/Underrun (OVR) - Data terlalu cepat";
    }
    if (error & HAL_I2C_ERROR_TIMEOUT) {
        stats.error_timeout++;
        return "Timeout - Operasi melebihi batas waktu";
    }
    stats.error_lainnya++;
    return "Error tidak dikenal";
}

/**
 * Pemulihan bus I2C dengan toggle SCL manual
 * Prosedur:
 *   1. DeInit I2C peripheral
 *   2. Konfigurasi SCL sebagai GPIO output
 *   3. Toggle SCL 9 kali (untuk melepas slave yang hang)
 *   4. Generate STOP condition (SDA LOW->HIGH saat SCL HIGH)
 *   5. ReInit I2C peripheral
 */
void I2C_BusRecovery(void) {
    printf("[RECOVERY] Memulai pemulihan bus I2C...\r\n");
    stats.recovery_count++;

    /* Langkah 1: DeInit I2C */
    printf("  [1/5] De-inisialisasi I2C peripheral\r\n");
    HAL_I2C_DeInit(&hi2c1);
    __HAL_RCC_I2C1_CLK_DISABLE();

    /* Langkah 2: Konfigurasi SCL (PB6) sebagai GPIO output push-pull */
    printf("  [2/5] Konfigurasi SCL sebagai GPIO output\r\n");
    GPIO_InitTypeDef g = {0};
    g.Pin = GPIO_PIN_6;
    g.Mode = GPIO_MODE_OUTPUT_PP;
    g.Speed = GPIO_SPEED_FREQ_HIGH;
    g.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOB, &g);

    /* Konfigurasi SDA (PB7) sebagai GPIO input untuk monitoring */
    g.Pin = GPIO_PIN_7;
    g.Mode = GPIO_MODE_INPUT;
    g.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOB, &g);

    /* Langkah 3: Toggle SCL 9 kali */
    printf("  [3/5] Toggle SCL 9 kali\r\n");
    for (int i = 0; i < 9; i++) {
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_RESET); /* SCL LOW */
        HAL_Delay(1);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET);   /* SCL HIGH */
        HAL_Delay(1);

        /* Cek apakah SDA sudah HIGH (bus bebas) */
        if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_7) == GPIO_PIN_SET) {
            printf("    SDA HIGH setelah %d clock pulses\r\n", i + 1);
            break;
        }
    }

    /* Langkah 4: Generate STOP condition */
    printf("  [4/5] Generate STOP condition\r\n");
    /* SDA sebagai output */
    g.Pin = GPIO_PIN_7;
    g.Mode = GPIO_MODE_OUTPUT_PP;
    g.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &g);

    /* STOP: SDA LOW -> HIGH saat SCL HIGH */
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET);   /* SCL HIGH */
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_RESET); /* SDA LOW */
    HAL_Delay(1);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_SET);   /* SDA HIGH (STOP) */
    HAL_Delay(1);

    /* Langkah 5: ReInit I2C */
    printf("  [5/5] Re-inisialisasi I2C peripheral\r\n");
    MX_I2C1_Init();
    HAL_Delay(10);

    printf("[RECOVERY] Pemulihan bus selesai\r\n");
}

/**
 * Baca register dengan retry dan error recovery otomatis
 */
HAL_StatusTypeDef I2C_ReadWithRetry(uint8_t dev_addr, uint8_t reg_addr,
                                     uint8_t *data, uint16_t len) {
    HAL_StatusTypeDef status;

    for (int attempt = 1; attempt <= MAX_RETRY; attempt++) {
        stats.total_transaksi++;

        status = HAL_I2C_Mem_Read(&hi2c1, (dev_addr << 1), reg_addr,
                                   I2C_MEMADD_SIZE_8BIT, data, len, 100);

        if (status == HAL_OK) {
            stats.sukses++;
            return HAL_OK;
        }

        /* Analisis error */
        uint32_t error = HAL_I2C_GetError(&hi2c1);
        printf("[ERROR] Percobaan %d/%d: %s (error=0x%lX)\r\n",
               attempt, MAX_RETRY, I2C_AnalyzeError(error), error);

        stats.retry_count++;

        /* Jika bus error, lakukan recovery */
        if (error & (HAL_I2C_ERROR_BERR | HAL_I2C_ERROR_ARLO | HAL_I2C_ERROR_TIMEOUT)) {
            printf("[INFO] Bus error terdeteksi, memulai recovery...\r\n");
            I2C_BusRecovery();
        } else {
            /* Untuk error lain, cukup delay dan coba lagi */
            HAL_Delay(RETRY_DELAY_MS);
        }
    }

    printf("[FATAL] Semua percobaan gagal!\r\n");
    return status;
}

/**
 * Tulis register dengan retry dan error recovery otomatis
 */
HAL_StatusTypeDef I2C_WriteWithRetry(uint8_t dev_addr, uint8_t reg_addr,
                                      uint8_t *data, uint16_t len) {
    HAL_StatusTypeDef status;

    for (int attempt = 1; attempt <= MAX_RETRY; attempt++) {
        stats.total_transaksi++;

        status = HAL_I2C_Mem_Write(&hi2c1, (dev_addr << 1), reg_addr,
                                    I2C_MEMADD_SIZE_8BIT, data, len, 100);

        if (status == HAL_OK) {
            stats.sukses++;
            return HAL_OK;
        }

        uint32_t error = HAL_I2C_GetError(&hi2c1);
        printf("[ERROR] Write percobaan %d/%d: %s\r\n",
               attempt, MAX_RETRY, I2C_AnalyzeError(error));

        stats.retry_count++;

        if (error & (HAL_I2C_ERROR_BERR | HAL_I2C_ERROR_ARLO | HAL_I2C_ERROR_TIMEOUT)) {
            I2C_BusRecovery();
        } else {
            HAL_Delay(RETRY_DELAY_MS);
        }
    }

    return status;
}

/**
 * Cetak statistik error
 */
void PrintStats(void) {
    uint32_t total_errors = stats.error_af + stats.error_berr + stats.error_arlo +
                            stats.error_ovr + stats.error_timeout + stats.error_lainnya;
    float success_rate = (stats.total_transaksi > 0) ?
                         (stats.sukses * 100.0f / stats.total_transaksi) : 0;

    printf("\r\n========== STATISTIK I2C ==========\r\n");
    printf("  Total transaksi : %lu\r\n", stats.total_transaksi);
    printf("  Sukses          : %lu (%.1f%%)\r\n", stats.sukses, success_rate);
    printf("  Total error     : %lu\r\n", total_errors);
    printf("    - AF (NACK)   : %lu\r\n", stats.error_af);
    printf("    - BERR        : %lu\r\n", stats.error_berr);
    printf("    - ARLO        : %lu\r\n", stats.error_arlo);
    printf("    - OVR         : %lu\r\n", stats.error_ovr);
    printf("    - Timeout     : %lu\r\n", stats.error_timeout);
    printf("    - Lainnya     : %lu\r\n", stats.error_lainnya);
    printf("  Recovery count  : %lu\r\n", stats.recovery_count);
    printf("  Retry count     : %lu\r\n", stats.retry_count);
    printf("===================================\r\n");

    printf("STATS,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%.1f\r\n",
           stats.total_transaksi, stats.sukses, stats.error_af,
           stats.error_berr, stats.error_arlo, stats.error_timeout,
           stats.recovery_count, stats.retry_count, success_rate);
}

int main(void) {
    HAL_Init();
    SystemClock_Config();
    MX_USART1_Init();
    MX_I2C1_Init();

    printf("\r\n================================================\r\n");
    printf("  STM32 I2C Error Recovery Demo\r\n");
    printf("  Target: 0x%02X, Max retry: %d\r\n", TARGET_ADDR, MAX_RETRY);
    printf("================================================\r\n");

    /* Cek koneksi awal */
    if (HAL_I2C_IsDeviceReady(&hi2c1, TARGET_ADDR_W, 3, 100) != HAL_OK) {
        printf("[WARN] Perangkat 0x%02X tidak ditemukan saat startup\r\n", TARGET_ADDR);
        printf("[INFO] Mencoba recovery...\r\n");
        I2C_BusRecovery();

        if (HAL_I2C_IsDeviceReady(&hi2c1, TARGET_ADDR_W, 3, 100) != HAL_OK) {
            printf("[WARN] Masih tidak terdeteksi. Melanjutkan untuk demonstrasi...\r\n");
        }
    } else {
        printf("[OK] Perangkat 0x%02X terdeteksi\r\n", TARGET_ADDR);
    }

    /* Wake up MPU6050 (jika terhubung) */
    uint8_t val = 0x00;
    I2C_WriteWithRetry(TARGET_ADDR, 0x6B, &val, 1);
    HAL_Delay(100);

    uint32_t pembacaan = 0;
    uint8_t buf[14];

    while (1) {
        pembacaan++;
        printf("\r\n--- Pembacaan #%lu ---\r\n", pembacaan);

        /* Baca data sensor dengan error handling */
        HAL_StatusTypeDef status = I2C_ReadWithRetry(TARGET_ADDR, 0x3B, buf, 14);

        if (status == HAL_OK) {
            int16_t ax = (int16_t)(buf[0]<<8 | buf[1]);
            int16_t ay = (int16_t)(buf[2]<<8 | buf[3]);
            int16_t az = (int16_t)(buf[4]<<8 | buf[5]);
            printf("  Accel: X=%d Y=%d Z=%d\r\n", ax, ay, az);
            printf("DATA,%lu,OK,%.3f,%.3f,%.3f\r\n",
                   pembacaan, ax/16384.0f, ay/16384.0f, az/16384.0f);
        } else {
            printf("DATA,%lu,FAIL,0,0,0\r\n", pembacaan);
        }

        /* Coba baca alamat yang tidak ada (untuk simulasi error) */
        if (pembacaan % 5 == 0) {
            printf("\r\n  [SIMULASI] Mencoba alamat 0x55 (tidak ada perangkat)...\r\n");
            uint8_t dummy;
            HAL_StatusTypeDef sim_status = HAL_I2C_Mem_Read(&hi2c1, (0x55 << 1), 0x00,
                                                             I2C_MEMADD_SIZE_8BIT,
                                                             &dummy, 1, 50);
            if (sim_status != HAL_OK) {
                uint32_t err = HAL_I2C_GetError(&hi2c1);
                printf("  [SIMULASI] Error: %s (0x%lX)\r\n", I2C_AnalyzeError(err), err);
            }
        }

        /* Cetak statistik setiap 10 pembacaan */
        if (pembacaan % 10 == 0) {
            PrintStats();
        }

        HAL_Delay(1000);
    }
}
