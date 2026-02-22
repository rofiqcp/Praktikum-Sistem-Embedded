/* =========================================================================
 * STM32_09_Task_Watchdog
 * =========================================================================
 * Deskripsi  : Implementasi IWDG (Independent Watchdog) dengan FreeRTOS
 *              Mendemonstrasikan cara menggunakan hardware watchdog
 *              dalam sistem multi-task untuk mendeteksi task yang hang.
 *
 * Konsep     : IWDG menggunakan oscillator LSI (~40kHz) yang independen
 *              dari system clock. Jika tidak di-refresh dalam timeout
 *              yang ditentukan, MCU akan di-reset secara hardware.
 *
 * Konfigurasi IWDG:
 *   LSI    = ~40 kHz
 *   Prescaler = /256 -> counter clock = 40000/256 ≈ 156.25 Hz
 *   Reload = 625 -> timeout ≈ 625/156.25 = 4 detik
 *
 * Task       : 1. WatchdogFeeder - Secara periodik me-refresh IWDG
 *              2. NormalWorker   - Task kerja normal
 *              3. ProblematicTask- Task yang sengaja hang setelah N siklus
 *              4. MonitorTask    - Memantau status dan feed count
 *
 * Target     : STM32F103C8 (Blue Pill) - Cortex-M3 @ 72MHz
 * Framework  : STM32Cube HAL + FreeRTOS
 * ========================================================================= */

/* -- Header Files -------------------------------------------------------- */
#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

/* -- Definisi Konstanta -------------------------------------------------- */
/* IWDG Configuration */
#define IWDG_PRESCALER          IWDG_PRESCALER_256
#define IWDG_RELOAD_VALUE       625     /* ~4 detik timeout */
#define IWDG_TIMEOUT_MS         4000    /* Estimasi timeout dalam ms */
#define IWDG_FEED_INTERVAL_MS   1000    /* Refresh setiap 1 detik */

/* Task priorities */
#define FEEDER_TASK_PRIORITY    4
#define WORKER_TASK_PRIORITY    2
#define PROBLEM_TASK_PRIORITY   2
#define MONITOR_TASK_PRIORITY   3

/* Task stack sizes (words) */
#define FEEDER_STACK_SIZE       192
#define WORKER_STACK_SIZE       256
#define PROBLEM_STACK_SIZE      256
#define MONITOR_STACK_SIZE      384

/* Jumlah siklus sebelum task problematic hang */
#define PROBLEM_HANG_AFTER      15      /* Hang setelah 15 siklus */
#define PROBLEM_TASK_DELAY_MS   1500

/* Jumlah siklus sebelum feeder berhenti (simulasi kegagalan) */
#define FEEDER_STOP_AFTER       30      /* Berhenti feed setelah 30 siklus */

/* LED dan Button */
#define LED_PORT                GPIOC
#define LED_PIN                 GPIO_PIN_13
#define BUTTON_PORT             GPIOA
#define BUTTON_PIN              GPIO_PIN_0

/* -- Handle Peripheral --------------------------------------------------- */
static UART_HandleTypeDef huart1;
static IWDG_HandleTypeDef hiwdg;

/* -- Handle Task --------------------------------------------------------- */
static TaskHandle_t xFeederTaskHandle   = NULL;
static TaskHandle_t xWorkerTaskHandle   = NULL;
static TaskHandle_t xProblemTaskHandle  = NULL;
static TaskHandle_t xMonitorTaskHandle  = NULL;

/* -- Variabel Global ----------------------------------------------------- */
static volatile uint32_t ulSystemTick       = 0;
static volatile uint32_t ulFeedCount        = 0;
static volatile uint32_t ulWorkerCounter    = 0;
static volatile uint32_t ulProblemCounter   = 0;
static volatile uint32_t ulMonitorCycle     = 0;

/* Status flags */
static volatile uint8_t  ucFeederActive     = 1;   /* 1=aktif, 0=berhenti */
static volatile uint8_t  ucProblemHanging   = 0;   /* 1=task hang */
static volatile uint8_t  ucIWDGResetDetected = 0;  /* 1=reset oleh IWDG */
static volatile uint32_t ulExpectedResetTick = 0;

/* Alasan reset */
typedef enum {
    RESET_POWER_ON    = 0,
    RESET_IWDG        = 1,
    RESET_SOFTWARE    = 2,
    RESET_PIN         = 3,
    RESET_UNKNOWN     = 4
} ResetReason_t;

