/**
 * ============================================================================
 * PROGRAM 02: Task Priority - Demonstrasi Prioritas & Preemption FreeRTOS
 * ============================================================================
 * 
 * Deskripsi:
 *   Program ini mendemonstrasikan mekanisme prioritas task dan preemptive
 *   scheduling pada FreeRTOS. Tiga task dengan prioritas berbeda (Low=1,
 *   Medium=3, High=5) melakukan CPU-intensive work, dan kita mengamati
 *   bagaimana scheduler memberikan CPU kepada task berprioritas tinggi.
 * 
 * Konsep yang Dipelajari:
 *   1. Preemptive Scheduling - Task prioritas tinggi mengambil alih CPU
 *   2. vTaskPrioritySet()    - Mengubah prioritas task saat runtime
 *   3. uxTaskPriorityGet()   - Membaca prioritas task saat ini
 *   4. CPU Time Distribution - Bagaimana CPU dibagi antar task
 *   5. Priority Inversion    - Potensi masalah pada sistem prioritas
 * 
 * Arsitektur Fase:
 *   Fase 1 (8 detik): Prioritas normal → Low=1, Med=3, High=5
 *   Fase 2 (8 detik): Prioritas ditukar → Low=5, Med=3, High=1
 *   Fase 3 (8 detik): Prioritas dikembalikan → Low=1, Med=3, High=5
 *   (Berulang)
 * 
 * Hardware:
 *   - STM32F103C8 Blue Pill (72 MHz, 20KB SRAM)
 *   - LED PC13: Indikator Task Medium
 *   - LED PB0:  Indikator Task Low
 *   - LED PB1:  Indikator Task High
 *   - USART1: PA9(TX)/PA10(RX) @ 115200 baud
 * 
 * Output Format (untuk Python parser):
 *   [DATA]EXEC,<nama_task>,<prioritas>,<elapsed_us>,<iteration>,<tick>
 *   [DATA]PHASE,<nomor_fase>,<deskripsi>,<tick>
 *   [DATA]PRIORITY,<nama>,<old_prio>,<new_prio>,<tick>
 *   [DATA]ORDER,<urutan_eksekusi>,<tick>
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

/* Handle UART */
static UART_HandleTypeDef huart1;

/* Handle task FreeRTOS */
static TaskHandle_t xTaskLowHandle   = NULL;
static TaskHandle_t xTaskMedHandle   = NULL;
static TaskHandle_t xTaskHighHandle  = NULL;
static TaskHandle_t xMonitorHandle   = NULL;

/* Flag scheduler sudah berjalan */
static volatile uint8_t ucSchedulerStarted = 0;

/**
 * Struktur data untuk mencatat eksekusi task
 * Setiap task memiliki counter dan timing sendiri
 */
typedef struct {
    uint32_t ulExecutionCount;      /* Jumlah kali task menyelesaikan work   */
    uint32_t ulLastElapsedUs;       /* Waktu eksekusi terakhir (mikrodetik)  */
    uint32_t ulTotalElapsedUs;      /* Total waktu eksekusi kumulatif        */
    uint32_t ulMinElapsedUs;        /* Waktu eksekusi minimum                */
    uint32_t ulMaxElapsedUs;        /* Waktu eksekusi maksimum               */
    UBaseType_t uxCurrentPriority;  /* Prioritas saat ini                    */
    TickType_t xLastCompleteTick;   /* Tick saat task terakhir selesai       */
} TaskStats_t;

static volatile TaskStats_t xTaskLowStats  = {0, 0, 0, 0xFFFFFFFF, 0, TASK_LOW_PRIORITY, 0};
static volatile TaskStats_t xTaskMedStats  = {0, 0, 0, 0xFFFFFFFF, 0, TASK_MED_PRIORITY, 0};
static volatile TaskStats_t xTaskHighStats = {0, 0, 0, 0xFFFFFFFF, 0, TASK_HIGH_PRIORITY, 0};

