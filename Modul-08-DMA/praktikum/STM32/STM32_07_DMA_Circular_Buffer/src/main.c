/* ============================================================
 * STM32_07_DMA_Circular_Buffer
 * ============================================================
 * Deskripsi:
 *   Program ini mendemonstrasikan penerimaan data UART1
 *   menggunakan DMA dalam mode circular buffer. Teknik
 *   head/tail pointer digunakan untuk mendeteksi data baru
 *   tanpa kehilangan byte apapun.
 *
 * Hardware:
 *   - STM32F103C8T6 Blue Pill
 *   - USB-Serial adapter terhubung ke PA9 (TX) dan PA10 (RX)
 *   - LED indikator: PC13 (active low)
 *
 * Prinsip Kerja:
 *   1. DMA1 Channel 5 dikonfigurasi circular untuk USART1_RX
 *   2. DMA terus menerima data ke buffer melingkar
 *   3. Software melacak posisi "tail" (sudah dibaca)
 *   4. Posisi "head" dihitung dari DMA CNDTR register
 *   5. Data baru = head - tail (dengan wraparound)
 *   6. Data diproses (uppercase/echo) dan dikirim kembali
 *
 * Keunggulan DMA Circular vs Interrupt per-byte:
 *   - Tidak ada overhead interrupt per karakter
 *   - Tidak ada risiko buffer overflow pada data cepat
 *   - CPU hanya perlu memeriksa secara periodik
 *   - Sangat efisien untuk data burst/streaming
 *
 * Koneksi:
 *   PA9  → USB-Serial RX  (STM32 TX → adapter RX)
 *   PA10 → USB-Serial TX  (STM32 RX → adapter TX)
 * ============================================================ */

#include "stm32f1xx_hal.h"
#include "config.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

/* ============================================================
 * Deklarasi Handle Periferal
 * ============================================================ */
UART_HandleTypeDef huart1;          /* Handle USART1 */
DMA_HandleTypeDef hdma_usart1_rx;   /* Handle DMA1 Ch5 (USART1_RX) */
DMA_HandleTypeDef hdma_usart1_tx;   /* Handle DMA1 Ch4 (USART1_TX) */

/* ============================================================
 * Circular Buffer untuk DMA RX
 * DMA menulis ke buffer ini secara terus-menerus dan melingkar
 * ============================================================ */
volatile uint8_t rx_dma_buffer[RX_BUFFER_SIZE];

/* Buffer untuk TX (pengiriman respons) */
uint8_t tx_buffer[TX_BUFFER_SIZE];

/* Buffer untuk memproses data yang diterima */
uint8_t process_buffer[PROCESS_BUFFER_SIZE];

/* ============================================================
 * Variabel Kontrol Circular Buffer
 * tail_pos: posisi terakhir yang sudah dibaca oleh software
 * DMA CNDTR register memberikan "head" position secara implisit
 * ============================================================ */
volatile uint16_t tail_pos = 0;

/* Statistik */
volatile uint32_t total_bytes_received = 0;
volatile uint32_t total_messages = 0;
volatile uint32_t overrun_count = 0;

/* Flag untuk DMA events */
volatile uint8_t dma_half_complete = 0;
volatile uint8_t dma_full_complete = 0;
volatile uint8_t uart_idle_detected = 0;

/* ============================================================
 * Deklarasi Fungsi Prototipe
 * ============================================================ */
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_USART1_Init(void);
static void DWT_Init(void);
static uint16_t Get_DMA_Head(void);
static uint16_t Get_Available_Data(void);
static uint16_t Read_Circular_Buffer(uint8_t *dest, uint16_t max_len);
static void Process_Received_Data(uint8_t *data, uint16_t len);
static void Send_Response(const char *msg, uint16_t len);
static void Print_Stats(void);
void Error_Handler(void);

/* ============================================================
 * Retarget printf ke UART1
 * CATATAN: printf menggunakan TX polling karena DMA TX
 * digunakan untuk echo responses
 * ============================================================ */