static ResetReason_t eResetReason = RESET_UNKNOWN;

/* Statistik per-boot */
static volatile uint32_t ulBootCount = 0;    /* Disimpan di backup register */
static volatile uint32_t ulLastFeedTick = 0;

/* -- Static memory untuk Idle dan Timer task ----------------------------- */
static StaticTask_t xIdleTaskTCB;
static StackType_t  uxIdleTaskStack[configMINIMAL_STACK_SIZE];

static StaticTask_t xTimerTaskTCB;
static StackType_t  uxTimerTaskStack[configTIMER_TASK_STACK_DEPTH];

/* -- Prototipe Fungsi ---------------------------------------------------- */
static void SystemClock_Config(void);
static void GPIO_Init(void);
static void UART1_Init(void);
static void IWDG_Init(void);
static void Error_Handler(void);
static ResetReason_t eCheckResetReason(void);
static void vPrintResetInfo(ResetReason_t eReason);

static void vWatchdogFeederTask(void *pvParameters);
static void vNormalWorkerTask(void *pvParameters);
static void vProblematicTask(void *pvParameters);
static void vMonitorTask(void *pvParameters);

static void vPrintBanner(void);
static void vPrintIWDGConfig(void);
static void vPrintCountdown(uint32_t ulRemainingMs);

/* -- Implementasi _write untuk printf via UART --------------------------- */
int _write(int file, char *ptr, int len)
{
    HAL_UART_Transmit(&huart1, (uint8_t *)ptr, len, HAL_MAX_DELAY);
    return len;
}

/* =========================================================================
 * SysTick Handler
 * ========================================================================= */
void SysTick_Handler(void)
{
    HAL_IncTick();
    ulSystemTick++;

    if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED)
    {
        xPortSysTickHandler();
    }
}

/* =========================================================================
 * Callback hooks
 * ========================================================================= */
void vApplicationMallocFailedHook(void)
{
    printf("[DATA] MALLOC_FAIL,tick=%lu\r\n", (unsigned long)ulSystemTick);
    for (;;)
    {
        HAL_GPIO_TogglePin(LED_PORT, LED_PIN);
        HAL_Delay(100);
    }
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    printf("[DATA] STACK_OVERFLOW,task=%s,tick=%lu\r\n",
           pcTaskName, (unsigned long)ulSystemTick);
    for (;;)
    {
        HAL_GPIO_TogglePin(LED_PORT, LED_PIN);
        HAL_Delay(200);
    }
}

void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer,
                                   StackType_t **ppxIdleTaskStackBuffer,
                                   uint32_t *pulIdleTaskStackSize)
{
    *ppxIdleTaskTCBBuffer   = &xIdleTaskTCB;
    *ppxIdleTaskStackBuffer = uxIdleTaskStack;
    *pulIdleTaskStackSize   = configMINIMAL_STACK_SIZE;
}

void vApplicationGetTimerTaskMemory(StaticTask_t **ppxTimerTaskTCBBuffer,
                                    StackType_t **ppxTimerTaskStackBuffer,
                                    uint32_t *pulTimerTaskStackSize)
{
    *ppxTimerTaskTCBBuffer   = &xTimerTaskTCB;
    *ppxTimerTaskStackBuffer = uxTimerTaskStack;
    *pulTimerTaskStackSize   = configTIMER_TASK_STACK_DEPTH;
}

/* =========================================================================
 * Konfigurasi System Clock: HSE 8MHz -> PLL x9 -> 72MHz
 * ========================================================================= */
static void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    /* HSE + LSI (untuk IWDG) */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE
                                     | RCC_OSCILLATORTYPE_LSI;
    RCC_OscInitStruct.HSEState       = RCC_HSE_ON;
    RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    RCC_OscInitStruct.LSIState       = RCC_LSI_ON;    /* LSI untuk IWDG */
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

/* =========================================================================
 * Inisialisasi GPIO
 * ========================================================================= */
static void GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    /* LED PC13 */
    GPIO_InitStruct.Pin   = LED_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_PORT, &GPIO_InitStruct);
    HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET); /* Mati */

    /* Button PA0 */
    GPIO_InitStruct.Pin  = BUTTON_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;
    HAL_GPIO_Init(BUTTON_PORT, &GPIO_InitStruct);
}

