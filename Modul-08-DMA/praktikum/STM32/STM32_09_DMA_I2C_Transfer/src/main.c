/**
 * ============================================================================
 * STM32_09_DMA_I2C_Transfer - Transfer I2C menggunakan DMA
 * ============================================================================
 * Deskripsi : Demonstrasi transfer data I2C menggunakan DMA pada STM32F103.
 *             Membandingkan waktu transfer I2C polling vs DMA.
 *             Mode demo: tanpa perangkat I2C eksternal, menunjukkan
 *             inisialisasi dan setup transfer DMA I2C.
 *
 * Fitur     :
 *   - Inisialisasi I2C1 dengan DMA (Channel 6 TX, Channel 7 RX)
 *   - Mode demo simulasi transfer I2C
 *   - Perbandingan timing polling vs DMA
 *   - Callback DMA untuk notifikasi transfer selesai
 *
 * Hardware  :
 *   - STM32F103C8 (Blue Pill) @ 72MHz
 *   - LED pada PC13 (indikator status)
 *   - USART1 PA9(TX)/PA10(RX) @ 115200 baud
 *   - I2C1 PB6(SCL)/PB7(SDA) @ 100kHz
 *
 * Catatan   : Program ini berjalan dalam mode demo tanpa perangkat I2C
 *             eksternal. Transfer ke alamat slave akan menghasilkan NACK,
 *             tapi demonstrasi setup DMA dan timing tetap valid.
 * ============================================================================
 */

#include "stm32f1xx_hal.h"
#include "config.h"
#include <stdio.h>
#include <string.h>

/* ==================== Handle Periferal ==================== */
static UART_HandleTypeDef huart1;
static I2C_HandleTypeDef  hi2c1;
static DMA_HandleTypeDef  hdma_i2c1_tx;
static DMA_HandleTypeDef  hdma_i2c1_rx;

/* ==================== Buffer Data ==================== */
/* Buffer transmit dan receive untuk DMA */
static uint8_t i2c_tx_buffer[I2C_BUFFER_SIZE];
static uint8_t i2c_rx_buffer[I2C_BUFFER_SIZE];

/* Buffer untuk pengujian polling */
static uint8_t poll_tx_buffer[I2C_BUFFER_SIZE];
static uint8_t poll_rx_buffer[I2C_BUFFER_SIZE];

/* ==================== Variabel Status ==================== */
/* Flag status transfer DMA */
static volatile uint8_t dma_tx_complete = 0;
static volatile uint8_t dma_rx_complete = 0;
static volatile uint8_t dma_error_flag  = 0;

/* Statistik transfer */
static uint32_t total_dma_transfers   = 0;
static uint32_t total_poll_transfers  = 0;
static uint32_t dma_error_count       = 0;
static uint32_t poll_error_count      = 0;

/* Nomor tes saat ini */
static uint32_t test_number = 0;

/* ==================== Prototipe Fungsi ==================== */
static void SystemClock_Config(void);
static void GPIO_Init(void);
static void USART1_Init(void);
static void I2C1_Init(void);
static void DMA_Init(void);
static void Error_Handler(void);

/* Fungsi demo dan pengujian */
static void Demo_I2C_DMA_Setup(void);
static void Demo_I2C_Polling_Transfer(uint16_t size);
static void Demo_I2C_DMA_Transfer(uint16_t size);
static void Demo_Compare_Timing(void);
static void Demo_Buffer_Patterns(void);
static void Demo_Multi_Size_Test(void);
static void Fill_Buffer(uint8_t *buf, uint16_t size, uint8_t pattern);
static uint8_t Verify_Buffer(uint8_t *buf, uint16_t size, uint8_t expected);
static void Print_Buffer_Hex(const char *label, uint8_t *buf, uint16_t size);
static void Print_Separator(void);

/* ==================== Retarget printf ke USART1 ==================== */
/**
 * Fungsi _write() digunakan untuk mengarahkan output printf() ke USART1.
 * Setiap karakter dikirim melalui HAL_UART_Transmit secara blocking.
 */
int _write(int file, char *ptr, int len)
{
    (void)file;
    HAL_UART_Transmit(&huart1, (uint8_t *)ptr, len, HAL_MAX_DELAY);
    return len;
}

