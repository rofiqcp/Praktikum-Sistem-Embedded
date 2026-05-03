/**
 * ============================================================================
 * STM32_12_Task_Cooperative — Demonstrasi taskYIELD() & Cooperative Scheduling
 * ============================================================================
 *
 * Deskripsi:
 *   Program ini mendemonstrasikan perbedaan antara preemptive scheduling,
 *   cooperative yield menggunakan taskYIELD(), dan efek CPU hogging saat
 *   task tidak mau melepaskan CPU secara sukarela.
 *
 * Fase Eksperimen:
 *   Fase 1 (10 detik): Preemptive + Time Slicing
 *     - 3 worker task dengan prioritas sama (2) menggunakan vTaskDelay()
 *     - Round-robin otomatis oleh scheduler — distribusi merata
 *
 *   Fase 2 (10 detik): Cooperative Yield
 *     - Worker menggunakan taskYIELD() tanpa vTaskDelay()
 *     - CPU dilepas secara sukarela — perilaku kooperatif
 *
 *   Fase 3 (10 detik): CPU Hog Demonstration
 *     - Worker-0 masuk busy loop tanpa yield
 *     - Worker-1 dan Worker-2 tetap yield tetapi kelaparan (starvation)
 *     - Menunjukkan bahaya task yang tidak kooperatif
 *
 * Hardware:
 *   - Blue Pill STM32F103C8T6
 *   - LED bawaan PC13 (active low)
 *   - UART1: PA9 (TX), PA10 (RX) @ 115200 baud
 *
 * Clock: HSE 8MHz → PLL x9 → SYSCLK 72MHz
 *        APB1 = 36MHz, APB2 = 72MHz
 *
 * ============================================================================
 */

/* ===========================================================================
 * INCLUDE FILES
 * =========================================================================== */
#if defined(STM32F103xB)
#include "stm32f1xx_hal.h"
#elif defined(STM32F401xC) || defined(STM32F411xE)
#include "stm32f4xx_hal.h"
#endif
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

/* ===========================================================================
 * DEFINISI KONSTANTA
 * =========================================================================== */

/* Konfigurasi LED bawaan Blue Pill (PC13, active low) */
#define LED_PORT                GPIOC
#define LED_PIN                 GPIO_PIN_13
#define LED_ON()                HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_RESET)
#define LED_OFF()               HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET)
#define LED_TOGGLE()            HAL_GPIO_TogglePin(LED_PORT, LED_PIN)

/* Konfigurasi UART debug */
#define DEBUG_UART              USART1
#define DEBUG_BAUDRATE          115200

/* Konfigurasi ukuran stack task (dalam words) */
#define WORKER_STACK_SIZE       256
#define MONITOR_STACK_SIZE      384

/* Konfigurasi prioritas task */
#define WORKER_PRIORITY         2       /* Semua worker SAMA prioritas */
#define MONITOR_PRIORITY        4       /* Monitor lebih tinggi agar selalu bisa print */

/* Jumlah worker task */
#define NUM_WORKERS             3

/* Durasi setiap fase dalam milidetik */
#define PHASE_DURATION_MS       10000   /* 10 detik per fase */

/* Interval pelaporan monitor (ms) */
#define MONITOR_INTERVAL_MS     2000    /* Laporan setiap 2 detik */

/* Jumlah fase */
#define PHASE_PREEMPTIVE        1
#define PHASE_COOPERATIVE       2
#define PHASE_CPU_HOG           3
#define PHASE_COOLDOWN          4
#define NUM_PHASES              3

/* Jumlah iterasi kerja per loop (simulasi beban CPU) */
#define WORK_ITERATIONS         5000

/* Delay untuk fase preemptive (ms) */
#define PREEMPTIVE_DELAY_MS     50

/* Interval heartbeat LED per fase (beda pola) */
#define LED_FAST_MS             100
#define LED_MEDIUM_MS           300
#define LED_SLOW_MS             500

/* ===========================================================================
 * VARIABEL GLOBAL
 * =========================================================================== */

/* Handle UART */
static UART_HandleTypeDef huart1;

/* Handle task-task */
static TaskHandle_t xWorkerHandle[NUM_WORKERS];
static TaskHandle_t xMonitorHandle;

/* Mutex untuk akses UART (printf) */
static SemaphoreHandle_t xUartMutex;

/* Counter eksekusi per worker per fase */
static volatile uint32_t ulWorkerCount[NUM_WORKERS];

/* Counter eksekusi snapshot per worker untuk pelaporan periodik */
static volatile uint32_t ulWorkerSnapshot[NUM_WORKERS];

/* Fase saat ini (1-3) */
static volatile uint8_t ucCurrentPhase = 0;

/* Flag bahwa semua task harus berhenti sementara antar fase */
static volatile uint8_t ucPhaseTransition = 0;

/* Timestamp mulai fase (dalam tick) */
static volatile TickType_t xPhaseStartTick = 0;

