/**
 * ==========================================================
 *  Modul 06 - STM32_05_I2C_EEPROM_AT24C32
 *  Membaca dan menulis EEPROM AT24C32 via I2C
 * ==========================================================
 *  Deskripsi:
 *    AT24C32: 4KB EEPROM dengan alamat 16-bit.
 *    Page write (32 byte), sequential read, verifikasi data.
 *    Menggunakan I2C_MEMADD_SIZE_16BIT untuk alamat 2-byte.
 *  Koneksi:
 *    PB6 = SCL, PB7 = SDA (I2C1)
 *    AT24C32 address: 0x50
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

#define AT24C32_ADDR        0x50
#define AT24C32_ADDR_WRITE  (AT24C32_ADDR << 1)
#define AT24C32_PAGE_SIZE   32    /* Ukuran page AT24C32: 32 byte */
#define AT24C32_TOTAL_SIZE  4096  /* Total kapasitas: 4KB */

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

/* =================== AT24C32 EEPROM Driver =================== */

/**
 * Tulis satu byte ke EEPROM
 * Alamat 16-bit (I2C_MEMADD_SIZE_16BIT)
 */
HAL_StatusTypeDef EEPROM_WriteByte(uint16_t alamat, uint8_t data) {
    HAL_StatusTypeDef status;
    status = HAL_I2C_Mem_Write(&hi2c1, AT24C32_ADDR_WRITE, alamat,
                                I2C_MEMADD_SIZE_16BIT, &data, 1, HAL_MAX_DELAY);
    HAL_Delay(5); /* Tunggu write cycle selesai (max 5ms) */
    return status;
}

/**
 * Baca satu byte dari EEPROM
 */
HAL_StatusTypeDef EEPROM_ReadByte(uint16_t alamat, uint8_t *data) {
    return HAL_I2C_Mem_Read(&hi2c1, AT24C32_ADDR_WRITE, alamat,
                             I2C_MEMADD_SIZE_16BIT, data, 1, HAL_MAX_DELAY);
}

/**
 * Page write: tulis hingga 32 byte sekaligus
 * Data harus tidak melewati batas page
 */
HAL_StatusTypeDef EEPROM_WritePage(uint16_t alamat, uint8_t *data, uint16_t panjang) {
    /* Pastikan tidak melewati batas page */
    uint16_t page_offset = alamat % AT24C32_PAGE_SIZE;
    uint16_t space_in_page = AT24C32_PAGE_SIZE - page_offset;

    if (panjang > space_in_page) {
        panjang = space_in_page;
    }

    HAL_StatusTypeDef status;
    status = HAL_I2C_Mem_Write(&hi2c1, AT24C32_ADDR_WRITE, alamat,
                                I2C_MEMADD_SIZE_16BIT, data, panjang, HAL_MAX_DELAY);
    HAL_Delay(5); /* Tunggu write cycle */
    return status;
}

/**
 * Tulis buffer besar ke EEPROM (otomatis memecah menjadi page writes)
 */
HAL_StatusTypeDef EEPROM_WriteBuffer(uint16_t alamat, uint8_t *data, uint16_t panjang) {
    uint16_t offset = 0;

    while (offset < panjang) {
        uint16_t addr = alamat + offset;
        uint16_t page_offset = addr % AT24C32_PAGE_SIZE;
        uint16_t space_in_page = AT24C32_PAGE_SIZE - page_offset;
        uint16_t chunk = (panjang - offset);
        if (chunk > space_in_page) chunk = space_in_page;

        HAL_StatusTypeDef status = HAL_I2C_Mem_Write(&hi2c1, AT24C32_ADDR_WRITE,
                                    addr, I2C_MEMADD_SIZE_16BIT,
                                    &data[offset], chunk, HAL_MAX_DELAY);
        if (status != HAL_OK) return status;
        HAL_Delay(5);
        offset += chunk;
    }
    return HAL_OK;
}

/**
 * Sequential read: baca beberapa byte berurutan
 */
HAL_StatusTypeDef EEPROM_ReadBuffer(uint16_t alamat, uint8_t *data, uint16_t panjang) {
    return HAL_I2C_Mem_Read(&hi2c1, AT24C32_ADDR_WRITE, alamat,
                             I2C_MEMADD_SIZE_16BIT, data, panjang, HAL_MAX_DELAY);
}

/**
 * Dump isi EEPROM dalam format hex
 */
