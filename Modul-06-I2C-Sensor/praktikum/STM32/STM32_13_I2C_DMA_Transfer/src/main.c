/**
 * ============================================================================
 * PROGRAM STM32_13: I2C DMA Transfer (Fitur Khusus STM32)
 * ============================================================================
 *
 * FITUR KHUSUS STM32 YANG TIDAK ADA DI ESP32:
 * ============================================
 * STM32 memiliki hardware DMA (Direct Memory Access) yang terintegrasi
 * dengan peripheral I2C. Ini memungkinkan transfer data I2C TANPA
 * menggunakan CPU sama sekali:
 *
 *   1. CPU mengkonfigurasi DMA: sumber, tujuan, jumlah byte
 *   2. CPU memulai transfer DMA
 *   3. DMA controller menangani seluruh transfer I2C byte-per-byte
 *   4. CPU BEBAS melakukan pekerjaan lain selama transfer berlangsung
 *   5. DMA menghasilkan interrupt ketika transfer selesai
 *   6. CPU mendapat notifikasi via callback
 *
 * Pada ESP32:
 *   - I2C TIDAK memiliki mode DMA
 *   - Setiap byte harus ditangani oleh CPU (polling atau interrupt)
 *   - CPU tidak bisa melakukan hal lain selama transfer I2C
 *   - Untuk transfer data besar, CPU blocking time signifikan
 *
 * FUNGSI HAL KHUSUS STM32:
 *   - HAL_I2C_Mem_Read_DMA()   : Baca memori via I2C dengan DMA
 *   - HAL_I2C_Mem_Write_DMA()  : Tulis memori via I2C dengan DMA
 *   - HAL_I2C_MemRxCpltCallback() : Callback saat DMA read selesai
 *   - HAL_I2C_MemTxCpltCallback() : Callback saat DMA write selesai
 *   - HAL_I2C_IsDeviceReady()  : Cek apakah device I2C siap (ACK polling)
 *
 * KONFIGURASI HARDWARE:
 *   - PB6 : I2C1_SCL
 *   - PB7 : I2C1_SDA
 *   - PA2 : USART2 TX
 *   - PA3 : USART2 RX
 *   - PC13: LED indikator
 *
 * TARGET DEVICE: AT24C32 EEPROM
 *   - Alamat I2C: 0x50 (7-bit) = 0xA0 (8-bit HAL)
 *   - Kapasitas : 32Kbit = 4096 bytes
 *   - Page size : 32 bytes
 *   - Alamat memori: 16-bit (0x0000 - 0x0FFF)
 *   - Write cycle time: ~5ms
 *
 * DMA CHANNEL MAPPING:
 *   F103: DMA1_Channel6 (I2C1_TX), DMA1_Channel7 (I2C1_RX)
 *   F401: DMA1_Stream6/Ch1 (I2C1_TX), DMA1_Stream0/Ch1 (I2C1_RX)
 *   F411: DMA1_Stream6/Ch1 (I2C1_TX), DMA1_Stream0/Ch1 (I2C1_RX)
 * ============================================================================
 */

#include <stdio.h>
#include <string.h>

#if defined(STM32F103xB)
  #include "stm32f1xx_hal.h"
#elif defined(STM32F401xC) || defined(STM32F411xE)
  #include "stm32f4xx_hal.h"
#else
  #error "Board tidak didukung!"
#endif

/* ---- Konfigurasi AT24C32 EEPROM ---- */
#define EEPROM_ADDR         0xA0     /* Alamat 8-bit (0x50 << 1) */
#define EEPROM_PAGE_SIZE    32       /* Bytes per page */
#define EEPROM_MEM_SIZE     I2C_MEMADD_SIZE_16BIT  /* Alamat 16-bit */
#define EEPROM_WRITE_TIME   5        /* Write cycle time (ms) */

/* ---- Test data sizes untuk perbandingan ---- */
#define TEST_SIZE_1          8
#define TEST_SIZE_2         32
#define TEST_SIZE_3        128
#define TEST_SIZE_MAX      256

/* ---- Handle Peripheral ---- */
static UART_HandleTypeDef  huart2;
static I2C_HandleTypeDef   hi2c1;
static DMA_HandleTypeDef   hdma_i2c1_rx;
static DMA_HandleTypeDef   hdma_i2c1_tx;

/* ---- Buffer Data ---- */
static uint8_t tx_buffer[TEST_SIZE_MAX];
static uint8_t rx_buffer[TEST_SIZE_MAX];

/* ---- Flag DMA completion ---- */
static volatile uint8_t  g_dma_tx_complete = 0;
static volatile uint8_t  g_dma_rx_complete = 0;
static volatile uint8_t  g_dma_error       = 0;
static volatile uint32_t g_dma_tx_time     = 0;
static volatile uint32_t g_dma_rx_time     = 0;
static volatile uint32_t g_dma_start_tick  = 0;

/* ---- Deklarasi Fungsi ---- */
static void SystemClock_Config(void);
static void GPIO_Init(void);
static void UART2_Init(void);
static void I2C1_Init(void);
static void DMA_Init(void);
static void DWT_Init(void);
static uint32_t DWT_GetMicros(void);
static void Print_Header(void);
static uint8_t EEPROM_IsReady(uint32_t trials);
static void EEPROM_WritePage_Polling(uint16_t addr, uint8_t *data, uint16_t len);
static void EEPROM_ReadPage_Polling(uint16_t addr, uint8_t *data, uint16_t len);
static void Demo_DeviceDetection(void);
static void Demo_PollingTransfer(uint16_t size);
static void Demo_InterruptTransfer(uint16_t size);
static void Demo_DMATransfer(uint16_t size);
static void Demo_Comparison(void);
static void Demo_CPUFreeTime(void);
static void Print_Results(void);

