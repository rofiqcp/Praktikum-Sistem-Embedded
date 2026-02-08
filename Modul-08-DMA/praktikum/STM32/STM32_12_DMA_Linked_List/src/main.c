/**
 * ============================================================================
 * STM32_12_DMA_Linked_List - Software Scatter-Gather DMA
 * ============================================================================
 * Deskripsi : Implementasi teknik linked-list DMA secara software pada
 *             STM32F103 yang tidak memiliki hardware scatter-gather.
 *
 *             Cara kerja:
 *             1. Definisikan daftar deskriptor transfer (src, dst, size)
 *             2. Mulai DMA untuk deskriptor pertama
 *             3. Pada interrupt Transfer Complete, muat deskriptor berikutnya
 *             4. Restart DMA untuk deskriptor baru
 *             5. Ulangi sampai semua deskriptor selesai
 *
 * Use Case  : Mengumpulkan data dari beberapa buffer terpisah (non-contiguous)
 *             ke satu buffer destinasi yang kontinu.
 *             Mirip dengan operasi scatter-gather pada DMA modern.
 *
 * Hardware  :
 *   - STM32F103C8 (Blue Pill) @ 72MHz
 *   - LED pada PC13 (indikator)
 *   - USART1 PA9(TX)/PA10(RX) @ 115200 baud
 *   - DMA1 Channel 1 (Memory-to-Memory)
 * ============================================================================
 */

#include "stm32f1xx_hal.h"
#include "config.h"
#include <stdio.h>
#include <string.h>

/* ==================== Handle Periferal ==================== */
static UART_HandleTypeDef huart1;
static DMA_HandleTypeDef  hdma_m2m;

/* ==================== Struktur Deskriptor DMA ==================== */
/**
 * DMA_Descriptor_t - Deskriptor transfer DMA
 *
 * Setiap deskriptor mendefinisikan satu operasi transfer:
 *   - src_addr  : alamat sumber data
 *   - dst_addr  : alamat tujuan data
 *   - size      : ukuran transfer (bytes)
 *   - completed : flag apakah transfer ini sudah selesai
 *
 * Linked-list diimplementasikan sebagai array deskriptor
 * dengan indeks yang di-increment pada setiap completion.
 */
typedef struct {
    uint32_t src_addr;      /* Alamat sumber (memory) */
    uint32_t dst_addr;      /* Alamat tujuan (memory) */
    uint16_t size;          /* Ukuran transfer dalam bytes */
    uint8_t  completed;     /* Status: 0=pending, 1=selesai */
    uint8_t  id;            /* ID deskriptor untuk identifikasi */
} DMA_Descriptor_t;

/* ==================== Buffer Data ==================== */
/* Buffer sumber - terpisah (non-contiguous) di memori */
static uint8_t src_buf_0[DESC_BUF_SIZE_0] __attribute__((aligned(4)));
static uint8_t src_buf_1[DESC_BUF_SIZE_1] __attribute__((aligned(4)));
static uint8_t src_buf_2[DESC_BUF_SIZE_2] __attribute__((aligned(4)));
static uint8_t src_buf_3[DESC_BUF_SIZE_3] __attribute__((aligned(4)));
static uint8_t src_buf_4[DESC_BUF_SIZE_4] __attribute__((aligned(4)));

/* Buffer destinasi - kontinu untuk menampung semua data */
static uint8_t dest_buffer[DEST_BUFFER_SIZE] __attribute__((aligned(4)));

/* ==================== Linked List Deskriptor ==================== */
static DMA_Descriptor_t descriptor_list[MAX_DESCRIPTORS];
static volatile uint8_t  current_descriptor = 0;     /* Indeks deskriptor aktif */
static volatile uint8_t  total_descriptors  = 0;     /* Total deskriptor */
static volatile uint8_t  chain_complete     = 0;     /* Seluruh chain selesai */
static volatile uint8_t  chain_error        = 0;     /* Error dalam chain */
static volatile uint32_t chain_start_cycle  = 0;     /* DWT cycle saat mulai */
static volatile uint32_t chain_end_cycle    = 0;     /* DWT cycle saat selesai */

/* Statistik */
static uint32_t total_chains_completed = 0;
static uint32_t total_bytes_transferred = 0;
static uint32_t total_dma_errors = 0;
static uint32_t test_number = 0;