/* Total count per fase untuk kalkulasi fairness */
static volatile uint32_t ulPhaseCount[NUM_PHASES + 1][NUM_WORKERS];

/* Total count gabungan per fase */
static volatile uint32_t ulPhaseTotalCount[NUM_PHASES + 1];

/* Fairness index per fase (dihitung oleh monitor) */
static volatile float fPhaseFairness[NUM_PHASES + 1];

/* Nomor siklus monitor */
static volatile uint32_t ulMonitorCycle = 0;

/* Flag CPU hog — hanya Worker-0 yang jadi CPU hog di fase 3 */
#define CPU_HOG_WORKER_ID       0

/* ===========================================================================
 * DEKLARASI FUNGSI (FORWARD DECLARATIONS)
 * =========================================================================== */

/* Konfigurasi sistem */
static void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART1_Init(void);

/* Task FreeRTOS */
static void vWorkerTask(void *pvParameters);
static void vMonitorTask(void *pvParameters);

/* Fungsi utilitas */
static void vSafePrintf(const char *fmt, ...);
static float fCalculateFairness(uint32_t *pulCounts, int iNumWorkers);
static void vPrintPhaseHeader(uint8_t ucPhase);
static void vPrintPhaseSummary(uint8_t ucPhase);
static void vPrintFinalReport(void);
static void vDoSimulatedWork(uint32_t ulIterations);

/* ===========================================================================
 * RETARGET PRINTF KE UART1
 * =========================================================================== */

/**
 * @brief Redirect printf ke UART1 via HAL_UART_Transmit
 */
int _write(int file, char *ptr, int len)
{
    (void)file;
    HAL_UART_Transmit(&huart1, (uint8_t *)ptr, len, HAL_MAX_DELAY);
    return len;
}

/* ===========================================================================
 * INTERRUPT HANDLERS
 * =========================================================================== */

/**
 * @brief Handler SysTick — diperlukan oleh FreeRTOS untuk tick scheduling
 */
void SysTick_Handler(void)
{
    HAL_IncTick();
    extern void xPortSysTickHandler(void);
    if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED)
    {
        xPortSysTickHandler();
    }
}

/* ===========================================================================
 * HOOK FUNCTIONS
 * =========================================================================== */

/**
 * @brief Hook saat alokasi memori (pvPortMalloc) gagal
 */
void vApplicationMallocFailedHook(void)
{
    printf("[ERROR] Malloc gagal! Heap FreeRTOS penuh.\r\n");
    for (;;)
    {
        LED_TOGGLE();
        for (volatile uint32_t i = 0; i < 200000; i++);
    }
}

/**
 * @brief Hook saat stack overflow terdeteksi pada suatu task
 */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    printf("[ERROR] Stack overflow pada task: %s\r\n", pcTaskName);
    for (;;)
    {
        LED_TOGGLE();
        for (volatile uint32_t i = 0; i < 100000; i++);
    }
}

/* ===========================================================================
 * STATIC ALLOCATION SUPPORT
 * =========================================================================== */

static StaticTask_t xIdleTaskTCB;
static StackType_t uxIdleTaskStack[configMINIMAL_STACK_SIZE];

void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer,
                                   StackType_t **ppxIdleTaskStackBuffer,
                                   uint32_t *pulIdleTaskStackSize)
{
    *ppxIdleTaskTCBBuffer   = &xIdleTaskTCB;
    *ppxIdleTaskStackBuffer = uxIdleTaskStack;
    *pulIdleTaskStackSize   = configMINIMAL_STACK_SIZE;
}

static StaticTask_t xTimerTaskTCB;
static StackType_t uxTimerTaskStack[configTIMER_TASK_STACK_DEPTH];

void vApplicationGetTimerTaskMemory(StaticTask_t **ppxTimerTaskTCBBuffer,
                                    StackType_t **ppxTimerTaskStackBuffer,
                                    uint32_t *pulTimerTaskStackSize)
{
    *ppxTimerTaskTCBBuffer   = &xTimerTaskTCB;
    *ppxTimerTaskStackBuffer = uxTimerTaskStack;
    *pulTimerTaskStackSize   = configTIMER_TASK_STACK_DEPTH;
}

/* ===========================================================================
 * KONFIGURASI SISTEM CLOCK
 * HSE 8MHz → PLL x9 → SYSCLK 72MHz
 * =========================================================================== */

static void SystemClock_Config(void)
{
#if defined(STM32F103xB)
    /* F1: HSE 8MHz -> PLL x9 =72MHz */
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
        for (;;);
    }

    RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                        RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
    {
        for (;;);
    }