/**
 * Fase saat ini (1, 2, atau 3)
 * Volatile karena diubah oleh monitor task dan dibaca oleh worker task
 */
static volatile uint8_t ucCurrentPhase = 1;

/**
 * Array untuk mencatat urutan eksekusi task
 * 'L' = Low, 'M' = Medium, 'H' = High
 */
#define EXEC_ORDER_SIZE  64
static volatile char cExecOrder[EXEC_ORDER_SIZE];
static volatile uint8_t ucExecOrderIdx = 0;

/* Buffer print */
static char pcPrintBuffer[PRINT_BUFFER_SIZE];

/* DWT Cycle Counter untuk pengukuran presisi */
#define DWT_CONTROL     (*((volatile uint32_t*)0xE0001000))
#define DWT_CYCCNT      (*((volatile uint32_t*)0xE0001004))
#define SCB_DEMCR       (*((volatile uint32_t*)0xE000EDFC))
#define TRCENA_BIT      (1UL << 24)

/* ==================== DEKLARASI FUNGSI PROTOTYPE ========================= */
static void SystemClock_Config(void);
static void GPIO_Init(void);
static void UART1_Init(void);
static void DWT_Init(void);
static uint32_t DWT_GetCycles(void);

static void vTaskLow(void *pvParameters);
static void vTaskMed(void *pvParameters);
static void vTaskHigh(void *pvParameters);
static void vTaskMonitor(void *pvParameters);

static void vDoWork(const char *pcName, TaskStats_t volatile *pxStats, 
                    GPIO_TypeDef *pLedPort, uint16_t usLedPin);
static void vRecordExecOrder(char cTaskId);
static void vPrintSeparator(char cChar, uint8_t ucLen);
static const char* pcTaskStateToString(eTaskState eState);

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
        HAL_GPIO_TogglePin(LED_BUILTIN_PORT, LED_BUILTIN_PIN);
        for (volatile uint32_t i = 0; i < 200000; i++);
    }
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
    (void)xTask;
    printf("\r\n[ERROR] !! STACK OVERFLOW: %s !!\r\n", pcTaskName);
    taskDISABLE_INTERRUPTS();
    for (;;) {
        HAL_GPIO_TogglePin(LED_BUILTIN_PORT, LED_BUILTIN_PIN);
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

/**
 * Konfigurasi Clock: HSE 8MHz → PLL x9 → SYSCLK 72MHz
 */
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

    /* Aktifkan clock GPIO */
    LED_BUILTIN_CLK_EN();
    LED_LOW_CLK_EN();
    LED_HIGH_CLK_EN();
    BTN_CLK_EN();

    /* LED Built-in PC13 (active LOW) - Indikator Task Medium */
    GPIO_InitStruct.Pin   = LED_BUILTIN_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_BUILTIN_PORT, &GPIO_InitStruct);
    HAL_GPIO_WritePin(LED_BUILTIN_PORT, LED_BUILTIN_PIN, GPIO_PIN_SET);

    /* LED PB0 - Indikator Task Low */
    GPIO_InitStruct.Pin = LED_LOW_PIN;
    HAL_GPIO_Init(LED_LOW_PORT, &GPIO_InitStruct);
    HAL_GPIO_WritePin(LED_LOW_PORT, LED_LOW_PIN, GPIO_PIN_RESET);

    /* LED PB1 - Indikator Task High */
    GPIO_InitStruct.Pin = LED_HIGH_PIN;
    HAL_GPIO_Init(LED_HIGH_PORT, &GPIO_InitStruct);
    HAL_GPIO_WritePin(LED_HIGH_PORT, LED_HIGH_PIN, GPIO_PIN_RESET);

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
 * DWT CYCCNT adalah counter 32-bit yang menghitung siklus clock CPU.
 * Pada 72 MHz, satu siklus ≈ 13.9 ns, memberikan resolusi sangat tinggi.
 * Counter overflow setelah ~59.6 detik pada 72 MHz.
 * 
 * Digunakan untuk mengukur waktu eksekusi CPU work secara presisi.
 */