void EEPROM_HexDump(uint16_t alamat, uint16_t panjang) {
    uint8_t buf[16];
    printf("\r\n--- Hex Dump dari 0x%04X ---\r\n", alamat);
    printf("Addr   00 01 02 03 04 05 06 07  08 09 0A 0B 0C 0D 0E 0F  ASCII\r\n");
    printf("-----  -----------------------------------------------  ----------------\r\n");

    for (uint16_t i = 0; i < panjang; i += 16) {
        uint16_t len = (panjang - i) > 16 ? 16 : (panjang - i);
        EEPROM_ReadBuffer(alamat + i, buf, len);

        printf("0x%04X ", alamat + i);
        for (uint16_t j = 0; j < 16; j++) {
            if (j == 8) printf(" ");
            if (j < len) printf("%02X ", buf[j]);
            else printf("   ");
        }
        printf(" ");
        for (uint16_t j = 0; j < len; j++) {
            printf("%c", (buf[j] >= 32 && buf[j] < 127) ? buf[j] : '.');
        }
        printf("\r\n");
    }
}

int main(void) {
    HAL_Init();
    SystemClock_Config();
    MX_USART1_Init();
    MX_I2C1_Init();

    printf("\r\n===================================\r\n");
    printf("  STM32 AT24C32 EEPROM I2C\r\n");
    printf("  Alamat: 0x50, Kapasitas: 4KB\r\n");
    printf("===================================\r\n");

    /* Cek koneksi */
    if (HAL_I2C_IsDeviceReady(&hi2c1, AT24C32_ADDR_WRITE, 3, 100) != HAL_OK) {
        printf("[ERROR] AT24C32 tidak ditemukan!\r\n");
        while (1) { HAL_Delay(1000); }
    }
    printf("[OK] AT24C32 terdeteksi\r\n");

    /* === Test 1: Tulis dan baca byte tunggal === */
    printf("\r\n--- Test 1: Byte Write/Read ---\r\n");
    uint16_t test_addr = 0x0000;
    uint8_t test_data = 0xA5;
    uint8_t read_data = 0;

    printf("Menulis 0x%02X ke alamat 0x%04X...\r\n", test_data, test_addr);
    EEPROM_WriteByte(test_addr, test_data);

    EEPROM_ReadByte(test_addr, &read_data);
    printf("Membaca dari 0x%04X: 0x%02X\r\n", test_addr, read_data);
    printf("Verifikasi: %s\r\n", (read_data == test_data) ? "BERHASIL" : "GAGAL");

    /* === Test 2: Page Write === */
    printf("\r\n--- Test 2: Page Write ---\r\n");
    uint8_t page_data[32];
    uint8_t page_read[32];

    /* Isi data test: 0x00 - 0x1F */
    for (int i = 0; i < 32; i++) {
        page_data[i] = (uint8_t)i;
    }

    printf("Menulis 32 byte ke alamat 0x0020...\r\n");
    EEPROM_WritePage(0x0020, page_data, 32);

    printf("Membaca 32 byte dari alamat 0x0020...\r\n");
    EEPROM_ReadBuffer(0x0020, page_read, 32);

    int errors = 0;
    for (int i = 0; i < 32; i++) {
        if (page_read[i] != page_data[i]) {
            printf("  MISMATCH di offset %d: tulis=0x%02X baca=0x%02X\r\n",
                   i, page_data[i], page_read[i]);
            errors++;
        }
    }
    printf("Verifikasi page write: %d error dari 32 byte\r\n", errors);

    /* === Test 3: String Write === */
    printf("\r\n--- Test 3: String Write ---\r\n");
    const char *pesan = "Hello STM32 EEPROM!";
    uint16_t str_addr = 0x0100;
    char str_read[32] = {0};

    printf("Menulis string: \"%s\"\r\n", pesan);
    EEPROM_WriteBuffer(str_addr, (uint8_t*)pesan, strlen(pesan) + 1);

    EEPROM_ReadBuffer(str_addr, (uint8_t*)str_read, strlen(pesan) + 1);
    printf("Membaca string: \"%s\"\r\n", str_read);
    printf("Verifikasi: %s\r\n", (strcmp(pesan, str_read) == 0) ? "BERHASIL" : "GAGAL");

    /* === Test 4: Hex Dump === */
    EEPROM_HexDump(0x0000, 64);
    EEPROM_HexDump(0x0100, 32);

    /* === Test 5: Tulis berulang dengan counter === */
    printf("\r\n--- Test 5: Counter Persist ---\r\n");
    uint32_t counter;
    uint16_t counter_addr = 0x0200;

    /* Baca counter yang tersimpan */
    EEPROM_ReadBuffer(counter_addr, (uint8_t*)&counter, sizeof(counter));
    printf("Counter tersimpan: %lu\r\n", counter);

    /* Jika nilai tidak valid, mulai dari 0 */
    if (counter > 1000000) counter = 0;

    while (1) {
        counter++;

        /* Simpan counter ke EEPROM setiap 10 iterasi */
        if (counter % 10 == 0) {
            EEPROM_WriteBuffer(counter_addr, (uint8_t*)&counter, sizeof(counter));
            printf("SAVE,%lu,0x%04X\r\n", counter, counter_addr);
        }

        printf("DATA,counter=%lu\r\n", counter);
        HAL_Delay(1000);
    }
}