#elif defined(STM32F401xC)
    /* F401: HSE 25MHz -> PLL (M=25, N=336, P=4) =84MHz */
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState       = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState   = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource  = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM       = 25;
    RCC_OscInitStruct.PLL.PLLN       = 336;
    RCC_OscInitStruct.PLL.PLLP       = RCC_PLLP_DIV4;
    RCC_OscInitStruct.PLL.PLLQ       = 7;

    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        for (;;);
    }

    RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                        RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
    {
        for (;;);
    }
#elif defined(STM32F411xE)
    /* F411: HSE 25MHz -> PLL (M=25, N=400, P=4) =100MHz */
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState       = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState   = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource  = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM       = 25;
    RCC_OscInitStruct.PLL.PLLN       = 400;
    RCC_OscInitStruct.PLL.PLLP       = RCC_PLLP_DIV4;
    RCC_OscInitStruct.PLL.PLLQ       = 7;

    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        for (;;);
    }

    RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                        RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
    {
        for (;;);
    }
#else
    #error "Unsupported STM32 target"
#endif
}

/* ===========================================================================
 * INISIALISASI GPIO — LED bawaan PC13
 * =========================================================================== */

static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOC_CLK_ENABLE();

    GPIO_InitStruct.Pin   = LED_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_PORT, &GPIO_InitStruct);

    LED_OFF();
}

/* ===========================================================================
 * INISIALISASI USART1 — PA9(TX), PA10(RX) @ 115200 baud
 * =========================================================================== */

static void MX_USART1_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_USART1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    /* PA9 TX — Alternate Function Push-Pull */
    GPIO_InitStruct.Pin   = GPIO_PIN_9;
    GPIO_InitStruct.Mode  = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
#if defined(STM32F401xC) || defined(STM32F411xE)
    GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
#endif
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* PA10 RX */
    GPIO_InitStruct.Pin   = GPIO_PIN_10;
#if defined(STM32F103xB)
    GPIO_InitStruct.Mode = GPIO_MODE_AF_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
#elif defined(STM32F401xC) || defined(STM32F411xE)
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
#endif
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* Konfigurasi USART1 */
    huart1.Instance          = USART1;
    huart1.Init.BaudRate     = DEBUG_BAUDRATE;
    huart1.Init.WordLength   = UART_WORDLENGTH_8B;
    huart1.Init.StopBits     = UART_STOPBITS_1;
    huart1.Init.Parity       = UART_PARITY_NONE;
    huart1.Init.Mode         = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;

    if (HAL_UART_Init(&huart1) != HAL_OK)
    {
        for (;;);
    }
}

/* ===========================================================================
 * FUNGSI UTILITAS
 * =========================================================================== */

/**
 * @brief Printf yang aman dari race condition (thread-safe via mutex)
 */
static void vSafePrintf(const char *fmt, ...)
{
    char buf[256];
    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    if (xUartMutex != NULL)
    {
        if (xSemaphoreTake(xUartMutex, pdMS_TO_TICKS(200)) == pdTRUE)
        {
            HAL_UART_Transmit(&huart1, (uint8_t *)buf, len, HAL_MAX_DELAY);
            xSemaphoreGive(xUartMutex);
        }
    }
    else
    {
        HAL_UART_Transmit(&huart1, (uint8_t *)buf, len, HAL_MAX_DELAY);
    }
}

/**
 * @brief Simulasi beban kerja CPU — loop komputasi tanpa efek samping
 * @param ulIterations : Jumlah iterasi perhitungan
 */
static void vDoSimulatedWork(uint32_t ulIterations)
{
    volatile uint32_t ulResult = 0;
    for (uint32_t i = 0; i < ulIterations; i++)
    {
        ulResult += (i * 7) ^ (i >> 2);
    }
    (void)ulResult;
}

/**
 * @brief Hitung Jain's Fairness Index untuk distribusi eksekusi
 *        Nilai 1.0 = sempurna merata, mendekati 1/n = sangat tidak merata
 *
 * @param pulCounts   : Array counter per worker
 * @param iNumWorkers : Jumlah worker
 * @return Fairness index (0.0 hingga 1.0)
 */
static float fCalculateFairness(uint32_t *pulCounts, int iNumWorkers)
{
    if (iNumWorkers <= 0) return 0.0f;

    float fSum = 0.0f;
    float fSumSq = 0.0f;

    for (int i = 0; i < iNumWorkers; i++)
    {
        float fVal = (float)pulCounts[i];
        fSum   += fVal;
        fSumSq += fVal * fVal;
    }

    /* Jain's Fairness Index = (sum(xi))^2 / (n * sum(xi^2)) */
    if (fSumSq == 0.0f) return 1.0f; /* Semua nol → "merata" */

    return (fSum * fSum) / ((float)iNumWorkers * fSumSq);
}

/**
 * @brief Cetak header untuk fase baru
 */
