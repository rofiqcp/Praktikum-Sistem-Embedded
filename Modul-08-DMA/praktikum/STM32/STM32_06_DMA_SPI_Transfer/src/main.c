/* ============================================================
 * STM32_06_DMA_SPI_Transfer
 * ============================================================
 * Deskripsi:
 *   Program ini mendemonstrasikan transfer SPI1 menggunakan
 *   DMA pada STM32F103. Fitur utama:
 *   - SPI1 Master mode dengan DMA TX (Channel 3) dan RX (Channel 2)
 *   - Loopback test: hubungkan MOSI (PA7) ke MISO (PA6)
 *   - Perbandingan kecepatan DMA SPI vs Polling SPI
 *   - Pengukuran presisi menggunakan DWT cycle counter
 *
 * Hardware:
 *   - STM32F103C8T6 Blue Pill
 *   - Loopback: PA7 (MOSI) → PA6 (MISO) dihubungkan langsung
 *   - PA5 = SCK (clock)
 *   - PA4 = CS (software chip select)
 *   - UART1: PA9 (TX), PA10 (RX) debug serial
 *
 * Koneksi Loopback:
 *   PA7 ──── PA6   (MOSI → MISO langsung)
 *   PA5 ──── (SCK, biarkan terbuka untuk loopback)
 *   PA4 ──── (CS, dikendalikan software)
 *
 * Catatan Penting:
 *   Pada mode loopback, data yang dikirim via MOSI langsung
 *   masuk ke MISO, sehingga TX buffer == RX buffer setelah
 *   transfer berhasil.
 * ============================================================ */

#include "stm32f1xx_hal.h"
#include "config.h"
#include <stdio.h>
#include <string.h>

/* ============================================================
 * Deklarasi Handle Periferal
 * ============================================================ */
SPI_HandleTypeDef hspi1;            /* Handle SPI1 */
DMA_HandleTypeDef hdma_spi1_rx;     /* Handle DMA1 Channel 2 (SPI1_RX) */
DMA_HandleTypeDef hdma_spi1_tx;     /* Handle DMA1 Channel 3 (SPI1_TX) */
UART_HandleTypeDef huart1;          /* Handle USART1 (debug) */

/* ============================================================
 * Buffer Transfer SPI
 * Buffer TX diisi dengan pola data tertentu
 * Buffer RX menerima data dari loopback MOSI→MISO
 * ============================================================ */
uint8_t spi_tx_buffer[SPI_BUFFER_SIZE];
uint8_t spi_rx_buffer[SPI_BUFFER_SIZE];

/* Flag DMA transfer selesai */
volatile uint8_t spi_tx_complete = 0;
volatile uint8_t spi_rx_complete = 0;
volatile uint8_t spi_txrx_complete = 0;

/* Hasil pengukuran kecepatan */
typedef struct {
    uint32_t size;               /* Ukuran transfer (bytes) */
    uint32_t polling_cycles;     /* Cycle count polling */
    uint32_t dma_cycles;         /* Cycle count DMA */
    float    polling_us;         /* Waktu polling (microseconds) */
    float    dma_us;             /* Waktu DMA (microseconds) */
    float    speedup;            /* Faktor percepatan DMA vs polling */
    uint8_t  data_match;         /* 1 jika TX == RX */
} SpeedTestResult;

/* Array ukuran test yang akan diuji */
static const uint16_t test_sizes[NUM_TEST_SIZES] = {
    TEST_SIZE_1, TEST_SIZE_2, TEST_SIZE_3, TEST_SIZE_4, TEST_SIZE_5
};