/* =========================================================================
 * Inisialisasi UART1: PA9=TX, PA10=RX, 115200 8N1
 * ========================================================================= */
static void UART1_Init(void)
{
    __HAL_RCC_USART1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Pin   = GPIO_PIN_9;
    GPIO_InitStruct.Mode  = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin  = GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    huart1.Instance          = USART1;
    huart1.Init.BaudRate     = 115200;
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

/* =========================================================================
 * Inisialisasi IWDG (Independent Watchdog)
 * LSI ~40kHz, Prescaler /256, Reload 625 -> Timeout ~4 detik
 * ========================================================================= */
static void IWDG_Init(void)
{
    hiwdg.Instance       = IWDG;
    hiwdg.Init.Prescaler = IWDG_PRESCALER;
    hiwdg.Init.Reload    = IWDG_RELOAD_VALUE;

    if (HAL_IWDG_Init(&hiwdg) != HAL_OK)
    {
        printf("[ERROR] Gagal menginisialisasi IWDG!\r\n");
        Error_Handler();
    }

    printf("[IWDG] Watchdog berhasil diinisialisasi\r\n");
}

/* =========================================================================
 * Cek alasan reset terakhir
 * Membaca flag di register RCC
 * ========================================================================= */
static ResetReason_t eCheckResetReason(void)
{
    ResetReason_t eReason = RESET_UNKNOWN;

    if (__HAL_RCC_GET_FLAG(RCC_FLAG_IWDGRST))
    {
        eReason = RESET_IWDG;
    }
    else if (__HAL_RCC_GET_FLAG(RCC_FLAG_SFTRST))
    {
        eReason = RESET_SOFTWARE;
    }
    else if (__HAL_RCC_GET_FLAG(RCC_FLAG_PINRST))
    {
        eReason = RESET_PIN;
    }
    else if (__HAL_RCC_GET_FLAG(RCC_FLAG_PORRST))
    {
        eReason = RESET_POWER_ON;
    }

    /* Bersihkan semua flag reset */
    __HAL_RCC_CLEAR_RESET_FLAGS();

    return eReason;
}

/* =========================================================================
 * Cetak informasi reset
 * ========================================================================= */
static void vPrintResetInfo(ResetReason_t eReason)
{
    printf("\r\n");
    printf("=========================================================\r\n");
    printf(" INFORMASI RESET\r\n");
    printf("=========================================================\r\n");

    switch (eReason)
    {
        case RESET_POWER_ON:
            printf(" Alasan: POWER-ON RESET (boot pertama)\r\n");
            printf("[DATA] RESET_REASON,type=POWER_ON\r\n");
            break;

        case RESET_IWDG:
            printf(" Alasan: IWDG WATCHDOG RESET!\r\n");
            printf(" >>> Watchdog tidak di-refresh tepat waktu! <<<\r\n");
            printf(" >>> Sistem di-reset otomatis oleh hardware <<<\r\n");
            printf("[DATA] RESET_REASON,type=IWDG_WATCHDOG\r\n");
            ucIWDGResetDetected = 1;
            break;

        case RESET_SOFTWARE:
            printf(" Alasan: SOFTWARE RESET\r\n");
            printf("[DATA] RESET_REASON,type=SOFTWARE\r\n");
            break;

        case RESET_PIN:
            printf(" Alasan: PIN RESET (tombol reset ditekan)\r\n");
            printf("[DATA] RESET_REASON,type=PIN\r\n");
            break;

        default:
            printf(" Alasan: TIDAK DIKETAHUI\r\n");
            printf("[DATA] RESET_REASON,type=UNKNOWN\r\n");
            break;
    }

    printf("=========================================================\r\n");
    printf("\r\n");
}

/* =========================================================================
 * Error Handler
 * ========================================================================= */
static void Error_Handler(void)
{
    __disable_irq();
    while (1)
    {
        HAL_GPIO_TogglePin(LED_PORT, LED_PIN);
        for (volatile uint32_t i = 0; i < 500000; i++);
    }
}

/* =========================================================================
 * Banner informasi program
 * ========================================================================= */
static void vPrintBanner(void)
{
    printf("\r\n");
    printf("=========================================================\r\n");
    printf(" STM32 FreeRTOS - Task Watchdog (IWDG)\r\n");
    printf(" Program 09: Independent Watchdog dengan Multi-Task\r\n");
    printf("=========================================================\r\n");
    printf(" Target : STM32F103C8 (Blue Pill) @ 72MHz\r\n");
    printf(" RTOS   : FreeRTOS (Preemptive Scheduler)\r\n");
    printf(" IWDG   : Independent Watchdog Timer\r\n");
    printf("---------------------------------------------------------\r\n");
    printf(" Konfigurasi Task:\r\n");
    printf("   FeederTask  : Prioritas %d - Me-refresh IWDG\r\n",
           FEEDER_TASK_PRIORITY);
    printf("   WorkerTask  : Prioritas %d - Kerja normal\r\n",
           WORKER_TASK_PRIORITY);
    printf("   ProblemTask : Prioritas %d - Akan hang\r\n",
           PROBLEM_TASK_PRIORITY);
    printf("   MonitorTask : Prioritas %d - Monitoring\r\n",
           MONITOR_TASK_PRIORITY);
    printf("---------------------------------------------------------\r\n");
    printf(" Skenario:\r\n");
    printf("   1. Semua task berjalan normal\r\n");
    printf("   2. FeederTask me-refresh IWDG setiap %d ms\r\n",
           IWDG_FEED_INTERVAL_MS);
    printf("   3. ProblemTask hang setelah %d siklus\r\n", PROBLEM_HANG_AFTER);
    printf("   4. FeederTask berhenti setelah %d siklus\r\n", FEEDER_STOP_AFTER);
    printf("   5. IWDG tidak di-refresh -> SYSTEM RESET!\r\n");
    printf("   6. Setelah reset, deteksi alasan via RCC flags\r\n");
    printf("=========================================================\r\n");
    printf("[DATA] INIT,feed_interval=%d,timeout=%d,hang_after=%d,stop_after=%d\r\n",
           IWDG_FEED_INTERVAL_MS, IWDG_TIMEOUT_MS,
           PROBLEM_HANG_AFTER, FEEDER_STOP_AFTER);
    printf("\r\n");
}

/* =========================================================================
 * Cetak konfigurasi IWDG
 * ========================================================================= */
static void vPrintIWDGConfig(void)
{
    printf("\r\n");
    printf("----- Konfigurasi IWDG -----\r\n");
    printf("  LSI Clock    : ~40 kHz (internal RC)\r\n");
    printf("  Prescaler    : /256\r\n");
    printf("  Counter Clock: ~156.25 Hz\r\n");
    printf("  Reload Value : %d\r\n", IWDG_RELOAD_VALUE);
    printf("  Timeout      : ~%.1f detik\r\n",
           (float)IWDG_RELOAD_VALUE / (40000.0f / 256.0f));
    printf("  Feed Interval: %d ms\r\n", IWDG_FEED_INTERVAL_MS);
    printf("  Safety Margin: ~%.1f detik\r\n",
           ((float)IWDG_RELOAD_VALUE / (40000.0f / 256.0f))
           - ((float)IWDG_FEED_INTERVAL_MS / 1000.0f));
    printf("\r\n");
    printf("  CATATAN: LSI tidak akurat (30-60 kHz)\r\n");
    printf("  Timeout sebenarnya bisa bervariasi!\r\n");
    printf("----------------------------\r\n");
    printf("[DATA] IWDG_CONFIG,prescaler=256,reload=%d,timeout_ms=%d\r\n",
           IWDG_RELOAD_VALUE, IWDG_TIMEOUT_MS);
    printf("\r\n");
}

/* =========================================================================
 * Cetak countdown menuju reset yang diharapkan
 * ========================================================================= */
static void vPrintCountdown(uint32_t ulRemainingMs)
{
    printf("  >>> COUNTDOWN: ~%lu.%lu detik sampai IWDG RESET <<<\r\n",
           (unsigned long)(ulRemainingMs / 1000),
           (unsigned long)((ulRemainingMs % 1000) / 100));
    printf("[DATA] COUNTDOWN,remaining_ms=%lu,tick=%lu\r\n",
           (unsigned long)ulRemainingMs,
           (unsigned long)xTaskGetTickCount());
}

/* =========================================================================
 * Watchdog Feeder Task
 * Secara periodik memanggil HAL_IWDG_Refresh() untuk me-reset counter IWDG
 * Berhenti setelah FEEDER_STOP_AFTER siklus untuk mendemonstrasikan reset
 * ========================================================================= */
static void vWatchdogFeederTask(void *pvParameters)
{
    (void)pvParameters;
    TickType_t xLastWakeTime = xTaskGetTickCount();

    printf("[FeederTask] Dimulai - Interval: %d ms\r\n", IWDG_FEED_INTERVAL_MS);
    printf("[FeederTask] Akan berhenti setelah %d siklus\r\n", FEEDER_STOP_AFTER);

    for (;;)
    {
        if (ucFeederActive)
        {
            ulFeedCount++;
            ulLastFeedTick = xTaskGetTickCount();

            /* Refresh IWDG - reset counter ke nilai reload */
            HAL_IWDG_Refresh(&hiwdg);

            /* LED toggle sebagai heartbeat */
            HAL_GPIO_TogglePin(LED_PORT, LED_PIN);

            printf("[FeederTask] IWDG refresh #%lu (tick=%lu)\r\n",
                   (unsigned long)ulFeedCount,
                   (unsigned long)ulLastFeedTick);

            printf("[DATA] FEED,count=%lu,tick=%lu,active=1\r\n",
                   (unsigned long)ulFeedCount,
                   (unsigned long)ulLastFeedTick);

            /* Cek apakah sudah waktunya berhenti */
            if (ulFeedCount >= FEEDER_STOP_AFTER)
            {
                ucFeederActive = 0;
                ulExpectedResetTick = xTaskGetTickCount() + IWDG_TIMEOUT_MS;

                printf("\r\n");
                printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\r\n");
                printf("!! FEEDER BERHENTI!                    !!\r\n");
                printf("!! IWDG tidak akan di-refresh lagi!    !!\r\n");
                printf("!! System reset dalam ~%d detik!       !!\r\n",
                       IWDG_TIMEOUT_MS / 1000);
                printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\r\n");
                printf("[DATA] FEEDER_STOPPED,feed_count=%lu,tick=%lu\r\n",
                       (unsigned long)ulFeedCount,
                       (unsigned long)xTaskGetTickCount());
                printf("[DATA] EXPECTED_RESET,tick=%lu\r\n",
                       (unsigned long)ulExpectedResetTick);
            }
        }
        else
        {
            /* Feeder tidak aktif - hitung mundur ke reset */
            uint32_t ulNow = xTaskGetTickCount();
            uint32_t ulSinceLastFeed = ulNow - ulLastFeedTick;

            if (ulSinceLastFeed < IWDG_TIMEOUT_MS)
            {
                uint32_t ulRemaining = IWDG_TIMEOUT_MS - ulSinceLastFeed;
                vPrintCountdown(ulRemaining);
            }
            else
            {
                printf("[FeederTask] IWDG seharusnya sudah reset!\r\n");
                printf("[FeederTask] (LSI tidak akurat, mungkin sedikit lebih lama)\r\n");
                printf("[DATA] RESET_OVERDUE,elapsed=%lu,tick=%lu\r\n",
                       (unsigned long)ulSinceLastFeed,
                       (unsigned long)ulNow);
            }

            /* LED pattern error: kedip cepat */
            for (int i = 0; i < 5; i++)
            {
                HAL_GPIO_TogglePin(LED_PORT, LED_PIN);
                vTaskDelay(pdMS_TO_TICKS(50));
            }
        }

        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(IWDG_FEED_INTERVAL_MS));
    }
}