static void vPrintPhaseHeader(uint8_t ucPhase)
{
    vSafePrintf("\r\n");
    vSafePrintf("╔══════════════════════════════════════════════════════════════╗\r\n");

    switch (ucPhase)
    {
        case PHASE_PREEMPTIVE:
            vSafePrintf("║  FASE 1: PREEMPTIVE + TIME SLICING (10 detik)               ║\r\n");
            vSafePrintf("║  Worker menggunakan vTaskDelay() — round-robin otomatis      ║\r\n");
            break;
        case PHASE_COOPERATIVE:
            vSafePrintf("║  FASE 2: COOPERATIVE YIELD (10 detik)                        ║\r\n");
            vSafePrintf("║  Worker menggunakan taskYIELD() — CPU dilepas sukarela       ║\r\n");
            break;
        case PHASE_CPU_HOG:
            vSafePrintf("║  FASE 3: CPU HOG DEMONSTRATION (10 detik)                    ║\r\n");
            vSafePrintf("║  Worker-0 busy loop tanpa yield — starvation demo            ║\r\n");
            break;
        default:
            vSafePrintf("║  FASE %d: UNKNOWN                                             ║\r\n", ucPhase);
            break;
    }

    vSafePrintf("╚══════════════════════════════════════════════════════════════╝\r\n");
    vSafePrintf("\r\n");
}

/**
 * @brief Cetak ringkasan setelah fase selesai
 */
static void vPrintPhaseSummary(uint8_t ucPhase)
{
    uint32_t ulTotal = 0;
    uint32_t ulCounts[NUM_WORKERS];

    for (int i = 0; i < NUM_WORKERS; i++)
    {
        ulCounts[i] = ulPhaseCount[ucPhase][i];
        ulTotal += ulCounts[i];
    }

    float fFairness = fCalculateFairness(ulCounts, NUM_WORKERS);
    ulPhaseTotalCount[ucPhase] = ulTotal;
    fPhaseFairness[ucPhase] = fFairness;

    vSafePrintf("\r\n");
    vSafePrintf("┌──────────────────────────────────────────────────────────────┐\r\n");
    vSafePrintf("│            RINGKASAN FASE %d                                 │\r\n", ucPhase);
    vSafePrintf("├──────────────────────────────────────────────────────────────┤\r\n");

    for (int i = 0; i < NUM_WORKERS; i++)
    {
        float fPct = (ulTotal > 0) ? ((float)ulCounts[i] / (float)ulTotal * 100.0f) : 0.0f;
        vSafePrintf("│  Worker-%d: %-8lu eksekusi  (%5.1f%%)                       │\r\n",
                    i, (unsigned long)ulCounts[i], fPct);
    }

    vSafePrintf("├──────────────────────────────────────────────────────────────┤\r\n");
    vSafePrintf("│  Total Eksekusi : %-10lu                                 │\r\n", (unsigned long)ulTotal);
    vSafePrintf("│  Fairness Index : %.4f  ", fFairness);

    if (fFairness > 0.95f)
        vSafePrintf("(SANGAT MERATA)              │\r\n");
    else if (fFairness > 0.80f)
        vSafePrintf("(CUKUP MERATA)               │\r\n");
    else if (fFairness > 0.50f)
        vSafePrintf("(KURANG MERATA)              │\r\n");
    else
        vSafePrintf("(SANGAT TIDAK MERATA)        │\r\n");

    vSafePrintf("└──────────────────────────────────────────────────────────────┘\r\n");

    /* Data tag untuk Python */
    vSafePrintf("[DATA]PHASE phase=%d,total_count=%lu,fairness=%.4f\r\n",
                ucPhase, (unsigned long)ulTotal, fFairness);
}

/**
 * @brief Cetak laporan perbandingan akhir semua fase
 */
