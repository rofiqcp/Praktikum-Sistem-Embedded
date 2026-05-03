/**
 * ============================================================================
 * PROGRAM 03: Task Delay Periodic - Perbandingan vTaskDelay vs vTaskDelayUntil
 * ============================================================================
 * 
 * Deskripsi:
 *   Program ini mendemonstrasikan perbedaan antara vTaskDelay() dan
 *   vTaskDelayUntil() untuk task periodik. Menggunakan DWT cycle counter
 *   untuk mengukur periode aktual dengan resolusi tinggi (13.9 ns pada 72MHz).
 * 
 * Konsep yang Dipelajari:
 *   1. vTaskDelay()      - Delay relatif (mengakumulasi drift)
 *   2. vTaskDelayUntil() - Delay absolut (kompensasi waktu eksekusi)
 *   3. DWT CYCCNT        - Hardware cycle counter untuk pengukuran presisi
 *   4. Jitter Analysis   - Variasi waktu antar eksekusi periodik
 *   5. Timing Drift      - Akumulasi kesalahan waktu
 * 
 * Perbedaan Kunci:
 * 
 *   vTaskDelay(100ms):
 *     ┌──work──┐          ┌──work──┐          ┌──work──┐
 *     │  10ms  │ 100ms    │  10ms  │ 100ms    │  10ms  │
 *     └────────┘──────────└────────┘──────────└────────┘
 *     Periode aktual = work + delay = 110ms (DRIFT!)
 * 
 *   vTaskDelayUntil(100ms):
 *     ┌──work──┐        ┌──work──┐        ┌──work──┐
 *     │  10ms  │ 90ms   │  10ms  │ 90ms   │  10ms  │
 *     └────────┘────────└────────┘────────└────────┘
 *     Periode aktual = 100ms (TEPAT, kompensasi otomatis)
 * 
 * Hardware:
 *   - STM32F103C8 Blue Pill (72 MHz, 20KB SRAM)
 *   - LED PC13: Indikator vTaskDelay
 *   - LED PB0:  Indikator vTaskDelayUntil
 *   - LED PB1:  Indikator sistem aktif
 *   - USART1: PA9(TX)/PA10(RX) @ 115200 baud
 * 
 * Output Format:
 *   [DATA]DELAY,<sample>,<period_us>,<deviation_us>,<drift_us>,<tick>
 *   [DATA]UNTIL,<sample>,<period_us>,<deviation_us>,<drift_us>,<tick>
 *   [DATA]STATS,<type>,<mean_us>,<min_us>,<max_us>,<jitter_us>,<total_drift>
 *   [DATA]COMPARE,<delay_mean>,<until_mean>,<delay_jitter>,<until_jitter>
 * 
 * ============================================================================
 */

/* ========================== HEADER INCLUDES ============================== */
#if defined(STM32F103xB)
#include "stm32f1xx_hal.h"
#elif defined(STM32F401xC) || defined(STM32F411xE)
#include "stm32f4xx_hal.h"
#endif
#include "FreeRTOS.h"
#include "task.h"
#include "config.h"
#include <stdio.h>
#include <string.h>

/* Deklarasi xPortSysTickHandler untuk fix warning */
extern void xPortSysTickHandler(void);

/* ======================== VARIABEL GLOBAL ================================ */

static UART_HandleTypeDef huart1;

/* Handle task */
static TaskHandle_t xTaskDelayHandle  = NULL;
static TaskHandle_t xTaskUntilHandle  = NULL;
static TaskHandle_t xMonitorHandle    = NULL;

static volatile uint8_t ucSchedulerStarted = 0;

/**
 * Struktur untuk menyimpan data pengukuran timing
 * Setiap task periodik memiliki instance sendiri
 */
typedef struct {
    uint32_t ulSampleCount;             /* Jumlah sampel yang dikumpulkan    */
    uint32_t ulPeriodUs[MAX_SAMPLES];   /* Periode aktual setiap sampel (us) */
    uint32_t ulLastCycles;              /* Cycle counter pada iterasi lalu   */
    uint32_t ulExpectedPeriodUs;        /* Periode yang diharapkan (us)      */
    
    /* Statistik running */
    uint32_t ulSumPeriodUs;             /* Total periode untuk rata-rata     */
    uint32_t ulMinPeriodUs;             /* Periode minimum yang tercatat     */
    uint32_t ulMaxPeriodUs;             /* Periode maksimum yang tercatat    */
    int32_t  lTotalDriftUs;             /* Total drift kumulatif (signed)    */
    uint32_t ulJitterUs;                /* Jitter (max - min)                */
    
    /* Indeks sampel circular */
    uint32_t ulWriteIndex;              /* Indeks tulis ke array sampel      */
    uint8_t  ucBufferFull;              /* Flag buffer penuh                 */
} TimingData_t;