/* ===========================================================================
 * Redirect printf ke USART2
 * =========================================================================== */
int _write(int file, char *ptr, int len) {
    HAL_UART_Transmit(&huart2, (uint8_t *)ptr, len, HAL_MAX_DELAY);
    return len;
}

/* ===========================================================================
 * Interrupt Handlers
 * =========================================================================== */
void SysTick_Handler(void)  { HAL_IncTick(); }
void NMI_Handler(void)      { }
void HardFault_Handler(void){ while (1); }

/* ---- I2C1 Event & Error IRQ ---- */
void I2C1_EV_IRQHandler(void) { HAL_I2C_EV_IRQHandler(&hi2c1); }
void I2C1_ER_IRQHandler(void) { HAL_I2C_ER_IRQHandler(&hi2c1); }

/* ---- DMA IRQ Handlers (berbeda untuk F1 dan F4) ---- */
#if defined(STM32F103xB)
/*
 * F103 DMA Channel Mapping:
 *   DMA1_Channel6 = I2C1_TX
 *   DMA1_Channel7 = I2C1_RX
 */
void DMA1_Channel6_IRQHandler(void) {
    HAL_DMA_IRQHandler(&hdma_i2c1_tx);
}
void DMA1_Channel7_IRQHandler(void) {
    HAL_DMA_IRQHandler(&hdma_i2c1_rx);
}

#else /* F401 / F411 */
/*
 * F4 DMA Stream Mapping:
 *   DMA1_Stream6 = I2C1_TX (Channel 1)
 *   DMA1_Stream0 = I2C1_RX (Channel 1)
 */
void DMA1_Stream6_IRQHandler(void) {
    HAL_DMA_IRQHandler(&hdma_i2c1_tx);
}
void DMA1_Stream0_IRQHandler(void) {
    HAL_DMA_IRQHandler(&hdma_i2c1_rx);
}
#endif

/* ===========================================================================
 * DMA Completion Callbacks (Inti fitur STM32-specific!)
 * ===========================================================================
 * Callback ini dipanggil ketika DMA selesai transfer I2C.
 * CPU TIDAK terlibat selama transfer - hanya mendapat notifikasi di akhir!
 * ESP32 TIDAK bisa melakukan ini karena I2C-nya tidak punya DMA.
 * =========================================================================== */
void HAL_I2C_MemTxCpltCallback(I2C_HandleTypeDef *hi2c) {
    if (hi2c->Instance == I2C1) {
        g_dma_tx_complete = 1;
        g_dma_tx_time = DWT_GetMicros() - g_dma_start_tick;
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
    }
}

void HAL_I2C_MemRxCpltCallback(I2C_HandleTypeDef *hi2c) {
    if (hi2c->Instance == I2C1) {
        g_dma_rx_complete = 1;
        g_dma_rx_time = DWT_GetMicros() - g_dma_start_tick;
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
    }
}

void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c) {
    if (hi2c->Instance == I2C1) {
        g_dma_error = 1;
    }
}

/* ===========================================================================
 * Konfigurasi System Clock
 * =========================================================================== */
static void SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInit = {0};
    RCC_ClkInitTypeDef RCC_ClkInit = {0};

#if defined(STM32F103xB)
    RCC_OscInit.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInit.HSEState       = RCC_HSE_ON;
    RCC_OscInit.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    RCC_OscInit.PLL.PLLState   = RCC_PLL_ON;
    RCC_OscInit.PLL.PLLSource  = RCC_PLLSOURCE_HSE;
    RCC_OscInit.PLL.PLLMUL     = RCC_PLL_MUL9;
    HAL_RCC_OscConfig(&RCC_OscInit);

    RCC_ClkInit.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                 RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInit.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInit.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInit.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInit.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&RCC_ClkInit, FLASH_LATENCY_2);

#elif defined(STM32F401xC)
    RCC_OscInit.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInit.HSEState       = RCC_HSE_ON;
    RCC_OscInit.PLL.PLLState   = RCC_PLL_ON;
    RCC_OscInit.PLL.PLLSource  = RCC_PLLSOURCE_HSE;
    RCC_OscInit.PLL.PLLM       = 25;
    RCC_OscInit.PLL.PLLN       = 336;
    RCC_OscInit.PLL.PLLP       = RCC_PLLP_DIV4;
    RCC_OscInit.PLL.PLLQ       = 7;
    HAL_RCC_OscConfig(&RCC_OscInit);

    RCC_ClkInit.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                 RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInit.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInit.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInit.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInit.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&RCC_ClkInit, FLASH_LATENCY_2);

#elif defined(STM32F411xE)
    RCC_OscInit.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInit.HSEState       = RCC_HSE_ON;
    RCC_OscInit.PLL.PLLState   = RCC_PLL_ON;
    RCC_OscInit.PLL.PLLSource  = RCC_PLLSOURCE_HSE;
    RCC_OscInit.PLL.PLLM       = 25;
    RCC_OscInit.PLL.PLLN       = 200;
    RCC_OscInit.PLL.PLLP       = RCC_PLLP_DIV2;
    RCC_OscInit.PLL.PLLQ       = 4;
    HAL_RCC_OscConfig(&RCC_OscInit);

    RCC_ClkInit.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                 RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInit.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInit.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInit.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInit.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&RCC_ClkInit, FLASH_LATENCY_3);
#endif
}

/* ===========================================================================
 * Inisialisasi GPIO (LED PC13)
 * =========================================================================== */