int _write(int file, char *ptr, int len)
{
    HAL_UART_Transmit(&huart1, (uint8_t *)ptr, len, HAL_MAX_DELAY);
    return len;
}

/* ============================================================
 * DWT Initialization
 * ============================================================ */
static void DWT_Init(void)
{
    DWT_DEMCR_REG |= (1 << 24);
    DWT_CYCCNT_REG = 0;
    DWT_CTRL_REG |= 1;
}

/* ============================================================
 * Get_DMA_Head
 * Menghitung posisi "head" dari DMA CNDTR register
 * 
 * CNDTR = jumlah byte yang BELUM ditransfer
 * Head = BufferSize - CNDTR
 * 
 * Contoh dengan buffer 256:
 *   CNDTR=256 → head=0   (DMA baru mulai)
 *   CNDTR=200 → head=56  (56 byte sudah diterima)
 *   CNDTR=1   → head=255 (255 byte sudah diterima)
 *   CNDTR=256 → head=0   (DMA wrap around, kembali ke awal)
 * ============================================================ */
static uint16_t Get_DMA_Head(void)
{
    /* Baca CNDTR - jumlah remaining transfer */
    uint16_t cndtr = (uint16_t)__HAL_DMA_GET_COUNTER(&hdma_usart1_rx);
    /* Hitung posisi head */
    uint16_t head = RX_BUFFER_SIZE - cndtr;
    /* Handle wraparound */
    if (head >= RX_BUFFER_SIZE) {
        head = 0;
    }
    return head;
}

/* ============================================================
 * Get_Available_Data
 * Hitung jumlah byte yang tersedia untuk dibaca
 * Menangani kasus wraparound pada circular buffer
 * 
 * Kasus 1: head >= tail (normal, tidak wrap)
 *   Available = head - tail
 *   [....tail####head....]
 *
 * Kasus 2: head < tail (wrapped around)
 *   Available = BufferSize - tail + head
 *   [####head....tail####]
 * ============================================================ */
static uint16_t Get_Available_Data(void)
{
    uint16_t head = Get_DMA_Head();
    uint16_t tail = tail_pos;

    if (head >= tail) {
        return head - tail;
    } else {
        return RX_BUFFER_SIZE - tail + head;
    }
}

/* ============================================================
 * Read_Circular_Buffer
 * Membaca data dari circular buffer ke buffer linear
 * Menangani wraparound secara transparan
 *
 * Args:
 *   dest    : Buffer tujuan (linear)
 *   max_len : Jumlah maksimum byte yang akan dibaca
 *
 * Returns:
 *   Jumlah byte yang berhasil dibaca
 * ============================================================ */
static uint16_t Read_Circular_Buffer(uint8_t *dest, uint16_t max_len)
{
    uint16_t available = Get_Available_Data();
    uint16_t to_read = (available < max_len) ? available : max_len;

    if (to_read == 0) return 0;

    uint16_t tail = tail_pos;

    for (uint16_t i = 0; i < to_read; i++) {
        dest[i] = rx_dma_buffer[tail];
        tail++;
        if (tail >= RX_BUFFER_SIZE) {
            tail = 0;  /* Wrap around */
        }
    }

    /* Update tail position */
    tail_pos = tail;
    total_bytes_received += to_read;

    return to_read;
}

/* ============================================================
 * Process_Received_Data
 * Memproses data yang diterima:
 *   1. Konversi huruf kecil ke huruf besar (uppercase)
 *   2. Tambahkan prefix "[ECHO] "
 *   3. Kirim kembali ke pengirim
 *   4. Cetak statistik dengan tag [DATA]
 * ============================================================ */