/* ============================================================
 * Deklarasi Fungsi Prototipe
 * ============================================================ */
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_SPI1_Init(void);
static void MX_USART1_Init(void);
static void DWT_Init(void);
static uint32_t DWT_GetCycles(void);
static void CS_Select(void);
static void CS_Deselect(void);
static void Fill_Test_Pattern(uint8_t *buffer, uint16_t size, uint8_t pattern);
static uint8_t Verify_Data(uint8_t *tx, uint8_t *rx, uint16_t size);
static SpeedTestResult Run_Speed_Test(uint16_t size);
static void Print_Test_Result(SpeedTestResult *result);
void Error_Handler(void);

/* ============================================================
 * Retarget printf ke UART1
 * ============================================================ */
int _write(int file, char *ptr, int len)
{
    HAL_UART_Transmit(&huart1, (uint8_t *)ptr, len, HAL_MAX_DELAY);
    return len;
}

/* ============================================================
 * DWT Cycle Counter
 * ============================================================ */
static void DWT_Init(void)
{
    DWT_DEMCR_REG |= (1 << 24);
    DWT_CYCCNT_REG = 0;
    DWT_CTRL_REG |= 1;
}

static uint32_t DWT_GetCycles(void)
{
    return DWT_CYCCNT_REG;
}

/* ============================================================
 * Chip Select Control (Software CS)
 * PA4 digunakan sebagai software chip select
 * CS active low: LOW = selected, HIGH = deselected
 * ============================================================ */
static void CS_Select(void)
{
    HAL_GPIO_WritePin(SPI_CS_PORT, SPI_CS_PIN, GPIO_PIN_RESET);
}

static void CS_Deselect(void)
{
    HAL_GPIO_WritePin(SPI_CS_PORT, SPI_CS_PIN, GPIO_PIN_SET);
}

/* ============================================================
 * Fill_Test_Pattern
 * Mengisi buffer dengan pola data untuk pengujian
 * Pola: incrementing byte dengan offset pattern
 * ============================================================ */
static void Fill_Test_Pattern(uint8_t *buffer, uint16_t size, uint8_t pattern)
{
    for (uint16_t i = 0; i < size; i++) {
        buffer[i] = (uint8_t)((i + pattern) & 0xFF);
    }
}

/* ============================================================
 * Verify_Data
 * Membandingkan buffer TX dan RX byte per byte
 * Return: 1 jika cocok, 0 jika tidak cocok
 * ============================================================ */
static uint8_t Verify_Data(uint8_t *tx, uint8_t *rx, uint16_t size)
{
    for (uint16_t i = 0; i < size; i++) {
        if (tx[i] != rx[i]) {
            printf("[ERROR] Data mismatch di index %d: TX=0x%02X, RX=0x%02X\r\n",
                   i, tx[i], rx[i]);
            return 0;
        }
    }
    return 1;
}

/* ============================================================
 * DMA Callbacks
 * ============================================================ */

/* Callback SPI TX+RX DMA selesai */
void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi->Instance == SPI_INSTANCE) {
        spi_txrx_complete = 1;
    }
}

/* Callback SPI TX DMA selesai */
void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi->Instance == SPI_INSTANCE) {
        spi_tx_complete = 1;
    }
}

/* Callback SPI RX DMA selesai */
void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi->Instance == SPI_INSTANCE) {
        spi_rx_complete = 1;
    }
}

/* ============================================================
 * Run_Speed_Test
 * Menjalankan perbandingan kecepatan polling vs DMA
 * untuk ukuran data tertentu
 * ============================================================ */