static void vPrintFinalReport(void)
{
    vSafePrintf("\r\n");
    vSafePrintf("╔══════════════════════════════════════════════════════════════╗\r\n");
    vSafePrintf("║           LAPORAN PERBANDINGAN AKHIR                        ║\r\n");
    vSafePrintf("╠══════════════════════════════════════════════════════════════╣\r\n");
    vSafePrintf("║ Fase │ Mode          │ Total       │ Fairness │ Kualitas    ║\r\n");
    vSafePrintf("╠══════════════════════════════════════════════════════════════╣\r\n");

    const char *pcModeNames[] = {"", "Preemptive", "Cooperative", "CPU Hog"};

    for (int p = 1; p <= NUM_PHASES; p++)
    {
        const char *pcQuality;
        if (fPhaseFairness[p] > 0.95f)      pcQuality = "Sempurna ";
        else if (fPhaseFairness[p] > 0.80f)  pcQuality = "Baik     ";
        else if (fPhaseFairness[p] > 0.50f)  pcQuality = "Kurang   ";
        else                                  pcQuality = "Buruk    ";

        vSafePrintf("║  %d   │ %-13s │ %-11lu │  %.4f  │ %s   ║\r\n",
                    p, pcModeNames[p],
                    (unsigned long)ulPhaseTotalCount[p],
                    fPhaseFairness[p],
                    pcQuality);
    }

    vSafePrintf("╚══════════════════════════════════════════════════════════════╝\r\n");
    vSafePrintf("\r\n");

    /* Analisis hasil */
    vSafePrintf("--- Analisis ---\r\n");
    vSafePrintf("  Fase 1 (Preemptive): Scheduler membagi CPU otomatis via time slicing.\r\n");
    vSafePrintf("    Semua task dengan prioritas sama mendapat giliran merata.\r\n");
    vSafePrintf("    vTaskDelay() membuat task masuk Blocked state.\r\n\r\n");

    vSafePrintf("  Fase 2 (Cooperative): taskYIELD() secara eksplisit menyerahkan CPU.\r\n");
    vSafePrintf("    Task tetap Ready (bukan Blocked) setelah yield.\r\n");
    vSafePrintf("    Distribusi lebih agresif karena tidak ada delay.\r\n\r\n");

    vSafePrintf("  Fase 3 (CPU Hog): Worker-0 TIDAK yield — monopoli CPU.\r\n");
    vSafePrintf("    Task lain kelaparan (starvation) karena tidak bisa jalan.\r\n");
    vSafePrintf("    Ini menunjukkan bahaya task yang tidak kooperatif.\r\n\r\n");

    vSafePrintf("[INFO] Eksperimen selesai. Memulai ulang dari Fase 1...\r\n\r\n");
}

/* ===========================================================================
 * TASK FUNCTIONS
 * =========================================================================== */

/**
 * @brief Worker task — perilaku berubah sesuai fase aktif
 *
 *   Fase 1: vTaskDelay() setelah kerja — preemptive normal
 *   Fase 2: taskYIELD() setelah kerja — cooperative yield
 *   Fase 3: Worker-0 busy loop, sisanya taskYIELD()
 *
 * @param pvParameters : ID worker (0, 1, 2)
 */
static void vWorkerTask(void *pvParameters)
{
    uint32_t ulWorkerID = (uint32_t)pvParameters;
    uint32_t ulLocalCount = 0;
    uint8_t ucLastPhase = 0;

    vSafePrintf("[WORKER-%lu] Dimulai (prioritas=%lu)\r\n",
                ulWorkerID, (unsigned long)uxTaskPriorityGet(NULL));

    for (;;)
    {
        /* Tunggu sampai fase dimulai */
        if (ucCurrentPhase == 0 || ucPhaseTransition)
        {
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }

        /* Deteksi pergantian fase — reset counter lokal */
        if (ucCurrentPhase != ucLastPhase)
        {
            ulLocalCount = 0;
            ucLastPhase = ucCurrentPhase;
        }

        /* Cek apakah fase masih berlangsung */
        TickType_t xElapsed = xTaskGetTickCount() - xPhaseStartTick;
        if (xElapsed > pdMS_TO_TICKS(PHASE_DURATION_MS))
        {
            /* Fase sudah habis waktunya — tunggu monitor mengatur fase berikutnya */
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }

        /* ────────────────────────────────────────────── */
        /* EKSEKUSI SESUAI FASE                          */
        /* ────────────────────────────────────────────── */

        switch (ucCurrentPhase)
        {
            /* ── FASE 1: Preemptive + Time Slicing ── */
            case PHASE_PREEMPTIVE:
            {
                /* Lakukan pekerjaan */
                vDoSimulatedWork(WORK_ITERATIONS);

                /* Increment counter */
                ulLocalCount++;
                ulWorkerCount[ulWorkerID]++;
                ulPhaseCount[PHASE_PREEMPTIVE][ulWorkerID]++;

                /* Cetak data tag periodik */
                if (ulLocalCount % 20 == 0)
                {
                    vSafePrintf("[DATA]WORKER id=%lu,count=%lu,phase=1\r\n",
                                ulWorkerID, (unsigned long)ulLocalCount);
                }

                /* vTaskDelay — task masuk Blocked, scheduler bisa pilih task lain */
                vTaskDelay(pdMS_TO_TICKS(PREEMPTIVE_DELAY_MS));
                break;
            }

            /* ── FASE 2: Cooperative Yield ── */
            case PHASE_COOPERATIVE:
            {
                /* Lakukan pekerjaan */
                vDoSimulatedWork(WORK_ITERATIONS);

                /* Increment counter */
                ulLocalCount++;
                ulWorkerCount[ulWorkerID]++;
                ulPhaseCount[PHASE_COOPERATIVE][ulWorkerID]++;

                /* Cetak data tag periodik */
                if (ulLocalCount % 500 == 0)
                {
                    vSafePrintf("[DATA]WORKER id=%lu,count=%lu,phase=2\r\n",
                                ulWorkerID, (unsigned long)ulLocalCount);
                }

                /* taskYIELD — secara sukarela lepas CPU ke task lain yang Ready */
                /* Task ini tetap Ready (tidak Blocked) */
                taskYIELD();
                break;
            }

            /* ── FASE 3: CPU Hog ── */
            case PHASE_CPU_HOG:
            {
                if (ulWorkerID == CPU_HOG_WORKER_ID)
                {
                    /* Worker-0: MONOPOLI CPU — busy loop tanpa yield */
                    /* Ini menyebabkan task lain dengan prioritas sama kelaparan */

                    vDoSimulatedWork(WORK_ITERATIONS);

                    ulLocalCount++;
                    ulWorkerCount[ulWorkerID]++;
                    ulPhaseCount[PHASE_CPU_HOG][ulWorkerID]++;

                    /* Cetak sangat jarang agar tidak bloking UART terlalu lama */
                    if (ulLocalCount % 2000 == 0)
                    {
                        vSafePrintf("[DATA]WORKER id=%lu,count=%lu,phase=3\r\n",
                                    ulWorkerID, (unsigned long)ulLocalCount);
                    }

                    /* TIDAK ADA yield atau delay — CPU hog sejati! */
                    /* Namun, karena configUSE_PREEMPTION=1 dan time slicing aktif,
                       scheduler tetap bisa preempt. Untuk mendemonstrasikan starvation
                       yang lebih nyata, kita gunakan critical section pendek */

                    /* Tambahan: kurangi time slicing effect dengan loop lebih panjang */
                    vDoSimulatedWork(WORK_ITERATIONS * 3);
                }
                else
                {
                    /* Worker-1, Worker-2: tetap cooperative */
                    vDoSimulatedWork(WORK_ITERATIONS);

                    ulLocalCount++;
                    ulWorkerCount[ulWorkerID]++;
                    ulPhaseCount[PHASE_CPU_HOG][ulWorkerID]++;

                    if (ulLocalCount % 500 == 0)
                    {
                        vSafePrintf("[DATA]WORKER id=%lu,count=%lu,phase=3\r\n",
                                    ulWorkerID, (unsigned long)ulLocalCount);
                    }

                    /* Yield — siap memberi kesempatan tapi CPU hog mendominasi */
                    taskYIELD();
                }
                break;
            }

            default:
                vTaskDelay(pdMS_TO_TICKS(50));
                break;
        }
    }
}

