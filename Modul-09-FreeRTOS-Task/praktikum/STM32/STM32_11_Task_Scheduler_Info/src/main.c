/**
 * ============================================================================
 * STM32_11_Task_Scheduler_Info — Informasi Scheduler & Daftar Task FreeRTOS
 * ============================================================================
 *
 * Deskripsi:
 *   Program ini mendemonstrasikan cara mendapatkan informasi lengkap tentang
 *   task-task yang berjalan di FreeRTOS scheduler menggunakan API:
 *   - vTaskList()                : Daftar semua task dalam format tabel
 *   - uxTaskGetNumberOfTasks()   : Jumlah total task yang terdaftar
 *   - uxTaskGetSystemState()     : Informasi detail setiap task (TaskStatus_t)
 *   - eTaskGetState()            : Cek state individu task
 *
 * Fitur:
 *   - 3 worker task + 1 monitor task untuk demonstrasi
 *   - Monitor task mencetak tabel informasi task secara periodik
 *   - Pembuatan/penghapusan task temporer untuk menunjukkan perubahan tabel
 *   - Demonstrasi berbagai state task: Running, Ready, Blocked, Suspended
 *   - Suspend/resume task untuk menunjukkan perubahan state
 *   - Output [DATA] untuk parsing Python
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
#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

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

/* Konfigurasi ukuran stack task (dalam words, bukan bytes) */
#define WORKER_STACK_SIZE       256
#define MONITOR_STACK_SIZE      512
#define TEMP_STACK_SIZE         192

/* Konfigurasi prioritas task */
#define WORKER_PRIORITY         2
#define MONITOR_PRIORITY        3
#define TEMP_TASK_PRIORITY      1

/* Jumlah worker task */
#define NUM_WORKERS             3

/* Interval cetak informasi scheduler (ms) */
#define SCHEDULER_INFO_INTERVAL 5000

/* Jumlah maksimum task yang bisa dilacak */
#define MAX_TASKS               15

/* Ukuran buffer untuk vTaskList() */
#define TASK_LIST_BUFFER_SIZE   512

/* Fase demonstrasi */
#define PHASE_NORMAL            0
#define PHASE_SUSPEND_DEMO      1
#define PHASE_TEMP_TASK         2
#define PHASE_RESUME_ALL        3
#define NUM_PHASES              4

/* ===========================================================================
 * VARIABEL GLOBAL
 * =========================================================================== */

/* Handle UART */
static UART_HandleTypeDef huart1;

/* Handle task-task */
static TaskHandle_t xWorkerHandle[NUM_WORKERS];
static TaskHandle_t xMonitorHandle;
static TaskHandle_t xTempTaskHandle = NULL;

/* Mutex untuk akses UART (printf) */
static SemaphoreHandle_t xUartMutex;

/* Counter eksekusi per worker */
static volatile uint32_t ulWorkerCount[NUM_WORKERS];

/* Fase demonstrasi saat ini */
static volatile uint8_t ucCurrentPhase = PHASE_NORMAL;

/* Nomor siklus monitor */
static volatile uint32_t ulMonitorCycle = 0;

/* Flag untuk task temporer */
static volatile uint8_t ucTempTaskActive = 0;

/* Buffer untuk vTaskList */
static char pcTaskListBuffer[TASK_LIST_BUFFER_SIZE];

/* Nama state task dalam bahasa Indonesia */
static const char *pcStateNames[] = {
    "Running",      /* eRunning = 0 */
    "Ready",        /* eReady = 1 */
    "Blocked",      /* eBlocked = 2 */
    "Suspended",    /* eSuspended = 3 */
    "Deleted",      /* eDeleted = 4 */
    "Invalid"       /* eInvalid = 5 */
};

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
static void vTempTask(void *pvParameters);

/* Fungsi utilitas */
static void vPrintSchedulerInfo(void);
static void vPrintSystemState(void);
static void vPrintTaskStates(void);
static void vPrintDataTags(void);
static const char* pcGetStateName(eTaskState eState);
static void vSafePrintf(const char *fmt, ...);