static void DWT_Init(void) {
    SCB_DEMCR |= TRCENA_BIT;       /* Aktifkan trace module             */
    DWT_CYCCNT  = 0;                /* Reset counter                     */
    DWT_CONTROL |= 1;               /* Aktifkan cycle counter            */
}

/**
 * Baca nilai DWT cycle counter saat ini
 * @return Jumlah siklus clock sejak counter direset/overflow
 */
static uint32_t DWT_GetCycles(void) {
    return DWT_CYCCNT;
}

/* ========================= UTILITY FUNCTIONS ============================= */

static void vPrintSeparator(char cChar, uint8_t ucLen) {
    for (uint8_t i = 0; i < ucLen; i++) printf("%c", cChar);
    printf("\r\n");
}

static const char* pcTaskStateToString(eTaskState eState) {
    switch (eState) {
        case eRunning:   return "RUNNING";
        case eReady:     return "READY";
        case eBlocked:   return "BLOCKED";
        case eSuspended: return "SUSPENDED";
        case eDeleted:   return "DELETED";
        default:         return "UNKNOWN";
    }
}

/**
 * Catat urutan eksekusi task ke dalam array
 * 
 * Fungsi ini dipanggil oleh setiap worker task setelah menyelesaikan
 * satu unit pekerjaan. Hasilnya adalah string seperti "HHMHLMH..."
 * yang menunjukkan urutan task yang mendapat CPU time.
 * 
 * @param cTaskId  Karakter identifikasi task ('L', 'M', atau 'H')
 */
static void vRecordExecOrder(char cTaskId) {
    if (ucExecOrderIdx < EXEC_ORDER_SIZE - 1) {
        cExecOrder[ucExecOrderIdx++] = cTaskId;
        cExecOrder[ucExecOrderIdx]   = '\0';
    }
}

/* ====================== CPU WORK FUNCTION ================================ */

/**
 * Simulasi beban kerja CPU (CPU-bound work)
 * 
 * Fungsi ini melakukan loop counting sebanyak CPU_WORK_ITERATIONS kali,
 * mensimulasikan task yang membutuhkan waktu CPU signifikan.
 * 
 * Mengukur waktu eksekusi menggunakan DWT cycle counter dan
 * mengirim data via UART untuk analisis.
 * 
 * Pada preemptive scheduling:
 *   - Task High (prio 5) akan preempt task Low/Med saat ready
 *   - Task Med (prio 3) akan preempt task Low saat ready
 *   - Task Low (prio 1) hanya berjalan saat task lain blocked
 * 
 * @param pcName    Nama task untuk identifikasi
 * @param pxStats   Pointer ke struktur statistik task
 * @param pLedPort  Port GPIO LED indikator
 * @param usLedPin  Pin GPIO LED indikator
 */