static SpeedTestResult Run_Speed_Test(uint16_t size)
{
    SpeedTestResult result;
    result.size = size;
    uint32_t start, end;
    uint32_t polling_total = 0, dma_total = 0;

    printf("[INFO] Test ukuran %u bytes...\r\n", size);

    /* --------------------------------------------------------
     * Test 1: SPI Polling Transfer
     * Menggunakan HAL_SPI_TransmitReceive() yang blocking
     * CPU sibuk menunggu setiap byte selesai
     * -------------------------------------------------------- */
    for (int iter = 0; iter < NUM_ITERATIONS; iter++) {
        /* Isi buffer TX dengan pola baru setiap iterasi */
        Fill_Test_Pattern(spi_tx_buffer, size, (uint8_t)iter);
        memset(spi_rx_buffer, 0, size);

        CS_Select();

        /* Ukur waktu polling transfer */
        start = DWT_GetCycles();
        HAL_SPI_TransmitReceive(&hspi1, spi_tx_buffer, spi_rx_buffer,
                                size, DMA_TIMEOUT_MS);
        end = DWT_GetCycles();

        CS_Deselect();
        polling_total += (end - start);
    }
    result.polling_cycles = polling_total / NUM_ITERATIONS;

    /* --------------------------------------------------------
     * Test 2: SPI DMA Transfer
     * Menggunakan HAL_SPI_TransmitReceive_DMA() yang non-blocking
     * CPU bebas melakukan hal lain selama transfer
     * -------------------------------------------------------- */
    for (int iter = 0; iter < NUM_ITERATIONS; iter++) {
        Fill_Test_Pattern(spi_tx_buffer, size, (uint8_t)(iter + 100));
        memset(spi_rx_buffer, 0, size);
        spi_txrx_complete = 0;

        CS_Select();

        /* Ukur waktu DMA transfer */
        start = DWT_GetCycles();
        HAL_SPI_TransmitReceive_DMA(&hspi1, spi_tx_buffer, spi_rx_buffer, size);

        /* Tunggu DMA selesai */
        while (!spi_txrx_complete) {
            /* Dalam aplikasi nyata, CPU bisa mengerjakan tugas lain di sini */
        }
        end = DWT_GetCycles();

        CS_Deselect();
        dma_total += (end - start);
    }
    result.dma_cycles = dma_total / NUM_ITERATIONS;

    /* Verifikasi data terakhir */
    result.data_match = Verify_Data(spi_tx_buffer, spi_rx_buffer, size);

    /* Hitung waktu dalam microseconds (72 MHz clock) */
    result.polling_us = (float)result.polling_cycles / (SYSCLK_FREQ_HZ / 1000000);
    result.dma_us = (float)result.dma_cycles / (SYSCLK_FREQ_HZ / 1000000);

    /* Hitung faktor percepatan */
    if (result.dma_cycles > 0) {
        result.speedup = (float)result.polling_cycles / (float)result.dma_cycles;
    } else {
        result.speedup = 0.0f;
    }

    return result;
}

/* ============================================================
 * Print_Test_Result
 * Cetak hasil pengujian dengan format [DATA] untuk parsing
 * ============================================================ */
static void Print_Test_Result(SpeedTestResult *result)
{
    printf("[DATA] SIZE=%lu POLL_CYC=%lu DMA_CYC=%lu "
           "POLL_US=%.1f DMA_US=%.1f SPEEDUP=%.2f MATCH=%u\r\n",
           result->size, result->polling_cycles, result->dma_cycles,
           result->polling_us, result->dma_us,
           result->speedup, result->data_match);

    /* Hitung throughput (KB/s) */
    float poll_kbps = 0, dma_kbps = 0;
    if (result->polling_us > 0) {
        poll_kbps = (result->size * 1000.0f) / result->polling_us;
    }
    if (result->dma_us > 0) {
        dma_kbps = (result->size * 1000.0f) / result->dma_us;
    }
    printf("[DATA] THROUGHPUT SIZE=%lu POLL_KBPS=%.1f DMA_KBPS=%.1f\r\n",
           result->size, poll_kbps, dma_kbps);
}

/* ============================================================
 * MAIN
 * ============================================================ */