/* ===========================================================================
 * RETARGET PRINTF KE UART1
 * =========================================================================== */

/**
 * @brief Redirect printf ke UART1 via HAL_UART_Transmit
 * @param file  : File descriptor (tidak digunakan)
 * @param ptr   : Pointer ke data yang akan dikirim
 * @param len   : Panjang data
 * @return Jumlah byte yang terkirim
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
 *        Memanggil xPortSysTickHandler() jika scheduler sudah berjalan
 */
void SysTick_Handler(void)
{
    HAL_IncTick();

    /* Panggil handler FreeRTOS hanya jika scheduler sudah dimulai */
    extern void xPortSysTickHandler(void);
    if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED)
    {
        xPortSysTickHandler();
    }
}

/* ===========================================================================
 * HOOK FUNCTIONS — Dipanggil otomatis oleh kernel FreeRTOS
 * =========================================================================== */

/**
 * @brief Hook saat alokasi memori (pvPortMalloc) gagal
 *        Menandakan heap FreeRTOS penuh
 */
void vApplicationMallocFailedHook(void)
{
    printf("[ERROR] Malloc gagal! Heap FreeRTOS penuh.\r\n");
    /* Kedipkan LED cepat sebagai indikator error */
    for (;;)
    {
        LED_TOGGLE();
        /* Delay sederhana tanpa FreeRTOS */
        for (volatile uint32_t i = 0; i < 200000; i++);
    }
}

/**
 * @brief Hook saat stack overflow terdeteksi pada suatu task
 * @param xTask       : Handle task yang mengalami overflow
 * @param pcTaskName  : Nama task yang bermasalah
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
 * Diperlukan karena configSUPPORT_STATIC_ALLOCATION = 1
 * =========================================================================== */

/* Buffer statis untuk Idle Task */
static StaticTask_t xIdleTaskTCB;
static StackType_t uxIdleTaskStack[configMINIMAL_STACK_SIZE];

/**
 * @brief Menyediakan memori statis untuk Idle Task
 */
void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer,
                                   StackType_t **ppxIdleTaskStackBuffer,
                                   uint32_t *pulIdleTaskStackSize)
{
    *ppxIdleTaskTCBBuffer   = &xIdleTaskTCB;
    *ppxIdleTaskStackBuffer = uxIdleTaskStack;
    *pulIdleTaskStackSize   = configMINIMAL_STACK_SIZE;
}

/* Buffer statis untuk Timer Task */
static StaticTask_t xTimerTaskTCB;
static StackType_t uxTimerTaskStack[configTIMER_TASK_STACK_DEPTH];

/**
 * @brief Menyediakan memori statis untuk Timer Task
 */
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
 * APB1 = 36MHz (max), APB2 = 72MHz
 * =========================================================================== */

/**
 * @brief Konfigurasi clock sistem STM32F103
 *        Menggunakan HSE 8MHz sebagai sumber clock eksternal,
 *        dikalikan PLL x9 menghasilkan 72MHz.
 */
static void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    /* Konfigurasi oscillator HSE dan PLL */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState       = RCC_HSE_ON;
    RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    RCC_OscInitStruct.PLL.PLLState   = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource  = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL     = RCC_PLL_MUL9; /* 8MHz x 9 = 72MHz */

    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        /* Gagal konfigurasi oscillator — loop error */
        for (;;);
    }

    /* Konfigurasi bus clock: HCLK=72MHz, APB1=36MHz, APB2=72MHz */
    RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                       RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;     /* HCLK = 72MHz */
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;       /* APB1 = 36MHz */
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;       /* APB2 = 72MHz */

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
    {
        for (;;);
    }
}

/* ===========================================================================
 * INISIALISASI GPIO
 * LED bawaan pada PC13 (active low)
 * =========================================================================== */

/**
 * @brief Inisialisasi GPIO untuk LED bawaan PC13
 *        LED Blue Pill bersifat active low (LOW = nyala, HIGH = mati)
 */