static void Process_Received_Data(uint8_t *data, uint16_t len)
{
    if (len == 0) return;

    total_messages++;

    /* ----------------------------------------------------------
     * Buat salinan dengan modifikasi uppercase
     * ---------------------------------------------------------- */
    uint8_t modified[PROCESS_BUFFER_SIZE];
    uint16_t mod_len = 0;

    /* Prefix echo */
    const char *prefix = "[ECHO] ";
    uint8_t prefix_len = (uint8_t)strlen(prefix);
    memcpy(modified, prefix, prefix_len);
    mod_len = prefix_len;

    /* Konversi ke uppercase dan salin */
    for (uint16_t i = 0; i < len && mod_len < PROCESS_BUFFER_SIZE - 4; i++) {
        if (data[i] >= 'a' && data[i] <= 'z') {
            modified[mod_len++] = data[i] - 32;  /* Lowercase → Uppercase */
        } else if (data[i] == '\r' || data[i] == '\n') {
            continue;  /* Skip CR/LF */
        } else {
            modified[mod_len++] = data[i];
        }
    }

    /* Tambahkan newline */
    modified[mod_len++] = '\r';
    modified[mod_len++] = '\n';

    /* Kirim respons */
    Send_Response((const char *)modified, mod_len);

    /* ----------------------------------------------------------
     * Cetak data dengan tag [DATA] untuk Python parser
     * ---------------------------------------------------------- */
    printf("[DATA] RX_LEN=%u TOTAL_BYTES=%lu MSG_COUNT=%lu "
           "HEAD=%u TAIL=%u AVAIL=%u\r\n",
           len, total_bytes_received, total_messages,
           Get_DMA_Head(), tail_pos, Get_Available_Data());

    /* Cetak hex dump data yang diterima (maks 32 byte) */
    printf("[DATA] HEX");
    uint16_t dump_len = (len > 32) ? 32 : len;
    for (uint16_t i = 0; i < dump_len; i++) {
        printf(" %02X", data[i]);
    }
    if (len > 32) printf(" ...");
    printf("\r\n");
}

/* ============================================================
 * Send_Response
 * Mengirim data respons via UART1 (polling mode)
 * ============================================================ */
static void Send_Response(const char *msg, uint16_t len)
{
    HAL_UART_Transmit(&huart1, (const uint8_t *)msg, len, 100);
}

/* ============================================================
 * Print_Stats
 * Cetak statistik periodik
 * ============================================================ */
static void Print_Stats(void)
{
    uint16_t head = Get_DMA_Head();
    uint16_t available = Get_Available_Data();

    /* Hitung persentase buffer terpakai */
    uint16_t used;
    if (head >= tail_pos) {
        used = head - tail_pos;
    } else {
        used = RX_BUFFER_SIZE - tail_pos + head;
    }
    uint8_t usage_pct = (uint8_t)((used * 100) / RX_BUFFER_SIZE);

    printf("[DATA] STATS TOTAL_BYTES=%lu TOTAL_MSG=%lu "
           "HEAD=%u TAIL=%u AVAIL=%u USAGE=%u%% "
           "OVERRUN=%lu UPTIME=%lu\r\n",
           total_bytes_received, total_messages,
           head, tail_pos, available, usage_pct,
           overrun_count, HAL_GetTick() / 1000);
}

/* ============================================================
 * DMA Callbacks
 * ============================================================ */

/* DMA half-complete: setengah buffer sudah terisi */
void HAL_UART_RxHalfCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1) {
        dma_half_complete = 1;
    }
}

/* DMA full-complete: seluruh buffer sudah terisi (wrap) */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1) {
        dma_full_complete = 1;
    }
}

/* ============================================================
 * MAIN
 * ============================================================ */
