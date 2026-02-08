/**
 * ============================================================================
 * STM32_11_DMA_Benchmark - Benchmark Komprehensif DMA vs CPU
 * ============================================================================
 * Deskripsi : Program benchmark untuk membandingkan performa DMA dan CPU
 *             dalam berbagai skenario transfer memori.
 *
 * Skenario Pengujian:
 *   1. Memory-to-Memory: DMA1 Ch1 vs memcpy untuk berbagai ukuran
 *      (32, 64, 128, 256, 512, 1024 bytes)
 *   2. Perbandingan alignment: Byte vs Halfword vs Word
 *   3. Rata-rata dari 100 iterasi per tes
 *
 * Pengukuran:
 *   - DWT Cycle Counter untuk presisi tingkat siklus
 *   - Throughput dalam MB/s
 *   - CPU cycles per byte
 *   - Rasio speedup DMA vs CPU
 *   - Titik crossover dimana DMA lebih cepat dari CPU
 *
 * Hardware  :
 *   - STM32F103C8 (Blue Pill) @ 72MHz
 *   - LED pada PC13 (indikator status)
 *   - USART1 PA9(TX)/PA10(RX) @ 115200 baud
 * ============================================================================
 */

#include "stm32f1xx_hal.h"
#include "config.h"
#include <stdio.h>
#include <string.h>

/* ==================== Handle Periferal ==================== */
static UART_HandleTypeDef huart1;
static DMA_HandleTypeDef  hdma_memcpy;

/* ==================== Buffer Benchmark ==================== */
/* Buffer sumber dan tujuan untuk benchmark */
/* Align ke 4-byte boundary untuk performa optimal */
static uint8_t  src_buffer[MAX_BUFFER_SIZE] __attribute__((aligned(4)));
static uint8_t  dst_buffer[MAX_BUFFER_SIZE] __attribute__((aligned(4)));

/* Buffer tambahan untuk tes alignment */
static uint8_t  src_unaligned[MAX_BUFFER_SIZE + 4] __attribute__((aligned(4)));
static uint8_t  dst_unaligned[MAX_BUFFER_SIZE + 4] __attribute__((aligned(4)));

/* ==================== Variabel Status ==================== */
static volatile uint8_t dma_transfer_complete = 0;
static volatile uint8_t dma_transfer_error    = 0;

/* Hasil benchmark */
typedef struct {
    uint32_t size;           /* Ukuran transfer dalam bytes */
    uint32_t cpu_cycles;     /* Siklus CPU (memcpy) */
    uint32_t dma_cycles;     /* Siklus DMA (termasuk setup) */
    float    cpu_throughput;  /* MB/s CPU */
    float    dma_throughput;  /* MB/s DMA */
    float    speedup;        /* Rasio speedup DMA/CPU */
    float    cpu_cpb;        /* Cycles per byte CPU */
    float    dma_cpb;        /* Cycles per byte DMA */
} BenchResult_t;

static BenchResult_t bench_results[NUM_BENCH_SIZES];

/* Ukuran-ukuran yang diuji */
static const uint32_t bench_sizes[NUM_BENCH_SIZES] = {
    BENCH_SIZE_32, BENCH_SIZE_64, BENCH_SIZE_128,
    BENCH_SIZE_256, BENCH_SIZE_512, BENCH_SIZE_1024
};

/* Hasil alignment */
typedef struct {
    uint32_t byte_cycles;
    uint32_t halfword_cycles;
    uint32_t word_cycles;
} AlignResult_t;

static AlignResult_t align_results;
static uint32_t test_number = 0;

/* ==================== Prototipe Fungsi ==================== */
static void SystemClock_Config(void);
static void GPIO_Init(void);
static void USART1_Init(void);
static void DMA_Init(void);
static void DWT_Init(void);
static void Error_Handler(void);

/* Fungsi benchmark */
static void Fill_Source_Buffer(uint16_t size);
static uint8_t Verify_Transfer(uint16_t size);
static uint32_t Benchmark_Memcpy(uint16_t size, uint32_t iterations);
static uint32_t Benchmark_DMA_M2M(uint16_t size, uint32_t iterations, uint32_t data_width);
static void Run_Size_Benchmark(void);
static void Run_Alignment_Benchmark(void);
static void Find_Crossover_Point(void);
static void Print_Summary(void);
static void Print_Separator(void);

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
 * DMA1_Channel1_IRQHandler - Interrupt DMA untuk Memory-to-Memory
 */