static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* Aktifkan clock GPIO port C */
    __HAL_RCC_GPIOC_CLK_ENABLE();

    /* Konfigurasi PC13 sebagai output push-pull untuk LED */
    GPIO_InitStruct.Pin   = LED_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_PORT, &GPIO_InitStruct);

    /* Matikan LED saat awal (set HIGH karena active low) */
    LED_OFF();
}

/* ===========================================================================
 * INISIALISASI USART1
 * PA9 (TX) dan PA10 (RX) @ 115200 baud
 * =========================================================================== */

/**
 * @brief Inisialisasi USART1 untuk komunikasi serial debug
 *        Digunakan untuk output printf ke terminal serial
 */
static void MX_USART1_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* Aktifkan clock untuk USART1 dan GPIOA */
    __HAL_RCC_USART1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    /* Konfigurasi PA9 sebagai TX (Alternate Function Push-Pull) */
    GPIO_InitStruct.Pin   = GPIO_PIN_9;
    GPIO_InitStruct.Mode  = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* Konfigurasi PA10 sebagai RX (Input Floating) */
    GPIO_InitStruct.Pin   = GPIO_PIN_10;
    GPIO_InitStruct.Mode  = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* Konfigurasi parameter USART1 */
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
 * @brief Printf yang aman dari race condition menggunakan mutex
 *        Hanya bisa dipakai setelah scheduler berjalan
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
        if (xSemaphoreTake(xUartMutex, pdMS_TO_TICKS(100)) == pdTRUE)
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
 * @brief Mendapatkan nama state task dalam bahasa Indonesia
 * @param eState : State task dari eTaskGetState()
 * @return Pointer ke string nama state
 */
static const char* pcGetStateName(eTaskState eState)
{
    if (eState <= eInvalid)
    {
        return pcStateNames[eState];
    }
    return "Unknown";
}

/**
 * @brief Konversi karakter state dari vTaskList ke nama lengkap
 * @param cState : Karakter state ('X', 'R', 'B', 'S', 'D')
 * @return Pointer ke string nama state
 */
static const char* pcStateCharToName(char cState)
{
    switch (cState)
    {
        case 'X': return "Running";
        case 'R': return "Ready";
        case 'B': return "Blocked";
        case 'S': return "Suspended";
        case 'D': return "Deleted";
        default:  return "Unknown";
    }
}

/* ===========================================================================
 * FUNGSI CETAK INFORMASI SCHEDULER
 * =========================================================================== */

/**
 * @brief Cetak daftar task menggunakan vTaskList()
 *        Menghasilkan tabel berformat: Nama | State | Prioritas | Stack | Nomor
 */