static void vDoWork(const char *pcName, TaskStats_t volatile *pxStats,
                    GPIO_TypeDef *pLedPort, uint16_t usLedPin) {
    /* Nyalakan LED saat task mulai bekerja */
    HAL_GPIO_WritePin(pLedPort, usLedPin, GPIO_PIN_SET);

    /* Ambil timestamp awal menggunakan DWT */
    uint32_t ulStartCycles = DWT_GetCycles();

    /* Simulasi beban kerja CPU - loop counting */
    volatile uint32_t ulCounter = 0;
    for (uint32_t i = 0; i < CPU_WORK_ITERATIONS; i++) {
        ulCounter++;
    }

    /* Ambil timestamp akhir */
    uint32_t ulEndCycles = DWT_GetCycles();

    /* Hitung waktu elapsed dalam mikrodetik */
    uint32_t ulElapsedCycles = ulEndCycles - ulStartCycles;
    uint32_t ulElapsedUs = ulElapsedCycles / SYSTEM_CLOCK_MHZ;

    /* Update statistik task */
    pxStats->ulExecutionCount++;
    pxStats->ulLastElapsedUs = ulElapsedUs;
    pxStats->ulTotalElapsedUs += ulElapsedUs;
    pxStats->uxCurrentPriority = uxTaskPriorityGet(NULL);
    pxStats->xLastCompleteTick = xTaskGetTickCount();

    if (ulElapsedUs < pxStats->ulMinElapsedUs) {
        pxStats->ulMinElapsedUs = ulElapsedUs;
    }
    if (ulElapsedUs > pxStats->ulMaxElapsedUs) {
        pxStats->ulMaxElapsedUs = ulElapsedUs;
    }

    /* Matikan LED setelah selesai bekerja */
    HAL_GPIO_WritePin(pLedPort, usLedPin, GPIO_PIN_RESET);

    /* Cetak hasil eksekusi */
    printf("[%s] Selesai #%lu | Elapsed: %lu us | Prio: %lu | Phase: %u\r\n",
           pcName,
           (unsigned long)pxStats->ulExecutionCount,
           (unsigned long)ulElapsedUs,
           (unsigned long)pxStats->uxCurrentPriority,
           ucCurrentPhase);

    /* Data untuk Python parser */
    printf("[DATA]EXEC,%s,%lu,%lu,%lu,%lu\r\n",
           pcName,
           (unsigned long)pxStats->uxCurrentPriority,
           (unsigned long)ulElapsedUs,
           (unsigned long)pxStats->ulExecutionCount,
           (unsigned long)xTaskGetTickCount());
}

/* ========================== TASK FUNCTIONS =============================== */

/**
 * Task Low Priority (Prioritas 1)
 * 
 * Task ini memiliki prioritas terendah di antara worker tasks.
 * Dalam mode preemptive:
 *   - Akan di-preempt oleh Task Med dan Task High
 *   - Hanya mendapat CPU time saat kedua task lain sedang blocked
 *   - Waktu eksekusi terukur lebih lama karena preemption
 * 
 * Saat prioritas ditukar (Fase 2):
 *   - Prioritas berubah menjadi 5 (tertinggi)
 *   - Akan mendapat CPU time terlebih dahulu
 *   - Waktu eksekusi terukur menjadi lebih singkat (tanpa preemption)
 */
static void vTaskLow(void *pvParameters) {
    (void)pvParameters;

    printf("[%s] Task dimulai - Prioritas: %d\r\n", TASK_LOW_NAME, TASK_LOW_PRIORITY);

    for (;;) {
        /* Lakukan CPU-intensive work */
        vDoWork(TASK_LOW_NAME, &xTaskLowStats, 
                TASK_LOW_LED_PORT, TASK_LOW_LED_PIN);

        /* Catat urutan eksekusi */
        vRecordExecOrder('L');

        /* Delay sebelum iterasi berikutnya */
        vTaskDelay(pdMS_TO_TICKS(TASK_WORK_PERIOD_MS));
    }
}

/**
 * Task Medium Priority (Prioritas 3)
 * 
 * Task ini berada di tengah hierarki prioritas.
 * Dalam mode preemptive:
 *   - Akan di-preempt oleh Task High
 *   - Akan mem-preempt Task Low
 *   - Mendapat CPU time setelah Task High selesai/blocked
 */
static void vTaskMed(void *pvParameters) {
    (void)pvParameters;

    printf("[%s] Task dimulai - Prioritas: %d\r\n", TASK_MED_NAME, TASK_MED_PRIORITY);

    for (;;) {
        vDoWork(TASK_MED_NAME, &xTaskMedStats,
                TASK_MED_LED_PORT, TASK_MED_LED_PIN);

        vRecordExecOrder('M');

        vTaskDelay(pdMS_TO_TICKS(TASK_WORK_PERIOD_MS));
    }
}