/* ==================== Interrupt Handlers ==================== */
/**
 * SysTick_Handler - Dipanggil setiap 1ms oleh SysTick timer.
 * Mengelola timing internal HAL (HAL_Delay, timeout, dll).
 */
void SysTick_Handler(void)
{
    HAL_IncTick();
}

/**
 * DMA1_Channel6_IRQHandler - Interrupt DMA untuk I2C1 TX
 * Menangani transfer complete, half transfer, dan error.
 */
void DMA1_Channel6_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&hdma_i2c1_tx);
}

/**
 * DMA1_Channel7_IRQHandler - Interrupt DMA untuk I2C1 RX
 * Menangani penerimaan data via DMA.
 */
void DMA1_Channel7_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&hdma_i2c1_rx);
}

/**
 * I2C1_EV_IRQHandler - Interrupt event I2C1
 * Menangani event I2C seperti address match, transfer complete.
 */
void I2C1_EV_IRQHandler(void)
{
    HAL_I2C_EV_IRQHandler(&hi2c1);
}

/**
 * I2C1_ER_IRQHandler - Interrupt error I2C1
 * Menangani error I2C seperti bus error, arbitration lost.
 */
void I2C1_ER_IRQHandler(void)
{
    HAL_I2C_ER_IRQHandler(&hi2c1);
}

/* ==================== Callback DMA I2C ==================== */
/**
 * HAL_I2C_MasterTxCpltCallback - Callback saat transmit DMA selesai
 * Dipanggil oleh HAL saat seluruh data telah dikirim via DMA.
 */
void HAL_I2C_MasterTxCpltCallback(I2C_HandleTypeDef *hi2c)
{
    if (hi2c->Instance == I2C1)
    {
        dma_tx_complete = 1;
        total_dma_transfers++;
        /* Kedipkan LED sebagai indikator TX selesai */
        HAL_GPIO_TogglePin(LED_PORT, LED_PIN);
    }
}

/**
 * HAL_I2C_MasterRxCpltCallback - Callback saat receive DMA selesai
 * Dipanggil oleh HAL saat seluruh data telah diterima via DMA.
 */
void HAL_I2C_MasterRxCpltCallback(I2C_HandleTypeDef *hi2c)
{
    if (hi2c->Instance == I2C1)
    {
        dma_rx_complete = 1;
        total_dma_transfers++;
    }
}

/**
 * HAL_I2C_ErrorCallback - Callback saat terjadi error I2C
 * Error bisa berupa: NACK, bus error, arbitration lost, dll.
 */
void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c)
{
    if (hi2c->Instance == I2C1)
    {
        dma_error_flag = 1;
        dma_error_count++;
    }
}

/* ==================== Konfigurasi Sistem Clock ==================== */
/**
 * SystemClock_Config - Konfigurasi clock sistem ke 72MHz
 *
 * Sumber clock: HSE 8MHz (kristal eksternal Blue Pill)
 * PLL multiplier: x9 → 72MHz
 * AHB prescaler: /1 → 72MHz (HCLK)
 * APB1 prescaler: /2 → 36MHz (max untuk APB1)
 * APB2 prescaler: /1 → 72MHz
 * Flash latency: 2 wait states (untuk 72MHz)
 */
static void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    /* Konfigurasi HSE dan PLL */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState       = RCC_HSE_ON;
    RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    RCC_OscInitStruct.PLL.PLLState   = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource  = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL     = RCC_PLL_MUL9;

    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }

    /* Konfigurasi bus clock */
    RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                     | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
    {
        Error_Handler();
    }
}

/* ==================== Inisialisasi GPIO ==================== */
/**
 * GPIO_Init - Konfigurasi pin LED PC13
 * LED pada Blue Pill aktif LOW (common anode).
 */
static void GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    LED_GPIO_CLK_ENABLE();

    /* LED PC13 - Output Push-Pull */
    GPIO_InitStruct.Pin   = LED_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_PORT, &GPIO_InitStruct);

    /* LED mati awal (HIGH = mati pada Blue Pill) */
    HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET);
}