static void vPrintSchedulerInfo(void)
{
    UBaseType_t uxNumTasks;

    /* Dapatkan jumlah total task */
    uxNumTasks = uxTaskGetNumberOfTasks();

    vSafePrintf("\r\n");
    vSafePrintf("╔══════════════════════════════════════════════════════════════╗\r\n");
    vSafePrintf("║          INFORMASI SCHEDULER FreeRTOS — Siklus %-4lu         ║\r\n", ulMonitorCycle);
    vSafePrintf("╠══════════════════════════════════════════════════════════════╣\r\n");
    vSafePrintf("║ Total Task Terdaftar : %-3lu                                  ║\r\n", uxNumTasks);
    vSafePrintf("║ Free Heap            : %-5u bytes                          ║\r\n",
                (unsigned int)xPortGetFreeHeapSize());
    vSafePrintf("║ Min Free Heap        : %-5u bytes                          ║\r\n",
                (unsigned int)xPortGetMinimumEverFreeHeapSize());
    vSafePrintf("║ Fase Saat Ini        : %u (%s)                     ║\r\n",
                ucCurrentPhase,
                ucCurrentPhase == PHASE_NORMAL        ? "Normal    " :
                ucCurrentPhase == PHASE_SUSPEND_DEMO  ? "Suspend   " :
                ucCurrentPhase == PHASE_TEMP_TASK     ? "Temp Task " :
                                                        "Resume All");
    vSafePrintf("╠══════════════════════════════════════════════════════════════╣\r\n");

    /* Cetak header tabel */
    vSafePrintf("║ %-14s │ %-9s │ %-4s │ %-6s │ %-4s       ║\r\n",
                "Nama Task", "State", "Prio", "Stack", "No");
    vSafePrintf("╠══════════════════════════════════════════════════════════════╣\r\n");

    /* Gunakan vTaskList() untuk mendapatkan informasi tabel */
    memset(pcTaskListBuffer, 0, sizeof(pcTaskListBuffer));
    vTaskList(pcTaskListBuffer);

    /* Parse dan cetak setiap baris dari vTaskList output */
    /* Format vTaskList: "NamaTask\t\tState\tPriority\tStack\tTaskNumber\r\n" */
    char *pcLine = strtok(pcTaskListBuffer, "\r\n");
    while (pcLine != NULL)
    {
        char cName[configMAX_TASK_NAME_LEN];
        char cState;
        unsigned int uPriority, uStack, uNumber;

        /* Parse setiap baris — vTaskList menggunakan tab sebagai separator */
        int nParsed = sscanf(pcLine, "%15s %c %u %u %u",
                             cName, &cState, &uPriority, &uStack, &uNumber);

        if (nParsed >= 5)
        {
            vSafePrintf("║ %-14s │ %-9s │ %-4u │ %-6u │ %-4u       ║\r\n",
                        cName, pcStateCharToName(cState), uPriority, uStack, uNumber);
        }

        pcLine = strtok(NULL, "\r\n");
    }

    vSafePrintf("╚══════════════════════════════════════════════════════════════╝\r\n");
}

/**
 * @brief Cetak informasi detail menggunakan uxTaskGetSystemState()
 *        Memberikan data lebih lengkap termasuk runtime counter
 */
static void vPrintSystemState(void)
{
    TaskStatus_t xTaskStatusArray[MAX_TASKS];
    UBaseType_t uxArraySize;
    uint32_t ulTotalRunTime;

    /* Dapatkan informasi detail semua task */
    uxArraySize = uxTaskGetSystemState(xTaskStatusArray, MAX_TASKS, &ulTotalRunTime);

    if (uxArraySize == 0)
    {
        vSafePrintf("[WARN] Tidak ada data system state tersedia.\r\n");
        return;
    }

    vSafePrintf("\r\n");
    vSafePrintf("┌──────────────────────────────────────────────────────────────┐\r\n");
    vSafePrintf("│        DETAIL SYSTEM STATE (uxTaskGetSystemState)            │\r\n");
    vSafePrintf("├──────────────────────────────────────────────────────────────┤\r\n");
    vSafePrintf("│ %-14s │ %-9s │ %-8s │ %-4s │ %-6s │ %-4s │\r\n",
                "Nama", "State", "BasePrio", "Prio", "HWM", "No");
    vSafePrintf("├──────────────────────────────────────────────────────────────┤\r\n");

    /* Iterasi semua task yang ditemukan */
    for (UBaseType_t i = 0; i < uxArraySize; i++)
    {
        TaskStatus_t *pxTask = &xTaskStatusArray[i];

        vSafePrintf("│ %-14s │ %-9s │ %-8lu │ %-4lu │ %-6u │ %-4lu │\r\n",
                    pxTask->pcTaskName,
                    pcGetStateName(pxTask->eCurrentState),
                    (unsigned long)pxTask->uxBasePriority,
                    (unsigned long)pxTask->uxCurrentPriority,
                    (unsigned int)pxTask->usStackHighWaterMark,
                    (unsigned long)pxTask->xTaskNumber);
    }

    vSafePrintf("└──────────────────────────────────────────────────────────────┘\r\n");
}

/**
 * @brief Cetak state individual dari setiap worker task menggunakan eTaskGetState()
 *        Mendemonstrasikan cara cek state task satu per satu
 */