int main(void)
{
    HAL_Init();
    SystemClock_Config();

    MX_GPIO_Init();
    MX_DMA_Init();
    MX_SPI1_Init();
    MX_USART1_Init();
    DWT_Init();

    /* LED ON sebentar sebagai tanda mulai */
    LED_ON();
    HAL_Delay(200);
    LED_OFF();

    /* Cetak header */
    printf("\r\n========================================\r\n");
    printf("STM32_06_DMA_SPI_Transfer\r\n");
    printf("========================================\r\n");
    printf("Target     : STM32F103C8T6 Blue Pill\r\n");
    printf("SYSCLK     : %lu MHz\r\n", SYSCLK_FREQ_HZ / 1000000);
    printf("SPI1       : Master, %lu MHz clock\r\n",
           SYSCLK_FREQ_HZ / 8 / 1000000);  /* Prescaler /8 */
    printf("  SCK      : PA5\r\n");
    printf("  MOSI     : PA7\r\n");
    printf("  MISO     : PA6 (loopback dari MOSI)\r\n");
    printf("  CS       : PA4 (software)\r\n");
    printf("DMA TX     : DMA1 Channel 3\r\n");
    printf("DMA RX     : DMA1 Channel 2\r\n");
    printf("Buffer     : %d bytes\r\n", SPI_BUFFER_SIZE);
    printf("Iterasi    : %d per test\r\n", NUM_ITERATIONS);
    printf("UART       : %d baud (PA9/PA10)\r\n", DEBUG_UART_BAUD);
    printf("========================================\r\n");
    printf("\r\n[INFO] PENTING: Hubungkan PA7 (MOSI) ke PA6 (MISO)\r\n");
    printf("[INFO] untuk mode loopback!\r\n\r\n");

    /* ============================================================
     * Test Awal: Verifikasi loopback berfungsi
     * ============================================================ */
    printf("[INFO] === Tes Loopback Dasar ===\r\n");
    Fill_Test_Pattern(spi_tx_buffer, 8, 0xAA);
    memset(spi_rx_buffer, 0, 8);
    CS_Select();
    HAL_SPI_TransmitReceive(&hspi1, spi_tx_buffer, spi_rx_buffer, 8, 1000);
    CS_Deselect();

    printf("[INFO] TX: ");
    for (int i = 0; i < 8; i++) printf("%02X ", spi_tx_buffer[i]);
    printf("\r\n[INFO] RX: ");
    for (int i = 0; i < 8; i++) printf("%02X ", spi_rx_buffer[i]);
    printf("\r\n");

    if (Verify_Data(spi_tx_buffer, spi_rx_buffer, 8)) {
        printf("[INFO] Loopback OK! Data TX == RX\r\n\r\n");
    } else {
        printf("[WARN] Loopback GAGAL! Periksa koneksi PA7→PA6\r\n");
        printf("[WARN] Melanjutkan test tetapi data mungkin tidak cocok\r\n\r\n");
    }

    /* Variabel untuk loop utama */
    uint32_t test_round = 0;

    /* ============================================================
     * Loop Utama - Jalankan test berulang
     * ============================================================ */
    while (1)
    {
        test_round++;
        printf("\r\n========================================\r\n");
        printf("[INFO] === Ronde Test #%lu ===\r\n", test_round);
        printf("========================================\r\n");

        /* Simpan semua hasil untuk ringkasan */
        SpeedTestResult results[NUM_TEST_SIZES];

        /* --------------------------------------------------------
         * Jalankan test untuk setiap ukuran data
         * -------------------------------------------------------- */
        for (int i = 0; i < NUM_TEST_SIZES; i++) {
            results[i] = Run_Speed_Test(test_sizes[i]);
            Print_Test_Result(&results[i]);
            LED_TOGGLE();
            HAL_Delay(100);
        }

        /* --------------------------------------------------------
         * Cetak ringkasan perbandingan
         * -------------------------------------------------------- */
        printf("\r\n[DATA] SUMMARY ROUND=%lu\r\n", test_round);
        printf("[INFO] +---------+-----------+-----------+---------+-------+\r\n");
        printf("[INFO] |  Size   | Poll(us)  |  DMA(us)  | Speedup | Match |\r\n");
        printf("[INFO] +---------+-----------+-----------+---------+-------+\r\n");
        for (int i = 0; i < NUM_TEST_SIZES; i++) {
            printf("[INFO] | %4lu B  | %8.1f  | %8.1f  |  %5.2fx | %s  |\r\n",
                   results[i].size, results[i].polling_us, results[i].dma_us,
                   results[i].speedup,
                   results[i].data_match ? " OK " : "FAIL");
        }
        printf("[INFO] +---------+-----------+-----------+---------+-------+\r\n");

        /* Tunggu sebelum ronde berikutnya */
        printf("\r\n[INFO] Menunggu %d ms sebelum ronde berikutnya...\r\n",
               PRINT_INTERVAL_MS);
        HAL_Delay(PRINT_INTERVAL_MS);
    }
}