/**
 * Task High Priority (Prioritas 5)
 * 
 * Task ini memiliki prioritas tertinggi di antara worker tasks.
 * Dalam mode preemptive:
 *   - Tidak pernah di-preempt oleh worker task lain
 *   - Selalu mendapat CPU terlebih dahulu saat ready
 *   - Waktu eksekusi terukur paling konsisten (tanpa interupsi)
 */
static void vTaskHigh(void *pvParameters) {
    (void)pvParameters;

    printf("[%s] Task dimulai - Prioritas: %d\r\n", TASK_HIGH_NAME, TASK_HIGH_PRIORITY);

    for (;;) {
        vDoWork(TASK_HIGH_NAME, &xTaskHighStats,
                TASK_HIGH_LED_PORT, TASK_HIGH_LED_PIN);

        vRecordExecOrder('H');

        vTaskDelay(pdMS_TO_TICKS(TASK_WORK_PERIOD_MS));
    }
}

/**
 * Task Monitor: Mengelola fase dan mencetak statistik
 * 
 * Task ini bertanggung jawab untuk:
 *   1. Mengubah fase (Normal → Swap → Restore)
 *   2. Mengubah prioritas task saat pergantian fase
 *   3. Mencetak statistik eksekusi detail
 *   4. Mencetak urutan eksekusi untuk analisis
 * 
 * Prioritas: 6 (tertinggi, agar selalu bisa mengontrol fase)
 */