static volatile TimingData_t xDelayData = {
    .ulSampleCount = 0, .ulLastCycles = 0,
    .ulExpectedPeriodUs = TASK_DELAY_PERIOD_MS * 1000,
    .ulSumPeriodUs = 0, .ulMinPeriodUs = 0xFFFFFFFF,
    .ulMaxPeriodUs = 0, .lTotalDriftUs = 0, .ulJitterUs = 0,
    .ulWriteIndex = 0, .ucBufferFull = 0
};

static volatile TimingData_t xUntilData = {
    .ulSampleCount = 0, .ulLastCycles = 0,
    .ulExpectedPeriodUs = TASK_UNTIL_PERIOD_MS * 1000,
    .ulSumPeriodUs = 0, .ulMinPeriodUs = 0xFFFFFFFF,
    .ulMaxPeriodUs = 0, .lTotalDriftUs = 0, .ulJitterUs = 0,
    .ulWriteIndex = 0, .ucBufferFull = 0
};

static char pcPrintBuffer[PRINT_BUFFER_SIZE];

/* ==================== DEKLARASI FUNGSI PROTOTYPE ========================= */
static void SystemClock_Config(void);
static void GPIO_Init(void);
static void UART1_Init(void);
static void DWT_Init(void);
static uint32_t DWT_GetCycles(void);

static void vTaskDelayDemo(void *pvParameters);
static void vTaskDelayUntilDemo(void *pvParameters);
static void vTaskMonitor(void *pvParameters);

static void vSimulateWorkload(void);
static void vRecordTiming(TimingData_t volatile *pxData, uint32_t ulCurrentCycles);
static void vPrintTimingStats(const char *pcName, TimingData_t volatile *pxData);
static void vPrintSeparator(char cChar, uint8_t ucLen);

/* ========================= RETARGET PRINTF =============================== */
int _write(int file, char *ptr, int len) {
    (void)file;
    HAL_UART_Transmit(&huart1, (uint8_t*)ptr, len, HAL_MAX_DELAY);
    return len;
}

/* ===================== INTERRUPT HANDLERS ================================ */
void SysTick_Handler(void) {
    HAL_IncTick();
    if (ucSchedulerStarted) {
        xPortSysTickHandler();
    }
}

/* =================== HOOK FUNCTIONS FreeRTOS ============================= */

void vApplicationMallocFailedHook(void) {
    printf("\r\n[ERROR] !! MALLOC GAGAL !!\r\n");
    taskDISABLE_INTERRUPTS();
    for (;;) {
        HAL_GPIO_TogglePin(LED_DELAY_PORT, LED_DELAY_PIN);
        for (volatile uint32_t i = 0; i < 200000; i++);
    }
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
    (void)xTask;
    printf("\r\n[ERROR] !! STACK OVERFLOW: %s !!\r\n", pcTaskName);
    taskDISABLE_INTERRUPTS();
    for (;;) {
        HAL_GPIO_TogglePin(LED_DELAY_PORT, LED_DELAY_PIN);
        for (volatile uint32_t i = 0; i < 100000; i++);
    }
}

void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer,
                                   StackType_t **ppxIdleTaskStackBuffer,
                                   uint32_t *pulIdleTaskStackSize) {
    static StaticTask_t xIdleTaskTCB;
    static StackType_t uxIdleTaskStack[configMINIMAL_STACK_SIZE];
    *ppxIdleTaskTCBBuffer   = &xIdleTaskTCB;
    *ppxIdleTaskStackBuffer = uxIdleTaskStack;
    *pulIdleTaskStackSize   = configMINIMAL_STACK_SIZE;
}

void vApplicationGetTimerTaskMemory(StaticTask_t **ppxTimerTaskTCBBuffer,
                                    StackType_t **ppxTimerTaskStackBuffer,
                                    uint32_t *pulTimerTaskStackSize) {
    static StaticTask_t xTimerTaskTCB;
    static StackType_t uxTimerTaskStack[configTIMER_TASK_STACK_DEPTH];
    *ppxTimerTaskTCBBuffer   = &xTimerTaskTCB;
    *ppxTimerTaskStackBuffer = uxTimerTaskStack;
    *pulTimerTaskStackSize   = configTIMER_TASK_STACK_DEPTH;
}

/* ==================== KONFIGURASI SYSTEM CLOCK =========================== */

static void SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

#if defined(STM32F103xB)
    /* STM32F1xx (72 MHz) */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState       = RCC_HSE_ON;
    RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    RCC_OscInitStruct.PLL.PLLState   = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource  = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL     = RCC_PLL_MUL9;

    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        for (;;);
    }

    RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                       RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) {
        for (;;);
    }

#elif defined(STM32F401xC)
    /* STM32F401CC (84 MHz) */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState       = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState   = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource  = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM       = 25;
    RCC_OscInitStruct.PLL.PLLN       = 336;
    RCC_OscInitStruct.PLL.PLLP       = RCC_PLLP_DIV4;
    RCC_OscInitStruct.PLL.PLLQ       = 7;

    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        for (;;);
    }

    RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                       RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) {
        for (;;);
    }