/* ============================================================
 * SystemClock_Config
 * HSE 8MHz → PLL x9 → SYSCLK 72MHz
 * ============================================================ */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    /* Konfigurasi HSE dan PLL */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }

    /* Konfigurasi bus clock */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) {
        Error_Handler();
    }
}

/* ============================================================
 * MX_GPIO_Init
 * - PC13: LED output
 * - PA4: SPI CS output (software)
 * ============================================================ */
static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    /* LED PC13 */
    GPIO_InitStruct.Pin = LED_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_PORT, &GPIO_InitStruct);
    LED_OFF();

    /* SPI CS (PA4) - Software chip select */
    GPIO_InitStruct.Pin = SPI_CS_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(SPI_CS_PORT, &GPIO_InitStruct);
    CS_Deselect();  /* CS high (deselected) saat idle */
}

/* ============================================================
 * MX_DMA_Init
 * Inisialisasi DMA1 untuk SPI1:
 *   - Channel 2: SPI1_RX
 *   - Channel 3: SPI1_TX
 * ============================================================ */
static void MX_DMA_Init(void)
{
    __HAL_RCC_DMA1_CLK_ENABLE();

    /* NVIC untuk DMA1 Channel 2 (SPI1_RX) */
    HAL_NVIC_SetPriority(SPI_RX_DMA_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(SPI_RX_DMA_IRQn);

    /* NVIC untuk DMA1 Channel 3 (SPI1_TX) */
    HAL_NVIC_SetPriority(SPI_TX_DMA_IRQn, 0, 1);
    HAL_NVIC_EnableIRQ(SPI_TX_DMA_IRQn);
}

/* ============================================================
 * MX_SPI1_Init
 * Konfigurasi SPI1 Master:
 *   - Full duplex
 *   - 8-bit data
 *   - Software NSS
 *   - Prescaler /8 → 9 MHz SPI clock
 *   - CPOL=0, CPHA=0 (Mode 0)
 * ============================================================ */
static void MX_SPI1_Init(void)
{
    hspi1.Instance = SPI_INSTANCE;
    hspi1.Init.Mode = SPI_MODE_MASTER;
    hspi1.Init.Direction = SPI_DIRECTION_2LINES;        /* Full duplex */
    hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
    hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;           /* CPOL = 0 */
    hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;               /* CPHA = 0 */
    hspi1.Init.NSS = SPI_NSS_SOFT;                       /* Software CS */
    hspi1.Init.BaudRatePrescaler = SPI_PRESCALER;        /* /8 = 9MHz */
    hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
    hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
    hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    hspi1.Init.CRCPolynomial = 10;
    if (HAL_SPI_Init(&hspi1) != HAL_OK) {
        Error_Handler();
    }
}

/* ============================================================
 * HAL_SPI_MspInit
 * Inisialisasi low-level SPI1:
 *   - Clock SPI1 dan GPIOA
 *   - PA5 (SCK): AF push-pull
 *   - PA7 (MOSI): AF push-pull
 *   - PA6 (MISO): Input floating
 *   - DMA Channel 2 (RX) dan Channel 3 (TX)
 * ============================================================ */
void HAL_SPI_MspInit(SPI_HandleTypeDef *hspi)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    if (hspi->Instance == SPI_INSTANCE) {
        /* Aktifkan clock */
        __HAL_RCC_SPI1_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();

        /* PA5 (SCK) dan PA7 (MOSI): AF push-pull, high speed */
        GPIO_InitStruct.Pin = SPI_SCK_PIN | SPI_MOSI_PIN;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
        HAL_GPIO_Init(SPI_GPIO_PORT, &GPIO_InitStruct);

        /* PA6 (MISO): Input floating */
        GPIO_InitStruct.Pin = SPI_MISO_PIN;
        GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(SPI_GPIO_PORT, &GPIO_InitStruct);

        /* DMA1 Channel 2 - SPI1_RX */
        hdma_spi1_rx.Instance = SPI_RX_DMA_CHANNEL;
        hdma_spi1_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
        hdma_spi1_rx.Init.PeriphInc = DMA_PINC_DISABLE;
        hdma_spi1_rx.Init.MemInc = DMA_MINC_ENABLE;
        hdma_spi1_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
        hdma_spi1_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
        hdma_spi1_rx.Init.Mode = DMA_NORMAL;
        hdma_spi1_rx.Init.Priority = DMA_PRIORITY_HIGH;
        if (HAL_DMA_Init(&hdma_spi1_rx) != HAL_OK) {
            Error_Handler();
        }
        __HAL_LINKDMA(hspi, hdmarx, hdma_spi1_rx);

        /* DMA1 Channel 3 - SPI1_TX */
        hdma_spi1_tx.Instance = SPI_TX_DMA_CHANNEL;
        hdma_spi1_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
        hdma_spi1_tx.Init.PeriphInc = DMA_PINC_DISABLE;
        hdma_spi1_tx.Init.MemInc = DMA_MINC_ENABLE;
        hdma_spi1_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
        hdma_spi1_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
        hdma_spi1_tx.Init.Mode = DMA_NORMAL;
        hdma_spi1_tx.Init.Priority = DMA_PRIORITY_MEDIUM;
        if (HAL_DMA_Init(&hdma_spi1_tx) != HAL_OK) {
            Error_Handler();
        }
        __HAL_LINKDMA(hspi, hdmatx, hdma_spi1_tx);
    }
}