static void vTaskMonitor(void *pvParameters) {
    (void)pvParameters;

    TickType_t xPhaseStartTick = xTaskGetTickCount();
    uint32_t ulMonitorCycle = 0;

    printf("[%s] Task monitor dimulai - Prioritas: %d\r\n",
           MONITOR_TASK_NAME, MONITOR_PRIORITY);

    for (;;) {
        ulMonitorCycle++;
        TickType_t xCurrentTick = xTaskGetTickCount();
        uint32_t ulElapsedMs = (xCurrentTick - xPhaseStartTick) * 
                               (1000 / configTICK_RATE_HZ);

        /* ---- Cek apakah waktunya berganti fase ---- */
        if (ulElapsedMs >= PHASE_DURATION_MS) {
            xPhaseStartTick = xCurrentTick;

            /* Reset urutan eksekusi */
            ucExecOrderIdx = 0;
            cExecOrder[0] = '\0';

            /* Tentukan fase berikutnya */
            ucCurrentPhase++;
            if (ucCurrentPhase > TOTAL_PHASES) {
                ucCurrentPhase = 1;
            }

            printf("\r\n");
            vPrintSeparator('*', 65);

            switch (ucCurrentPhase) {
                case 1: {
                    /* Fase 1: Prioritas Normal */
                    printf(">>> FASE 1: Prioritas Normal (Low=1, Med=3, High=5)\r\n");

                    UBaseType_t uxOldLow  = uxTaskPriorityGet(xTaskLowHandle);
                    UBaseType_t uxOldHigh = uxTaskPriorityGet(xTaskHighHandle);

                    vTaskPrioritySet(xTaskLowHandle,  TASK_LOW_PRIORITY);
                    vTaskPrioritySet(xTaskMedHandle,   TASK_MED_PRIORITY);
                    vTaskPrioritySet(xTaskHighHandle,  TASK_HIGH_PRIORITY);

                    printf("[DATA]PRIORITY,%s,%lu,%d,%lu\r\n", TASK_LOW_NAME,
                           (unsigned long)uxOldLow, TASK_LOW_PRIORITY,
                           (unsigned long)xCurrentTick);
                    printf("[DATA]PRIORITY,%s,%lu,%d,%lu\r\n", TASK_HIGH_NAME,
                           (unsigned long)uxOldHigh, TASK_HIGH_PRIORITY,
                           (unsigned long)xCurrentTick);
                    printf("[DATA]PHASE,1,Normal,%lu\r\n", (unsigned long)xCurrentTick);
                    break;
                }

                case 2: {
                    /**
                     * Fase 2: Tukar Prioritas
                     * 
                     * vTaskPrioritySet() - Mengubah prioritas task saat runtime
                     * 
                     * Ketika prioritas task diubah:
                     *   - Jika prioritas baru > prioritas task yang sedang berjalan:
                     *     → Context switch terjadi segera (preemption)
                     *   - Jika prioritas baru < prioritas saat ini:
                     *     → Task tetap berjalan sampai yield/block/preempted
                     * 
                     * Di sini kita menukar prioritas Low dan High:
                     *   Low: 1 → 5 (menjadi tertinggi)
                     *   High: 5 → 1 (menjadi terendah)
                     */
                    printf(">>> FASE 2: Tukar Prioritas (Low=5, Med=3, High=1)\r\n");

                    UBaseType_t uxOldLow  = uxTaskPriorityGet(xTaskLowHandle);
                    UBaseType_t uxOldHigh = uxTaskPriorityGet(xTaskHighHandle);

                    /* Tukar prioritas Low dan High */
                    vTaskPrioritySet(xTaskLowHandle,  TASK_HIGH_PRIORITY);
                    vTaskPrioritySet(xTaskHighHandle,  TASK_LOW_PRIORITY);

                    printf("  %s: %lu → %d\r\n", TASK_LOW_NAME,
                           (unsigned long)uxOldLow, TASK_HIGH_PRIORITY);
                    printf("  %s: %lu → %d\r\n", TASK_HIGH_NAME,
                           (unsigned long)uxOldHigh, TASK_LOW_PRIORITY);

                    printf("[DATA]PRIORITY,%s,%lu,%d,%lu\r\n", TASK_LOW_NAME,
                           (unsigned long)uxOldLow, TASK_HIGH_PRIORITY,
                           (unsigned long)xCurrentTick);
                    printf("[DATA]PRIORITY,%s,%lu,%d,%lu\r\n", TASK_HIGH_NAME,
                           (unsigned long)uxOldHigh, TASK_LOW_PRIORITY,
                           (unsigned long)xCurrentTick);
                    printf("[DATA]PHASE,2,Swapped,%lu\r\n", (unsigned long)xCurrentTick);
                    break;
                }

                case 3: {
                    /* Fase 3: Kembalikan Prioritas */
                    printf(">>> FASE 3: Restore Prioritas (Low=1, Med=3, High=5)\r\n");

                    UBaseType_t uxOldLow  = uxTaskPriorityGet(xTaskLowHandle);
                    UBaseType_t uxOldHigh = uxTaskPriorityGet(xTaskHighHandle);

                    vTaskPrioritySet(xTaskLowHandle,  TASK_LOW_PRIORITY);
                    vTaskPrioritySet(xTaskMedHandle,   TASK_MED_PRIORITY);
                    vTaskPrioritySet(xTaskHighHandle,  TASK_HIGH_PRIORITY);

                    printf("[DATA]PRIORITY,%s,%lu,%d,%lu\r\n", TASK_LOW_NAME,
                           (unsigned long)uxOldLow, TASK_LOW_PRIORITY,
                           (unsigned long)xCurrentTick);
                    printf("[DATA]PRIORITY,%s,%lu,%d,%lu\r\n", TASK_HIGH_NAME,
                           (unsigned long)uxOldHigh, TASK_HIGH_PRIORITY,
                           (unsigned long)xCurrentTick);
                    printf("[DATA]PHASE,3,Restored,%lu\r\n", (unsigned long)xCurrentTick);
                    break;
                }
            }
            vPrintSeparator('*', 65);
        }

        /* ---- Cetak Statistik Periodik ---- */
        printf("\r\n");
        vPrintSeparator('=', 65);
        printf("  MONITOR SIKLUS #%lu | Fase: %u | Tick: %lu\r\n",
               (unsigned long)ulMonitorCycle, ucCurrentPhase,
               (unsigned long)xCurrentTick);
        vPrintSeparator('-', 65);

        /* Header tabel statistik */
        printf("  %-12s | Prio | Count | Last(us) | Min(us) | Max(us) | Stack\r\n",
               "Task");
        vPrintSeparator('-', 65);

        /* Statistik Task Low */
        printf("  %-12s | %4lu | %5lu | %8lu | %7lu | %7lu | %lu\r\n",
               TASK_LOW_NAME,
               (unsigned long)xTaskLowStats.uxCurrentPriority,
               (unsigned long)xTaskLowStats.ulExecutionCount,
               (unsigned long)xTaskLowStats.ulLastElapsedUs,
               (unsigned long)(xTaskLowStats.ulMinElapsedUs == 0xFFFFFFFF ? 0 : xTaskLowStats.ulMinElapsedUs),
               (unsigned long)xTaskLowStats.ulMaxElapsedUs,
               (unsigned long)uxTaskGetStackHighWaterMark(xTaskLowHandle));

        /* Statistik Task Medium */
        printf("  %-12s | %4lu | %5lu | %8lu | %7lu | %7lu | %lu\r\n",
               TASK_MED_NAME,
               (unsigned long)xTaskMedStats.uxCurrentPriority,
               (unsigned long)xTaskMedStats.ulExecutionCount,
               (unsigned long)xTaskMedStats.ulLastElapsedUs,
               (unsigned long)(xTaskMedStats.ulMinElapsedUs == 0xFFFFFFFF ? 0 : xTaskMedStats.ulMinElapsedUs),
               (unsigned long)xTaskMedStats.ulMaxElapsedUs,
               (unsigned long)uxTaskGetStackHighWaterMark(xTaskMedHandle));

        /* Statistik Task High */
        printf("  %-12s | %4lu | %5lu | %8lu | %7lu | %7lu | %lu\r\n",
               TASK_HIGH_NAME,
               (unsigned long)xTaskHighStats.uxCurrentPriority,
               (unsigned long)xTaskHighStats.ulExecutionCount,
               (unsigned long)xTaskHighStats.ulLastElapsedUs,
               (unsigned long)(xTaskHighStats.ulMinElapsedUs == 0xFFFFFFFF ? 0 : xTaskHighStats.ulMinElapsedUs),
               (unsigned long)xTaskHighStats.ulMaxElapsedUs,
               (unsigned long)uxTaskGetStackHighWaterMark(xTaskHighHandle));

        vPrintSeparator('-', 65);

        /* Cetak urutan eksekusi */
        printf("  Urutan Eksekusi: %s\r\n", (char*)cExecOrder);
        printf("[DATA]ORDER,%s,%lu\r\n", (char*)cExecOrder, (unsigned long)xCurrentTick);

        /* Cetak heap info */
        printf("  Heap Free: %lu bytes | Tasks: %lu\r\n",
               (unsigned long)xPortGetFreeHeapSize(),
               (unsigned long)uxTaskGetNumberOfTasks());
        printf("[DATA]HEAP,%lu,%lu\r\n",
               (unsigned long)xPortGetFreeHeapSize(),
               (unsigned long)xCurrentTick);

        /* Tabel vTaskList setiap 3 cycle */
        if (ulMonitorCycle % 3 == 0) {
            printf("\r\n  === vTaskList ===\r\n");
            char pcTaskListBuf[512];
            vTaskList(pcTaskListBuf);
            printf("%s", pcTaskListBuf);
        }

        vPrintSeparator('=', 65);

        /* Delay sampai cycle berikutnya */
        vTaskDelay(pdMS_TO_TICKS(MONITOR_PERIOD_MS));
    }
}