/* =========================================================================
 * Normal Worker Task
 * Task yang bekerja normal dan tidak berinteraksi dengan watchdog
 * ========================================================================= */
static void vNormalWorkerTask(void *pvParameters)
{
    (void)pvParameters;
    TickType_t xLastWakeTime = xTaskGetTickCount();

    printf("[WorkerTask] Dimulai - Task kerja normal\r\n");

    for (;;)
    {
        ulWorkerCounter++;

        /* Simulasi kerja normal */
        volatile uint32_t ulResult = 0;
        for (volatile uint32_t i = 0; i < 1000; i++)
        {
            ulResult += i * ulWorkerCounter;
        }

        /* Cetak status periodik */
        if (ulWorkerCounter % 5 == 0)
        {
            printf("[WorkerTask] Siklus %lu, hasil=%lu (tick=%lu)\r\n",
                   (unsigned long)ulWorkerCounter,
                   (unsigned long)ulResult,
                   (unsigned long)xTaskGetTickCount());

            printf("[DATA] WORKER,cycle=%lu,result=%lu,tick=%lu\r\n",
                   (unsigned long)ulWorkerCounter,
                   (unsigned long)ulResult,
                   (unsigned long)xTaskGetTickCount());
        }

        /* Cek status feeder */
        if (!ucFeederActive)
        {
            printf("[WorkerTask] PERINGATAN: Feeder tidak aktif! "
                   "Reset akan segera terjadi!\r\n");
        }

        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(800));
    }
}