static void GPIO_Init(void) {
    GPIO_InitTypeDef GPIO_Init_s = {0};

    __HAL_RCC_GPIOC_CLK_ENABLE();

    GPIO_Init_s.Pin   = GPIO_PIN_13;
    GPIO_Init_s.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_Init_s.Speed = GPIO_SPEED_FREQ_LOW;
#if !defined(STM32F103xB)
    GPIO_Init_s.Pull  = GPIO_NOPULL;
#endif
    HAL_GPIO_Init(GPIOC, &GPIO_Init_s);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
}

/* ===========================================================================
 * Inisialisasi USART2 (PA2=TX, PA3=RX)
 * =========================================================================== */
static void UART2_Init(void) {
    GPIO_InitTypeDef GPIO_Init_s = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_USART2_CLK_ENABLE();

#if defined(STM32F103xB)
    GPIO_Init_s.Pin   = GPIO_PIN_2;
    GPIO_Init_s.Mode  = GPIO_MODE_AF_PP;
    GPIO_Init_s.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_Init_s);

    GPIO_Init_s.Pin   = GPIO_PIN_3;
    GPIO_Init_s.Mode  = GPIO_MODE_INPUT;
    GPIO_Init_s.Pull  = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_Init_s);
#else
    GPIO_Init_s.Pin       = GPIO_PIN_2 | GPIO_PIN_3;
    GPIO_Init_s.Mode      = GPIO_MODE_AF_PP;
    GPIO_Init_s.Pull      = GPIO_PULLUP;
    GPIO_Init_s.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_Init_s.Alternate = GPIO_AF7_USART2;
    HAL_GPIO_Init(GPIOA, &GPIO_Init_s);
#endif

    huart2.Instance          = USART2;
    huart2.Init.BaudRate     = 115200;
    huart2.Init.WordLength   = UART_WORDLENGTH_8B;
    huart2.Init.StopBits     = UART_STOPBITS_1;
    huart2.Init.Parity       = UART_PARITY_NONE;
    huart2.Init.Mode         = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
#if !defined(STM32F103xB)
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
#endif
    HAL_UART_Init(&huart2);
}

/* ===========================================================================
 * Inisialisasi DMA untuk I2C1
 * KONFIGURASI DMA BERBEDA untuk F103 vs F401/F411!
 * =========================================================================== */