/* ==================== Inisialisasi USART1 ==================== */
/**
 * USART1_Init - Konfigurasi USART1 untuk debug output
 * PA9 = TX, PA10 = RX, 115200 baud, 8N1
 */
static void USART1_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    USART_GPIO_CLK_ENABLE();
    USART_CLK_ENABLE();

    /* PA9 = USART1_TX - Alternate Function Push-Pull */
    GPIO_InitStruct.Pin   = USART_TX_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(USART_TX_PORT, &GPIO_InitStruct);

    /* PA10 = USART1_RX - Input Floating */
    GPIO_InitStruct.Pin  = USART_RX_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(USART_RX_PORT, &GPIO_InitStruct);

    /* Konfigurasi USART1 */
    huart1.Instance          = USART1;
    huart1.Init.BaudRate     = USART_BAUDRATE;
    huart1.Init.WordLength   = UART_WORDLENGTH_8B;
    huart1.Init.StopBits     = UART_STOPBITS_1;
    huart1.Init.Parity       = UART_PARITY_NONE;
    huart1.Init.Mode         = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;

    if (HAL_UART_Init(&huart1) != HAL_OK)
    {
        Error_Handler();
    }
}

/* ==================== Inisialisasi DMA ==================== */
/**
 * DMA_Init - Konfigurasi DMA1 untuk I2C1 TX dan RX
 *
 * Channel 6 = I2C1_TX:
 *   - Memory ke Peripheral
 *   - Increment memory address
 *   - Peripheral address tetap (I2C1->DR)
 *
 * Channel 7 = I2C1_RX:
 *   - Peripheral ke Memory
 *   - Increment memory address
 *   - Peripheral address tetap (I2C1->DR)
 */