/* ==================== Prototipe Fungsi ==================== */
static void SystemClock_Config(void);
static void GPIO_Init(void);
static void USART1_Init(void);
static void DMA_Init(void);
static void DWT_Init(void);
static void Error_Handler(void);

/* Fungsi linked-list DMA */
static void Init_Descriptors(void);
static void Fill_Source_Buffers(uint8_t base_pattern);
static void Start_DMA_Chain(void);
static void Execute_Next_Descriptor(void);
static uint8_t Verify_Chain_Transfer(void);
static void Print_Descriptor_List(void);
static void Print_Buffer_Contents(const char *label, uint8_t *buf, uint16_t size);
static void Print_Separator(void);

/* Demo fungsi */
static void Demo_Basic_Chain(void);
static void Demo_Verify_Data(void);
static void Demo_Timing_Analysis(void);
static void Demo_Multiple_Chains(void);
static void Demo_Reverse_Gather(void);

/* ==================== Retarget printf ke USART1 ==================== */
int _write(int file, char *ptr, int len)
{
    (void)file;
    HAL_UART_Transmit(&huart1, (uint8_t *)ptr, len, HAL_MAX_DELAY);
    return len;
}

/* ==================== Interrupt Handlers ==================== */
void SysTick_Handler(void)
{
    HAL_IncTick();
}

/**
 * DMA1_Channel1_IRQHandler - Interrupt DMA Memory-to-Memory
 * Dipanggil setiap kali satu deskriptor selesai.
 */
void DMA1_Channel1_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&hdma_m2m);
}

/* ==================== Callback DMA ==================== */
/**
 * DMA_XferCpltCallback - Callback Transfer Complete
 *
 * Ini adalah inti dari teknik software linked-list:
 * 1. Tandai deskriptor saat ini sebagai selesai
 * 2. Jika masih ada deskriptor, muat yang berikutnya
 * 3. Jika semua selesai, set flag chain_complete
 */
static void DMA_XferCpltCallback(DMA_HandleTypeDef *hdma)
{
    if (hdma->Instance == LL_DMA_CHANNEL)
    {
        /* Tandai deskriptor aktif sebagai selesai */
        if (current_descriptor < total_descriptors)
        {
            descriptor_list[current_descriptor].completed = 1;
            current_descriptor++;
        }

        /* Cek apakah masih ada deskriptor berikutnya */
        if (current_descriptor < total_descriptors)
        {
            /* Masih ada! Lanjutkan ke deskriptor berikutnya */
            Execute_Next_Descriptor();
        }
        else
        {
            /* Semua deskriptor selesai */
            chain_complete = 1;
            chain_end_cycle = DWT_CYCCNT;
            total_chains_completed++;
            HAL_GPIO_TogglePin(LED_PORT, LED_PIN);
        }
    }
}

/**
 * DMA_XferErrorCallback - Callback Error
 */
static void DMA_XferErrorCallback(DMA_HandleTypeDef *hdma)
{
    if (hdma->Instance == LL_DMA_CHANNEL)
    {
        chain_error = 1;
        total_dma_errors++;
    }
}

/* ==================== Konfigurasi Sistem Clock ==================== */
static void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

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
static void GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    LED_GPIO_CLK_ENABLE();

    GPIO_InitStruct.Pin   = LED_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_PORT, &GPIO_InitStruct);
    HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET);
}