/* =========================================================================
 * Problematic Task
 * Task yang sengaja hang setelah beberapa siklus
 * Mendemonstrasikan apa yang terjadi jika satu task freeze
 * ========================================================================= */
static void vProblematicTask(void *pvParameters)
{
    (void)pvParameters;
    TickType_t xLastWakeTime = xTaskGetTickCount();

    printf("[ProblemTask] Dimulai - Akan hang setelah %d siklus\r\n",
           PROBLEM_HANG_AFTER);

    for (;;)
    {
        ulProblemCounter++;

        if (ulProblemCounter <= PROBLEM_HANG_AFTER)
        {
            /* Operasi normal */
            volatile uint32_t ulWork = 0;
            for (volatile uint32_t i = 0; i < 500; i++)
            {
                ulWork += i;
            }

            uint32_t ulRemaining = PROBLEM_HANG_AFTER - ulProblemCounter;

            printf("[ProblemTask] Siklus %lu/%d (sisa %lu sebelum hang)\r\n",
                   (unsigned long)ulProblemCounter,
                   PROBLEM_HANG_AFTER,
                   (unsigned long)ulRemaining);

            printf("[DATA] PROBLEM,cycle=%lu,remaining=%lu,status=normal,tick=%lu\r\n",
                   (unsigned long)ulProblemCounter,
                   (unsigned long)ulRemaining,
                   (unsigned long)xTaskGetTickCount());

            /* Peringatan mendekati hang */
            if (ulRemaining <= 3)
            {
                printf("[ProblemTask] >>> PERINGATAN: %lu siklus lagi sebelum HANG! <<<\r\n",
                       (unsigned long)ulRemaining);
            }
        }
        else
        {
            /* HANG! Masuk infinite loop */
            if (!ucProblemHanging)
            {
                ucProblemHanging = 1;

                printf("\r\n");
                printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\r\n");
                printf("!! PROBLEM TASK HANG!                  !!\r\n");
                printf("!! Task masuk infinite loop!           !!\r\n");
                printf("!! Dalam sistem nyata, ini bisa karena:!!\r\n");
                printf("!!   - Deadlock                        !!\r\n");
                printf("!!   - Bug infinite loop               !!\r\n");
                printf("!!   - Menunggu resource yang tidak ada !!\r\n");
                printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\r\n");
                printf("[DATA] PROBLEM_HANG,cycle=%lu,tick=%lu\r\n",
                       (unsigned long)ulProblemCounter,
                       (unsigned long)xTaskGetTickCount());
            }

            /* Infinite loop - task ini tidak akan pernah keluar */
            /* CATATAN: Ini TIDAK langsung menyebabkan IWDG reset
             * karena feeder task masih berjalan di task terpisah.
             * IWDG hanya reset jika FEEDER juga berhenti. */
            while (1)
            {
                /* Stuck - tapi task lain masih bisa berjalan
                 * karena preemptive scheduler */
                __NOP();
                /* Busy loop tanpa delay - akan menggunakan CPU
                 * tapi scheduler akan preempt untuk task lain */
                volatile uint32_t ulDummy = 0;
                for (volatile uint32_t i = 0; i < 100000; i++)
                {
                    ulDummy++;
                }
                taskYIELD(); /* Beri kesempatan task lain */
            }
        }

        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(PROBLEM_TASK_DELAY_MS));
    }
}