int main(void)
{
    /* Inisialisasi HAL */
    HAL_Init();

    /* Konfigurasi System Clock: HSE 8MHz → PLL x9 → 72MHz */
    SystemClock_Config();

    /* Inisialisasi periferal */
    MX_GPIO_Init();
    MX_DMA_Init();
    MX_USART1_Init();
    DWT_Init();

    /* LED menyala sebentar */
    LED_ON();
    HAL_Delay(200);
    LED_OFF();

    /* Cetak header */
    printf("\r\n========================================\r\n");
    printf("STM32_07_DMA_Circular_Buffer\r\n");
    printf("========================================\r\n");
    printf("Target     : STM32F103C8T6 Blue Pill\r\n");
    printf("SYSCLK     : %lu MHz\r\n", SYSCLK_FREQ_HZ / 1000000);
    printf("UART       : %d baud (PA9 TX, PA10 RX)\r\n", DEBUG_UART_BAUD);
    printf("DMA RX     : DMA1 Channel 5 (Circular)\r\n");
    printf("Buffer     : %d bytes circular\r\n", RX_BUFFER_SIZE);
    printf("========================================\r\n");
    printf("\r\n[INFO] Kirim data via serial terminal.\r\n");
    printf("[INFO] Data akan di-echo kembali dalam UPPERCASE.\r\n");
    printf("[INFO] Ketik pesan dan tekan Enter.\r\n\r\n");

    /* ----------------------------------------------------------
     * Mulai DMA RX Circular
     * HAL_UART_Receive_DMA() dengan circular mode akan:
     *   1. Mengkonfigurasi DMA channel 5
     *   2. Mengaktifkan DMA request dari USART1
     *   3. DMA akan terus menerima data tanpa berhenti
     * ---------------------------------------------------------- */
    HAL_StatusTypeDef status = HAL_UART_Receive_DMA(&huart1,
                                                     (uint8_t *)rx_dma_buffer,
                                                     RX_BUFFER_SIZE);
    if (status != HAL_OK) {
        printf("[ERROR] Gagal memulai UART DMA RX! Status: %d\r\n", status);
        Error_Handler();
    }
    printf("[INFO] DMA Circular RX aktif. Menunggu data masuk...\r\n\r\n");

    /* Aktifkan UART IDLE interrupt untuk deteksi akhir frame */
    __HAL_UART_ENABLE_IT(&huart1, UART_IT_IDLE);

    /* Variabel loop */
    uint32_t last_stats_tick = HAL_GetTick();
    uint32_t last_check_tick = HAL_GetTick();

    /* ============================================================
     * Loop Utama
     * Polling circular buffer untuk data baru
     * ============================================================ */
    while (1)
    {
        uint32_t current_tick = HAL_GetTick();

        /* ----------------------------------------------------------
         * Cek data baru secara periodik
         * Menggunakan teknik head/tail untuk mendeteksi data
         * yang belum dibaca
         * ---------------------------------------------------------- */
        if (current_tick - last_check_tick >= 10) {  /* Setiap 10ms */
            last_check_tick = current_tick;

            uint16_t available = Get_Available_Data();

            if (available > 0) {
                /* Baca data dari circular buffer ke process buffer */
                uint16_t bytes_read = Read_Circular_Buffer(
                    process_buffer, PROCESS_BUFFER_SIZE - 1);

                if (bytes_read > 0) {
                    process_buffer[bytes_read] = '\0';  /* Null terminate */

                    /* Proses dan echo data */
                    Process_Received_Data(process_buffer, bytes_read);

                    /* Toggle LED sebagai indikator data diterima */
                    LED_TOGGLE();
                }
            }
        }

        /* ----------------------------------------------------------
         * DMA Half Complete event
         * ---------------------------------------------------------- */
        if (dma_half_complete) {
            dma_half_complete = 0;
            printf("[INFO] DMA Half-Complete (buffer 50%% terisi)\r\n");
        }

        /* ----------------------------------------------------------
         * DMA Full Complete event (buffer wrap around)
         * ---------------------------------------------------------- */
        if (dma_full_complete) {
            dma_full_complete = 0;
            printf("[INFO] DMA Full-Complete (buffer wrap-around)\r\n");
        }

        /* ----------------------------------------------------------
         * Cetak statistik periodik
         * ---------------------------------------------------------- */
        if (current_tick - last_stats_tick >= STATS_PRINT_INTERVAL_MS) {
            last_stats_tick = current_tick;
            Print_Stats();
        }
    }
}