static void DMA_Init(void) {
    __HAL_RCC_DMA1_CLK_ENABLE();

#if defined(STM32F103xB)
    /* ================================================================
     * F103: DMA1 berbasis CHANNEL (bukan Stream)
     * I2C1_TX = DMA1_Channel6
     * I2C1_RX = DMA1_Channel7
     * ================================================================ */

    /* DMA untuk I2C1 TX (Channel 6) */
    hdma_i2c1_tx.Instance                 = DMA1_Channel6;
    hdma_i2c1_tx.Init.Direction           = DMA_MEMORY_TO_PERIPH;
    hdma_i2c1_tx.Init.PeriphInc           = DMA_PINC_DISABLE;
    hdma_i2c1_tx.Init.MemInc              = DMA_MINC_ENABLE;
    hdma_i2c1_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_i2c1_tx.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
    hdma_i2c1_tx.Init.Mode                = DMA_NORMAL;
    hdma_i2c1_tx.Init.Priority            = DMA_PRIORITY_MEDIUM;
    HAL_DMA_Init(&hdma_i2c1_tx);

    /* DMA untuk I2C1 RX (Channel 7) */
    hdma_i2c1_rx.Instance                 = DMA1_Channel7;
    hdma_i2c1_rx.Init.Direction           = DMA_PERIPH_TO_MEMORY;
    hdma_i2c1_rx.Init.PeriphInc           = DMA_PINC_DISABLE;
    hdma_i2c1_rx.Init.MemInc              = DMA_MINC_ENABLE;
    hdma_i2c1_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_i2c1_rx.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
    hdma_i2c1_rx.Init.Mode                = DMA_NORMAL;
    hdma_i2c1_rx.Init.Priority            = DMA_PRIORITY_MEDIUM;
    HAL_DMA_Init(&hdma_i2c1_rx);

    /* Aktifkan DMA interrupt di NVIC */
    HAL_NVIC_SetPriority(DMA1_Channel6_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(DMA1_Channel6_IRQn);
    HAL_NVIC_SetPriority(DMA1_Channel7_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(DMA1_Channel7_IRQn);

#else /* F401 / F411 */
    /* ================================================================
     * F4: DMA1 berbasis STREAM + CHANNEL
     * I2C1_TX = DMA1_Stream6, Channel 1
     * I2C1_RX = DMA1_Stream0, Channel 1
     * ================================================================ */

    /* DMA untuk I2C1 TX (Stream 6, Channel 1) */
    hdma_i2c1_tx.Instance                 = DMA1_Stream6;
    hdma_i2c1_tx.Init.Channel             = DMA_CHANNEL_1;
    hdma_i2c1_tx.Init.Direction           = DMA_MEMORY_TO_PERIPH;
    hdma_i2c1_tx.Init.PeriphInc           = DMA_PINC_DISABLE;
    hdma_i2c1_tx.Init.MemInc              = DMA_MINC_ENABLE;
    hdma_i2c1_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_i2c1_tx.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
    hdma_i2c1_tx.Init.Mode                = DMA_NORMAL;
    hdma_i2c1_tx.Init.Priority            = DMA_PRIORITY_MEDIUM;
    hdma_i2c1_tx.Init.FIFOMode            = DMA_FIFOMODE_DISABLE;
    HAL_DMA_Init(&hdma_i2c1_tx);

    /* DMA untuk I2C1 RX (Stream 0, Channel 1) */
    hdma_i2c1_rx.Instance                 = DMA1_Stream0;
    hdma_i2c1_rx.Init.Channel             = DMA_CHANNEL_1;
    hdma_i2c1_rx.Init.Direction           = DMA_PERIPH_TO_MEMORY;
    hdma_i2c1_rx.Init.PeriphInc           = DMA_PINC_DISABLE;
    hdma_i2c1_rx.Init.MemInc              = DMA_MINC_ENABLE;
    hdma_i2c1_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_i2c1_rx.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
    hdma_i2c1_rx.Init.Mode                = DMA_NORMAL;
    hdma_i2c1_rx.Init.Priority            = DMA_PRIORITY_MEDIUM;
    hdma_i2c1_rx.Init.FIFOMode            = DMA_FIFOMODE_DISABLE;
    HAL_DMA_Init(&hdma_i2c1_rx);

    /* Aktifkan DMA interrupt di NVIC */
    HAL_NVIC_SetPriority(DMA1_Stream6_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(DMA1_Stream6_IRQn);
    HAL_NVIC_SetPriority(DMA1_Stream0_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(DMA1_Stream0_IRQn);
#endif
}

/* ===========================================================================
 * Inisialisasi I2C1 (PB6=SCL, PB7=SDA)
 * =========================================================================== */
static void I2C1_Init(void) {
    GPIO_InitTypeDef GPIO_Init_s = {0};

    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_I2C1_CLK_ENABLE();

#if defined(STM32F103xB)
    /* F103: PB6 (SCL), PB7 (SDA) - AF Open-Drain */
    GPIO_Init_s.Pin   = GPIO_PIN_6 | GPIO_PIN_7;
    GPIO_Init_s.Mode  = GPIO_MODE_AF_OD;
    GPIO_Init_s.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &GPIO_Init_s);

    hi2c1.Instance             = I2C1;
    hi2c1.Init.ClockSpeed      = 400000;   /* 400kHz Fast Mode */
    hi2c1.Init.DutyCycle        = I2C_DUTYCYCLE_2;
    hi2c1.Init.OwnAddress1     = 0;
    hi2c1.Init.AddressingMode  = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.OwnAddress2     = 0;
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode   = I2C_NOSTRETCH_DISABLE;

#else
    /* F401/F411: PB6 (SCL), PB7 (SDA) - AF4, Open-Drain */
    GPIO_Init_s.Pin       = GPIO_PIN_6 | GPIO_PIN_7;
    GPIO_Init_s.Mode      = GPIO_MODE_AF_OD;
    GPIO_Init_s.Pull      = GPIO_PULLUP;
    GPIO_Init_s.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_Init_s.Alternate = GPIO_AF4_I2C1;
    HAL_GPIO_Init(GPIOB, &GPIO_Init_s);

    hi2c1.Instance             = I2C1;
    hi2c1.Init.ClockSpeed      = 400000;
    hi2c1.Init.DutyCycle        = I2C_DUTYCYCLE_2;
    hi2c1.Init.OwnAddress1     = 0;
    hi2c1.Init.AddressingMode  = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.OwnAddress2     = 0;
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode   = I2C_NOSTRETCH_DISABLE;
#endif

    HAL_I2C_Init(&hi2c1);

    /* Link DMA handles ke I2C handle (KRITIS untuk DMA transfer!) */
    __HAL_LINKDMA(&hi2c1, hdmatx, hdma_i2c1_tx);
    __HAL_LINKDMA(&hi2c1, hdmarx, hdma_i2c1_rx);

    /* Aktifkan I2C Event dan Error interrupt */
    HAL_NVIC_SetPriority(I2C1_EV_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(I2C1_EV_IRQn);
    HAL_NVIC_SetPriority(I2C1_ER_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(I2C1_ER_IRQn);
}

/* ===========================================================================
 * Inisialisasi DWT Cycle Counter
 * =========================================================================== */
static void DWT_Init(void) {
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

static uint32_t DWT_GetMicros(void) {
    return DWT->CYCCNT / (SystemCoreClock / 1000000U);
}

/* ===========================================================================
 * EEPROM Helper: Cek kesiapan device menggunakan HAL_I2C_IsDeviceReady()
 * Fungsi ini khusus STM32 HAL - melakukan ACK polling otomatis!
 * =========================================================================== */
static uint8_t EEPROM_IsReady(uint32_t trials) {
    return (HAL_I2C_IsDeviceReady(&hi2c1, EEPROM_ADDR, trials, 100) == HAL_OK);
}

/* ===========================================================================
 * EEPROM Page Write (Polling) - untuk perbandingan
 * =========================================================================== */
static void EEPROM_WritePage_Polling(uint16_t addr, uint8_t *data, uint16_t len) {
    uint16_t remaining = len;
    uint16_t offset    = 0;

    while (remaining > 0) {
        uint16_t chunk = remaining;
        if (chunk > EEPROM_PAGE_SIZE) chunk = EEPROM_PAGE_SIZE;

        /* Pastikan tidak melewati batas page */
        uint16_t page_offset  = (addr + offset) % EEPROM_PAGE_SIZE;
        uint16_t page_remain  = EEPROM_PAGE_SIZE - page_offset;
        if (chunk > page_remain) chunk = page_remain;

        HAL_I2C_Mem_Write(&hi2c1, EEPROM_ADDR, addr + offset,
                           EEPROM_MEM_SIZE, data + offset, chunk, 100);

        /* Tunggu write cycle selesai menggunakan ACK polling */
        while (!EEPROM_IsReady(10)) {
            /* Tunggu EEPROM selesai menulis (~5ms) */
        }

        offset    += chunk;
        remaining -= chunk;
    }
}

/* ===========================================================================
 * EEPROM Page Read (Polling) - untuk perbandingan
 * =========================================================================== */
static void EEPROM_ReadPage_Polling(uint16_t addr, uint8_t *data, uint16_t len) {
    HAL_I2C_Mem_Read(&hi2c1, EEPROM_ADDR, addr,
                      EEPROM_MEM_SIZE, data, len, 100);
}

/* ===========================================================================
 * Print Header
 * =========================================================================== */
static void Print_Header(void) {
    printf("\r\n");
    printf("============================================================\r\n");
    printf("  STM32_13: I2C DMA Transfer (Hardware DMA untuk I2C)\r\n");
    printf("  FITUR KHUSUS STM32 - Tidak tersedia di ESP32!\r\n");
    printf("============================================================\r\n");
#if defined(STM32F103xB)
    printf("  Board  : BluePill STM32F103C8 @ %luMHz\r\n", SystemCoreClock / 1000000UL);
    printf("  DMA    : DMA1 Ch6(TX) Ch7(RX)\r\n");
#elif defined(STM32F401xC)
    printf("  Board  : BlackPill STM32F401CC @ %luMHz\r\n", SystemCoreClock / 1000000UL);
    printf("  DMA    : DMA1 Stream6(TX) Stream0(RX)\r\n");
#elif defined(STM32F411xE)
    printf("  Board  : BlackPill STM32F411CE @ %luMHz\r\n", SystemCoreClock / 1000000UL);
    printf("  DMA    : DMA1 Stream6(TX) Stream0(RX)\r\n");
#endif
    printf("  I2C    : PB6(SCL) PB7(SDA) @ 400kHz\r\n");
    printf("  Target : AT24C32 EEPROM (0x50, 4KB, 16-bit addr)\r\n");
    printf("  UART   : PA2/PA3 (USART2, 115200 baud)\r\n");
    printf("============================================================\r\n\r\n");
    printf("PENJELASAN I2C DMA (fitur STM32):\r\n");
    printf("  DMA (Direct Memory Access) mentransfer data antara\r\n");
    printf("  memori dan peripheral I2C TANPA intervensi CPU:\r\n");
    printf("  1. CPU setup: sumber, tujuan, jumlah byte\r\n");
    printf("  2. DMA controller handle transfer byte-per-byte\r\n");
    printf("  3. CPU BEBAS melakukan pekerjaan lain\r\n");
    printf("  4. Callback dipanggil saat transfer selesai\r\n\r\n");
    printf("  ESP32 I2C TIDAK mendukung DMA:\r\n");
    printf("  -> Setiap byte ditangani oleh CPU\r\n");
    printf("  -> CPU blocking selama transfer\r\n");
    printf("  -> Tidak efisien untuk transfer data besar\r\n");
    printf("============================================================\r\n\r\n");
}

/* ===========================================================================
 * Demo 1: Deteksi Device I2C menggunakan HAL_I2C_IsDeviceReady()
 * =========================================================================== */
static void Demo_DeviceDetection(void) {
    printf("=== DEMO 1: Deteksi Device I2C ===\r\n");
    printf("Menggunakan HAL_I2C_IsDeviceReady() - fungsi khusus STM32 HAL\r\n\r\n");

    /* Scan alamat I2C 0x08 - 0x77 */
    uint8_t found = 0;
    printf("Scanning I2C bus...\r\n");

    for (uint8_t addr = 0x08; addr <= 0x77; addr++) {
        if (HAL_I2C_IsDeviceReady(&hi2c1, addr << 1, 2, 10) == HAL_OK) {
            printf("  [FOUND] Device di alamat 0x%02X", addr);
            if (addr == 0x50)
                printf(" <- AT24C32 EEPROM");
            else if (addr >= 0x50 && addr <= 0x57)
                printf(" <- EEPROM (A0-A2 variant)");
            printf("\r\n");
            found++;
        }
    }

    if (found == 0) {
        printf("  [WARN] Tidak ada device ditemukan!\r\n");
        printf("  Pastikan AT24C32 EEPROM terhubung ke PB6(SCL) dan PB7(SDA)\r\n");
        printf("  Program tetap berjalan untuk demonstrasi...\r\n");
    } else {
        printf("  Total: %u device ditemukan\r\n", found);
    }
    printf("\r\n");
}

/* Variabel global untuk menyimpan hasil benchmark */
static uint32_t g_poll_write_us[4], g_poll_read_us[4];
static uint32_t g_it_write_us[4],   g_it_read_us[4];
static uint32_t g_dma_write_us[4],  g_dma_read_us[4];
static const uint16_t g_test_sizes[] = {TEST_SIZE_1, TEST_SIZE_2,
                                         TEST_SIZE_3, TEST_SIZE_MAX};

/* ===========================================================================
 * Demo 2: Polling Transfer (baseline)
 * CPU BLOCKING selama seluruh transfer - cara ESP32 bekerja
 * =========================================================================== */
static void Demo_PollingTransfer(uint16_t size) {
    uint16_t test_addr = 0x0000;
    uint32_t t_start, t_end;

    printf("[POLLING] Transfer %u bytes...\r\n", size);

    /* Siapkan data test */
    for (uint16_t i = 0; i < size; i++)
        tx_buffer[i] = (uint8_t)(i & 0xFF);

    /* === WRITE (Polling) === */
    t_start = DWT_GetMicros();
    EEPROM_WritePage_Polling(test_addr, tx_buffer, size);
    t_end = DWT_GetMicros();

    uint32_t write_time = t_end - t_start;
    printf("  Write: %lu us (CPU blocking 100%%)\r\n", write_time);

    /* === READ (Polling) === */
    memset(rx_buffer, 0, size);
    t_start = DWT_GetMicros();
    EEPROM_ReadPage_Polling(test_addr, rx_buffer, size);
    t_end = DWT_GetMicros();

    uint32_t read_time = t_end - t_start;
    printf("  Read : %lu us (CPU blocking 100%%)\r\n", read_time);

    /* Verifikasi data */
    uint8_t ok = 1;
    for (uint16_t i = 0; i < size; i++) {
        if (rx_buffer[i] != tx_buffer[i]) { ok = 0; break; }
    }
    printf("  Verifikasi: %s\r\n\r\n", ok ? "OK" : "GAGAL!");

    /* Simpan hasil */
    uint8_t idx = 0;
    for (uint8_t i = 0; i < 4; i++) {
        if (g_test_sizes[i] == size) { idx = i; break; }
    }
    g_poll_write_us[idx] = write_time;
    g_poll_read_us[idx]  = read_time;
}

/* ===========================================================================
 * Demo 3: Interrupt Transfer
 * CPU tidak blocking tapi masih menangani setiap byte via interrupt
 * =========================================================================== */
static void Demo_InterruptTransfer(uint16_t size) {
    uint16_t test_addr = 0x0100; /* Alamat berbeda dari polling test */
    uint32_t t_start, t_end;

    printf("[INTERRUPT] Transfer %u bytes...\r\n", size);

    for (uint16_t i = 0; i < size; i++)
        tx_buffer[i] = (uint8_t)((i + 0x55) & 0xFF);

    /* === WRITE (Interrupt) - page by page === */
    t_start = DWT_GetMicros();
    {
        uint16_t remaining = size;
        uint16_t offset    = 0;

        while (remaining > 0) {
            uint16_t chunk = remaining;
            if (chunk > EEPROM_PAGE_SIZE) chunk = EEPROM_PAGE_SIZE;

            uint16_t page_offset = (test_addr + offset) % EEPROM_PAGE_SIZE;
            uint16_t page_remain = EEPROM_PAGE_SIZE - page_offset;
            if (chunk > page_remain) chunk = page_remain;

            g_dma_tx_complete = 0;
            HAL_I2C_Mem_Write_IT(&hi2c1, EEPROM_ADDR, test_addr + offset,
                                  EEPROM_MEM_SIZE, tx_buffer + offset, chunk);

            /* Tunggu interrupt selesai */
            uint32_t timeout = HAL_GetTick();
            while (!g_dma_tx_complete && (HAL_GetTick() - timeout) < 100);

            /* Tunggu write cycle */
            while (!EEPROM_IsReady(10));

            offset    += chunk;
            remaining -= chunk;
        }
    }
    t_end = DWT_GetMicros();

    uint32_t write_time = t_end - t_start;
    printf("  Write: %lu us (CPU interrupt per byte)\r\n", write_time);

    /* === READ (Interrupt) === */
    memset(rx_buffer, 0, size);
    g_dma_rx_complete = 0;

    t_start = DWT_GetMicros();
    HAL_I2C_Mem_Read_IT(&hi2c1, EEPROM_ADDR, test_addr,
                         EEPROM_MEM_SIZE, rx_buffer, size);

    uint32_t timeout = HAL_GetTick();
    while (!g_dma_rx_complete && (HAL_GetTick() - timeout) < 100);
    t_end = DWT_GetMicros();

    uint32_t read_time = t_end - t_start;
    printf("  Read : %lu us (CPU interrupt per byte)\r\n", read_time);

    uint8_t ok = 1;
    for (uint16_t i = 0; i < size; i++) {
        if (rx_buffer[i] != tx_buffer[i]) { ok = 0; break; }
    }
    printf("  Verifikasi: %s\r\n\r\n", ok ? "OK" : "GAGAL!");

    uint8_t idx = 0;
    for (uint8_t i = 0; i < 4; i++) {
        if (g_test_sizes[i] == size) { idx = i; break; }
    }
    g_it_write_us[idx] = write_time;
    g_it_read_us[idx]  = read_time;
}

/* ===========================================================================
 * Demo 4: DMA Transfer (FITUR UTAMA STM32!)
 * CPU SEPENUHNYA BEBAS selama transfer - hardware DMA handle semuanya
 * =========================================================================== */
static void Demo_DMATransfer(uint16_t size) {
    uint16_t test_addr = 0x0200; /* Alamat berbeda */
    uint32_t t_start, t_end;

    printf("[DMA] Transfer %u bytes (CPU bebas!)...\r\n", size);

    for (uint16_t i = 0; i < size; i++)
        tx_buffer[i] = (uint8_t)((i + 0xAA) & 0xFF);

    /* === WRITE (DMA) - page by page === */
    t_start = DWT_GetMicros();
    {
        uint16_t remaining = size;
        uint16_t offset    = 0;

        while (remaining > 0) {
            uint16_t chunk = remaining;
            if (chunk > EEPROM_PAGE_SIZE) chunk = EEPROM_PAGE_SIZE;

            uint16_t page_offset = (test_addr + offset) % EEPROM_PAGE_SIZE;
            uint16_t page_remain = EEPROM_PAGE_SIZE - page_offset;
            if (chunk > page_remain) chunk = page_remain;

            g_dma_tx_complete = 0;
            g_dma_error       = 0;
            g_dma_start_tick  = DWT_GetMicros();

            /*
             * HAL_I2C_Mem_Write_DMA() - INTI FITUR STM32!
             * Setelah pemanggilan ini, DMA controller mengambil alih.
             * CPU bisa melakukan pekerjaan lain sepenuhnya!
             */
            HAL_I2C_Mem_Write_DMA(&hi2c1, EEPROM_ADDR, test_addr + offset,
                                   EEPROM_MEM_SIZE, tx_buffer + offset, chunk);

            /* CPU bebas di sini! Bisa melakukan komputasi lain.
             * Kita hanya menunggu untuk benchmark, tapi di aplikasi nyata
             * CPU bisa menjalankan task lain. */
            uint32_t timeout = HAL_GetTick();
            while (!g_dma_tx_complete && !g_dma_error &&
                   (HAL_GetTick() - timeout) < 100);

            /* Tunggu write cycle */
            while (!EEPROM_IsReady(10));

            offset    += chunk;
            remaining -= chunk;
        }
    }
    t_end = DWT_GetMicros();

    uint32_t write_time = t_end - t_start;
    printf("  Write: %lu us (CPU BEBAS selama DMA transfer!)\r\n", write_time);

    /* === READ (DMA) === */
    memset(rx_buffer, 0, size);
    g_dma_rx_complete = 0;
    g_dma_error       = 0;
    g_dma_start_tick  = DWT_GetMicros();

    t_start = DWT_GetMicros();

    /*
     * HAL_I2C_Mem_Read_DMA() - INTI FITUR STM32!
     * DMA membaca data dari EEPROM ke buffer tanpa CPU!
     */
    HAL_I2C_Mem_Read_DMA(&hi2c1, EEPROM_ADDR, test_addr,
                          EEPROM_MEM_SIZE, rx_buffer, size);

    uint32_t timeout = HAL_GetTick();
    while (!g_dma_rx_complete && !g_dma_error &&
           (HAL_GetTick() - timeout) < 100);
    t_end = DWT_GetMicros();

    uint32_t read_time = t_end - t_start;
    printf("  Read : %lu us (CPU BEBAS selama DMA transfer!)\r\n", read_time);

    if (g_dma_error) {
        printf("  [ERROR] DMA transfer error!\r\n");
    }

    uint8_t ok = 1;
    for (uint16_t i = 0; i < size; i++) {
        if (rx_buffer[i] != tx_buffer[i]) { ok = 0; break; }
    }
    printf("  Verifikasi: %s\r\n", ok ? "OK" : "GAGAL!");
    printf("  DMA TX callback time: %lu us\r\n", g_dma_tx_time);
    printf("  DMA RX callback time: %lu us\r\n\r\n", g_dma_rx_time);

    uint8_t idx = 0;
    for (uint8_t i = 0; i < 4; i++) {
        if (g_test_sizes[i] == size) { idx = i; break; }
    }
    g_dma_write_us[idx] = write_time;
    g_dma_read_us[idx]  = read_time;
}

/* ===========================================================================
 * Demo 5: CPU Free Time - demonstrasi CPU bekerja selama DMA transfer
 * =========================================================================== */
static void Demo_CPUFreeTime(void) {
    printf("=== DEMO 5: CPU Free Time selama DMA Transfer ===\r\n");
    printf("Menulis 32 bytes ke EEPROM via DMA sambil CPU menghitung...\r\n\r\n");

    for (uint16_t i = 0; i < 32; i++)
        tx_buffer[i] = (uint8_t)i;

    g_dma_tx_complete = 0;
    g_dma_error       = 0;

    /* Mulai DMA write */
    HAL_I2C_Mem_Write_DMA(&hi2c1, EEPROM_ADDR, 0x0300,
                           EEPROM_MEM_SIZE, tx_buffer, 32);

    /* Sementara DMA bekerja, CPU menghitung angka prima! */
    uint32_t prime_count = 0;
    uint32_t loop_count  = 0;

    while (!g_dma_tx_complete && !g_dma_error) {
        /* Cek apakah loop_count+2 adalah bilangan prima */
        uint32_t n = loop_count + 2;
        uint8_t is_prime = 1;
        for (uint32_t d = 2; d * d <= n; d++) {
            if (n % d == 0) { is_prime = 0; break; }
        }
        if (is_prime) prime_count++;
        loop_count++;
    }

    printf("  Selama DMA transfer 32 bytes ke EEPROM:\r\n");
    printf("  -> CPU berhasil melakukan %lu iterasi pengecekan prima\r\n", loop_count);
    printf("  -> Ditemukan %lu bilangan prima\r\n", prime_count);
    printf("  -> CPU 100%% produktif selama DMA transfer!\r\n");
    printf("\r\n");
    printf("  Pada ESP32 dengan I2C polling:\r\n");
    printf("  -> CPU menunggu (0 iterasi, 0 bilangan prima)\r\n");
    printf("  -> CPU 100%% idle/blocking selama transfer!\r\n\r\n");

    /* Tunggu write cycle */
    while (!EEPROM_IsReady(10));
}

/* ===========================================================================
 * Print Perbandingan Hasil
 * =========================================================================== */
static void Print_Results(void) {
    printf("============================================================\r\n");
    printf("  PERBANDINGAN: Polling vs Interrupt vs DMA\r\n");
    printf("============================================================\r\n\r\n");

    printf("  WRITE TIMES (microseconds):\r\n");
    printf("  +--------+------------+------------+------------+\r\n");
    printf("  | Size   | Polling    | Interrupt  | DMA        |\r\n");
    printf("  +--------+------------+------------+------------+\r\n");
    for (uint8_t i = 0; i < 4; i++) {
        printf("  | %4u B | %8lu us | %8lu us | %8lu us |\r\n",
               g_test_sizes[i],
               g_poll_write_us[i], g_it_write_us[i], g_dma_write_us[i]);
    }
    printf("  +--------+------------+------------+------------+\r\n\r\n");

    printf("  READ TIMES (microseconds):\r\n");
    printf("  +--------+------------+------------+------------+\r\n");
    printf("  | Size   | Polling    | Interrupt  | DMA        |\r\n");
    printf("  +--------+------------+------------+------------+\r\n");
    for (uint8_t i = 0; i < 4; i++) {
        printf("  | %4u B | %8lu us | %8lu us | %8lu us |\r\n",
               g_test_sizes[i],
               g_poll_read_us[i], g_it_read_us[i], g_dma_read_us[i]);
    }
    printf("  +--------+------------+------------+------------+\r\n\r\n");

    printf("  CPU USAGE SELAMA TRANSFER:\r\n");
    printf("  +-------------------+---------+-----------+---------+\r\n");
    printf("  | Mode              | CPU Use | Bisa task | ESP32?  |\r\n");
    printf("  +-------------------+---------+-----------+---------+\r\n");
    printf("  | Polling           | 100%%    | TIDAK     | Ya      |\r\n");
    printf("  | Interrupt         | ~10%%    | Sebagian  | Ya      |\r\n");
    printf("  | DMA (STM32 only!) | ~0%%     | YA (100%%)| TIDAK   |\r\n");
    printf("  +-------------------+---------+-----------+---------+\r\n\r\n");

    printf("  KESIMPULAN:\r\n");
    printf("  I2C DMA pada STM32 memberikan:\r\n");
    printf("  1. CPU sepenuhnya bebas selama transfer data\r\n");
    printf("  2. Ideal untuk transfer data besar (sensor array, EEPROM)\r\n");
    printf("  3. Memungkinkan multitasking tanpa RTOS\r\n");
    printf("  4. Sangat penting untuk aplikasi real-time\r\n");
    printf("  5. ESP32 hanya bisa polling/interrupt untuk I2C\r\n");
    printf("============================================================\r\n\r\n");
}

/* ===========================================================================
 * FUNGSI UTAMA (main)
 * =========================================================================== */
int main(void) {
    HAL_Init();
    SystemClock_Config();

    /* Inisialisasi peripheral */
    GPIO_Init();
    UART2_Init();
    DWT_Init();

    /* DMA HARUS diinisialisasi SEBELUM I2C! */
    DMA_Init();
    I2C1_Init();

    Print_Header();

    /* Reset buffer hasil */
    memset(g_poll_write_us, 0, sizeof(g_poll_write_us));
    memset(g_poll_read_us,  0, sizeof(g_poll_read_us));
    memset(g_it_write_us,   0, sizeof(g_it_write_us));
    memset(g_it_read_us,    0, sizeof(g_it_read_us));
    memset(g_dma_write_us,  0, sizeof(g_dma_write_us));
    memset(g_dma_read_us,   0, sizeof(g_dma_read_us));

    /* ---- Demo 1: Deteksi device ---- */
    Demo_DeviceDetection();

    /* Cek apakah EEPROM ditemukan */
    if (!EEPROM_IsReady(3)) {
        printf("[WARN] AT24C32 EEPROM tidak merespons!\r\n");
        printf("       Pastikan hardware terhubung dengan benar.\r\n");
        printf("       Program tetap menjalankan demo (mungkin timeout)...\r\n\r\n");
    }

    /* ---- Demo 2-4: Perbandingan transfer mode ---- */
    printf("============================================================\r\n");
    printf("  BENCHMARK: Polling vs Interrupt vs DMA Transfer\r\n");
    printf("============================================================\r\n\r\n");

    for (uint8_t i = 0; i < 4; i++) {
        printf("--- Test size: %u bytes ---\r\n\r\n", g_test_sizes[i]);

        Demo_PollingTransfer(g_test_sizes[i]);
        HAL_Delay(10);

        Demo_InterruptTransfer(g_test_sizes[i]);
        HAL_Delay(10);

        Demo_DMATransfer(g_test_sizes[i]);
        HAL_Delay(10);

        printf("--------------------------------------------\r\n\r\n");
    }

    /* ---- Demo 5: CPU free time ---- */
    Demo_CPUFreeTime();

    /* ---- Print hasil perbandingan ---- */
    Print_Results();

    /* ---- Loop kontinu: periodik read EEPROM dengan DMA ---- */
    printf("=== Mode Kontinu: Baca EEPROM periodik dengan DMA ===\r\n\r\n");

    uint32_t last_read = 0;
    uint32_t cycle     = 0;

    while (1) {
        if ((HAL_GetTick() - last_read) >= 2000) {
            last_read = HAL_GetTick();
            cycle++;

            /* Baca 32 bytes dari EEPROM menggunakan DMA */
            g_dma_rx_complete = 0;
            g_dma_error       = 0;
            g_dma_start_tick  = DWT_GetMicros();

            HAL_I2C_Mem_Read_DMA(&hi2c1, EEPROM_ADDR, 0x0000,
                                  EEPROM_MEM_SIZE, rx_buffer, 32);

            /* Tunggu selesai */
            uint32_t timeout = HAL_GetTick();
            while (!g_dma_rx_complete && !g_dma_error &&
                   (HAL_GetTick() - timeout) < 100);

            if (!g_dma_error) {
                printf("[DMA-READ #%lu] Addr=0x0000, 32 bytes, time=%luus: ",
                       cycle, g_dma_rx_time);
                /* Print 8 byte pertama */
                for (uint8_t i = 0; i < 8; i++)
                    printf("%02X ", rx_buffer[i]);
                printf("...\r\n");
            } else {
                printf("[DMA-READ #%lu] ERROR!\r\n", cycle);
            }

            HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
        }
    }
}