/* =========================================================================
 * Monitor Task
 * Memantau status keseluruhan sistem dan mencetak laporan
 * ========================================================================= */
static void vMonitorTask(void *pvParameters)
{
    (void)pvParameters;
    TickType_t xLastWakeTime = xTaskGetTickCount();

    printf("[MonitorTask] Dimulai - Monitoring sistem\r\n");

    for (;;)
    {
        ulMonitorCycle++;

        printf("\r\n");
        printf("----- Monitor Siklus %lu (tick=%lu) -----\r\n",
               (unsigned long)ulMonitorCycle,
               (unsigned long)xTaskGetTickCount());

        /* Status IWDG */
        printf("  IWDG Feed Count : %lu\r\n", (unsigned long)ulFeedCount);
        printf("  IWDG Feeder     : %s\r\n",
               ucFeederActive ? "AKTIF" : "BERHENTI!");
        printf("  Last Feed Tick  : %lu\r\n", (unsigned long)ulLastFeedTick);

        /* Waktu sejak feed terakhir */
        uint32_t ulSinceLastFeed = xTaskGetTickCount() - ulLastFeedTick;
        printf("  Sejak feed akhir: %lu ms\r\n", (unsigned long)ulSinceLastFeed);

        /* Status task */
        printf("  Worker Counter  : %lu\r\n", (unsigned long)ulWorkerCounter);
        printf("  Problem Counter : %lu\r\n", (unsigned long)ulProblemCounter);
        printf("  Problem Status  : %s\r\n",
               ucProblemHanging ? "HANG!" : "Normal");

        /* IWDG reset detected */
        if (ucIWDGResetDetected)
        {
            printf("  >>> IWDG Reset terdeteksi dari boot sebelumnya!\r\n");
        }

        /* Free heap */
        printf("  Free Heap       : %lu bytes\r\n",
               (unsigned long)xPortGetFreeHeapSize());

        /* Health check bar */
        printf("  Kesehatan Sistem: ");
        if (ucFeederActive && !ucProblemHanging)
        {
            printf("[################....] BAIK\r\n");
        }
        else if (ucFeederActive && ucProblemHanging)
        {
            printf("[########............] PERINGATAN\r\n");
        }
        else if (!ucFeederActive && !ucProblemHanging)
        {
            printf("[####................] BURUK\r\n");
        }
        else
        {
            printf("[....................] KRITIS!\r\n");
        }

        /* Data terstruktur */
        printf("[DATA] MONITOR,cycle=%lu,feeds=%lu,feeder=%d,problem=%d,"
               "worker=%lu,since_feed=%lu,tick=%lu\r\n",
               (unsigned long)ulMonitorCycle,
               (unsigned long)ulFeedCount,
               ucFeederActive,
               ucProblemHanging,
               (unsigned long)ulWorkerCounter,
               (unsigned long)ulSinceLastFeed,
               (unsigned long)xTaskGetTickCount());

        /* Countdown jika feeder berhenti */
        if (!ucFeederActive)
        {
            if (ulSinceLastFeed < IWDG_TIMEOUT_MS)
            {
                vPrintCountdown(IWDG_TIMEOUT_MS - ulSinceLastFeed);
            }
            else
            {
                printf("  >>> IWDG RESET seharusnya sudah terjadi! <<<\r\n");
            }
        }

        /* Panduan arsitektur watchdog di multi-task */
        if (ulMonitorCycle == 3)
        {
            printf("\r\n");
            printf("=========================================================\r\n");
            printf(" PANDUAN: Arsitektur Watchdog di Sistem Multi-Task\r\n");
            printf("=========================================================\r\n");
            printf(" Pola 1: Single Feeder (yang digunakan di sini)\r\n");
            printf("   - Satu task khusus untuk feed IWDG\r\n");
            printf("   - Sederhana tapi tidak mendeteksi task hang\r\n");
            printf("   - Kecuali feeder sendiri yang hang\r\n");
            printf("\r\n");
            printf(" Pola 2: Cooperative Watchdog (lebih baik)\r\n");
            printf("   - Setiap task mengirim 'check-in' ke monitor\r\n");
            printf("   - Monitor hanya feed IWDG jika SEMUA task\r\n");
            printf("     sudah check-in dalam interval yang ditentukan\r\n");
            printf("   - Jika satu task hang -> tidak ada feed -> reset\r\n");
            printf("\r\n");
            printf(" Pola 3: Task Watchdog Timer (FreeRTOS Timer)\r\n");
            printf("   - Gunakan software timer untuk timeout per-task\r\n");
            printf("   - Timer di-reset setiap task menyelesaikan siklus\r\n");
            printf("   - Timeout = task hang -> aksi recovery\r\n");
            printf("=========================================================\r\n");
            printf("[DATA] GUIDE_PRINTED,tick=%lu\r\n",
                   (unsigned long)xTaskGetTickCount());
        }

        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(2000));
    }
}