/* ============================ MAIN FUNCTION ============================== */

int main(void) {
    /* Inisialisasi Hardware */
    HAL_Init();
    SystemClock_Config();
    GPIO_Init();
    UART1_Init();
    DWT_Init();

    /* Banner startup */
    printf("\r\n\r\n");
    vPrintSeparator('*', 65);
    printf("*   STM32F103 FreeRTOS - Program 02: Task Priority            *\r\n");
    printf("*   Demonstrasi Preemptive Scheduling & Priority Management   *\r\n");
    vPrintSeparator('*', 65);
    printf("\r\n");

    printf("  [INFO] System Clock : %lu MHz\r\n",
           (unsigned long)(HAL_RCC_GetSysClockFreq() / 1000000));
    printf("  [INFO] CPU Work     : %lu iterasi per task\r\n",
           (unsigned long)CPU_WORK_ITERATIONS);
    printf("  [INFO] Phase Duration: %d ms\r\n", PHASE_DURATION_MS);
    printf("  [INFO] FreeRTOS Heap: %u bytes\r\n", (unsigned int)configTOTAL_HEAP_SIZE);
    printf("\r\n");

    printf("  [HEAP] Sebelum task: %lu bytes free\r\n\r\n",
           (unsigned long)xPortGetFreeHeapSize());

    /* Inisialisasi array urutan eksekusi */
    memset((void*)cExecOrder, 0, sizeof(cExecOrder));

    /* Buat task */
    BaseType_t xResult;

    xResult = xTaskCreate(vTaskLow, TASK_LOW_NAME, TASK_LOW_STACK,
                          NULL, TASK_LOW_PRIORITY, &xTaskLowHandle);
    printf("  [%s] Task '%s' (Prio %d): %s\r\n",
           xResult == pdPASS ? "OK" : "GAGAL", TASK_LOW_NAME, TASK_LOW_PRIORITY,
           xResult == pdPASS ? "Berhasil" : "Gagal");

    xResult = xTaskCreate(vTaskMed, TASK_MED_NAME, TASK_MED_STACK,
                          NULL, TASK_MED_PRIORITY, &xTaskMedHandle);
    printf("  [%s] Task '%s' (Prio %d): %s\r\n",
           xResult == pdPASS ? "OK" : "GAGAL", TASK_MED_NAME, TASK_MED_PRIORITY,
           xResult == pdPASS ? "Berhasil" : "Gagal");

    xResult = xTaskCreate(vTaskHigh, TASK_HIGH_NAME, TASK_HIGH_STACK,
                          NULL, TASK_HIGH_PRIORITY, &xTaskHighHandle);
    printf("  [%s] Task '%s' (Prio %d): %s\r\n",
           xResult == pdPASS ? "OK" : "GAGAL", TASK_HIGH_NAME, TASK_HIGH_PRIORITY,
           xResult == pdPASS ? "Berhasil" : "Gagal");

    xResult = xTaskCreate(vTaskMonitor, MONITOR_TASK_NAME, MONITOR_STACK_SIZE,
                          NULL, MONITOR_PRIORITY, &xMonitorHandle);
    printf("  [%s] Task '%s' (Prio %d): %s\r\n",
           xResult == pdPASS ? "OK" : "GAGAL", MONITOR_TASK_NAME, MONITOR_PRIORITY,
           xResult == pdPASS ? "Berhasil" : "Gagal");

    printf("\r\n  [HEAP] Setelah task: %lu bytes free\r\n",
           (unsigned long)xPortGetFreeHeapSize());
    printf("  [DATA]PHASE,1,Normal,%lu\r\n", (unsigned long)0UL);

    printf("\r\n  Memulai FreeRTOS Scheduler...\r\n");
    vPrintSeparator('=', 65);

    ucSchedulerStarted = 1;
    vTaskStartScheduler();

    /* Seharusnya tidak sampai sini */
    printf("\r\n[FATAL] Scheduler gagal!\r\n");
    for (;;) {
        HAL_GPIO_TogglePin(LED_BUILTIN_PORT, LED_BUILTIN_PIN);
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