/**
 * @brief Monitor task — mengelola transisi fase dan mencetak laporan
 *        Prioritas lebih tinggi agar selalu bisa berjalan meskipun ada CPU hog
 */
static void vMonitorTask(void *pvParameters)
{
    (void)pvParameters;

    /* Tunggu semua worker siap */
    vTaskDelay(pdMS_TO_TICKS(2000));

    printf("\r\n");
    printf("============================================================\r\n");
    printf("   STM32_12 — Cooperative Scheduling Monitor\r\n");
    printf("   %d worker task, prioritas sama = %d\r\n", NUM_WORKERS, WORKER_PRIORITY);
    printf("   Durasi per fase: %d detik\r\n", PHASE_DURATION_MS / 1000);
    printf("============================================================\r\n\r\n");

    /* ── Loop utama: jalankan semua fase berulang ── */
    for (;;)
    {
        /* ================================================================ */
        /* ITERASI SEMUA FASE                                               */
        /* ================================================================ */

        for (uint8_t ucPhase = PHASE_PREEMPTIVE; ucPhase <= PHASE_CPU_HOG; ucPhase++)
        {
            /* ── Persiapan fase baru ── */
            ucPhaseTransition = 1;

            /* Reset counter untuk fase ini */
            for (int i = 0; i < NUM_WORKERS; i++)
            {
                ulWorkerCount[i] = 0;
                ulWorkerSnapshot[i] = 0;
                ulPhaseCount[ucPhase][i] = 0;
            }

            /* Cetak header fase */
            vPrintPhaseHeader(ucPhase);

            /* Set fase aktif dan timestamp */
            xPhaseStartTick = xTaskGetTickCount();
            ucCurrentPhase = ucPhase;
            ucPhaseTransition = 0;

            /* LED pattern berdasarkan fase */
            uint32_t ulLedInterval;
            switch (ucPhase)
            {
                case PHASE_PREEMPTIVE:  ulLedInterval = LED_SLOW_MS;   break;
                case PHASE_COOPERATIVE: ulLedInterval = LED_MEDIUM_MS; break;
                case PHASE_CPU_HOG:     ulLedInterval = LED_FAST_MS;   break;
                default:                ulLedInterval = LED_SLOW_MS;   break;
            }

            /* ── Monitor selama fase berjalan ── */
            TickType_t xMonitorInterval = pdMS_TO_TICKS(MONITOR_INTERVAL_MS);
            TickType_t xLastReportTime = xTaskGetTickCount();
            uint32_t ulReportNum = 0;

            while (1)
            {
                TickType_t xNow = xTaskGetTickCount();
                TickType_t xElapsed = xNow - xPhaseStartTick;

                /* Cek apakah fase sudah selesai */
                if (xElapsed >= pdMS_TO_TICKS(PHASE_DURATION_MS))
                {
                    break;
                }

                /* Toggle LED dengan interval sesuai fase */
                LED_TOGGLE();

                /* Cetak laporan periodik */
                if ((xNow - xLastReportTime) >= xMonitorInterval)
                {
                    ulReportNum++;
                    ulMonitorCycle++;

                    uint32_t ulElapsedSec = xElapsed / configTICK_RATE_HZ;
                    uint32_t ulRemainingSec = (PHASE_DURATION_MS / 1000) - ulElapsedSec;

                    vSafePrintf("\r\n--- Laporan Fase %d #%lu (sisa %lus) ---\r\n",
                                ucPhase, (unsigned long)ulReportNum,
                                (unsigned long)ulRemainingSec);

                    /* Hitung delta sejak snapshot terakhir */
                    uint32_t ulDeltaCounts[NUM_WORKERS];
                    uint32_t ulTotalDelta = 0;

                    for (int i = 0; i < NUM_WORKERS; i++)
                    {
                        ulDeltaCounts[i] = ulWorkerCount[i] - ulWorkerSnapshot[i];
                        ulTotalDelta += ulDeltaCounts[i];
                        ulWorkerSnapshot[i] = ulWorkerCount[i];
                    }

                    /* Cetak count per worker */
                    for (int i = 0; i < NUM_WORKERS; i++)
                    {
                        float fPct = (ulTotalDelta > 0) ?
                                     ((float)ulDeltaCounts[i] / (float)ulTotalDelta * 100.0f) : 0.0f;

                        vSafePrintf("  Worker-%d: +%-6lu (total: %-8lu) [%5.1f%%]\r\n",
                                    i,
                                    (unsigned long)ulDeltaCounts[i],
                                    (unsigned long)ulWorkerCount[i],
                                    fPct);

                        /* Data tag per worker */
                        vSafePrintf("[DATA]WORKER id=%d,count=%lu,phase=%d\r\n",
                                    i, (unsigned long)ulWorkerCount[i], ucPhase);
                    }

                    /* Hitung fairness sementara */
                    float fFairness = fCalculateFairness(ulDeltaCounts, NUM_WORKERS);
                    vSafePrintf("  Delta total: %lu | Fairness: %.4f\r\n",
                                (unsigned long)ulTotalDelta, fFairness);

                    /* Stack high water mark per worker */
                    vSafePrintf("  Stack HWM: ");
                    for (int i = 0; i < NUM_WORKERS; i++)
                    {
                        if (xWorkerHandle[i] != NULL)
                        {
                            vSafePrintf("W%d=%lu ",
                                        i,
                                        (unsigned long)uxTaskGetStackHighWaterMark(xWorkerHandle[i]));
                        }
                    }
                    vSafePrintf("\r\n");

                    /* Heap info */
                    vSafePrintf("  Free heap: %u bytes | Min: %u bytes\r\n",
                                (unsigned int)xPortGetFreeHeapSize(),
                                (unsigned int)xPortGetMinimumEverFreeHeapSize());

                    xLastReportTime = xNow;
                }

                /* Delay monitor — interval berdasarkan pola LED */
                vTaskDelay(pdMS_TO_TICKS(ulLedInterval));
            }

            /* ── Fase selesai — cetak ringkasan ── */
            ucPhaseTransition = 1;

            /* Tunggu sebentar agar worker berhenti */
            vTaskDelay(pdMS_TO_TICKS(500));

            /* Cetak ringkasan fase */
            vPrintPhaseSummary(ucPhase);

            /* Jeda antar fase */
            vSafePrintf("\r\n[INFO] Jeda 3 detik sebelum fase berikutnya...\r\n\r\n");
            LED_OFF();
            vTaskDelay(pdMS_TO_TICKS(3000));
        }

        /* ── Semua fase selesai — cetak laporan akhir ── */
        vPrintFinalReport();

        /* Jeda sebelum mengulang siklus */
        vSafePrintf("[INFO] Jeda 5 detik sebelum siklus berikutnya...\r\n\r\n");

        /* Kedipkan LED 5 kali sebagai tanda siklus selesai */
        for (int i = 0; i < 10; i++)
        {
            LED_TOGGLE();
            vTaskDelay(pdMS_TO_TICKS(250));
        }

        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}

/* ===========================================================================
 * FUNGSI UTAMA — Entry Point Program
 * =========================================================================== */

/**
 * @brief Fungsi utama program
 */
int main(void)
{
    /* ── Inisialisasi HAL dan hardware ── */
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USART1_Init();

    /* ── Banner startup ── */
    printf("\r\n");
    printf("============================================================\r\n");
    printf("  STM32_12_Task_Cooperative\r\n");
    printf("  Demonstrasi taskYIELD() & Cooperative Scheduling\r\n");
    printf("============================================================\r\n");
    printf("  MCU     : STM32F103C8T6 (Blue Pill)\r\n");
    printf("  Clock   : 72 MHz (HSE 8MHz x PLL9)\r\n");
    printf("  UART    : 115200 baud (PA9-TX, PA10-RX)\r\n");
    printf("  LED     : PC13 (active low)\r\n");
    printf("  Heap    : %u bytes\r\n", (unsigned int)configTOTAL_HEAP_SIZE);
    printf("  Workers : %d task (prioritas sama = %d)\r\n", NUM_WORKERS, WORKER_PRIORITY);
    printf("============================================================\r\n");
    printf("\r\n");

    printf("[INIT] Eksperimen Cooperative Scheduling:\r\n");
    printf("  Fase 1 (10s): Preemptive + Time Slicing (vTaskDelay)\r\n");
    printf("  Fase 2 (10s): Cooperative Yield (taskYIELD)\r\n");
    printf("  Fase 3 (10s): CPU Hog (Worker-0 monopoli CPU)\r\n");
    printf("\r\n");

    /* ── Buat mutex UART ── */
    printf("[INIT] Membuat mutex UART...\r\n");
    xUartMutex = xSemaphoreCreateMutex();
    if (xUartMutex == NULL)
    {
        printf("[ERROR] Gagal membuat mutex UART!\r\n");
        for (;;);
    }

    /* ── Inisialisasi counter ── */
    memset((void *)ulWorkerCount, 0, sizeof(ulWorkerCount));
    memset((void *)ulWorkerSnapshot, 0, sizeof(ulWorkerSnapshot));
    memset((void *)ulPhaseCount, 0, sizeof(ulPhaseCount));
    memset((void *)ulPhaseTotalCount, 0, sizeof(ulPhaseTotalCount));
    memset((void *)fPhaseFairness, 0, sizeof(fPhaseFairness));

    /* ── Buat Worker Tasks — SEMUA dengan prioritas SAMA ── */
    printf("[INIT] Membuat %d worker task (semua prio=%d)...\r\n",
           NUM_WORKERS, WORKER_PRIORITY);

    char pcWorkerName[configMAX_TASK_NAME_LEN];
    for (int i = 0; i < NUM_WORKERS; i++)
    {
        snprintf(pcWorkerName, sizeof(pcWorkerName), "Worker-%d", i);

        BaseType_t xResult = xTaskCreate(
            vWorkerTask,
            pcWorkerName,
            WORKER_STACK_SIZE,
            (void *)(uint32_t)i,
            WORKER_PRIORITY,
            &xWorkerHandle[i]
        );

        if (xResult == pdPASS)
        {
            printf("[INIT]   %s dibuat (prio=%d, stack=%d words)\r\n",
                   pcWorkerName, WORKER_PRIORITY, WORKER_STACK_SIZE);
        }
        else
        {
            printf("[ERROR]  Gagal membuat %s!\r\n", pcWorkerName);
        }
    }

    /* ── Buat Monitor Task — prioritas lebih tinggi ── */
    printf("[INIT] Membuat monitor task (prio=%d)...\r\n", MONITOR_PRIORITY);

    BaseType_t xResult = xTaskCreate(
        vMonitorTask,
        "Monitor",
        MONITOR_STACK_SIZE,
        NULL,
        MONITOR_PRIORITY,
        &xMonitorHandle
    );

    if (xResult == pdPASS)
    {
        printf("[INIT]   Monitor dibuat (prio=%d, stack=%d words)\r\n",
               MONITOR_PRIORITY, MONITOR_STACK_SIZE);
    }
    else
    {
        printf("[ERROR]  Gagal membuat Monitor task!\r\n");
    }

    /* ── Ringkasan ── */
    printf("\r\n");
    printf("[INIT] Catatan penting:\r\n");
    printf("  - configUSE_PREEMPTION=1 → scheduler tetap preemptive\r\n");
    printf("  - Time slicing aktif untuk task prioritas sama\r\n");
    printf("  - Fase 3 menunjukkan CPU hog MESKIPUN preemptive aktif\r\n");
    printf("    karena Worker-0 menggunakan loop sangat panjang\r\n");
    printf("\r\n");
    printf("[INIT] Free heap: %u bytes\r\n",
           (unsigned int)xPortGetFreeHeapSize());
    printf("[INIT] Memulai FreeRTOS scheduler...\r\n");
    printf("============================================================\r\n\r\n");

    /* ── Mulai FreeRTOS Scheduler ── */
    vTaskStartScheduler();

    /* Seharusnya tidak pernah sampai sini */
    printf("[FATAL] Scheduler berhenti!\r\n");
    for (;;)
    {
        LED_TOGGLE();
        for (volatile uint32_t i = 0; i < 500000; i++);
    }

    return 0;
}