static void vPrintTaskStates(void)
{
    vSafePrintf("\r\n--- State Individual Task (eTaskGetState) ---\r\n");

    /* Cek state setiap worker task */
    for (int i = 0; i < NUM_WORKERS; i++)
    {
        if (xWorkerHandle[i] != NULL)
        {
            eTaskState eState = eTaskGetState(xWorkerHandle[i]);
            const char *pcName = pcTaskGetName(xWorkerHandle[i]);
            UBaseType_t uxHWM = uxTaskGetStackHighWaterMark(xWorkerHandle[i]);
            UBaseType_t uxPrio = uxTaskPriorityGet(xWorkerHandle[i]);

            vSafePrintf("  Worker-%d [%s]: State=%-9s | Prio=%lu | HWM=%lu | Count=%lu\r\n",
                        i, pcName, pcGetStateName(eState),
                        (unsigned long)uxPrio,
                        (unsigned long)uxHWM,
                        (unsigned long)ulWorkerCount[i]);
        }
    }

    /* Cek state task temporer jika ada */
    if (xTempTaskHandle != NULL && ucTempTaskActive)
    {
        eTaskState eState = eTaskGetState(xTempTaskHandle);
        vSafePrintf("  TempTask: State=%-9s\r\n", pcGetStateName(eState));
    }
    else
    {
        vSafePrintf("  TempTask: [tidak aktif]\r\n");
    }

    vSafePrintf("\r\n");
}

/**
 * @brief Cetak data tag untuk parsing Python
 *        Format: [DATA]TASK name=X,state=X,priority=X,stack=X,number=X
 *                [DATA]SYSTEM total_tasks=X,free_heap=X,min_heap=X
 */
static void vPrintDataTags(void)
{
    TaskStatus_t xTaskStatusArray[MAX_TASKS];
    UBaseType_t uxArraySize;
    uint32_t ulTotalRunTime;

    /* Dapatkan system state untuk data tag */
    uxArraySize = uxTaskGetSystemState(xTaskStatusArray, MAX_TASKS, &ulTotalRunTime);

    /* Cetak DATA tag untuk setiap task */
    for (UBaseType_t i = 0; i < uxArraySize; i++)
    {
        TaskStatus_t *pxTask = &xTaskStatusArray[i];

        vSafePrintf("[DATA]TASK name=%s,state=%s,priority=%lu,stack=%u,number=%lu\r\n",
                    pxTask->pcTaskName,
                    pcGetStateName(pxTask->eCurrentState),
                    (unsigned long)pxTask->uxCurrentPriority,
                    (unsigned int)pxTask->usStackHighWaterMark,
                    (unsigned long)pxTask->xTaskNumber);
    }

    /* Cetak DATA tag sistem */
    vSafePrintf("[DATA]SYSTEM total_tasks=%lu,free_heap=%u,min_heap=%u\r\n",
                (unsigned long)uxTaskGetNumberOfTasks(),
                (unsigned int)xPortGetFreeHeapSize(),
                (unsigned int)xPortGetMinimumEverFreeHeapSize());
}

/* ===========================================================================
 * TASK FUNCTIONS
 * =========================================================================== */

/**
 * @brief Worker task — melakukan pekerjaan sederhana secara periodik
 *        Setiap worker memiliki pola delay berbeda untuk variasi state
 *
 * @param pvParameters : Nomor ID worker (0, 1, 2)
 */