#elif defined(STM32F411xE)
    /* STM32F411CE (100 MHz) */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState       = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState   = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource  = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM       = 25;
    RCC_OscInitStruct.PLL.PLLN       = 400;
    RCC_OscInitStruct.PLL.PLLP       = RCC_PLLP_DIV4;
    RCC_OscInitStruct.PLL.PLLQ       = 9;

    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        for (;;);
    }

    RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                       RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK) {
        for (;;);
    }

#endif
}

/* ======================== INISIALISASI GPIO ============================== */

static void GPIO_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    LED_DELAY_CLK_EN();
    LED_UNTIL_CLK_EN();
    LED_STATUS_CLK_EN();
    BTN_CLK_EN();

    /* LED PC13 - Indikator vTaskDelay (active LOW) */
    GPIO_InitStruct.Pin   = LED_DELAY_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_DELAY_PORT, &GPIO_InitStruct);
    HAL_GPIO_WritePin(LED_DELAY_PORT, LED_DELAY_PIN, GPIO_PIN_SET);

    /* LED PB0 - Indikator vTaskDelayUntil */
    GPIO_InitStruct.Pin = LED_UNTIL_PIN;
    HAL_GPIO_Init(LED_UNTIL_PORT, &GPIO_InitStruct);
    HAL_GPIO_WritePin(LED_UNTIL_PORT, LED_UNTIL_PIN, GPIO_PIN_RESET);

    /* LED PB1 - Indikator Status */
    GPIO_InitStruct.Pin = LED_STATUS_PIN;
    HAL_GPIO_Init(LED_STATUS_PORT, &GPIO_InitStruct);
    HAL_GPIO_WritePin(LED_STATUS_PORT, LED_STATUS_PIN, GPIO_PIN_RESET);

    /* Tombol PA0 */
    GPIO_InitStruct.Pin  = BTN_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(BTN_PORT, &GPIO_InitStruct);
}

/* ======================== INISIALISASI UART ============================== */