/* ============================================================
 * USART1 IRQ Handler
 * Menangani IDLE line interrupt untuk deteksi akhir frame
 * ============================================================ */
void USART1_IRQHandler(void)
{
    /* Cek apakah IDLE flag aktif */
    if (__HAL_UART_GET_FLAG(&huart1, UART_FLAG_IDLE)) {
        /* Clear IDLE flag dengan membaca SR lalu DR */
        __HAL_UART_CLEAR_IDLEFLAG(&huart1);
        uart_idle_detected = 1;
    }

    /* Handle interrupt lainnya via HAL */
    HAL_UART_IRQHandler(&huart1);
}

/* ============================================================
 * SystemClock_Config
 * HSE 8MHz → PLL x9 → SYSCLK 72MHz
 * ============================================================ */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    /* HSE + PLL */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }

    /* Bus clocks */
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
 * - PC13: LED output (active low)
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
}

/* ============================================================
 * MX_DMA_Init
 * Inisialisasi DMA1 untuk USART1:
 *   - Channel 4: USART1_TX (opsional, untuk DMA TX)
 *   - Channel 5: USART1_RX (utama, circular mode)
 * ============================================================ */
static void MX_DMA_Init(void)
{
    __HAL_RCC_DMA1_CLK_ENABLE();

    /* NVIC untuk DMA1 Channel 5 (USART1_RX) */
    HAL_NVIC_SetPriority(UART_RX_DMA_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(UART_RX_DMA_IRQn);

    /* NVIC untuk USART1 (IDLE interrupt) */
    HAL_NVIC_SetPriority(USART1_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(USART1_IRQn);
}

/* ============================================================
 * MX_USART1_Init
 * USART1 dengan DMA RX circular:
 *   - Baud rate: 115200
 *   - 8N1
 *   - DMA RX enabled (circular)
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
 * Inisialisasi low-level UART + DMA:
 *   - Clock USART1, GPIOA, DMA1
 *   - PA9 (TX): AF push-pull
 *   - PA10 (RX): Input floating
 *   - DMA1 Channel 5: USART1_RX circular
 * ============================================================ */
void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    if (huart->Instance == USART1) {
        /* Aktifkan clock */
        __HAL_RCC_USART1_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();
        __HAL_RCC_DMA1_CLK_ENABLE();

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

        /* ----------------------------------------------------------
         * DMA1 Channel 5 - USART1_RX (CIRCULAR MODE)
         * Ini adalah kunci dari program ini:
         *   - Direction: Periferal → Memori
         *   - PeriphInc: Disable (alamat USART1->DR tetap)
         *   - MemInc: Enable (alamat memori naik)
         *   - Mode: CIRCULAR (otomatis kembali ke awal buffer)
         *   - Priority: HIGH
         * ---------------------------------------------------------- */
        hdma_usart1_rx.Instance = UART_RX_DMA_CHANNEL;
        hdma_usart1_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
        hdma_usart1_rx.Init.PeriphInc = DMA_PINC_DISABLE;
        hdma_usart1_rx.Init.MemInc = DMA_MINC_ENABLE;
        hdma_usart1_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
        hdma_usart1_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
        hdma_usart1_rx.Init.Mode = DMA_CIRCULAR;           /* CIRCULAR! */
        hdma_usart1_rx.Init.Priority = DMA_PRIORITY_HIGH;
        if (HAL_DMA_Init(&hdma_usart1_rx) != HAL_OK) {
            Error_Handler();
        }

        /* Link DMA handle ke UART handle */
        __HAL_LINKDMA(huart, hdmarx, hdma_usart1_rx);
    }
}

/* ============================================================
 * Interrupt Handlers
 * ============================================================ */

/* SysTick - 1ms tick untuk HAL */
void SysTick_Handler(void)
{
    HAL_IncTick();
}

/* DMA1 Channel 5 IRQ - USART1_RX DMA */
void DMA1_Channel5_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&hdma_usart1_rx);
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