static void vWorkerTask(void *pvParameters)
{
    uint32_t ulWorkerID = (uint32_t)pvParameters;
    TickType_t xLastWakeTime;
    uint32_t ulDelayMs;

    /* Setiap worker memiliki delay berbeda untuk variasi perilaku */
    switch (ulWorkerID)
    {
        case 0: ulDelayMs = 1000; break;   /* Worker-0: delay 1 detik */
        case 1: ulDelayMs = 1500; break;   /* Worker-1: delay 1.5 detik */
        case 2: ulDelayMs = 2000; break;   /* Worker-2: delay 2 detik */
        default: ulDelayMs = 1000; break;
    }

    xLastWakeTime = xTaskGetTickCount();

    vSafePrintf("[INFO] Worker-%lu dimulai (delay=%lums, prio=%lu)\r\n",
                ulWorkerID, ulDelayMs,
                (unsigned long)uxTaskPriorityGet(NULL));

    for (;;)
    {
        /* Simulasi pekerjaan: increment counter */
        ulWorkerCount[ulWorkerID]++;

        /* Simulasi proses komputasi singkat */
        volatile uint32_t ulDummy = 0;
        for (uint32_t i = 0; i < 10000; i++)
        {
            ulDummy += i;
        }

        /* Log periodik setiap 5 iterasi */
        if (ulWorkerCount[ulWorkerID] % 5 == 0)
        {
            vSafePrintf("[WORKER-%lu] Iterasi ke-%lu | HWM=%lu words\r\n",
                        ulWorkerID,
                        (unsigned long)ulWorkerCount[ulWorkerID],
                        (unsigned long)uxTaskGetStackHighWaterMark(NULL));
        }

        /* Delay periodik — task masuk state Blocked selama delay */
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(ulDelayMs));
    }
}

/**
 * @brief Task temporer yang dibuat dan dihapus untuk demonstrasi
 *        Menunjukkan perubahan tabel task saat task baru muncul/hilang
 *
 * @param pvParameters : tidak digunakan
 */