static void UART1_Init(void) {
    UART_CLK_EN();
    UART_GPIO_CLK_EN();

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin   = UART_TX_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(UART_TX_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin  = UART_RX_PIN;
#if defined(STM32F103xB)
    GPIO_InitStruct.Mode = GPIO_MODE_AF_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
#elif defined(STM32F401xC) || defined(STM32F411xE)
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
#endif
    HAL_GPIO_Init(UART_RX_PORT, &GPIO_InitStruct);

    huart1.Instance          = UART_INSTANCE;
    huart1.Init.BaudRate     = UART_BAUDRATE;
    huart1.Init.WordLength   = UART_WORDLENGTH_8B;
    huart1.Init.StopBits     = UART_STOPBITS_1;
    huart1.Init.Parity       = UART_PARITY_NONE;
    huart1.Init.Mode         = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;

    if (HAL_UART_Init(&huart1) != HAL_OK) {
        for (;;);
    }
}

/* ========================== DWT INIT ===================================== */

/**
 * Inisialisasi DWT (Data Watchpoint and Trace) Cycle Counter
 * 
 * DWT CYCCNT pada Cortex-M3:
 *   - Counter 32-bit yang menghitung siklus clock CPU
 *   - Pada 72 MHz: 1 siklus = 13.89 ns
 *   - Overflow setelah: 2^32 / 72MHz ≈ 59.65 detik
 *   - Resolusi jauh lebih baik dari SysTick (1ms)
 * 
 * Sangat penting untuk mengukur:
 *   - Periode aktual task periodik
 *   - Jitter antar eksekusi
 *   - Akumulasi drift
 */
static void DWT_Init(void) {
    /* Aktifkan trace module di System Control Block */
    SCB_DEMCR |= TRCENA_BIT;
    
    /* Unlock DWT (diperlukan pada beberapa implementasi Cortex-M) */
    DWT_LAR = DWT_LAR_UNLOCK;
    
    /* Reset dan aktifkan cycle counter */
    DWT_CYCCNT  = 0;
    DWT_CONTROL |= DWT_CTRL_ENABLE_BIT;
}

static uint32_t DWT_GetCycles(void) {
    return DWT_CYCCNT;
}

/* ========================= UTILITY FUNCTIONS ============================= */

static void vPrintSeparator(char cChar, uint8_t ucLen) {
    for (uint8_t i = 0; i < ucLen; i++) printf("%c", cChar);
    printf("\r\n");
}

/**
 * Simulasi beban kerja (workload) pada task periodik
 * 
 * Mengapa penting dalam demo ini:
 *   - vTaskDelay(100ms) sebenarnya menghasilkan periode = work_time + 100ms
 *   - vTaskDelayUntil(100ms) menghasilkan periode = 100ms (otomatis kompensasi)
 *   - Semakin lama work_time, semakin besar drift pada vTaskDelay
 * 
 * WORKLOAD_ITERATIONS dikonfigurasi di config.h agar bisa diubah
 * untuk menunjukkan efek yang lebih dramatis.
 */
static void vSimulateWorkload(void) {
    volatile uint32_t ulDummy = 0;
    for (uint32_t i = 0; i < WORKLOAD_ITERATIONS; i++) {
        ulDummy += i;
        /* Operasi tambahan untuk memastikan compiler tidak mengoptimasi */
        if (ulDummy > 0xFFFFFFF0) ulDummy = 0;
    }
}

/**
 * Catat data timing dari satu iterasi task periodik
 * 
 * Menghitung:
 *   1. Periode aktual (berapa lama antara dua iterasi berturutan)
 *   2. Deviasi dari periode yang diharapkan
 *   3. Drift kumulatif (total akumulasi kesalahan waktu)
 * 
 * @param pxData         Pointer ke struktur timing data
 * @param ulCurrentCycles Nilai DWT CYCCNT saat ini
 */
static void vRecordTiming(TimingData_t volatile *pxData, uint32_t ulCurrentCycles) {
    /* Pada iterasi pertama, simpan timestamp saja */
    if (pxData->ulLastCycles == 0) {
        pxData->ulLastCycles = ulCurrentCycles;
        return;
    }

    /* Hitung periode dalam siklus clock */
    uint32_t ulElapsedCycles = ulCurrentCycles - pxData->ulLastCycles;
    pxData->ulLastCycles = ulCurrentCycles;

    /* Konversi ke mikrodetik */
    uint32_t ulPeriodUs = CYCLES_TO_US(ulElapsedCycles);

    /* Simpan sampel ke buffer circular */
    uint32_t ulIdx = pxData->ulWriteIndex;
    pxData->ulPeriodUs[ulIdx] = ulPeriodUs;
    pxData->ulWriteIndex = (ulIdx + 1) % MAX_SAMPLES;
    if (pxData->ulWriteIndex == 0) {
        pxData->ucBufferFull = 1;
    }
    pxData->ulSampleCount++;

    /* Update statistik running */
    pxData->ulSumPeriodUs += ulPeriodUs;

    if (ulPeriodUs < pxData->ulMinPeriodUs) {
        pxData->ulMinPeriodUs = ulPeriodUs;
    }
    if (ulPeriodUs > pxData->ulMaxPeriodUs) {
        pxData->ulMaxPeriodUs = ulPeriodUs;
    }

    /* Hitung deviasi dari target */
    int32_t lDeviation = (int32_t)ulPeriodUs - (int32_t)pxData->ulExpectedPeriodUs;
    pxData->lTotalDriftUs += lDeviation;

    /* Update jitter (max - min) */
    pxData->ulJitterUs = pxData->ulMaxPeriodUs - pxData->ulMinPeriodUs;
}

/**
 * Cetak statistik timing lengkap untuk satu task
 * 
 * Menghitung dan menampilkan:
 *   - Rata-rata periode
 *   - Minimum dan maksimum periode
 *   - Jitter (variasi)
 *   - Total drift kumulatif
 *   - Deviasi dari target
 */
static void vPrintTimingStats(const char *pcName, TimingData_t volatile *pxData) {
    if (pxData->ulSampleCount == 0) {
        printf("  %s: Belum ada data\r\n", pcName);
        return;
    }

    /* Hitung rata-rata */
    uint32_t ulMeanUs = pxData->ulSumPeriodUs / pxData->ulSampleCount;
    int32_t  lMeanDev = (int32_t)ulMeanUs - (int32_t)pxData->ulExpectedPeriodUs;

    printf("  %-14s | Sampel: %4lu\r\n", pcName, (unsigned long)pxData->ulSampleCount);
    printf("    Target   : %lu us (%lu ms)\r\n",
           (unsigned long)pxData->ulExpectedPeriodUs,
           (unsigned long)(pxData->ulExpectedPeriodUs / 1000));
    printf("    Rata-rata: %lu us (deviasi: %+ld us)\r\n",
           (unsigned long)ulMeanUs, (long)lMeanDev);
    printf("    Minimum  : %lu us\r\n",
           (unsigned long)(pxData->ulMinPeriodUs == 0xFFFFFFFF ? 0 : pxData->ulMinPeriodUs));
    printf("    Maksimum : %lu us\r\n", (unsigned long)pxData->ulMaxPeriodUs);
    printf("    Jitter   : %lu us (max - min)\r\n", (unsigned long)pxData->ulJitterUs);
    printf("    Drift    : %+ld us (kumulatif)\r\n", (long)pxData->lTotalDriftUs);

    /* Data untuk Python parser */
    printf("[DATA]STATS,%s,%lu,%lu,%lu,%lu,%ld\r\n",
           pcName,
           (unsigned long)ulMeanUs,
           (unsigned long)(pxData->ulMinPeriodUs == 0xFFFFFFFF ? 0 : pxData->ulMinPeriodUs),
           (unsigned long)pxData->ulMaxPeriodUs,
           (unsigned long)pxData->ulJitterUs,
           (long)pxData->lTotalDriftUs);
}

/* ========================== TASK FUNCTIONS =============================== */

/**
 * Task Delay Demo: Menggunakan vTaskDelay() - RELATIF
 * 
 * vTaskDelay(100ms) berarti:
 *   "Tunda task ini selama 100ms DARI SEKARANG"
 * 
 * Masalah:
 *   - Jika task membutuhkan waktu 10ms untuk bekerja (work)
 *   - Total periode = 10ms (work) + 100ms (delay) = 110ms
 *   - Setiap iterasi, drift bertambah 10ms
 *   - Setelah 10 iterasi: drift = 100ms
 *   - Setelah 100 iterasi: drift = 1000ms = 1 detik!
 * 
 * Kapan menggunakan vTaskDelay():
 *   - Task yang tidak memerlukan timing presisi
 *   - Delay sederhana (debounce, LED blink, dll)
 *   - Polling dengan interval kasar
 */
static void vTaskDelayDemo(void *pvParameters) {
    (void)pvParameters;

    uint32_t ulSample = 0;

    printf("[%s] Task dimulai - vTaskDelay(%d ms)\r\n",
           TASK_DELAY_NAME, TASK_DELAY_PERIOD_MS);
    printf("[%s] Akan mengakumulasi drift karena waktu kerja\r\n\r\n",
           TASK_DELAY_NAME);

    for (;;) {
        /* Ambil timestamp di awal iterasi */
        uint32_t ulCycles = DWT_GetCycles();

        /* Toggle LED sebagai indikator visual */
        LED_DELAY_TOGGLE();

        /* Catat timing (bandingkan dengan iterasi sebelumnya) */
        vRecordTiming(&xDelayData, ulCycles);

        /* Simulasi beban kerja - ini yang menyebabkan drift */
        vSimulateWorkload();

        ulSample++;

        /* Cetak data setiap 10 sampel */
        if (ulSample % 10 == 0 && xDelayData.ulSampleCount > 0) {
            uint32_t ulLastPeriod = 0;
            if (xDelayData.ulWriteIndex > 0) {
                ulLastPeriod = xDelayData.ulPeriodUs[xDelayData.ulWriteIndex - 1];
            } else if (xDelayData.ucBufferFull) {
                ulLastPeriod = xDelayData.ulPeriodUs[MAX_SAMPLES - 1];
            }
            
            int32_t lDeviation = (int32_t)ulLastPeriod - 
                                 (int32_t)xDelayData.ulExpectedPeriodUs;

            printf("[%s] Sampel #%lu | Period: %lu us | Dev: %+ld us | Drift: %+ld us\r\n",
                   TASK_DELAY_NAME, (unsigned long)ulSample,
                   (unsigned long)ulLastPeriod, (long)lDeviation,
                   (long)xDelayData.lTotalDriftUs);

            printf("[DATA]DELAY,%lu,%lu,%ld,%ld,%lu\r\n",
                   (unsigned long)ulSample,
                   (unsigned long)ulLastPeriod,
                   (long)lDeviation,
                   (long)xDelayData.lTotalDriftUs,
                   (unsigned long)xTaskGetTickCount());
        }

        /**
         * vTaskDelay() - Delay RELATIF
         * 
         * Menempatkan task ke state BLOCKED selama minimal N ticks.
         * Timer mulai dihitung DARI SAAT FUNGSI INI DIPANGGIL.
         * 
         * pdMS_TO_TICKS() mengkonversi milidetik ke jumlah tick:
         *   100ms × 1000 Hz = 100 ticks
         */
        vTaskDelay(pdMS_TO_TICKS(TASK_DELAY_PERIOD_MS));
    }
}

/**
 * Task DelayUntil Demo: Menggunakan vTaskDelayUntil() - ABSOLUT
 * 
 * vTaskDelayUntil(&xLastWakeTime, 100ms) berarti:
 *   "Tunda task ini sampai 100ms SETELAH TERAKHIR KALI BANGUN"
 * 
 * Keunggulan:
 *   - Otomatis mengkompensasi waktu eksekusi task
 *   - Jika task bekerja 10ms, delay aktual = 90ms → periode = 100ms
 *   - Tidak mengakumulasi drift
 *   - Periode sangat konsisten (mendekati target)
 * 
 * Cara kerja:
 *   1. Simpan waktu terakhir bangun (xLastWakeTime)
 *   2. Setelah task selesai bekerja, hitung:
 *      delay_needed = target_period - (now - xLastWakeTime)
 *   3. Block selama delay_needed
 *   4. Update xLastWakeTime += target_period
 * 
 * Kapan menggunakan vTaskDelayUntil():
 *   - Task kontrol periodik (PID controller)
 *   - Sampling sensor dengan rate tetap
 *   - Transmisi data periodik
 *   - Setiap task yang memerlukan timing presisi
 */
static void vTaskDelayUntilDemo(void *pvParameters) {
    (void)pvParameters;

    uint32_t ulSample = 0;

    /**
     * xLastWakeTime menyimpan tick count terakhir kali task ini bangun.
     * Diinisialisasi dengan xTaskGetTickCount() saat pertama kali.
     * vTaskDelayUntil() akan mengupdate variabel ini secara otomatis.
     */
    TickType_t xLastWakeTime = xTaskGetTickCount();

    printf("[%s] Task dimulai - vTaskDelayUntil(%d ms)\r\n",
           TASK_UNTIL_NAME, TASK_UNTIL_PERIOD_MS);
    printf("[%s] Akan mengkompensasi waktu kerja secara otomatis\r\n\r\n",
           TASK_UNTIL_NAME);

    for (;;) {
        /* Ambil timestamp di awal iterasi */
        uint32_t ulCycles = DWT_GetCycles();

        /* Toggle LED indikator */
        LED_UNTIL_TOGGLE();

        /* Catat timing */
        vRecordTiming(&xUntilData, ulCycles);

        /* Simulasi beban kerja (sama dengan vTaskDelay task) */
        vSimulateWorkload();

        ulSample++;

        /* Cetak data setiap 10 sampel */
        if (ulSample % 10 == 0 && xUntilData.ulSampleCount > 0) {
            uint32_t ulLastPeriod = 0;
            if (xUntilData.ulWriteIndex > 0) {
                ulLastPeriod = xUntilData.ulPeriodUs[xUntilData.ulWriteIndex - 1];
            } else if (xUntilData.ucBufferFull) {
                ulLastPeriod = xUntilData.ulPeriodUs[MAX_SAMPLES - 1];
            }

            int32_t lDeviation = (int32_t)ulLastPeriod -
                                 (int32_t)xUntilData.ulExpectedPeriodUs;

            printf("[%s] Sampel #%lu | Period: %lu us | Dev: %+ld us | Drift: %+ld us\r\n",
                   TASK_UNTIL_NAME, (unsigned long)ulSample,
                   (unsigned long)ulLastPeriod, (long)lDeviation,
                   (long)xUntilData.lTotalDriftUs);

            printf("[DATA]UNTIL,%lu,%lu,%ld,%ld,%lu\r\n",
                   (unsigned long)ulSample,
                   (unsigned long)ulLastPeriod,
                   (long)lDeviation,
                   (long)xUntilData.lTotalDriftUs,
                   (unsigned long)xTaskGetTickCount());
        }

        /**
         * vTaskDelayUntil() - Delay ABSOLUT
         * 
         * Parameter:
         *   1. &xLastWakeTime - Pointer ke variabel yang menyimpan
         *      tick count terakhir kali task bangun.
         *      Diupdate OTOMATIS oleh fungsi ini.
         *   2. xPeriod - Periode yang diinginkan dalam ticks
         * 
         * PENTING: xLastWakeTime HARUS diinisialisasi dengan 
         * xTaskGetTickCount() sebelum loop pertama!
         */
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(TASK_UNTIL_PERIOD_MS));
    }
}