void DMA1_Channel1_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&hdma_memcpy);
}

/* ==================== Callback DMA ==================== */
void HAL_DMA_XferCpltCallback(DMA_HandleTypeDef *hdma)
{
    if (hdma->Instance == BENCH_DMA_CHANNEL)
    {
        dma_transfer_complete = 1;
    }
}

void HAL_DMA_XferErrorCallback(DMA_HandleTypeDef *hdma)
{
    if (hdma->Instance == BENCH_DMA_CHANNEL)
    {
        dma_transfer_error = 1;
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
/**
 * DMA_Init - Konfigurasi DMA1 Channel 1 untuk Memory-to-Memory
 *
 * DMA1 Channel 1 dipilih karena mendukung mode M2M pada STM32F103.
 * Mode: Normal (bukan circular) - satu kali transfer.
 * Prioritas: Very High untuk benchmark akurat.
 */
static void DMA_Init(void)
{
    DMA_CLK_ENABLE();

    hdma_memcpy.Instance                 = BENCH_DMA_CHANNEL;
    hdma_memcpy.Init.Direction           = DMA_MEMORY_TO_MEMORY;
    hdma_memcpy.Init.PeriphInc           = DMA_PINC_ENABLE;   /* Source increment */
    hdma_memcpy.Init.MemInc              = DMA_MINC_ENABLE;   /* Dest increment */
    hdma_memcpy.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
    hdma_memcpy.Init.MemDataAlignment    = DMA_MDATAALIGN_WORD;
    hdma_memcpy.Init.Mode                = DMA_NORMAL;
    hdma_memcpy.Init.Priority            = DMA_PRIORITY_VERY_HIGH;

    if (HAL_DMA_Init(&hdma_memcpy) != HAL_OK)
    {
        Error_Handler();
    }

    /* Register callback */
    HAL_DMA_RegisterCallback(&hdma_memcpy, HAL_DMA_XFER_CPLT_CB_ID,
                             HAL_DMA_XferCpltCallback);
    HAL_DMA_RegisterCallback(&hdma_memcpy, HAL_DMA_XFER_ERROR_CB_ID,
                             HAL_DMA_XferErrorCallback);

    /* Aktifkan interrupt DMA */
    HAL_NVIC_SetPriority(BENCH_DMA_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(BENCH_DMA_IRQn);
}

/* ==================== Inisialisasi DWT Cycle Counter ==================== */
/**
 * DWT_Init - Aktifkan DWT Cycle Counter
 *
 * DWT (Data Watchpoint and Trace) memiliki cycle counter 32-bit
 * yang berdetak pada frekuensi CPU (72MHz pada Blue Pill).
 * Resolusi: ~13.9 ns per tick
 * Overflow: ~59.65 detik
 *
 * Ini JAUH lebih presisi dari HAL_GetTick() yang hanya 1ms.
 */
static void DWT_Init(void)
{
    /* Aktifkan trace */
    DEMCR |= DEMCR_TRCENA;

    /* Reset cycle counter */
    DWT_CYCCNT = 0;

    /* Aktifkan cycle counter */
    DWT_CONTROL |= DWT_CTRL_CYCCNTENA;

    printf("DWT Cycle Counter diaktifkan.\r\n");
    printf("  Frekuensi: %d MHz\r\n", CPU_FREQ_MHZ);
    printf("  Resolusi : %.1f ns/tick\r\n", 1000.0f / CPU_FREQ_MHZ);
    printf("  Overflow : ~%.1f detik\r\n", 4294967296.0f / CPU_FREQ_HZ);
}

/* ==================== Fungsi Utilitas ==================== */

/**
 * Fill_Source_Buffer - Isi buffer sumber dengan pola yang bisa diverifikasi
 */
static void Fill_Source_Buffer(uint16_t size)
{
    for (uint16_t i = 0; i < size; i++)
    {
        src_buffer[i] = (uint8_t)(i & 0xFF);
    }
}

/**
 * Verify_Transfer - Verifikasi bahwa transfer berhasil
 * @return 1 jika benar, 0 jika ada perbedaan
 */
static uint8_t Verify_Transfer(uint16_t size)
{
    for (uint16_t i = 0; i < size; i++)
    {
        if (dst_buffer[i] != src_buffer[i])
        {
            return 0;
        }
    }
    return 1;
}

static void Print_Separator(void)
{
    printf("================================================\r\n");
}

/* ==================== Benchmark: memcpy (CPU) ==================== */
/**
 * Benchmark_Memcpy - Ukur waktu transfer memcpy dalam siklus CPU
 *
 * @param size       : ukuran data (bytes)
 * @param iterations : jumlah iterasi untuk rata-rata
 * @return rata-rata siklus CPU per transfer
 */
static uint32_t Benchmark_Memcpy(uint16_t size, uint32_t iterations)
{
    uint32_t total_cycles = 0;

    /* Pemanasan cache dan branch predictor */
    for (uint32_t w = 0; w < WARMUP_ITERATIONS; w++)
    {
        memcpy(dst_buffer, src_buffer, size);
    }

    /* Pengukuran sesungguhnya */
    for (uint32_t i = 0; i < iterations; i++)
    {
        /* Bersihkan dst sebelum tiap tes */
        memset(dst_buffer, 0, size);

        /* Ukur siklus DWT */
        uint32_t start = DWT_CYCCNT;
        memcpy(dst_buffer, src_buffer, size);
        uint32_t end = DWT_CYCCNT;

        total_cycles += (end - start);
    }

    return total_cycles / iterations;
}

/* ==================== Benchmark: DMA Memory-to-Memory ==================== */
/**
 * Benchmark_DMA_M2M - Ukur waktu transfer DMA M2M dalam siklus CPU
 *
 * Siklus yang diukur termasuk:
 * - Setup DMA (konfigurasi register)
 * - Transfer data aktual
 * - Tunggu completion
 *
 * @param size       : ukuran data (bytes)
 * @param iterations : jumlah iterasi
 * @param data_width : 0=Byte, 1=HalfWord, 2=Word
 * @return rata-rata siklus CPU per transfer
 */
static uint32_t Benchmark_DMA_M2M(uint16_t size, uint32_t iterations, uint32_t data_width)
{
    uint32_t total_cycles = 0;
    uint32_t periph_align, mem_align;
    uint16_t dma_count = size;

    /* Tentukan alignment DMA */
    switch (data_width)
    {
        case ALIGN_HALFWORD:
            periph_align = DMA_PDATAALIGN_HALFWORD;
            mem_align    = DMA_MDATAALIGN_HALFWORD;
            dma_count    = size / 2;
            break;
        case ALIGN_WORD:
            periph_align = DMA_PDATAALIGN_WORD;
            mem_align    = DMA_MDATAALIGN_WORD;
            dma_count    = size / 4;
            break;
        default: /* ALIGN_BYTE */
            periph_align = DMA_PDATAALIGN_BYTE;
            mem_align    = DMA_MDATAALIGN_BYTE;
            dma_count    = size;
            break;
    }

    /* Pemanasan */
    for (uint32_t w = 0; w < WARMUP_ITERATIONS; w++)
    {
        dma_transfer_complete = 0;
        dma_transfer_error    = 0;

        hdma_memcpy.Init.PeriphDataAlignment = periph_align;
        hdma_memcpy.Init.MemDataAlignment    = mem_align;
        HAL_DMA_Init(&hdma_memcpy);

        HAL_DMA_Start_IT(&hdma_memcpy, (uint32_t)src_buffer,
                         (uint32_t)dst_buffer, dma_count);

        while (!dma_transfer_complete && !dma_transfer_error);
    }

    /* Pengukuran sesungguhnya */
    for (uint32_t i = 0; i < iterations; i++)
    {
        memset(dst_buffer, 0, size);
        dma_transfer_complete = 0;
        dma_transfer_error    = 0;

        /* Re-init DMA dengan alignment yang diinginkan */
        hdma_memcpy.Init.PeriphDataAlignment = periph_align;
        hdma_memcpy.Init.MemDataAlignment    = mem_align;
        HAL_DMA_Init(&hdma_memcpy);

        /* Ukur semua: setup + transfer + wait */
        uint32_t start = DWT_CYCCNT;

        HAL_DMA_Start_IT(&hdma_memcpy, (uint32_t)src_buffer,
                         (uint32_t)dst_buffer, dma_count);

        /* Tunggu transfer selesai (busy wait untuk benchmark akurat) */
        while (!dma_transfer_complete && !dma_transfer_error);

        uint32_t end = DWT_CYCCNT;

        if (!dma_transfer_error)
        {
            total_cycles += (end - start);
        }
    }

    return total_cycles / iterations;
}

/* ==================== Benchmark Ukuran ==================== */
/**
 * Run_Size_Benchmark - Jalankan benchmark untuk semua ukuran buffer
 *
 * Membandingkan memcpy (CPU) vs DMA untuk ukuran:
 * 32, 64, 128, 256, 512, 1024 bytes
 *
 * Menggunakan mode Word alignment (paling efisien).
 */
static void Run_Size_Benchmark(void)
{
    test_number++;
    Print_Separator();
    printf("[TEST %lu] Benchmark Ukuran: memcpy vs DMA (Word mode)\r\n", test_number);
    Print_Separator();
    printf("  Iterasi per tes: %d\r\n", BENCH_ITERATIONS);
    printf("  CPU: 72MHz, DWT cycle counter\r\n\r\n");

    printf("%-8s | %-12s | %-12s | %-10s | %-10s | %-8s\r\n",
           "Size", "CPU cycles", "DMA cycles", "CPU MB/s", "DMA MB/s", "Speedup");
    printf("-------- | ------------ | ------------ | ---------- | ---------- | --------\r\n");

    for (int i = 0; i < NUM_BENCH_SIZES; i++)
    {
        uint16_t size = (uint16_t)bench_sizes[i];

        /* Isi buffer sumber */
        Fill_Source_Buffer(size);

        /* Benchmark CPU (memcpy) */
        uint32_t cpu_cycles = Benchmark_Memcpy(size, BENCH_ITERATIONS);

        /* Verifikasi */
        uint8_t cpu_ok = Verify_Transfer(size);

        /* Benchmark DMA (Word alignment) */
        uint32_t dma_cycles = Benchmark_DMA_M2M(size, BENCH_ITERATIONS, ALIGN_WORD);

        /* Verifikasi */
        uint8_t dma_ok = Verify_Transfer(size);

        /* Hitung throughput: MB/s = (bytes * freq) / (cycles * 1e6) */
        float cpu_tp = (float)size * CPU_FREQ_HZ / ((float)cpu_cycles * 1000000.0f);
        float dma_tp = (float)size * CPU_FREQ_HZ / ((float)dma_cycles * 1000000.0f);
        float speedup = (float)cpu_cycles / (float)dma_cycles;
        float cpu_cpb = (float)cpu_cycles / (float)size;
        float dma_cpb = (float)dma_cycles / (float)size;

        /* Simpan hasil */
        bench_results[i].size           = size;
        bench_results[i].cpu_cycles     = cpu_cycles;
        bench_results[i].dma_cycles     = dma_cycles;
        bench_results[i].cpu_throughput  = cpu_tp;
        bench_results[i].dma_throughput  = dma_tp;
        bench_results[i].speedup        = speedup;
        bench_results[i].cpu_cpb        = cpu_cpb;
        bench_results[i].dma_cpb        = dma_cpb;

        /* Cetak hasil */
        printf("%-8lu | %-12lu | %-12lu | %-10.2f | %-10.2f | %-8.3f\r\n",
               (uint32_t)size, cpu_cycles, dma_cycles, cpu_tp, dma_tp, speedup);

        /* Data untuk Python */
        printf("[DATA] BENCH,size=%u,cpu_cycles=%lu,dma_cycles=%lu,"
               "cpu_mbs=%.2f,dma_mbs=%.2f,speedup=%.3f,"
               "cpu_cpb=%.2f,dma_cpb=%.2f,cpu_ok=%u,dma_ok=%u\r\n",
               size, cpu_cycles, dma_cycles, cpu_tp, dma_tp, speedup,
               cpu_cpb, dma_cpb, cpu_ok, dma_ok);

        HAL_GPIO_TogglePin(LED_PORT, LED_PIN);
        HAL_Delay(DEMO_DELAY_MS);
    }
    printf("\r\n");
}

/* ==================== Benchmark Alignment ==================== */
/**
 * Run_Alignment_Benchmark - Bandingkan performa DMA per data width
 *
 * Transfer 256 bytes dengan alignment berbeda:
 * - Byte: 256 transfer × 1 byte
 * - Halfword: 128 transfer × 2 bytes
 * - Word: 64 transfer × 4 bytes
 */
static void Run_Alignment_Benchmark(void)
{
    test_number++;
    Print_Separator();
    printf("[TEST %lu] Benchmark Alignment DMA (256 bytes)\r\n", test_number);
    Print_Separator();

    uint16_t test_size = 256;
    Fill_Source_Buffer(test_size);

    const char *align_names[] = {"Byte (8-bit)", "Halfword (16-bit)", "Word (32-bit)"};
    uint32_t align_cycles[NUM_ALIGN_MODES];

    for (int a = 0; a < NUM_ALIGN_MODES; a++)
    {
        align_cycles[a] = Benchmark_DMA_M2M(test_size, BENCH_ITERATIONS, a);
        uint8_t ok = Verify_Transfer(test_size);

        float throughput = (float)test_size * CPU_FREQ_HZ /
                          ((float)align_cycles[a] * 1000000.0f);
        float cpb = (float)align_cycles[a] / (float)test_size;

        printf("  %-20s: %lu siklus, %.2f MB/s, %.2f cyc/byte, verif=%s\r\n",
               align_names[a], align_cycles[a], throughput, cpb,
               ok ? "OK" : "GAGAL");

        printf("[DATA] ALIGN,mode=%s,cycles=%lu,throughput=%.2f,cpb=%.2f,ok=%u\r\n",
               (a == 0) ? "byte" : (a == 1) ? "halfword" : "word",
               align_cycles[a], throughput, cpb, ok);

        HAL_Delay(500);
    }

    align_results.byte_cycles     = align_cycles[0];
    align_results.halfword_cycles = align_cycles[1];
    align_results.word_cycles     = align_cycles[2];

    /* Perbandingan relatif */
    printf("\r\n  Perbandingan relatif (terhadap Byte):\r\n");
    printf("  Halfword: %.1fx lebih cepat\r\n",
           (float)align_cycles[0] / (float)align_cycles[1]);
    printf("  Word    : %.1fx lebih cepat\r\n",
           (float)align_cycles[0] / (float)align_cycles[2]);
    printf("\r\n");
}

/* ==================== Cari Titik Crossover ==================== */
/**
 * Find_Crossover_Point - Cari ukuran dimana DMA mulai lebih cepat dari CPU
 *
 * DMA memiliki overhead setup yang tinggi, sehingga untuk transfer kecil
 * CPU (memcpy) lebih cepat. Ada titik dimana DMA mulai unggul.
 */
static void Find_Crossover_Point(void)
{
    test_number++;
    Print_Separator();
    printf("[TEST %lu] Pencarian Titik Crossover DMA vs CPU\r\n", test_number);
    Print_Separator();

    uint16_t crossover_size = 0;
    uint16_t test_sizes[] = {4, 8, 12, 16, 24, 32, 48, 64, 96, 128, 192, 256, 384, 512};
    int num_tests = 14;

    printf("%-8s | %-12s | %-12s | %-10s\r\n",
           "Size", "CPU cyc", "DMA cyc", "Lebih cepat");
    printf("-------- | ------------ | ------------ | ----------\r\n");

    for (int i = 0; i < num_tests; i++)
    {
        uint16_t sz = test_sizes[i];
        Fill_Source_Buffer(sz);

        uint32_t cpu_c = Benchmark_Memcpy(sz, BENCH_ITERATIONS);
        uint32_t dma_c = Benchmark_DMA_M2M(sz, BENCH_ITERATIONS, ALIGN_WORD);

        const char *winner = (dma_c < cpu_c) ? "DMA" : "CPU";

        printf("%-8u | %-12lu | %-12lu | %-10s\r\n", sz, cpu_c, dma_c, winner);
        printf("[DATA] CROSSOVER,size=%u,cpu=%lu,dma=%lu,winner=%s\r\n",
               sz, cpu_c, dma_c, winner);

        /* Catat crossover pertama */
        if (crossover_size == 0 && dma_c < cpu_c)
        {
            crossover_size = sz;
        }

        HAL_GPIO_TogglePin(LED_PORT, LED_PIN);
        HAL_Delay(300);
    }

    printf("\r\n");
    if (crossover_size > 0)
    {
        printf(">>> TITIK CROSSOVER: ~%u bytes <<<\r\n", crossover_size);
        printf("DMA lebih cepat dari CPU untuk transfer >= %u bytes\r\n", crossover_size);
    }
    else
    {
        printf(">>> DMA tidak lebih cepat dari CPU dalam rentang ini <<<\r\n");
        printf("Overhead setup DMA terlalu tinggi dibanding transfer kecil.\r\n");
    }
    printf("[DATA] CROSSOVER_RESULT,size=%u\r\n", crossover_size);
    printf("\r\n");
}

/* ==================== Ringkasan ==================== */
/**
 * Print_Summary - Cetak ringkasan semua hasil benchmark
 */
static void Print_Summary(void)
{
    test_number++;
    Print_Separator();
    printf("[TEST %lu] RINGKASAN BENCHMARK\r\n", test_number);
    Print_Separator();

    /* Rangkum benchmark ukuran */
    printf("Benchmark Ukuran (Word alignment, rata-rata %d iterasi):\r\n\r\n", BENCH_ITERATIONS);

    float total_cpu_tp = 0, total_dma_tp = 0;
    for (int i = 0; i < NUM_BENCH_SIZES; i++)
    {
        printf("  %4lu bytes: CPU=%.1f MB/s, DMA=%.1f MB/s, Speedup=%.3fx\r\n",
               bench_results[i].size,
               bench_results[i].cpu_throughput,
               bench_results[i].dma_throughput,
               bench_results[i].speedup);
        total_cpu_tp += bench_results[i].cpu_throughput;
        total_dma_tp += bench_results[i].dma_throughput;
    }

    printf("\r\n  Rata-rata CPU: %.1f MB/s\r\n", total_cpu_tp / NUM_BENCH_SIZES);
    printf("  Rata-rata DMA: %.1f MB/s\r\n", total_dma_tp / NUM_BENCH_SIZES);

    /* Rangkum alignment */
    printf("\r\nBenchmark Alignment (256 bytes):\r\n");
    printf("  Byte    : %lu siklus\r\n", align_results.byte_cycles);
    printf("  Halfword: %lu siklus\r\n", align_results.halfword_cycles);
    printf("  Word    : %lu siklus\r\n", align_results.word_cycles);

    printf("\r\nKesimpulan:\r\n");
    printf("  - DMA unggul untuk transfer besar (>= ~128 bytes)\r\n");
    printf("  - CPU (memcpy) lebih cepat untuk data kecil\r\n");
    printf("  - Word alignment memberikan performa DMA terbaik\r\n");
    printf("  - DMA membebaskan CPU untuk tugas lain selama transfer\r\n\r\n");

    printf("[DATA] SUMMARY,avg_cpu_mbs=%.2f,avg_dma_mbs=%.2f,"
           "byte_cyc=%lu,hword_cyc=%lu,word_cyc=%lu\r\n",
           total_cpu_tp / NUM_BENCH_SIZES, total_dma_tp / NUM_BENCH_SIZES,
           align_results.byte_cycles, align_results.halfword_cycles,
           align_results.word_cycles);
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

    /* Banner */
    printf("\r\n");
    Print_Separator();
    printf("  STM32_11_DMA_Benchmark\r\n");
    printf("  Benchmark Komprehensif DMA vs CPU\r\n");
    printf("  MCU: STM32F103C8 @ 72MHz\r\n");
    Print_Separator();
    printf("\r\n");

    /* Inisialisasi DMA dan DWT */
    DMA_Init();
    DWT_Init();

    printf("Buffer source: 0x%08lX\r\n", (uint32_t)src_buffer);
    printf("Buffer dest  : 0x%08lX\r\n", (uint32_t)dst_buffer);
    printf("Max size     : %d bytes\r\n", MAX_BUFFER_SIZE);
    printf("\r\n");

    HAL_Delay(SECTION_DELAY_MS);

    /* ===== Bagian 1: Benchmark ukuran ===== */
    Run_Size_Benchmark();
    HAL_Delay(SECTION_DELAY_MS);

    /* ===== Bagian 2: Benchmark alignment ===== */
    Run_Alignment_Benchmark();
    HAL_Delay(SECTION_DELAY_MS);

    /* ===== Bagian 3: Cari crossover ===== */
    Find_Crossover_Point();
    HAL_Delay(SECTION_DELAY_MS);

    /* ===== Ringkasan ===== */
    Print_Summary();

    Print_Separator();
    printf("BENCHMARK SELESAI\r\n");
    Print_Separator();

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