/* ============================================================
 * MX_USART1_Init
 * ============================================================ */
static void MX_USART1_Init(void)
{
    huart1.Instance = DEBUG_UART;
    huart1.Init.BaudRate = DEBUG_UART_BAUD;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart1) != HAL_OK) {
        Error_Handler();
    }
}

/* ============================================================
 * HAL_UART_MspInit
 * ============================================================ */
void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    if (huart->Instance == USART1) {
        __HAL_RCC_USART1_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();

        /* PA9 TX: AF push-pull */
        GPIO_InitStruct.Pin = DEBUG_UART_TX_PIN;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
        HAL_GPIO_Init(DEBUG_UART_PORT, &GPIO_InitStruct);

        /* PA10 RX: Input floating */
        GPIO_InitStruct.Pin = DEBUG_UART_RX_PIN;
        GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(DEBUG_UART_PORT, &GPIO_InitStruct);
    }
}

/* ============================================================
 * Interrupt Handlers
 * ============================================================ */
void SysTick_Handler(void)
{
    HAL_IncTick();
}

void DMA1_Channel2_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&hdma_spi1_rx);
}

void DMA1_Channel3_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&hdma_spi1_tx);
}

/* ============================================================
 * Error_Handler
 * ============================================================ */
void Error_Handler(void)
{
    printf("[ERROR] Error_Handler dipanggil! Sistem berhenti.\r\n");
    __disable_irq();
    while (1) {
        LED_TOGGLE();
        for (volatile uint32_t i = 0; i < 200000; i++);
    }
}