/**
 * Task Monitor: Cetak perbandingan statistik
 * 
 * Task ini mencetak perbandingan komprehensif antara:
 *   - vTaskDelay: periode, drift, jitter
 *   - vTaskDelayUntil: periode, drift, jitter
 * 
 * Memungkinkan analisis visual perbedaan kedua metode.
 */
static void vTaskMonitor(void *pvParameters) {
    (void)pvParameters;

    uint32_t ulCycle = 0;

    printf("[%s] Task monitor dimulai - Period %d ms\r\n\r\n",
           MONITOR_TASK_NAME, MONITOR_PERIOD_MS);

    /* Tunggu beberapa detik agar data terkumpul */
    vTaskDelay(pdMS_TO_TICKS(3000));

    for (;;) {
        ulCycle++;
        TickType_t xTick = xTaskGetTickCount();
        uint32_t ulUptime = xTick / configTICK_RATE_HZ;

        printf("\r\n");
        vPrintSeparator('=', 70);
        printf("  PERBANDINGAN vTaskDelay vs vTaskDelayUntil\r\n");
        printf("  Monitor Cycle: %lu | Uptime: %lu detik | Tick: %lu\r\n",
               (unsigned long)ulCycle, (unsigned long)ulUptime,
               (unsigned long)xTick);
        vPrintSeparator('=', 70);

        /* ---- Statistik vTaskDelay ---- */
        printf("\r\n  [1] vTaskDelay(%d ms) - Delay RELATIF\r\n", TASK_DELAY_PERIOD_MS);
        vPrintSeparator('-', 50);
        vPrintTimingStats(TASK_DELAY_NAME, &xDelayData);

        /* ---- Statistik vTaskDelayUntil ---- */
        printf("\r\n  [2] vTaskDelayUntil(%d ms) - Delay ABSOLUT\r\n", TASK_UNTIL_PERIOD_MS);
        vPrintSeparator('-', 50);
        vPrintTimingStats(TASK_UNTIL_NAME, &xUntilData);

        /* ---- Perbandingan ---- */
        printf("\r\n  === PERBANDINGAN LANGSUNG ===\r\n");
        vPrintSeparator('-', 50);

        if (xDelayData.ulSampleCount > 0 && xUntilData.ulSampleCount > 0) {
            uint32_t ulDelayMean = xDelayData.ulSumPeriodUs / xDelayData.ulSampleCount;
            uint32_t ulUntilMean = xUntilData.ulSumPeriodUs / xUntilData.ulSampleCount;

            printf("  %-20s | %-12s | %-12s\r\n", "Metrik", "vTaskDelay", "vDelayUntil");
            vPrintSeparator('-', 50);
            printf("  %-20s | %10lu us | %10lu us\r\n", "Rata-rata Periode",
                   (unsigned long)ulDelayMean, (unsigned long)ulUntilMean);
            printf("  %-20s | %10lu us | %10lu us\r\n", "Jitter",
                   (unsigned long)xDelayData.ulJitterUs,
                   (unsigned long)xUntilData.ulJitterUs);
            printf("  %-20s | %+10ld us | %+10ld us\r\n", "Drift Total",
                   (long)xDelayData.lTotalDriftUs,
                   (long)xUntilData.lTotalDriftUs);
            printf("  %-20s | %10lu    | %10lu\r\n", "Jumlah Sampel",
                   (unsigned long)xDelayData.ulSampleCount,
                   (unsigned long)xUntilData.ulSampleCount);

            /* Data perbandingan untuk Python parser */
            printf("[DATA]COMPARE,%lu,%lu,%lu,%lu,%ld,%ld\r\n",
                   (unsigned long)ulDelayMean,
                   (unsigned long)ulUntilMean,
                   (unsigned long)xDelayData.ulJitterUs,
                   (unsigned long)xUntilData.ulJitterUs,
                   (long)xDelayData.lTotalDriftUs,
                   (long)xUntilData.lTotalDriftUs);

            /* Kesimpulan otomatis */
            printf("\r\n  >>> KESIMPULAN <<<\r\n");
            
            int32_t lDelayDev = (int32_t)ulDelayMean - 
                                (int32_t)xDelayData.ulExpectedPeriodUs;
            int32_t lUntilDev = (int32_t)ulUntilMean - 
                                (int32_t)xUntilData.ulExpectedPeriodUs;

            printf("  vTaskDelay   : Deviasi rata-rata %+ld us dari target\r\n",
                   (long)lDelayDev);
            printf("  vDelayUntil  : Deviasi rata-rata %+ld us dari target\r\n",
                   (long)lUntilDev);
            
            if (xDelayData.ulJitterUs > xUntilData.ulJitterUs) {
                printf("  vTaskDelayUntil LEBIH STABIL (jitter %lu vs %lu us)\r\n",
                       (unsigned long)xUntilData.ulJitterUs,
                       (unsigned long)xDelayData.ulJitterUs);
            }

            uint32_t ulAbsDriftDelay = (xDelayData.lTotalDriftUs >= 0) ? 
                (uint32_t)xDelayData.lTotalDriftUs : (uint32_t)(-xDelayData.lTotalDriftUs);
            uint32_t ulAbsDriftUntil = (xUntilData.lTotalDriftUs >= 0) ?
                (uint32_t)xUntilData.lTotalDriftUs : (uint32_t)(-xUntilData.lTotalDriftUs);
            
            if (ulAbsDriftDelay > ulAbsDriftUntil) {
                printf("  vTaskDelay mengakumulasi drift %lu us lebih banyak!\r\n",
                       (unsigned long)(ulAbsDriftDelay - ulAbsDriftUntil));
            }
        }

        /* Stack usage info */
        printf("\r\n  --- Stack Usage ---\r\n");
        printf("  %s: %lu words free\r\n", TASK_DELAY_NAME,
               (unsigned long)uxTaskGetStackHighWaterMark(xTaskDelayHandle));
        printf("  %s: %lu words free\r\n", TASK_UNTIL_NAME,
               (unsigned long)uxTaskGetStackHighWaterMark(xTaskUntilHandle));
        printf("  %s: %lu words free\r\n", MONITOR_TASK_NAME,
               (unsigned long)uxTaskGetStackHighWaterMark(xMonitorHandle));
        printf("  Heap Free: %lu bytes\r\n",
               (unsigned long)xPortGetFreeHeapSize());

        printf("[DATA]HEAP,%lu,%lu\r\n",
               (unsigned long)xPortGetFreeHeapSize(),
               (unsigned long)xTick);

        /* Toggle LED status */
        LED_STATUS_TOGGLE();

        /* vTaskList setiap 5 cycle */
        if (ulCycle % 5 == 0) {
            printf("\r\n  === vTaskList ===\r\n");
            char pcBuf[512];
            vTaskList(pcBuf);
            printf("%s", pcBuf);
        }

        vPrintSeparator('=', 70);

        vTaskDelay(pdMS_TO_TICKS(MONITOR_PERIOD_MS));
    }
}