/* ==================== Inisialisasi USART1 ==================== */
static void USART1_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    USART_GPIO_CLK_ENABLE();
    USART_CLK_ENABLE();

    GPIO_InitStruct.Pin   = USART_TX_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(USART_TX_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin  = USART_RX_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(USART_RX_PORT, &GPIO_InitStruct);

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
static void DMA_Init(void)
{
    DMA_CLK_ENABLE();

    hdma_m2m.Instance                 = LL_DMA_CHANNEL;
    hdma_m2m.Init.Direction           = DMA_MEMORY_TO_MEMORY;
    hdma_m2m.Init.PeriphInc           = DMA_PINC_ENABLE;   /* Source increment */
    hdma_m2m.Init.MemInc              = DMA_MINC_ENABLE;   /* Dest increment */
    hdma_m2m.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_m2m.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
    hdma_m2m.Init.Mode                = DMA_NORMAL;
    hdma_m2m.Init.Priority            = DMA_PRIORITY_HIGH;

    if (HAL_DMA_Init(&hdma_m2m) != HAL_OK)
    {
        Error_Handler();
    }

    /* Register callback untuk linked-list chain */
    HAL_DMA_RegisterCallback(&hdma_m2m, HAL_DMA_XFER_CPLT_CB_ID,
                             DMA_XferCpltCallback);
    HAL_DMA_RegisterCallback(&hdma_m2m, HAL_DMA_XFER_ERROR_CB_ID,
                             DMA_XferErrorCallback);

    HAL_NVIC_SetPriority(LL_DMA_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(LL_DMA_IRQn);
}

/* ==================== Inisialisasi DWT ==================== */
static void DWT_Init(void)
{
    DEMCR |= DEMCR_TRCENA;
    DWT_CYCCNT = 0;
    DWT_CONTROL |= DWT_CTRL_CYCCNTENA;
}

/* ==================== Fungsi Utilitas ==================== */
static void Print_Separator(void)
{
    printf("================================================\r\n");
}

static void Print_Buffer_Contents(const char *label, uint8_t *buf, uint16_t size)
{
    printf("  %s (%u bytes): ", label, size);
    uint16_t psize = (size > 16) ? 16 : size;
    for (uint16_t i = 0; i < psize; i++)
    {
        printf("%02X ", buf[i]);
    }
    if (size > 16) printf("...");
    printf("\r\n");
}

/* ==================== Inisialisasi Deskriptor ==================== */
/**
 * Init_Descriptors - Setup daftar deskriptor linked-list
 *
 * Konfigurasi 4 deskriptor yang akan mengumpulkan data dari
 * 4 buffer sumber terpisah ke satu buffer destinasi kontinu.
 *
 * Layout destinasi:
 * |--src_buf_0--|--src_buf_1--|--src_buf_2--|--src_buf_3--|
 * 0            32            80           104           168
 */
static void Init_Descriptors(void)
{
    uint32_t dst_offset = 0;

    /* Deskriptor 0: src_buf_0 → dest_buffer[0..31] */
    descriptor_list[0].src_addr  = (uint32_t)src_buf_0;
    descriptor_list[0].dst_addr  = (uint32_t)&dest_buffer[dst_offset];
    descriptor_list[0].size      = DESC_BUF_SIZE_0;
    descriptor_list[0].completed = 0;
    descriptor_list[0].id        = 0;
    dst_offset += DESC_BUF_SIZE_0;

    /* Deskriptor 1: src_buf_1 → dest_buffer[32..79] */
    descriptor_list[1].src_addr  = (uint32_t)src_buf_1;
    descriptor_list[1].dst_addr  = (uint32_t)&dest_buffer[dst_offset];
    descriptor_list[1].size      = DESC_BUF_SIZE_1;
    descriptor_list[1].completed = 0;
    descriptor_list[1].id        = 1;
    dst_offset += DESC_BUF_SIZE_1;

    /* Deskriptor 2: src_buf_2 → dest_buffer[80..103] */
    descriptor_list[2].src_addr  = (uint32_t)src_buf_2;
    descriptor_list[2].dst_addr  = (uint32_t)&dest_buffer[dst_offset];
    descriptor_list[2].size      = DESC_BUF_SIZE_2;
    descriptor_list[2].completed = 0;
    descriptor_list[2].id        = 2;
    dst_offset += DESC_BUF_SIZE_2;

    /* Deskriptor 3: src_buf_3 → dest_buffer[104..167] */
    descriptor_list[3].src_addr  = (uint32_t)src_buf_3;
    descriptor_list[3].dst_addr  = (uint32_t)&dest_buffer[dst_offset];
    descriptor_list[3].size      = DESC_BUF_SIZE_3;
    descriptor_list[3].completed = 0;
    descriptor_list[3].id        = 3;
    dst_offset += DESC_BUF_SIZE_3;

    total_descriptors  = NUM_DEMO_DESCRIPTORS;
    current_descriptor = 0;
    chain_complete     = 0;
    chain_error        = 0;

    printf("  Total bytes yang akan ditransfer: %lu\r\n", dst_offset);
}

/* ==================== Isi Buffer Sumber ==================== */
/**
 * Fill_Source_Buffers - Isi semua buffer sumber dengan pola unik
 * @param base_pattern : byte dasar pola (tiap buffer offset berbeda)
 */
static void Fill_Source_Buffers(uint8_t base_pattern)
{
    for (uint16_t i = 0; i < DESC_BUF_SIZE_0; i++)
        src_buf_0[i] = (uint8_t)(base_pattern + i);

    for (uint16_t i = 0; i < DESC_BUF_SIZE_1; i++)
        src_buf_1[i] = (uint8_t)(base_pattern + 0x40 + i);

    for (uint16_t i = 0; i < DESC_BUF_SIZE_2; i++)
        src_buf_2[i] = (uint8_t)(base_pattern + 0x80 + i);

    for (uint16_t i = 0; i < DESC_BUF_SIZE_3; i++)
        src_buf_3[i] = (uint8_t)(base_pattern + 0xC0 + i);

    /* Bersihkan destinasi */
    memset(dest_buffer, 0, DEST_BUFFER_SIZE);
}

/* ==================== Eksekusi Chain DMA ==================== */
/**
 * Start_DMA_Chain - Mulai eksekusi chain dari deskriptor pertama
 * Ini memulai transfer pertama; setelah selesai, callback akan
 * memuat deskriptor berikutnya secara otomatis.
 */
static void Start_DMA_Chain(void)
{
    current_descriptor = 0;
    chain_complete     = 0;
    chain_error        = 0;

    /* Reset semua flag completion */
    for (uint8_t i = 0; i < total_descriptors; i++)
    {
        descriptor_list[i].completed = 0;
    }

    /* Catat waktu mulai */
    chain_start_cycle = DWT_CYCCNT;

    /* Mulai transfer deskriptor pertama */
    Execute_Next_Descriptor();
}

/**
 * Execute_Next_Descriptor - Konfigurasi dan mulai DMA untuk deskriptor aktif
 *
 * Fungsi ini dipanggil:
 * 1. Pertama kali oleh Start_DMA_Chain()
 * 2. Selanjutnya oleh callback DMA_XferCpltCallback()
 */
static void Execute_Next_Descriptor(void)
{
    if (current_descriptor >= total_descriptors)
    {
        chain_complete = 1;
        return;
    }

    DMA_Descriptor_t *desc = &descriptor_list[current_descriptor];

    /* Re-inisialisasi DMA untuk transfer baru */
    /* Catatan: Pada STM32F103, DMA harus di-deinit dan re-init */
    /*          antara setiap transfer karena register harus di-reset. */
    HAL_DMA_DeInit(&hdma_m2m);

    hdma_m2m.Instance                 = LL_DMA_CHANNEL;
    hdma_m2m.Init.Direction           = DMA_MEMORY_TO_MEMORY;
    hdma_m2m.Init.PeriphInc           = DMA_PINC_ENABLE;
    hdma_m2m.Init.MemInc              = DMA_MINC_ENABLE;
    hdma_m2m.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_m2m.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
    hdma_m2m.Init.Mode                = DMA_NORMAL;
    hdma_m2m.Init.Priority            = DMA_PRIORITY_HIGH;

    HAL_DMA_Init(&hdma_m2m);

    /* Register callback lagi setelah DeInit */
    HAL_DMA_RegisterCallback(&hdma_m2m, HAL_DMA_XFER_CPLT_CB_ID,
                             DMA_XferCpltCallback);
    HAL_DMA_RegisterCallback(&hdma_m2m, HAL_DMA_XFER_ERROR_CB_ID,
                             DMA_XferErrorCallback);

    /* Mulai transfer dari deskriptor saat ini */
    HAL_StatusTypeDef status = HAL_DMA_Start_IT(&hdma_m2m,
                                                 desc->src_addr,
                                                 desc->dst_addr,
                                                 desc->size);

    if (status != HAL_OK)
    {
        chain_error = 1;
        total_dma_errors++;
    }
}

/* ==================== Verifikasi Transfer ==================== */
/**
 * Verify_Chain_Transfer - Verifikasi seluruh chain transfer
 *
 * Periksa apakah data di buffer destinasi cocok dengan sumber.
 * @return 1 jika semua benar, 0 jika ada error
 */
static uint8_t Verify_Chain_Transfer(void)
{
    uint8_t all_ok = 1;
    uint32_t dst_offset = 0;
    uint8_t *src_bufs[] = {src_buf_0, src_buf_1, src_buf_2, src_buf_3, src_buf_4};

    for (uint8_t d = 0; d < total_descriptors; d++)
    {
        uint16_t sz = descriptor_list[d].size;
        uint8_t match = 1;

        for (uint16_t i = 0; i < sz; i++)
        {
            if (dest_buffer[dst_offset + i] != src_bufs[d][i])
            {
                match = 0;
                all_ok = 0;
                printf("  ERROR: Desc[%u] byte[%u] expected=0x%02X got=0x%02X\r\n",
                       d, i, src_bufs[d][i], dest_buffer[dst_offset + i]);
                break;
            }
        }

        printf("  Desc[%u]: %s (%u bytes)\r\n", d, match ? "OK" : "GAGAL", sz);
        dst_offset += sz;
    }

    return all_ok;
}

/* ==================== Cetak Deskriptor ==================== */
static void Print_Descriptor_List(void)
{
    printf("  Daftar Deskriptor DMA:\r\n");
    printf("  %-4s | %-12s | %-12s | %-6s | %-8s\r\n",
           "ID", "Source", "Dest", "Size", "Status");
    printf("  ---- | ------------ | ------------ | ------ | --------\r\n");

    for (uint8_t i = 0; i < total_descriptors; i++)
    {
        DMA_Descriptor_t *d = &descriptor_list[i];
        printf("  %-4u | 0x%08lX | 0x%08lX | %-6u | %s\r\n",
               d->id, d->src_addr, d->dst_addr, d->size,
               d->completed ? "Selesai" : "Pending");

        printf("[DATA] DESC,id=%u,src=0x%08lX,dst=0x%08lX,size=%u,done=%u\r\n",
               d->id, d->src_addr, d->dst_addr, d->size, d->completed);
    }
}

/* ==================== Demo: Chain Dasar ==================== */
static void Demo_Basic_Chain(void)
{
    test_number++;
    Print_Separator();
    printf("[TEST %lu] Demo Chain DMA Dasar\r\n", test_number);
    Print_Separator();

    /* Isi buffer sumber */
    Fill_Source_Buffers(FILL_PATTERN_BASE);

    /* Setup deskriptor */
    Init_Descriptors();
    printf("\r\n");

    /* Tampilkan deskriptor */
    Print_Descriptor_List();
    printf("\r\n");

    /* Tampilkan isi buffer sumber */
    printf("  Buffer sumber:\r\n");
    Print_Buffer_Contents("src_buf_0", src_buf_0, DESC_BUF_SIZE_0);
    Print_Buffer_Contents("src_buf_1", src_buf_1, DESC_BUF_SIZE_1);
    Print_Buffer_Contents("src_buf_2", src_buf_2, DESC_BUF_SIZE_2);
    Print_Buffer_Contents("src_buf_3", src_buf_3, DESC_BUF_SIZE_3);
    printf("\r\n");

    /* Mulai chain transfer */
    printf("  Memulai DMA chain transfer...\r\n");
    Start_DMA_Chain();

    /* Tunggu chain selesai */
    uint32_t timeout = HAL_GetTick() + TRANSFER_TIMEOUT_MS;
    while (!chain_complete && !chain_error)
    {
        if (HAL_GetTick() > timeout)
        {
            printf("  TIMEOUT menunggu chain selesai!\r\n");
            break;
        }
    }

    if (chain_complete)
    {
        uint32_t elapsed_cycles = chain_end_cycle - chain_start_cycle;
        float elapsed_us = (float)elapsed_cycles / (CPU_FREQ_HZ / 1000000);

        printf("  Chain selesai dalam %lu siklus (%.2f us)\r\n",
               elapsed_cycles, elapsed_us);

        /* Hitung total bytes */
        uint32_t total_bytes = 0;
        for (uint8_t i = 0; i < total_descriptors; i++)
            total_bytes += descriptor_list[i].size;
        total_bytes_transferred += total_bytes;

        float throughput = (float)total_bytes * CPU_FREQ_HZ /
                          ((float)elapsed_cycles * 1000000.0f);

        printf("  Total bytes: %lu, Throughput: %.2f MB/s\r\n",
               total_bytes, throughput);

        printf("[DATA] CHAIN,descs=%u,bytes=%lu,cycles=%lu,us=%.2f,mbs=%.2f\r\n",
               total_descriptors, total_bytes, elapsed_cycles,
               elapsed_us, throughput);
    }
    else
    {
        printf("  Chain GAGAL! Error=%u\r\n", chain_error);
        printf("[DATA] CHAIN_ERROR,desc=%u,error=%u\r\n",
               current_descriptor, chain_error);
    }

    /* Tampilkan status akhir deskriptor */
    printf("\r\n  Status akhir:\r\n");
    Print_Descriptor_List();
}

/* ==================== Demo: Verifikasi Data ==================== */
static void Demo_Verify_Data(void)
{
    test_number++;
    Print_Separator();
    printf("[TEST %lu] Verifikasi Data Hasil Transfer\r\n", test_number);
    Print_Separator();

    /* Tampilkan isi buffer destinasi */
    printf("  Buffer destinasi setelah chain transfer:\r\n");

    uint32_t offset = 0;
    for (uint8_t d = 0; d < total_descriptors; d++)
    {
        char label[32];
        snprintf(label, sizeof(label), "dest[%lu..%lu]",
                 offset, offset + descriptor_list[d].size - 1);
        Print_Buffer_Contents(label, &dest_buffer[offset], descriptor_list[d].size);
        offset += descriptor_list[d].size;
    }

    printf("\r\n  Verifikasi integritas data:\r\n");
    uint8_t result = Verify_Chain_Transfer();
    printf("\r\n  Hasil: %s\r\n",
           result ? "SEMUA DATA TERVERIFIKASI BENAR" : "ADA KESALAHAN DATA");

    printf("[DATA] VERIFY,result=%s,descs=%u\r\n",
           result ? "PASS" : "FAIL", total_descriptors);
}

/* ==================== Demo: Analisis Timing ==================== */
static void Demo_Timing_Analysis(void)
{
    test_number++;
    Print_Separator();
    printf("[TEST %lu] Analisis Timing Chain DMA\r\n", test_number);
    Print_Separator();

    printf("  Menjalankan %d iterasi chain transfer...\r\n\r\n", TEST_ITERATIONS);

    uint32_t min_cycles = 0xFFFFFFFF;
    uint32_t max_cycles = 0;
    uint32_t total_cycles = 0;

    for (int iter = 0; iter < TEST_ITERATIONS; iter++)
    {
        /* Reset dan isi buffer */
        Fill_Source_Buffers((uint8_t)(FILL_PATTERN_BASE + iter * 0x10));
        Init_Descriptors();

        /* Jalankan chain */
        Start_DMA_Chain();

        /* Tunggu selesai */
        uint32_t timeout = HAL_GetTick() + TRANSFER_TIMEOUT_MS;
        while (!chain_complete && !chain_error)
        {
            if (HAL_GetTick() > timeout) break;
        }

        if (chain_complete)
        {
            uint32_t cycles = chain_end_cycle - chain_start_cycle;
            total_cycles += cycles;
            if (cycles < min_cycles) min_cycles = cycles;
            if (cycles > max_cycles) max_cycles = cycles;

            printf("  Iterasi %d: %lu siklus\r\n", iter + 1, cycles);
            printf("[DATA] TIMING,iter=%d,cycles=%lu\r\n", iter + 1, cycles);
        }
        else
        {
            printf("  Iterasi %d: ERROR\r\n", iter + 1);
        }

        HAL_Delay(50);
    }

    uint32_t avg_cycles = total_cycles / TEST_ITERATIONS;
    float avg_us = (float)avg_cycles / (CPU_FREQ_HZ / 1000000);

    printf("\r\n  Hasil Timing (%d iterasi):\r\n", TEST_ITERATIONS);
    printf("  Minimum : %lu siklus\r\n", min_cycles);
    printf("  Maksimum: %lu siklus\r\n", max_cycles);
    printf("  Rata-rata: %lu siklus (%.2f us)\r\n", avg_cycles, avg_us);
    printf("  Jitter  : %lu siklus\r\n", max_cycles - min_cycles);

    printf("[DATA] TIMING_SUMMARY,min=%lu,max=%lu,avg=%lu,avg_us=%.2f,jitter=%lu\r\n",
           min_cycles, max_cycles, avg_cycles, avg_us, max_cycles - min_cycles);
}

/* ==================== Demo: Multiple Chains ==================== */
static void Demo_Multiple_Chains(void)
{
    test_number++;
    Print_Separator();
    printf("[TEST %lu] Beberapa Chain Berturut-turut\r\n", test_number);
    Print_Separator();

    /* Jalankan 3 chain dengan pola berbeda */
    uint8_t patterns[] = {0x10, 0x50, 0x90};

    for (int c = 0; c < 3; c++)
    {
        printf("\r\n  --- Chain %d (pola 0x%02X) ---\r\n", c + 1, patterns[c]);

        Fill_Source_Buffers(patterns[c]);
        Init_Descriptors();
        Start_DMA_Chain();

        uint32_t timeout = HAL_GetTick() + TRANSFER_TIMEOUT_MS;
        while (!chain_complete && !chain_error)
        {
            if (HAL_GetTick() > timeout) break;
        }

        if (chain_complete)
        {
            uint32_t cycles = chain_end_cycle - chain_start_cycle;
            uint8_t ok = Verify_Chain_Transfer();

            uint32_t total_bytes = 0;
            for (uint8_t i = 0; i < total_descriptors; i++)
                total_bytes += descriptor_list[i].size;

            printf("  Status: %s, %lu siklus, %lu bytes\r\n",
                   ok ? "PASS" : "FAIL", cycles, total_bytes);
            printf("[DATA] MULTI_CHAIN,chain=%d,pattern=0x%02X,cycles=%lu,ok=%u\r\n",
                   c + 1, patterns[c], cycles, ok);
        }
        else
        {
            printf("  Chain %d GAGAL!\r\n", c + 1);
        }

        HAL_Delay(500);
    }
}

/* ==================== Demo: Reverse Gather ==================== */
/**
 * Demo_Reverse_Gather - Scatter: satu buffer besar ke beberapa buffer kecil
 * Kebalikan dari gather (mengumpulkan), ini menyebarkan data.
 */
static void Demo_Reverse_Gather(void)
{
    test_number++;
    Print_Separator();
    printf("[TEST %lu] Demo Scatter (Kebalikan Gather)\r\n", test_number);
    Print_Separator();

    /* Isi dest_buffer sebagai sumber */
    for (uint16_t i = 0; i < DEST_BUFFER_SIZE; i++)
    {
        dest_buffer[i] = (uint8_t)(0xF0 + (i & 0x0F));
    }

    /* Bersihkan buffer tujuan (yang biasanya adalah sumber) */
    memset(src_buf_0, 0, DESC_BUF_SIZE_0);
    memset(src_buf_1, 0, DESC_BUF_SIZE_1);
    memset(src_buf_2, 0, DESC_BUF_SIZE_2);
    memset(src_buf_3, 0, DESC_BUF_SIZE_3);

    /* Setup deskriptor terbalik: dest_buffer → src_buf_x */
    uint32_t src_offset = 0;

    descriptor_list[0].src_addr  = (uint32_t)&dest_buffer[src_offset];
    descriptor_list[0].dst_addr  = (uint32_t)src_buf_0;
    descriptor_list[0].size      = DESC_BUF_SIZE_0;
    descriptor_list[0].completed = 0;
    descriptor_list[0].id        = 0;
    src_offset += DESC_BUF_SIZE_0;

    descriptor_list[1].src_addr  = (uint32_t)&dest_buffer[src_offset];
    descriptor_list[1].dst_addr  = (uint32_t)src_buf_1;
    descriptor_list[1].size      = DESC_BUF_SIZE_1;
    descriptor_list[1].completed = 0;
    descriptor_list[1].id        = 1;
    src_offset += DESC_BUF_SIZE_1;

    descriptor_list[2].src_addr  = (uint32_t)&dest_buffer[src_offset];
    descriptor_list[2].dst_addr  = (uint32_t)src_buf_2;
    descriptor_list[2].size      = DESC_BUF_SIZE_2;
    descriptor_list[2].completed = 0;
    descriptor_list[2].id        = 2;
    src_offset += DESC_BUF_SIZE_2;

    descriptor_list[3].src_addr  = (uint32_t)&dest_buffer[src_offset];
    descriptor_list[3].dst_addr  = (uint32_t)src_buf_3;
    descriptor_list[3].size      = DESC_BUF_SIZE_3;
    descriptor_list[3].completed = 0;
    descriptor_list[3].id        = 3;

    total_descriptors  = NUM_DEMO_DESCRIPTORS;
    current_descriptor = 0;
    chain_complete     = 0;
    chain_error        = 0;

    printf("  Scatter: 1 buffer besar → 4 buffer kecil\r\n");
    Print_Descriptor_List();
    printf("\r\n");

    /* Jalankan scatter chain */
    Start_DMA_Chain();

    uint32_t timeout = HAL_GetTick() + TRANSFER_TIMEOUT_MS;
    while (!chain_complete && !chain_error)
    {
        if (HAL_GetTick() > timeout) break;
    }

    if (chain_complete)
    {
        uint32_t cycles = chain_end_cycle - chain_start_cycle;
        printf("  Scatter selesai dalam %lu siklus\r\n", cycles);

        /* Verifikasi */
        printf("  Hasil scatter:\r\n");
        Print_Buffer_Contents("src_buf_0", src_buf_0, DESC_BUF_SIZE_0);
        Print_Buffer_Contents("src_buf_1", src_buf_1, DESC_BUF_SIZE_1);
        Print_Buffer_Contents("src_buf_2", src_buf_2, DESC_BUF_SIZE_2);
        Print_Buffer_Contents("src_buf_3", src_buf_3, DESC_BUF_SIZE_3);

        printf("[DATA] SCATTER,cycles=%lu,descs=%u\r\n", cycles, total_descriptors);
    }
    else
    {
        printf("  Scatter GAGAL!\r\n");
    }
}

/* ==================== Error Handler ==================== */
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
int main(void)
{
    HAL_Init();
    SystemClock_Config();
    GPIO_Init();
    USART1_Init();
    DMA_Init();
    DWT_Init();

    /* Banner */
    printf("\r\n");
    Print_Separator();
    printf("  STM32_12_DMA_Linked_List\r\n");
    printf("  Software Scatter-Gather DMA\r\n");
    printf("  MCU: STM32F103C8 @ 72MHz\r\n");
    Print_Separator();
    printf("\r\n");

    printf("CATATAN: STM32F103 tidak memiliki hardware scatter-gather DMA.\r\n");
    printf("Program ini menggunakan teknik SOFTWARE linked-list:\r\n");
    printf("  - Array deskriptor (src, dst, size)\r\n");
    printf("  - Interrupt Transfer Complete memuat deskriptor berikutnya\r\n");
    printf("  - Chain transfer otomatis sampai semua deskriptor selesai\r\n\r\n");

    printf("Alamat buffer sumber:\r\n");
    printf("  src_buf_0: 0x%08lX (%u bytes)\r\n", (uint32_t)src_buf_0, DESC_BUF_SIZE_0);
    printf("  src_buf_1: 0x%08lX (%u bytes)\r\n", (uint32_t)src_buf_1, DESC_BUF_SIZE_1);
    printf("  src_buf_2: 0x%08lX (%u bytes)\r\n", (uint32_t)src_buf_2, DESC_BUF_SIZE_2);
    printf("  src_buf_3: 0x%08lX (%u bytes)\r\n", (uint32_t)src_buf_3, DESC_BUF_SIZE_3);
    printf("  dest_buf : 0x%08lX (%u bytes)\r\n\r\n",
           (uint32_t)dest_buffer, DEST_BUFFER_SIZE);

    HAL_Delay(DEMO_DELAY_MS);

    /* === Jalankan semua demo === */

    /* Demo 1: Chain DMA dasar */
    Demo_Basic_Chain();
    HAL_Delay(DEMO_DELAY_MS);

    /* Demo 2: Verifikasi data */
    Demo_Verify_Data();
    HAL_Delay(DEMO_DELAY_MS);

    /* Demo 3: Analisis timing */
    Demo_Timing_Analysis();
    HAL_Delay(DEMO_DELAY_MS);

    /* Demo 4: Multiple chains */
    Demo_Multiple_Chains();
    HAL_Delay(DEMO_DELAY_MS);

    /* Demo 5: Scatter (kebalikan gather) */
    Demo_Reverse_Gather();
    HAL_Delay(DEMO_DELAY_MS);

    /* === Ringkasan === */
    Print_Separator();
    printf("RINGKASAN LINKED LIST DMA\r\n");
    Print_Separator();
    printf("  Total tes            : %lu\r\n", test_number);
    printf("  Chain berhasil       : %lu\r\n", total_chains_completed);
    printf("  Total bytes transfer : %lu\r\n", total_bytes_transferred);
    printf("  Error DMA            : %lu\r\n", total_dma_errors);
    printf("[DATA] SUMMARY,tests=%lu,chains=%lu,bytes=%lu,errors=%lu\r\n",
           test_number, total_chains_completed,
           total_bytes_transferred, total_dma_errors);
    printf("\r\n");

    /* Loop utama - heartbeat */
    uint32_t loop = 0;
    while (1)
    {
        HAL_GPIO_TogglePin(LED_PORT, LED_PIN);
        loop++;

        if (loop % 10 == 0)
        {
            printf("[DATA] HEARTBEAT,loop=%lu,uptime=%lu\r\n",
                   loop, HAL_GetTick() / 1000);
        }

        HAL_Delay(1000);
    }
}