/* =========================================================================
 * MAIN - Entry Point
 * ========================================================================= */
int main(void)
{
    /* Inisialisasi HAL */
    HAL_Init();

    /* Konfigurasi System Clock: HSE 8MHz -> PLL -> 72MHz */
    SystemClock_Config();

    /* Inisialisasi peripheral */
    GPIO_Init();
    UART1_Init();

    /* Cek alasan reset SEBELUM clear flags */
    eResetReason = eCheckResetReason();

    /* Banner */
    vPrintBanner();

    /* Cetak informasi reset */
    vPrintResetInfo(eResetReason);

    /* Jika ini boot setelah IWDG reset, tampilkan analisis */
    if (eResetReason == RESET_IWDG)
    {
        printf("=========================================================\r\n");
        printf(" ANALISIS POST-RESET\r\n");
        printf("=========================================================\r\n");
        printf(" Sistem di-reset oleh IWDG (watchdog)!\r\n");
        printf(" Kemungkinan penyebab:\r\n");
        printf("   - Task feeder berhenti/hang\r\n");
        printf("   - Sistem terlalu sibuk (CPU overload)\r\n");
        printf("   - Bug menyebabkan task feeder tidak berjalan\r\n");
        printf("   - Stack overflow merusak task feeder\r\n");
        printf("\r\n");
        printf(" Tindakan yang bisa diambil:\r\n");
        printf("   - Log error ke Flash/EEPROM sebelum reset\r\n");
        printf("   - Masuk mode safe/recovery\r\n");
        printf("   - Kirim notifikasi error\r\n");
        printf("=========================================================\r\n");
        printf("[DATA] POST_RESET_ANALYSIS,reason=IWDG\r\n");
        printf("\r\n");
    }

    /* Cetak konfigurasi IWDG */
    vPrintIWDGConfig();

    /* Inisialisasi IWDG */
    printf("[INFO] Menginisialisasi IWDG...\r\n");
    IWDG_Init();

    /* Buat task */
    BaseType_t xResult;

    xResult = xTaskCreate(vWatchdogFeederTask, "Feeder",
                          FEEDER_STACK_SIZE, NULL,
                          FEEDER_TASK_PRIORITY, &xFeederTaskHandle);
    if (xResult != pdPASS) { printf("[ERROR] Gagal membuat FeederTask!\r\n"); Error_Handler(); }

    xResult = xTaskCreate(vNormalWorkerTask, "Worker",
                          WORKER_STACK_SIZE, NULL,
                          WORKER_TASK_PRIORITY, &xWorkerTaskHandle);
    if (xResult != pdPASS) { printf("[ERROR] Gagal membuat WorkerTask!\r\n"); Error_Handler(); }

    xResult = xTaskCreate(vProblematicTask, "ProblemTask",
                          PROBLEM_STACK_SIZE, NULL,
                          PROBLEM_TASK_PRIORITY, &xProblemTaskHandle);
    if (xResult != pdPASS) { printf("[ERROR] Gagal membuat ProblemTask!\r\n"); Error_Handler(); }

    xResult = xTaskCreate(vMonitorTask, "Monitor",
                          MONITOR_STACK_SIZE, NULL,
                          MONITOR_TASK_PRIORITY, &xMonitorTaskHandle);
    if (xResult != pdPASS) { printf("[ERROR] Gagal membuat MonitorTask!\r\n"); Error_Handler(); }

    printf("[INFO] Semua task berhasil dibuat\r\n");
    printf("[INFO] Free Heap: %lu bytes\r\n",
           (unsigned long)xPortGetFreeHeapSize());
    printf("[INFO] IWDG aktif - timeout ~%d detik\r\n", IWDG_TIMEOUT_MS / 1000);
    printf("[INFO] Memulai scheduler...\r\n\r\n");

    /* Mulai scheduler */
    vTaskStartScheduler();

    /* Seharusnya tidak pernah sampai sini */
    printf("[ERROR] Scheduler berhenti!\r\n");
    for (;;);
}