/* ============================ MAIN FUNCTION ============================== */

int main(void) {
    /* Inisialisasi hardware */
    HAL_Init();
    SystemClock_Config();
    GPIO_Init();
    UART1_Init();
    DWT_Init();

    /* Banner */
    printf("\r\n\r\n");
    vPrintSeparator('*', 70);
    printf("*   STM32F103 FreeRTOS - Program 03: Task Delay Periodic          *\r\n");
    printf("*   Perbandingan vTaskDelay() vs vTaskDelayUntil()                 *\r\n");
    vPrintSeparator('*', 70);
    printf("\r\n");

    printf("  [INFO] System Clock     : %lu MHz\r\n",
           (unsigned long)(HAL_RCC_GetSysClockFreq() / 1000000));
    printf("  [INFO] Target Period    : %d ms (%d us)\r\n",
           TASK_DELAY_PERIOD_MS, TASK_DELAY_PERIOD_MS * 1000);
    printf("  [INFO] Workload Iters   : %lu\r\n", (unsigned long)WORKLOAD_ITERATIONS);
    printf("  [INFO] Max Samples      : %d\r\n", MAX_SAMPLES);
    printf("  [INFO] DWT Counter      : Aktif (resolusi %.1f ns)\r\n",
           1000000000.0 / 72000000.0);
    printf("  [INFO] FreeRTOS Heap    : %u bytes\r\n",
           (unsigned int)configTOTAL_HEAP_SIZE);
    printf("\r\n");

    printf("  [HEAP] Sebelum task: %lu bytes free\r\n\r\n",
           (unsigned long)xPortGetFreeHeapSize());

    /* Buat task */
    BaseType_t xResult;

    xResult = xTaskCreate(vTaskDelayDemo, TASK_DELAY_NAME, TASK_DELAY_STACK,
                          NULL, TASK_DELAY_PRIORITY, &xTaskDelayHandle);
    printf("  [%s] Task '%s': %s\r\n",
           xResult == pdPASS ? "OK" : "GAGAL", TASK_DELAY_NAME,
           xResult == pdPASS ? "Berhasil" : "Gagal");

    xResult = xTaskCreate(vTaskDelayUntilDemo, TASK_UNTIL_NAME, TASK_UNTIL_STACK,
                          NULL, TASK_UNTIL_PRIORITY, &xTaskUntilHandle);
    printf("  [%s] Task '%s': %s\r\n",
           xResult == pdPASS ? "OK" : "GAGAL", TASK_UNTIL_NAME,
           xResult == pdPASS ? "Berhasil" : "Gagal");

    xResult = xTaskCreate(vTaskMonitor, MONITOR_TASK_NAME, MONITOR_STACK_SIZE,
                          NULL, MONITOR_PRIORITY, &xMonitorHandle);
    printf("  [%s] Task '%s': %s\r\n",
           xResult == pdPASS ? "OK" : "GAGAL", MONITOR_TASK_NAME,
           xResult == pdPASS ? "Berhasil" : "Gagal");

    printf("\r\n  [HEAP] Setelah task: %lu bytes free\r\n",
           (unsigned long)xPortGetFreeHeapSize());

    printf("\r\n  Memulai FreeRTOS Scheduler...\r\n");
    vPrintSeparator('=', 70);

    ucSchedulerStarted = 1;
    vTaskStartScheduler();

    printf("\r\n[FATAL] Scheduler gagal!\r\n");
    for (;;) {
        HAL_GPIO_TogglePin(LED_DELAY_PORT, LED_DELAY_PIN);
        HAL_Delay(50);
    }
}

/* ====================== HAL CALLBACK OVERRIDES =========================== */

void HAL_MspInit(void) {
#if defined(STM32F103xB)
    __HAL_RCC_AFIO_CLK_ENABLE();
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_AFIO_REMAP_SWJ_NOJTAG();
#elif defined(STM32F401xC) || defined(STM32F411xE)
    __HAL_RCC_PWR_CLK_ENABLE();
#else
    __HAL_RCC_PWR_CLK_ENABLE();
#endif
}