static void vTempTask(void *pvParameters)
{
    (void)pvParameters;

    vSafePrintf("[TEMP] Task temporer DIBUAT — akan aktif selama 8 detik\r\n");

    /* Task ini berjalan selama beberapa detik lalu menghapus diri sendiri */
    for (int i = 0; i < 8; i++)
    {
        vSafePrintf("[TEMP] Aktif... iterasi %d/8\r\n", i + 1);
        LED_TOGGLE();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    vSafePrintf("[TEMP] Task temporer akan DIHAPUS\r\n");

    /* Tandai tidak aktif sebelum delete */
    ucTempTaskActive = 0;
    xTempTaskHandle = NULL;

    /* Hapus diri sendiri — setelah ini task tidak ada lagi */
    vTaskDelete(NULL);
}

/**
 * @brief Monitor task — task utama yang mengumpulkan dan mencetak informasi scheduler
 *        Menjalankan berbagai fase demonstrasi secara berurutan
 *
 * Fase:
 *   0 (Normal)       : Cetak info scheduler dalam kondisi normal
 *   1 (Suspend Demo) : Suspend Worker-1, tunjukkan perubahan state
 *   2 (Temp Task)    : Buat task temporer, tunjukkan perubahan tabel
 *   3 (Resume All)   : Resume semua task, kembali ke kondisi normal
 *
 * @param pvParameters : tidak digunakan
 */
static void vMonitorTask(void *pvParameters)
{
    (void)pvParameters;
    TickType_t xLastWakeTime;

    /* Tunggu semua worker task siap */
    vTaskDelay(pdMS_TO_TICKS(2000));

    printf("\r\n");
    printf("============================================================\r\n");
    printf("   STM32_11 — Task Scheduler Info Monitor Dimulai\r\n");
    printf("   Monitoring %d worker task + task sistem\r\n", NUM_WORKERS);
    printf("   Interval laporan: %d ms\r\n", SCHEDULER_INFO_INTERVAL);
    printf("============================================================\r\n");

    xLastWakeTime = xTaskGetTickCount();

    for (;;)
    {
        ulMonitorCycle++;

        /* ──────────────────────────────────────────────────────────── */
        /* Tentukan fase berdasarkan siklus monitor */
        /* ──────────────────────────────────────────────────────────── */
        uint32_t ulPhaseIndex = ((ulMonitorCycle - 1) / 3) % NUM_PHASES;
        ucCurrentPhase = (uint8_t)ulPhaseIndex;

        /* ──────────────────────────────────────────────────────────── */
        /* Aksi sesuai fase */
        /* ──────────────────────────────────────────────────────────── */
        switch (ucCurrentPhase)
        {
            case PHASE_NORMAL:
                /* Fase normal — tidak ada aksi khusus */
                vSafePrintf("\r\n>>> FASE 0: Kondisi Normal — Semua task berjalan <<<\r\n");
                LED_OFF();
                break;

            case PHASE_SUSPEND_DEMO:
                /* Fase suspend — suspend Worker-1 untuk tunjukkan perubahan state */
                vSafePrintf("\r\n>>> FASE 1: Suspend Demo — Worker-1 di-suspend <<<\r\n");
                if (xWorkerHandle[1] != NULL)
                {
                    vTaskSuspend(xWorkerHandle[1]);
                    vSafePrintf("[MONITOR] Worker-1 DISUSPEND — cek state di tabel\r\n");
                }
                LED_ON();
                break;

            case PHASE_TEMP_TASK:
                /* Fase temp task — buat task baru sementara */
                vSafePrintf("\r\n>>> FASE 2: Temp Task — Buat task temporer <<<\r\n");

                /* Resume Worker-1 jika masih suspended */
                if (xWorkerHandle[1] != NULL)
                {
                    eTaskState eState = eTaskGetState(xWorkerHandle[1]);
                    if (eState == eSuspended)
                    {
                        vTaskResume(xWorkerHandle[1]);
                        vSafePrintf("[MONITOR] Worker-1 DIRESUME kembali\r\n");
                    }
                }

                /* Buat task temporer jika belum ada */
                if (!ucTempTaskActive && xTempTaskHandle == NULL)
                {
                    BaseType_t xResult = xTaskCreate(
                        vTempTask,
                        "TempTask",
                        TEMP_STACK_SIZE,
                        NULL,
                        TEMP_TASK_PRIORITY,
                        &xTempTaskHandle
                    );

                    if (xResult == pdPASS)
                    {
                        ucTempTaskActive = 1;
                        vSafePrintf("[MONITOR] Task temporer DIBUAT berhasil\r\n");
                    }
                    else
                    {
                        vSafePrintf("[ERROR] Gagal membuat task temporer!\r\n");
                    }
                }
                break;

            case PHASE_RESUME_ALL:
                /* Fase resume — pastikan semua task kembali normal */
                vSafePrintf("\r\n>>> FASE 3: Resume All — Kembalikan ke kondisi normal <<<\r\n");

                /* Pastikan semua worker berjalan */
                for (int i = 0; i < NUM_WORKERS; i++)
                {
                    if (xWorkerHandle[i] != NULL)
                    {
                        eTaskState eState = eTaskGetState(xWorkerHandle[i]);
                        if (eState == eSuspended)
                        {
                            vTaskResume(xWorkerHandle[i]);
                            vSafePrintf("[MONITOR] Worker-%d DIRESUME\r\n", i);
                        }
                    }
                }
                LED_OFF();
                break;

            default:
                break;
        }

        /* ──────────────────────────────────────────────────────────── */
        /* Cetak semua informasi scheduler */
        /* ──────────────────────────────────────────────────────────── */

        /* 1. Tabel dari vTaskList() */
        vPrintSchedulerInfo();

        /* 2. Detail dari uxTaskGetSystemState() */
        vPrintSystemState();

        /* 3. State individual via eTaskGetState() */
        vPrintTaskStates();

        /* 4. Data tags untuk Python parsing */
        vPrintDataTags();

        /* ──────────────────────────────────────────────────────────── */
        /* Ringkasan counter worker */
        /* ──────────────────────────────────────────────────────────── */
        vSafePrintf("\r\n--- Ringkasan Counter Worker ---\r\n");
        for (int i = 0; i < NUM_WORKERS; i++)
        {
            vSafePrintf("  Worker-%d: %lu iterasi\r\n", i, (unsigned long)ulWorkerCount[i]);
        }

        /* Toggle LED per siklus sebagai heartbeat */
        LED_TOGGLE();

        /* Tunggu interval berikutnya */
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(SCHEDULER_INFO_INTERVAL));
    }
}