static void DMA_Init(void)
{
    DMA_CLK_ENABLE();

    /* --- DMA untuk I2C1 TX (Channel 6) --- */
    hdma_i2c1_tx.Instance                 = I2C_TX_DMA_CHANNEL;
    hdma_i2c1_tx.Init.Direction           = DMA_MEMORY_TO_PERIPH;
    hdma_i2c1_tx.Init.PeriphInc           = DMA_PINC_DISABLE;
    hdma_i2c1_tx.Init.MemInc              = DMA_MINC_ENABLE;
    hdma_i2c1_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_i2c1_tx.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
    hdma_i2c1_tx.Init.Mode                = DMA_NORMAL;
    hdma_i2c1_tx.Init.Priority            = DMA_PRIORITY_MEDIUM;

    if (HAL_DMA_Init(&hdma_i2c1_tx) != HAL_OK)
    {
        Error_Handler();
    }

    /* Hubungkan DMA TX ke I2C1 */
    __HAL_LINKDMA(&hi2c1, hdmatx, hdma_i2c1_tx);

    /* --- DMA untuk I2C1 RX (Channel 7) --- */
    hdma_i2c1_rx.Instance                 = I2C_RX_DMA_CHANNEL;
    hdma_i2c1_rx.Init.Direction           = DMA_PERIPH_TO_MEMORY;
    hdma_i2c1_rx.Init.PeriphInc           = DMA_PINC_DISABLE;
    hdma_i2c1_rx.Init.MemInc              = DMA_MINC_ENABLE;
    hdma_i2c1_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_i2c1_rx.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
    hdma_i2c1_rx.Init.Mode                = DMA_NORMAL;
    hdma_i2c1_rx.Init.Priority            = DMA_PRIORITY_HIGH;

    if (HAL_DMA_Init(&hdma_i2c1_rx) != HAL_OK)
    {
        Error_Handler();
    }

    /* Hubungkan DMA RX ke I2C1 */
    __HAL_LINKDMA(&hi2c1, hdmarx, hdma_i2c1_rx);

    /* Aktifkan interrupt DMA */
    HAL_NVIC_SetPriority(I2C_TX_DMA_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(I2C_TX_DMA_IRQn);

    HAL_NVIC_SetPriority(I2C_RX_DMA_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(I2C_RX_DMA_IRQn);
}

/* ==================== Inisialisasi I2C1 ==================== */
/**
 * I2C1_Init - Konfigurasi I2C1 Master mode
 * PB6 = SCL, PB7 = SDA
 * Clock: 100kHz (Standard Mode)
 */
static void I2C1_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    I2C_GPIO_CLK_ENABLE();
    I2C_CLK_ENABLE();

    /* PB6 = I2C1_SCL - Alternate Function Open Drain */
    GPIO_InitStruct.Pin   = I2C_SCL_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(I2C_SCL_PORT, &GPIO_InitStruct);

    /* PB7 = I2C1_SDA - Alternate Function Open Drain */
    GPIO_InitStruct.Pin = I2C_SDA_PIN;
    HAL_GPIO_Init(I2C_SDA_PORT, &GPIO_InitStruct);

    /* Konfigurasi I2C1 */
    hi2c1.Instance             = I2C1;
    hi2c1.Init.ClockSpeed      = I2C_CLOCK_SPEED;
    hi2c1.Init.DutyCycle       = I2C_DUTYCYCLE_2;
    hi2c1.Init.OwnAddress1     = I2C_OWN_ADDRESS;
    hi2c1.Init.AddressingMode  = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.OwnAddress2     = 0;
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode   = I2C_NOSTRETCH_DISABLE;

    if (HAL_I2C_Init(&hi2c1) != HAL_OK)
    {
        Error_Handler();
    }

    /* Aktifkan interrupt I2C untuk event dan error */
    HAL_NVIC_SetPriority(I2C1_EV_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(I2C1_EV_IRQn);

    HAL_NVIC_SetPriority(I2C1_ER_IRQn, 0, 1);
    HAL_NVIC_EnableIRQ(I2C1_ER_IRQn);
}

/* ==================== Fungsi Utilitas ==================== */

/**
 * Fill_Buffer - Isi buffer dengan pola tertentu
 * @param buf     : pointer ke buffer
 * @param size    : ukuran buffer
 * @param pattern : byte pola awal (increment per indeks)
 */
static void Fill_Buffer(uint8_t *buf, uint16_t size, uint8_t pattern)
{
    for (uint16_t i = 0; i < size; i++)
    {
        buf[i] = (uint8_t)(pattern + i);
    }
}

/**
 * Verify_Buffer - Verifikasi isi buffer
 * @return 1 jika cocok, 0 jika berbeda
 */
static uint8_t Verify_Buffer(uint8_t *buf, uint16_t size, uint8_t expected)
{
    for (uint16_t i = 0; i < size; i++)
    {
        if (buf[i] != (uint8_t)(expected + i))
        {
            return 0;
        }
    }
    return 1;
}

/**
 * Print_Buffer_Hex - Cetak isi buffer dalam format heksadesimal
 */
static void Print_Buffer_Hex(const char *label, uint8_t *buf, uint16_t size)
{
    printf("  %s: ", label);
    uint16_t print_size = (size > 16) ? 16 : size;
    for (uint16_t i = 0; i < print_size; i++)
    {
        printf("%02X ", buf[i]);
    }
    if (size > 16) printf("...");
    printf("\r\n");
}

/**
 * Print_Separator - Cetak garis pemisah untuk output serial
 */
static void Print_Separator(void)
{
    printf("================================================\r\n");
}

/* ==================== Demo: Setup DMA I2C ==================== */
/**
 * Demo_I2C_DMA_Setup - Menampilkan konfigurasi DMA I2C yang telah dilakukan.
 * Menjelaskan mapping channel DMA ke I2C1 TX/RX.
 */
static void Demo_I2C_DMA_Setup(void)
{
    test_number++;
    Print_Separator();
    printf("[TEST %lu] Setup DMA I2C\r\n", test_number);
    Print_Separator();

    printf("Konfigurasi I2C1:\r\n");
    printf("  SCL        : PB6\r\n");
    printf("  SDA        : PB7\r\n");
    printf("  Clock      : %d Hz\r\n", I2C_CLOCK_SPEED);
    printf("  Alamat Own : 0x%02X\r\n", I2C_OWN_ADDRESS);
    printf("  Alamat Slave: 0x%02X\r\n", I2C_SLAVE_ADDRESS);
    printf("\r\n");

    printf("Konfigurasi DMA:\r\n");
    printf("  TX Channel : DMA1 Channel 6 (I2C1_TX)\r\n");
    printf("  RX Channel : DMA1 Channel 7 (I2C1_RX)\r\n");
    printf("  TX Mode    : Memory -> Periph, Normal\r\n");
    printf("  RX Mode    : Periph -> Memory, Normal\r\n");
    printf("  Data Width : Byte (8-bit)\r\n");
    printf("  Mem Incr   : Enabled\r\n");
    printf("  Periph Incr: Disabled (fixed DR register)\r\n");
    printf("\r\n");

    /* Cek status I2C */
    uint32_t i2c_state = HAL_I2C_GetState(&hi2c1);
    printf("[DATA] I2C_STATE=%lu,DMA_TX_CH=6,DMA_RX_CH=7,CLK=%d\r\n",
           i2c_state, I2C_CLOCK_SPEED);

    printf("Status I2C1: %s\r\n",
           (i2c_state == HAL_I2C_STATE_READY) ? "READY" : "NOT READY");
    printf("\r\n");
}

/* ==================== Demo: Transfer Polling ==================== */
/**
 * Demo_I2C_Polling_Transfer - Coba transfer I2C dalam mode polling
 * Karena tidak ada slave device, transfer akan menghasilkan error (NACK).
 * Kita catat waktu yang dibutuhkan untuk attempt tersebut.
 *
 * @param size : ukuran data yang akan ditransfer
 */
static void Demo_I2C_Polling_Transfer(uint16_t size)
{
    HAL_StatusTypeDef status;
    uint32_t start_tick, elapsed;

    /* Isi buffer TX dengan pola */
    Fill_Buffer(poll_tx_buffer, size, 0x10);
    memset(poll_rx_buffer, 0, size);

    /* --- Percobaan Transmit Polling --- */
    printf("  [Polling TX] Ukuran=%u bytes, Alamat=0x%02X\r\n",
           size, I2C_SLAVE_ADDRESS);

    start_tick = HAL_GetTick();
    status = HAL_I2C_Master_Transmit(&hi2c1, (I2C_SLAVE_ADDRESS << 1),
                                     poll_tx_buffer, size, POLLING_TIMEOUT_MS);
    elapsed = HAL_GetTick() - start_tick;

    if (status == HAL_OK)
    {
        printf("  [Polling TX] Berhasil dalam %lu ms\r\n", elapsed);
        total_poll_transfers++;
    }
    else
    {
        printf("  [Polling TX] Error (expected - tanpa slave): %d, waktu=%lu ms\r\n",
               status, elapsed);
        poll_error_count++;
        /* Reset I2C dari error state */
        HAL_I2C_DeInit(&hi2c1);
        I2C1_Init();
    }

    printf("[DATA] POLL_TX,size=%u,status=%d,time_ms=%lu\r\n",
           size, status, elapsed);
}

/* ==================== Demo: Transfer DMA ==================== */
/**
 * Demo_I2C_DMA_Transfer - Coba transfer I2C dalam mode DMA
 * Transfer DMA membebaskan CPU selama pengiriman data.
 *
 * @param size : ukuran data yang akan ditransfer
 */
static void Demo_I2C_DMA_Transfer(uint16_t size)
{
    HAL_StatusTypeDef status;
    uint32_t start_tick, elapsed;

    /* Isi buffer TX */
    Fill_Buffer(i2c_tx_buffer, size, 0x20);
    memset(i2c_rx_buffer, 0, size);

    /* Reset flag */
    dma_tx_complete = 0;
    dma_error_flag  = 0;

    /* --- Percobaan Transmit DMA --- */
    printf("  [DMA TX] Ukuran=%u bytes, Alamat=0x%02X\r\n",
           size, I2C_SLAVE_ADDRESS);

    start_tick = HAL_GetTick();
    status = HAL_I2C_Master_Transmit_DMA(&hi2c1, (I2C_SLAVE_ADDRESS << 1),
                                          i2c_tx_buffer, size);

    if (status == HAL_OK)
    {
        /* Tunggu transfer selesai atau error */
        uint32_t timeout = HAL_GetTick() + POLLING_TIMEOUT_MS;
        while (!dma_tx_complete && !dma_error_flag)
        {
            if (HAL_GetTick() > timeout)
            {
                printf("  [DMA TX] Timeout!\r\n");
                break;
            }
        }
        elapsed = HAL_GetTick() - start_tick;

        if (dma_tx_complete)
        {
            printf("  [DMA TX] Berhasil dalam %lu ms\r\n", elapsed);
        }
        else if (dma_error_flag)
        {
            printf("  [DMA TX] Error (expected - tanpa slave): waktu=%lu ms\r\n", elapsed);
            HAL_I2C_DeInit(&hi2c1);
            I2C1_Init();
            DMA_Init();
        }
    }
    else
    {
        elapsed = HAL_GetTick() - start_tick;
        printf("  [DMA TX] Gagal memulai DMA: %d, waktu=%lu ms\r\n",
               status, elapsed);
        dma_error_count++;
        HAL_I2C_DeInit(&hi2c1);
        I2C1_Init();
        DMA_Init();
    }

    printf("[DATA] DMA_TX,size=%u,status=%d,time_ms=%lu,error=%u\r\n",
           size, status, elapsed, dma_error_flag);
}

/* ==================== Demo: Perbandingan Timing ==================== */
/**
 * Demo_Compare_Timing - Bandingkan waktu transfer polling vs DMA
 * untuk beberapa ukuran buffer yang berbeda.
 */
static void Demo_Compare_Timing(void)
{
    test_number++;
    Print_Separator();
    printf("[TEST %lu] Perbandingan Timing: Polling vs DMA\r\n", test_number);
    Print_Separator();

    uint16_t sizes[] = {I2C_SMALL_BUFFER, I2C_MEDIUM_BUFFER, I2C_LARGE_BUFFER};
    const char *labels[] = {"Kecil", "Sedang", "Besar"};

    for (int i = 0; i < 3; i++)
    {
        printf("\n--- Tes Ukuran %s (%u bytes) ---\r\n", labels[i], sizes[i]);

        /* Tes Polling */
        Demo_I2C_Polling_Transfer(sizes[i]);
        HAL_Delay(100);

        /* Tes DMA */
        Demo_I2C_DMA_Transfer(sizes[i]);
        HAL_Delay(100);

        printf("[DATA] COMPARE,size=%u,label=%s\r\n", sizes[i], labels[i]);
    }
}

/* ==================== Demo: Pola Buffer ==================== */
/**
 * Demo_Buffer_Patterns - Demonstrasi persiapan buffer
 * untuk berbagai pola data I2C (register read, burst write, dll).
 */
static void Demo_Buffer_Patterns(void)
{
    test_number++;
    Print_Separator();
    printf("[TEST %lu] Pola Buffer I2C\r\n", test_number);
    Print_Separator();

    /* Pola 1: Register write (alamat register + data) */
    printf("Pola 1: Register Write Simulation\r\n");
    i2c_tx_buffer[0] = 0x6B;  /* Register address (misal PWR_MGMT_1) */
    i2c_tx_buffer[1] = 0x00;  /* Data (wake up) */
    Print_Buffer_Hex("TX Data", i2c_tx_buffer, 2);
    printf("[DATA] PATTERN,type=reg_write,reg=0x6B,data=0x00\r\n");

    /* Pola 2: Burst read preparation */
    printf("\nPola 2: Burst Read Preparation (6 bytes accelerometer)\r\n");
    i2c_tx_buffer[0] = 0x3B;  /* Start register (ACCEL_XOUT_H) */
    Fill_Buffer(i2c_rx_buffer, 6, 0x00);
    Print_Buffer_Hex("TX (reg addr)", i2c_tx_buffer, 1);
    printf("  Akan baca 6 bytes mulai dari register 0x3B\r\n");
    printf("[DATA] PATTERN,type=burst_read,start_reg=0x3B,count=6\r\n");

    /* Pola 3: Konfigurasi multi-register */
    printf("\nPola 3: Multi-Register Configuration\r\n");
    uint8_t config_pairs[][2] = {
        {0x6B, 0x00},  /* PWR_MGMT_1: wake up */
        {0x1B, 0x08},  /* GYRO_CONFIG: 500 dps */
        {0x1C, 0x10},  /* ACCEL_CONFIG: 8g */
        {0x19, 0x07},  /* SMPLRT_DIV: 1kHz */
    };
    for (int i = 0; i < 4; i++)
    {
        printf("  Config[%d]: Reg=0x%02X, Data=0x%02X\r\n",
               i, config_pairs[i][0], config_pairs[i][1]);
    }
    printf("[DATA] PATTERN,type=multi_config,count=4\r\n");

    /* Pola 4: DMA buffer alignment check */
    printf("\nPola 4: Verifikasi Alignment Buffer DMA\r\n");
    printf("  TX buffer addr: 0x%08lX\r\n", (uint32_t)i2c_tx_buffer);
    printf("  RX buffer addr: 0x%08lX\r\n", (uint32_t)i2c_rx_buffer);
    printf("  Buffer size   : %d bytes\r\n", I2C_BUFFER_SIZE);

    uint8_t tx_aligned = ((uint32_t)i2c_tx_buffer % 4 == 0) ? 1 : 0;
    uint8_t rx_aligned = ((uint32_t)i2c_rx_buffer % 4 == 0) ? 1 : 0;
    printf("  TX 4-byte aligned: %s\r\n", tx_aligned ? "Ya" : "Tidak");
    printf("  RX 4-byte aligned: %s\r\n", rx_aligned ? "Ya" : "Tidak");
    printf("[DATA] ALIGNMENT,tx_addr=0x%08lX,rx_addr=0x%08lX,tx_align=%u,rx_align=%u\r\n",
           (uint32_t)i2c_tx_buffer, (uint32_t)i2c_rx_buffer, tx_aligned, rx_aligned);
}

/* ==================== Demo: Multi-Size Test ==================== */
/**
 * Demo_Multi_Size_Test - Tes transfer DMA I2C dengan berbagai ukuran
 * Mengukur overhead setup DMA untuk ukuran data berbeda.
 */
static void Demo_Multi_Size_Test(void)
{
    test_number++;
    Print_Separator();
    printf("[TEST %lu] Transfer Multi-Ukuran DMA I2C\r\n", test_number);
    Print_Separator();

    uint16_t test_sizes[] = {1, 2, 4, 8, 16, 32, 64};
    int num_tests = 7;

    for (int i = 0; i < num_tests; i++)
    {
        uint16_t sz = test_sizes[i];
        printf("\n--- Transfer %u byte(s) ---\r\n", sz);

        /* Isi buffer */
        Fill_Buffer(i2c_tx_buffer, sz, (uint8_t)(0x30 + i));
        Print_Buffer_Hex("Data TX", i2c_tx_buffer, sz);

        /* Reset flag */
        dma_tx_complete = 0;
        dma_error_flag  = 0;

        uint32_t start = HAL_GetTick();
        HAL_StatusTypeDef status = HAL_I2C_Master_Transmit_DMA(
            &hi2c1, (I2C_SLAVE_ADDRESS << 1), i2c_tx_buffer, sz);

        if (status == HAL_OK)
        {
            uint32_t timeout = HAL_GetTick() + POLLING_TIMEOUT_MS;
            while (!dma_tx_complete && !dma_error_flag)
            {
                if (HAL_GetTick() > timeout) break;
            }
        }
        uint32_t elapsed = HAL_GetTick() - start;

        printf("  Status: %s, Waktu: %lu ms\r\n",
               (dma_tx_complete) ? "OK" : "ERROR/NACK", elapsed);
        printf("[DATA] MULTISIZE,size=%u,status=%d,time_ms=%lu\r\n",
               sz, status, elapsed);

        /* Reset I2C jika error */
        if (dma_error_flag || status != HAL_OK)
        {
            HAL_I2C_DeInit(&hi2c1);
            I2C1_Init();
            DMA_Init();
        }

        HAL_Delay(200);
    }
}

/* ==================== Error Handler ==================== */
/**
 * Error_Handler - Penanganan error fatal
 * LED berkedip cepat sebagai indikator error.
 */
static void Error_Handler(void)
{
    __disable_irq();
    while (1)
    {
        HAL_GPIO_TogglePin(LED_PORT, LED_PIN);
        for (volatile uint32_t i = 0; i < 200000; i++);
    }
}

/* ==================== Program Utama ==================== */
/**
 * main - Fungsi utama program DMA I2C Transfer
 *
 * Alur program:
 * 1. Inisialisasi HAL, clock, GPIO, USART, DMA, I2C
 * 2. Demo setup dan konfigurasi DMA I2C
 * 3. Perbandingan timing polling vs DMA
 * 4. Demonstrasi pola buffer I2C
 * 5. Tes transfer multi-ukuran
 * 6. Loop: tampilkan statistik periodik
 */
int main(void)
{
    /* Inisialisasi HAL */
    HAL_Init();

    /* Konfigurasi clock sistem ke 72MHz */
    SystemClock_Config();

    /* Inisialisasi periferal */
    GPIO_Init();
    USART1_Init();
    DMA_Init();
    I2C1_Init();

    /* Banner program */
    printf("\r\n");
    Print_Separator();
    printf("  STM32_09_DMA_I2C_Transfer\r\n");
    printf("  Demonstrasi Transfer I2C dengan DMA\r\n");
    printf("  MCU: STM32F103C8 @ 72MHz\r\n");
    Print_Separator();
    printf("\r\n");

    printf("CATATAN: Program dalam mode DEMO.\r\n");
    printf("Tanpa perangkat I2C slave, transfer akan menghasilkan NACK.\r\n");
    printf("Fokus: Setup DMA, timing overhead, dan pola buffer.\r\n\r\n");

    HAL_Delay(DEMO_DELAY_MS);

    /* --- Jalankan semua demo --- */

    /* Demo 1: Setup dan konfigurasi */
    Demo_I2C_DMA_Setup();
    HAL_Delay(DEMO_DELAY_MS);

    /* Demo 2: Perbandingan timing */
    Demo_Compare_Timing();
    HAL_Delay(DEMO_DELAY_MS);

    /* Demo 3: Pola buffer */
    Demo_Buffer_Patterns();
    HAL_Delay(DEMO_DELAY_MS);

    /* Demo 4: Transfer multi-ukuran */
    Demo_Multi_Size_Test();
    HAL_Delay(DEMO_DELAY_MS);

    /* --- Ringkasan Akhir --- */
    Print_Separator();
    printf("RINGKASAN PENGUJIAN DMA I2C\r\n");
    Print_Separator();
    printf("  Total tes         : %lu\r\n", test_number);
    printf("  DMA transfers     : %lu\r\n", total_dma_transfers);
    printf("  Polling transfers : %lu\r\n", total_poll_transfers);
    printf("  DMA errors        : %lu\r\n", dma_error_count);
    printf("  Polling errors    : %lu\r\n", poll_error_count);
    printf("[DATA] SUMMARY,tests=%lu,dma_ok=%lu,poll_ok=%lu,dma_err=%lu,poll_err=%lu\r\n",
           test_number, total_dma_transfers, total_poll_transfers,
           dma_error_count, poll_error_count);
    printf("\r\n");

    /* Loop utama - laporan periodik */
    uint32_t loop_count = 0;
    uint32_t last_report = HAL_GetTick();

    while (1)
    {
        /* Laporan status setiap 5 detik */
        if (HAL_GetTick() - last_report >= 5000)
        {
            last_report = HAL_GetTick();
            loop_count++;

            /* Cek status I2C */
            uint32_t i2c_state = HAL_I2C_GetState(&hi2c1);
            uint32_t i2c_error = HAL_I2C_GetError(&hi2c1);

            printf("[DATA] STATUS,loop=%lu,i2c_state=%lu,i2c_err=%lu,uptime=%lu\r\n",
                   loop_count, i2c_state, i2c_error, HAL_GetTick() / 1000);

            /* Toggle LED sebagai heartbeat */
            HAL_GPIO_TogglePin(LED_PORT, LED_PIN);
        }

        HAL_Delay(100);
    }
}