/* ===========================================================================
 * FUNGSI UTAMA — Entry Point Program
 * =========================================================================== */

/**
 * @brief Fungsi utama program
 *        Inisialisasi hardware → buat task → mulai scheduler
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
    printf("  STM32_11_Task_Scheduler_Info\r\n");
    printf("  Informasi Scheduler & Daftar Task FreeRTOS\r\n");
    printf("============================================================\r\n");
    printf("  MCU     : STM32F103C8T6 (Blue Pill)\r\n");
    printf("  Clock   : 72 MHz (HSE 8MHz x PLL9)\r\n");
    printf("  UART    : 115200 baud (PA9-TX, PA10-RX)\r\n");
    printf("  LED     : PC13 (active low)\r\n");
    printf("  Heap    : %u bytes\r\n", (unsigned int)configTOTAL_HEAP_SIZE);
    printf("  Workers : %d task\r\n", NUM_WORKERS);
    printf("============================================================\r\n");
    printf("\r\n");

    printf("[INIT] Membuat mutex UART...\r\n");
    xUartMutex = xSemaphoreCreateMutex();
    if (xUartMutex == NULL)
    {
        printf("[ERROR] Gagal membuat mutex UART!\r\n");
        for (;;);
    }

    /* ── Buat Worker Tasks ── */
    printf("[INIT] Membuat %d worker task...\r\n", NUM_WORKERS);

    /* Inisialisasi counter */
    for (int i = 0; i < NUM_WORKERS; i++)
    {
        ulWorkerCount[i] = 0;
    }

    /* Buat worker task dengan ID berbeda */
    char pcWorkerName[configMAX_TASK_NAME_LEN];
    for (int i = 0; i < NUM_WORKERS; i++)
    {
        snprintf(pcWorkerName, sizeof(pcWorkerName), "Worker-%d", i);

        BaseType_t xResult = xTaskCreate(
            vWorkerTask,
            pcWorkerName,
            WORKER_STACK_SIZE,
            (void *)(uint32_t)i,  /* Pass worker ID */
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

    /* ── Buat Monitor Task ── */
    printf("[INIT] Membuat monitor task...\r\n");

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

    /* ── Tampilkan ringkasan sebelum start ── */
    printf("\r\n");
    printf("[INIT] Ringkasan task yang dibuat:\r\n");
    printf("  - Worker-0: Prio=%d, Delay=1000ms\r\n", WORKER_PRIORITY);
    printf("  - Worker-1: Prio=%d, Delay=1500ms\r\n", WORKER_PRIORITY);
    printf("  - Worker-2: Prio=%d, Delay=2000ms\r\n", WORKER_PRIORITY);
    printf("  - Monitor : Prio=%d, Interval=%dms\r\n", MONITOR_PRIORITY, SCHEDULER_INFO_INTERVAL);
    printf("  + Idle Task (sistem)\r\n");
    printf("  + Timer Task (sistem)\r\n");
    printf("\r\n");
    printf("[INIT] Fase demonstrasi:\r\n");
    printf("  Fase 0: Normal — semua task aktif\r\n");
    printf("  Fase 1: Suspend Demo — Worker-1 di-suspend\r\n");
    printf("  Fase 2: Temp Task — buat/hapus task temporer\r\n");
    printf("  Fase 3: Resume All — kembalikan semua\r\n");
    printf("\r\n");

    printf("[INIT] Free heap sebelum scheduler: %u bytes\r\n",
           (unsigned int)xPortGetFreeHeapSize());
    printf("[INIT] Memulai FreeRTOS scheduler...\r\n");
    printf("============================================================\r\n\r\n");

    /* ── Mulai FreeRTOS Scheduler ── */
    vTaskStartScheduler();

    /* Seharusnya tidak pernah sampai sini */
    printf("[FATAL] Scheduler berhenti! Heap tidak cukup?\r\n");
    for (;;)
    {
        LED_TOGGLE();
        for (volatile uint32_t i = 0; i < 500000; i++);
    }

    return 0;
}
