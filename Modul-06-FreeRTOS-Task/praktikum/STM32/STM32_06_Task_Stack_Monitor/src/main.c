/* =========================================================================
 * STM32_06_Task_Stack_Monitor
 * =========================================================================
 * Deskripsi  : Monitoring penggunaan stack pada task FreeRTOS
 *              Menggunakan uxTaskGetStackHighWaterMark() untuk memantau
 *              sisa stack minimum (high water mark) dari setiap task.
 *              Membuat 3 task dengan ukuran stack berbeda dan beban kerja
 *              yang bervariasi untuk mendemonstrasikan penggunaan stack.
 *
 * Target     : STM32F103C8 (Blue Pill) - Cortex-M3 @ 72MHz
 * Framework  : STM32Cube HAL + FreeRTOS
 * Peripheral : UART1 (PA9/PA10), LED (PC13), Button (PA0)
 *
 * Koneksi    : PA9  -> USB-Serial RX (UART TX)
 *              PA10 -> USB-Serial TX (UART RX)
 *              PC13 -> LED onboard (active low)
 *              PA0  -> Push button (active high, optional)
 *
 * Catatan    : configCHECK_FOR_STACK_OVERFLOW=2 digunakan untuk deteksi
 *              overflow dengan metode stack painting (mengisi pattern)
 * ========================================================================= */

/* -- Header Files -------------------------------------------------------- */
#if defined(STM32F103xB)
#include "stm32f1xx_hal.h"
#elif defined(STM32F401xC) || defined(STM32F411xE)
#include "stm32f4xx_hal.h"
#endif
#include "FreeRTOS.h"
#include "task.h"

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

/* Deklarasi xPortSysTickHandler untuk fix warning */
extern void xPortSysTickHandler(void);

/* -- Definisi Konstanta -------------------------------------------------- */
/* Ukuran stack untuk setiap task (dalam words, 1 word = 4 bytes) */
#define SMALL_STACK_SIZE        128     /* 512 bytes - stack kecil */
#define MEDIUM_STACK_SIZE       256     /* 1024 bytes - stack sedang */
#define LARGE_STACK_SIZE        512     /* 2048 bytes - stack besar */
#define MONITOR_STACK_SIZE      256     /* Stack untuk monitor task */

/* Prioritas task */
#define SMALL_TASK_PRIORITY     2
#define MEDIUM_TASK_PRIORITY    2
#define LARGE_TASK_PRIORITY     2
#define MONITOR_TASK_PRIORITY   3       /* Monitor lebih tinggi */

/* Interval monitoring dalam ms */
#define MONITOR_INTERVAL_MS     3000
#define SMALL_TASK_DELAY_MS     500
#define MEDIUM_TASK_DELAY_MS    750
#define LARGE_TASK_DELAY_MS     1000

/* Batas peringatan penggunaan stack (persen) */
#define STACK_WARNING_THRESHOLD 80

/* Kedalaman rekursi untuk large stack task */
#define MAX_RECURSION_DEPTH     8

/* LED patterns */
#define LED_PORT                GPIOC
#define LED_PIN                 GPIO_PIN_13
#define BUTTON_PORT             GPIOA
#define BUTTON_PIN              GPIO_PIN_0

/* -- Handle Peripheral --------------------------------------------------- */
static UART_HandleTypeDef huart1;

/* -- Handle Task --------------------------------------------------------- */
static TaskHandle_t xSmallStackTaskHandle  = NULL;
static TaskHandle_t xMediumStackTaskHandle = NULL;
static TaskHandle_t xLargeStackTaskHandle  = NULL;
static TaskHandle_t xMonitorTaskHandle     = NULL;

/* -- Variabel Global ----------------------------------------------------- */
static volatile uint32_t ulSmallTaskCounter  = 0;
static volatile uint32_t ulMediumTaskCounter = 0;
static volatile uint32_t ulLargeTaskCounter  = 0;
static volatile uint32_t ulMonitorCycle      = 0;
static volatile uint32_t ulSystemTick        = 0;

/* Statistik stack */
typedef struct {
    const char *pcTaskName;
    uint32_t   ulAllocatedStack;    /* Ukuran stack yang dialokasikan (words) */
    uint32_t   ulHighWaterMark;     /* Sisa minimum stack (words) */
    uint32_t   ulUsedStack;         /* Stack yang digunakan (words) */
    uint32_t   ulUsagePercent;      /* Persentase penggunaan */
    uint8_t    ucWarning;           /* Flag peringatan */
} StackStats_t;

static StackStats_t xStackStats[4];

/* Fase demonstrasi */
static volatile uint8_t ucDemoPhase = 0;
/* 0: Normal operation
 * 1: Increased load
 * 2: Maximum load
 * 3: Report summary */

/* -- Static memory untuk Idle dan Timer task ----------------------------- */
static StaticTask_t xIdleTaskTCB;
static StackType_t  uxIdleTaskStack[configMINIMAL_STACK_SIZE];

static StaticTask_t xTimerTaskTCB;
static StackType_t  uxTimerTaskStack[configTIMER_TASK_STACK_DEPTH];

/* -- Prototipe Fungsi ---------------------------------------------------- */
static void SystemClock_Config(void);
static void GPIO_Init(void);
static void UART1_Init(void);
static void Error_Handler(void);

static void vSmallStackTask(void *pvParameters);
static void vMediumStackTask(void *pvParameters);
static void vLargeStackTask(void *pvParameters);
static void vMonitorTask(void *pvParameters);

static uint32_t ulRecursiveFunction(uint32_t ulDepth, uint32_t ulMaxDepth);
static void vPrintStackStats(StackStats_t *pxStats);
static void vUpdateStackStats(TaskHandle_t xTask, const char *pcName,
                              uint32_t ulAllocated, StackStats_t *pxStats);
static void vPrintBanner(void);
static void vPrintPhaseInfo(uint8_t ucPhase);

/* -- Implementasi _write untuk printf via UART --------------------------- */
int _write(int file, char *ptr, int len)
{
    HAL_UART_Transmit(&huart1, (uint8_t *)ptr, len, HAL_MAX_DELAY);
    return len;
}

/* =========================================================================
 * SysTick Handler - dipanggil setiap 1ms
 * Memanggil HAL_IncTick() dan xPortSysTickHandler() FreeRTOS
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
 * Callback saat alokasi memori gagal
 * Dipanggil oleh kernel ketika pvPortMalloc() mengembalikan NULL
 * ========================================================================= */
void vApplicationMallocFailedHook(void)
{
    printf("[DATA] MALLOC_FAIL,tick=%lu\r\n", (unsigned long)ulSystemTick);
    printf("[ERROR] Alokasi memori FreeRTOS gagal!\r\n");

    /* Kedipkan LED cepat sebagai indikasi error */
    for (;;)
    {
        HAL_GPIO_TogglePin(LED_PORT, LED_PIN);
        HAL_Delay(100);
    }
}

/* =========================================================================
 * Callback saat stack overflow terdeteksi
 * configCHECK_FOR_STACK_OVERFLOW = 2 (Metode 2: Stack Painting)
 * Metode 2: Kernel mengisi stack dengan pattern 0xA5A5A5A5 saat task
 *           dibuat, lalu memeriksa apakah pattern masih utuh.
 * ========================================================================= */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;

    /* PERINGATAN: printf mungkin tidak aman di sini karena stack rusak.
     * Dalam produksi, gunakan mekanisme sederhana seperti GPIO toggle. */
    printf("\r\n");
    printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\r\n");
    printf("!! STACK OVERFLOW TERDETEKSI!           !!\r\n");
    printf("!! Task: %-16s              !!\r\n", pcTaskName);
    printf("!! Metode: configCHECK_FOR_STACK_OVERFLOW=2\r\n");
    printf("!! Stack painting pattern corrupted     !!\r\n");
    printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\r\n");
    printf("[DATA] STACK_OVERFLOW,task=%s,tick=%lu\r\n",
           pcTaskName, (unsigned long)ulSystemTick);

    /* Kedipkan LED SOS pattern */
    for (;;)
    {
        /* S: ... */
        for (int i = 0; i < 3; i++) {
            HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_RESET);
            HAL_Delay(150);
            HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET);
            HAL_Delay(150);
        }
        HAL_Delay(300);
        /* O: --- */
        for (int i = 0; i < 3; i++) {
            HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_RESET);
            HAL_Delay(400);
            HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET);
            HAL_Delay(150);
        }
        HAL_Delay(300);
        /* S: ... */
        for (int i = 0; i < 3; i++) {
            HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_RESET);
            HAL_Delay(150);
            HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET);
            HAL_Delay(150);
        }
        HAL_Delay(1000);
    }
}

/* =========================================================================
 * Static memory callbacks untuk Idle dan Timer tasks
 * Diperlukan karena configSUPPORT_STATIC_ALLOCATION = 1
 * ========================================================================= */
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
 * Konfigurasi System Clock
 * HSE 8MHz -> PLL x9 -> SYSCLK 72MHz
 * AHB = 72MHz, APB1 = 36MHz, APB2 = 72MHz
 * ========================================================================= */
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

/* =========================================================================
 * Inisialisasi GPIO
 * PC13: LED onboard (output, active low)
 * PA0 : Push button (input, pull-down)
 * ========================================================================= */
static void GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* Aktifkan clock GPIO */
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    /* LED PC13 - output push-pull */
    GPIO_InitStruct.Pin   = LED_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_PORT, &GPIO_InitStruct);

    /* LED mati awal (active low: SET = mati) */
    HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET);

    /* Button PA0 - input dengan pull-down */
    GPIO_InitStruct.Pin  = BUTTON_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;
    HAL_GPIO_Init(BUTTON_PORT, &GPIO_InitStruct);
}

/* =========================================================================
 * Inisialisasi UART1
 * PA9  = TX, PA10 = RX
 * Baudrate: 115200, 8N1
 * ========================================================================= */
static void UART1_Init(void)
{
    __HAL_RCC_USART1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* PA9 = UART1 TX (Alternate Function Push-Pull) */
    GPIO_InitStruct.Pin   = GPIO_PIN_9;
    GPIO_InitStruct.Mode  = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* PA10 = UART1 RX (Input Floating) */
    GPIO_InitStruct.Pin  = GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* Konfigurasi UART */
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
 * Error Handler
 * ========================================================================= */
static void Error_Handler(void)
{
    __disable_irq();
    while (1)
    {
        HAL_GPIO_TogglePin(LED_PORT, LED_PIN);
        /* Delay manual karena SysTick mungkin tidak berfungsi */
        for (volatile uint32_t i = 0; i < 500000; i++);
    }
}

/* =========================================================================
 * Fungsi bantuan: Update statistik stack untuk satu task
 * ========================================================================= */
static void vUpdateStackStats(TaskHandle_t xTask, const char *pcName,
                              uint32_t ulAllocated, StackStats_t *pxStats)
{
    UBaseType_t uxHWM;

    pxStats->pcTaskName      = pcName;
    pxStats->ulAllocatedStack = ulAllocated;

    /* Baca high water mark - nilai minimum sisa stack yang pernah tercatat */
    uxHWM = uxTaskGetStackHighWaterMark(xTask);
    pxStats->ulHighWaterMark = (uint32_t)uxHWM;

    /* Hitung stack yang terpakai */
    pxStats->ulUsedStack = ulAllocated - (uint32_t)uxHWM;

    /* Hitung persentase penggunaan */
    if (ulAllocated > 0)
    {
        pxStats->ulUsagePercent = (pxStats->ulUsedStack * 100) / ulAllocated;
    }
    else
    {
        pxStats->ulUsagePercent = 0;
    }

    /* Set flag peringatan */
    pxStats->ucWarning = (pxStats->ulUsagePercent >= STACK_WARNING_THRESHOLD) ? 1 : 0;
}

/* =========================================================================
 * Fungsi bantuan: Cetak statistik stack satu task
 * ========================================================================= */
static void vPrintStackStats(StackStats_t *pxStats)
{
    /* Bar visual penggunaan stack */
    char cBar[21];
    uint32_t ulFilled = pxStats->ulUsagePercent / 5; /* 20 karakter = 100% */
    if (ulFilled > 20) ulFilled = 20;

    for (uint32_t i = 0; i < 20; i++)
    {
        cBar[i] = (i < ulFilled) ? '#' : '.';
    }
    cBar[20] = '\0';

    /* Output data terstruktur */
    printf("[DATA] STACK,task=%s,alloc=%lu,hwm=%lu,used=%lu,pct=%lu\r\n",
           pxStats->pcTaskName,
           (unsigned long)pxStats->ulAllocatedStack,
           (unsigned long)pxStats->ulHighWaterMark,
           (unsigned long)pxStats->ulUsedStack,
           (unsigned long)pxStats->ulUsagePercent);

    /* Output human-readable */
    printf("  %-12s: [%s] %3lu%% (%lu/%lu words)\r\n",
           pxStats->pcTaskName, cBar,
           (unsigned long)pxStats->ulUsagePercent,
           (unsigned long)pxStats->ulUsedStack,
           (unsigned long)pxStats->ulAllocatedStack);

    if (pxStats->ucWarning)
    {
        printf("  >>> PERINGATAN: Penggunaan stack > %d%%! Risiko overflow! <<<\r\n",
               STACK_WARNING_THRESHOLD);
        printf("[DATA] STACK_WARN,task=%s,pct=%lu\r\n",
               pxStats->pcTaskName,
               (unsigned long)pxStats->ulUsagePercent);
    }

    /* Sisa stack dalam bytes */
    printf("  Sisa minimum: %lu words (%lu bytes)\r\n",
           (unsigned long)pxStats->ulHighWaterMark,
           (unsigned long)(pxStats->ulHighWaterMark * 4));
}

/* =========================================================================
 * Banner informasi program
 * ========================================================================= */
static void vPrintBanner(void)
{
    printf("\r\n");
    printf("=========================================================\r\n");
    printf(" STM32 FreeRTOS - Task Stack Monitor\r\n");
    printf(" Program 06: Monitoring Penggunaan Stack\r\n");
    printf("=========================================================\r\n");
    printf(" Target : STM32F103C8 (Blue Pill) @ 72MHz\r\n");
    printf(" RTOS   : FreeRTOS with Stack Overflow Detection\r\n");
    printf(" Method : configCHECK_FOR_STACK_OVERFLOW = 2\r\n");
    printf("          (Stack Painting / Watermark)\r\n");
    printf("---------------------------------------------------------\r\n");
    printf(" Task Konfigurasi:\r\n");
    printf("   SmallTask  : Stack = %d words (%d bytes)\r\n",
           SMALL_STACK_SIZE, SMALL_STACK_SIZE * 4);
    printf("   MediumTask : Stack = %d words (%d bytes)\r\n",
           MEDIUM_STACK_SIZE, MEDIUM_STACK_SIZE * 4);
    printf("   LargeTask  : Stack = %d words (%d bytes)\r\n",
           LARGE_STACK_SIZE, LARGE_STACK_SIZE * 4);
    printf("   MonitorTask: Stack = %d words (%d bytes)\r\n",
           MONITOR_STACK_SIZE, MONITOR_STACK_SIZE * 4);
    printf("---------------------------------------------------------\r\n");
    printf(" Stack Overflow Detection Method 2:\r\n");
    printf("   - Kernel mengisi stack dengan pattern 0xA5A5A5A5\r\n");
    printf("   - Saat context switch, kernel memeriksa apakah\r\n");
    printf("     16 bytes terakhir stack masih berisi pattern\r\n");
    printf("   - Jika pattern rusak -> stack overflow terdeteksi\r\n");
    printf("   - Callback vApplicationStackOverflowHook dipanggil\r\n");
    printf("=========================================================\r\n");
    printf("[DATA] INIT,small=%d,medium=%d,large=%d,monitor=%d\r\n",
           SMALL_STACK_SIZE, MEDIUM_STACK_SIZE, LARGE_STACK_SIZE,
           MONITOR_STACK_SIZE);
    printf("\r\n");
}

/* =========================================================================
 * Informasi fase demonstrasi
 * ========================================================================= */
static void vPrintPhaseInfo(uint8_t ucPhase)
{
    printf("\r\n");
    printf("=========================================================\r\n");

    switch (ucPhase)
    {
        case 0:
            printf(" FASE 0: Operasi Normal\r\n");
            printf("   - Semua task berjalan dengan beban ringan\r\n");
            printf("   - SmallTask: increment counter\r\n");
            printf("   - MediumTask: array lokal 32 byte\r\n");
            printf("   - LargeTask: rekursi dangkal (depth=3)\r\n");
            break;
        case 1:
            printf(" FASE 1: Beban Meningkat\r\n");
            printf("   - SmallTask: increment + format string\r\n");
            printf("   - MediumTask: array lokal 64 byte + sorting\r\n");
            printf("   - LargeTask: rekursi sedang (depth=5)\r\n");
            break;
        case 2:
            printf(" FASE 2: Beban Maksimum\r\n");
            printf("   - SmallTask: multiple format string\r\n");
            printf("   - MediumTask: array 96 byte + operasi\r\n");
            printf("   - LargeTask: rekursi dalam (depth=%d)\r\n", MAX_RECURSION_DEPTH);
            printf("   >>> Perhatikan peningkatan penggunaan stack!\r\n");
            break;
        case 3:
            printf(" FASE 3: Ringkasan dan Laporan Akhir\r\n");
            printf("   - Menampilkan statistik lengkap\r\n");
            printf("   - Perbandingan antar fase\r\n");
            break;
        default:
            break;
    }

    printf("=========================================================\r\n");
    printf("[DATA] PHASE,%d,tick=%lu\r\n", ucPhase, (unsigned long)ulSystemTick);
    printf("\r\n");
}

/* =========================================================================
 * Fungsi rekursif untuk menggunakan stack secara mendalam
 * Setiap level rekursi menambah penggunaan stack dengan variabel lokal
 * ========================================================================= */
static uint32_t ulRecursiveFunction(uint32_t ulDepth, uint32_t ulMaxDepth)
{
    /* Variabel lokal menggunakan stack ~48 bytes per level */
    volatile uint32_t aulLocalBuffer[8];
    volatile uint32_t ulResult = 0;
    volatile uint32_t ulTemp;

    /* Isi buffer lokal untuk memastikan compiler tidak mengoptimasi */
    for (uint32_t i = 0; i < 8; i++)
    {
        aulLocalBuffer[i] = ulDepth * 100 + i;
    }

    /* Base case */
    if (ulDepth >= ulMaxDepth)
    {
        /* Hitung checksum dari buffer */
        for (uint32_t i = 0; i < 8; i++)
        {
            ulResult += aulLocalBuffer[i];
        }
        return ulResult;
    }

    /* Recursive call */
    ulTemp = ulRecursiveFunction(ulDepth + 1, ulMaxDepth);
    ulResult = ulTemp + aulLocalBuffer[0];

    return ulResult;
}

/* =========================================================================
 * Small Stack Task (128 words = 512 bytes)
 * Pekerjaan ringan: increment counter dan print sederhana
 * ========================================================================= */
static void vSmallStackTask(void *pvParameters)
{
    (void)pvParameters;
    TickType_t xLastWakeTime = xTaskGetTickCount();
    volatile uint32_t ulLocalVar = 0;

    printf("[SmallTask] Dimulai - Stack: %d words\r\n", SMALL_STACK_SIZE);

    for (;;)
    {
        ulSmallTaskCounter++;
        ulLocalVar++;

        switch (ucDemoPhase)
        {
            case 0:
            {
                /* Fase 0: Beban minimal - hanya increment */
                volatile uint32_t ulA = ulSmallTaskCounter;
                volatile uint32_t ulB = ulA * 2;
                (void)ulB;
                break;
            }
            case 1:
            {
                /* Fase 1: Beban sedang - format string kecil */
                char cBuf[32];
                snprintf(cBuf, sizeof(cBuf), "cnt=%lu",
                         (unsigned long)ulSmallTaskCounter);
                (void)cBuf;
                break;
            }
            case 2:
            default:
            {
                /* Fase 2: Beban tinggi - multiple format */
                char cBuf1[24];
                char cBuf2[24];
                snprintf(cBuf1, sizeof(cBuf1), "A=%lu",
                         (unsigned long)ulSmallTaskCounter);
                snprintf(cBuf2, sizeof(cBuf2), "B=%lu",
                         (unsigned long)ulLocalVar);
                (void)cBuf1;
                (void)cBuf2;
                break;
            }
        }

        /* Cetak status periodik */
        if (ulSmallTaskCounter % 10 == 0)
        {
            printf("[DATA] SMALL,cnt=%lu,tick=%lu,phase=%d\r\n",
                   (unsigned long)ulSmallTaskCounter,
                   (unsigned long)xTaskGetTickCount(),
                   ucDemoPhase);
        }

        /* Toggle LED setiap 5 iterasi */
        if (ulSmallTaskCounter % 5 == 0)
        {
            HAL_GPIO_TogglePin(LED_PORT, LED_PIN);
        }

        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(SMALL_TASK_DELAY_MS));
    }
}

/* =========================================================================
 * Medium Stack Task (256 words = 1024 bytes)
 * Penggunaan stack sedang: array lokal, string formatting
 * ========================================================================= */
static void vMediumStackTask(void *pvParameters)
{
    (void)pvParameters;
    TickType_t xLastWakeTime = xTaskGetTickCount();

    printf("[MediumTask] Dimulai - Stack: %d words\r\n", MEDIUM_STACK_SIZE);

    for (;;)
    {
        ulMediumTaskCounter++;

        switch (ucDemoPhase)
        {
            case 0:
            {
                /* Fase 0: Array lokal 32 byte */
                volatile uint8_t aucBuffer[32];
                for (uint32_t i = 0; i < 32; i++)
                {
                    aucBuffer[i] = (uint8_t)(i + ulMediumTaskCounter);
                }
                /* Checksum sederhana */
                volatile uint32_t ulSum = 0;
                for (uint32_t i = 0; i < 32; i++)
                {
                    ulSum += aucBuffer[i];
                }
                (void)ulSum;
                break;
            }
            case 1:
            {
                /* Fase 1: Array 64 byte + bubble sort parsial */
                volatile uint8_t aucBuffer[64];
                for (uint32_t i = 0; i < 64; i++)
                {
                    aucBuffer[i] = (uint8_t)((64 - i) + ulMediumTaskCounter);
                }
                /* Bubble sort parsial (beberapa iterasi saja) */
                for (uint32_t pass = 0; pass < 4; pass++)
                {
                    for (uint32_t i = 0; i < 63; i++)
                    {
                        if (aucBuffer[i] > aucBuffer[i + 1])
                        {
                            uint8_t tmp = aucBuffer[i];
                            aucBuffer[i] = aucBuffer[i + 1];
                            aucBuffer[i + 1] = tmp;
                        }
                    }
                }
                break;
            }
            case 2:
            default:
            {
                /* Fase 2: Array 96 byte + operasi intensif */
                volatile uint8_t aucBuffer[96];
                volatile uint32_t aulAccum[8];

                for (uint32_t i = 0; i < 96; i++)
                {
                    aucBuffer[i] = (uint8_t)(i ^ ulMediumTaskCounter);
                }

                /* Akumulasi per-segment */
                for (uint32_t s = 0; s < 8; s++)
                {
                    aulAccum[s] = 0;
                    for (uint32_t i = 0; i < 12; i++)
                    {
                        aulAccum[s] += aucBuffer[s * 12 + i];
                    }
                }

                /* Format hasil */
                char cReport[64];
                snprintf(cReport, sizeof(cReport), "seg0=%lu,seg7=%lu",
                         (unsigned long)aulAccum[0],
                         (unsigned long)aulAccum[7]);
                (void)cReport;
                break;
            }
        }

        /* Cetak status periodik */
        if (ulMediumTaskCounter % 8 == 0)
        {
            printf("[DATA] MEDIUM,cnt=%lu,tick=%lu,phase=%d\r\n",
                   (unsigned long)ulMediumTaskCounter,
                   (unsigned long)xTaskGetTickCount(),
                   ucDemoPhase);
        }

        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(MEDIUM_TASK_DELAY_MS));
    }
}

/* =========================================================================
 * Large Stack Task (512 words = 2048 bytes)
 * Penggunaan stack besar: fungsi rekursif, buffer besar
 * ========================================================================= */
static void vLargeStackTask(void *pvParameters)
{
    (void)pvParameters;
    TickType_t xLastWakeTime = xTaskGetTickCount();

    printf("[LargeTask] Dimulai - Stack: %d words\r\n", LARGE_STACK_SIZE);

    for (;;)
    {
        ulLargeTaskCounter++;
        volatile uint32_t ulRecurseResult = 0;

        switch (ucDemoPhase)
        {
            case 0:
            {
                /* Fase 0: Rekursi dangkal (depth=3) */
                ulRecurseResult = ulRecursiveFunction(0, 3);

                /* Buffer lokal kecil */
                volatile uint8_t aucBuf[32];
                for (uint32_t i = 0; i < 32; i++)
                {
                    aucBuf[i] = (uint8_t)(ulRecurseResult + i);
                }
                (void)aucBuf;
                break;
            }
            case 1:
            {
                /* Fase 1: Rekursi sedang (depth=5) */
                ulRecurseResult = ulRecursiveFunction(0, 5);

                /* Buffer lokal menengah */
                volatile uint8_t aucBuf[64];
                volatile uint32_t aulExtra[8];
                for (uint32_t i = 0; i < 64; i++)
                {
                    aucBuf[i] = (uint8_t)(ulRecurseResult ^ i);
                }
                for (uint32_t i = 0; i < 8; i++)
                {
                    aulExtra[i] = ulRecurseResult + i * 17;
                }
                (void)aucBuf;
                (void)aulExtra;
                break;
            }
            case 2:
            default:
            {
                /* Fase 2: Rekursi dalam (depth=MAX_RECURSION_DEPTH) */
                ulRecurseResult = ulRecursiveFunction(0, MAX_RECURSION_DEPTH);

                /* Buffer lokal besar */
                volatile uint8_t aucBuf[128];
                volatile uint32_t aulMatrix[4][4];

                for (uint32_t i = 0; i < 128; i++)
                {
                    aucBuf[i] = (uint8_t)(ulRecurseResult + i);
                }

                /* Simulasi operasi matrix */
                for (uint32_t r = 0; r < 4; r++)
                {
                    for (uint32_t c = 0; c < 4; c++)
                    {
                        aulMatrix[r][c] = ulRecurseResult + r * 4 + c;
                    }
                }

                /* Format laporan panjang */
                char cReport[80];
                snprintf(cReport, sizeof(cReport),
                         "recurse=%lu,m00=%lu,m33=%lu",
                         (unsigned long)ulRecurseResult,
                         (unsigned long)aulMatrix[0][0],
                         (unsigned long)aulMatrix[3][3]);
                (void)cReport;
                break;
            }
        }

        /* Cetak status periodik */
        if (ulLargeTaskCounter % 5 == 0)
        {
            printf("[DATA] LARGE,cnt=%lu,recurse=%lu,tick=%lu,phase=%d\r\n",
                   (unsigned long)ulLargeTaskCounter,
                   (unsigned long)ulRecurseResult,
                   (unsigned long)xTaskGetTickCount(),
                   ucDemoPhase);
        }

        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(LARGE_TASK_DELAY_MS));
    }
}

/* =========================================================================
 * Monitor Task - Memantau penggunaan stack semua task
 * Berjalan dengan prioritas lebih tinggi agar bisa menginterupsi task lain
 * ========================================================================= */
static void vMonitorTask(void *pvParameters)
{
    (void)pvParameters;
    TickType_t xLastWakeTime = xTaskGetTickCount();
    uint32_t ulPhaseTickStart = xTaskGetTickCount();
    uint32_t ulPhaseDuration = 15000; /* 15 detik per fase */

    printf("[MonitorTask] Dimulai - Interval: %d ms\r\n", MONITOR_INTERVAL_MS);
    printf("[MonitorTask] Fase berganti setiap %lu detik\r\n",
           (unsigned long)(ulPhaseDuration / 1000));
    printf("\r\n");

    /* Tampilkan info fase awal */
    vPrintPhaseInfo(0);

    for (;;)
    {
        ulMonitorCycle++;

        /* ---- Cek pergantian fase ---- */
        uint32_t ulElapsed = xTaskGetTickCount() - ulPhaseTickStart;
        if (ulElapsed >= pdMS_TO_TICKS(ulPhaseDuration) && ucDemoPhase < 3)
        {
            ucDemoPhase++;
            ulPhaseTickStart = xTaskGetTickCount();
            vPrintPhaseInfo(ucDemoPhase);
        }

        /* ---- Header monitoring ---- */
        printf("\r\n");
        printf("----- Stack Monitor (Siklus %lu, Fase %d) -----\r\n",
               (unsigned long)ulMonitorCycle, ucDemoPhase);
        printf("Waktu: %lu ms | Free Heap: %lu bytes\r\n",
               (unsigned long)xTaskGetTickCount(),
               (unsigned long)xPortGetFreeHeapSize());
        printf("[DATA] HEAP,free=%lu,tick=%lu\r\n",
               (unsigned long)xPortGetFreeHeapSize(),
               (unsigned long)xTaskGetTickCount());
        printf("\r\n");

        /* ---- Update dan cetak statistik setiap task ---- */

        /* 1. Small Stack Task */
        vUpdateStackStats(xSmallStackTaskHandle, "SmallTask",
                          SMALL_STACK_SIZE, &xStackStats[0]);
        vPrintStackStats(&xStackStats[0]);

        /* 2. Medium Stack Task */
        vUpdateStackStats(xMediumStackTaskHandle, "MediumTask",
                          MEDIUM_STACK_SIZE, &xStackStats[1]);
        vPrintStackStats(&xStackStats[1]);

        /* 3. Large Stack Task */
        vUpdateStackStats(xLargeStackTaskHandle, "LargeTask",
                          LARGE_STACK_SIZE, &xStackStats[2]);
        vPrintStackStats(&xStackStats[2]);

        /* 4. Monitor Task sendiri */
        vUpdateStackStats(xMonitorTaskHandle, "MonitorTask",
                          MONITOR_STACK_SIZE, &xStackStats[3]);
        vPrintStackStats(&xStackStats[3]);

        /* ---- Ringkasan ---- */
        printf("\r\n");
        printf("Ringkasan Siklus %lu:\r\n", (unsigned long)ulMonitorCycle);
        printf("  SmallTask  counter: %lu\r\n", (unsigned long)ulSmallTaskCounter);
        printf("  MediumTask counter: %lu\r\n", (unsigned long)ulMediumTaskCounter);
        printf("  LargeTask  counter: %lu\r\n", (unsigned long)ulLargeTaskCounter);

        /* Cek total peringatan */
        uint32_t ulWarnings = 0;
        for (int i = 0; i < 4; i++)
        {
            if (xStackStats[i].ucWarning) ulWarnings++;
        }

        if (ulWarnings > 0)
        {
            printf("\r\n  !!! %lu task melebihi batas %d%% penggunaan stack !!!\r\n",
                   (unsigned long)ulWarnings, STACK_WARNING_THRESHOLD);
        }
        else
        {
            printf("\r\n  Semua task dalam batas aman penggunaan stack.\r\n");
        }

        printf("[DATA] MONITOR,cycle=%lu,warnings=%lu,phase=%d,tick=%lu\r\n",
               (unsigned long)ulMonitorCycle,
               (unsigned long)ulWarnings,
               ucDemoPhase,
               (unsigned long)xTaskGetTickCount());

        /* ---- Fase 3: Laporan akhir ---- */
        if (ucDemoPhase == 3 && ulMonitorCycle > 0)
        {
            printf("\r\n");
            printf("=========================================================\r\n");
            printf(" LAPORAN AKHIR - Analisis Penggunaan Stack\r\n");
            printf("=========================================================\r\n");

            for (int i = 0; i < 4; i++)
            {
                const char *pcStatus;
                if (xStackStats[i].ulUsagePercent >= 90)
                    pcStatus = "KRITIS";
                else if (xStackStats[i].ulUsagePercent >= STACK_WARNING_THRESHOLD)
                    pcStatus = "PERINGATAN";
                else if (xStackStats[i].ulUsagePercent >= 50)
                    pcStatus = "SEDANG";
                else
                    pcStatus = "AMAN";

                printf(" %-12s: %3lu%% terpakai, sisa %lu words - %s\r\n",
                       xStackStats[i].pcTaskName,
                       (unsigned long)xStackStats[i].ulUsagePercent,
                       (unsigned long)xStackStats[i].ulHighWaterMark,
                       pcStatus);
            }

            printf("\r\n Rekomendasi Ukuran Stack (overhead 25%%):\r\n");
            for (int i = 0; i < 4; i++)
            {
                uint32_t ulRecommended = (xStackStats[i].ulUsedStack * 125) / 100;
                /* Bulatkan ke kelipatan 16 terdekat */
                ulRecommended = ((ulRecommended + 15) / 16) * 16;
                if (ulRecommended < configMINIMAL_STACK_SIZE)
                    ulRecommended = configMINIMAL_STACK_SIZE;

                printf("   %-12s: saat ini=%lu, rekomendasi=%lu words\r\n",
                       xStackStats[i].pcTaskName,
                       (unsigned long)xStackStats[i].ulAllocatedStack,
                       (unsigned long)ulRecommended);

                printf("[DATA] RECOMMEND,task=%s,current=%lu,recommended=%lu\r\n",
                       xStackStats[i].pcTaskName,
                       (unsigned long)xStackStats[i].ulAllocatedStack,
                       (unsigned long)ulRecommended);
            }

            printf("\r\n Penjelasan configCHECK_FOR_STACK_OVERFLOW:\r\n");
            printf("   Metode 1: Cek stack pointer saat context switch\r\n");
            printf("     - Cepat tapi bisa terlewat jika overflow terjadi\r\n");
            printf("       dan kembali sebelum switch\r\n");
            printf("   Metode 2: Stack Painting (yang digunakan)\r\n");
            printf("     - Mengisi stack dengan pattern 0xA5A5A5A5\r\n");
            printf("     - Lebih lambat tapi lebih akurat\r\n");
            printf("     - Memeriksa 16 bytes terakhir setiap switch\r\n");
            printf("=========================================================\r\n");

            printf("\r\n[DATA] SUMMARY_COMPLETE,tick=%lu\r\n",
                   (unsigned long)xTaskGetTickCount());

            /* Reset ke fase 0 untuk demo berulang */
            ucDemoPhase = 0;
            ulPhaseTickStart = xTaskGetTickCount();
            vPrintPhaseInfo(0);
        }

        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(MONITOR_INTERVAL_MS));
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

    /* Tampilkan banner */
    vPrintBanner();

    /* ---- Buat task dengan ukuran stack berbeda ---- */

    BaseType_t xResult;

    /* Task 1: Small Stack (128 words) */
    xResult = xTaskCreate(
        vSmallStackTask,
        "SmallTask",
        SMALL_STACK_SIZE,
        NULL,
        SMALL_TASK_PRIORITY,
        &xSmallStackTaskHandle
    );
    if (xResult != pdPASS)
    {
        printf("[ERROR] Gagal membuat SmallTask!\r\n");
        Error_Handler();
    }

    /* Task 2: Medium Stack (256 words) */
    xResult = xTaskCreate(
        vMediumStackTask,
        "MediumTask",
        MEDIUM_STACK_SIZE,
        NULL,
        MEDIUM_TASK_PRIORITY,
        &xMediumStackTaskHandle
    );
    if (xResult != pdPASS)
    {
        printf("[ERROR] Gagal membuat MediumTask!\r\n");
        Error_Handler();
    }

    /* Task 3: Large Stack (512 words) */
    xResult = xTaskCreate(
        vLargeStackTask,
        "LargeTask",
        LARGE_STACK_SIZE,
        NULL,
        LARGE_TASK_PRIORITY,
        &xLargeStackTaskHandle
    );
    if (xResult != pdPASS)
    {
        printf("[ERROR] Gagal membuat LargeTask!\r\n");
        Error_Handler();
    }

    /* Task 4: Monitor Task */
    xResult = xTaskCreate(
        vMonitorTask,
        "MonitorTask",
        MONITOR_STACK_SIZE,
        NULL,
        MONITOR_TASK_PRIORITY,
        &xMonitorTaskHandle
    );
    if (xResult != pdPASS)
    {
        printf("[ERROR] Gagal membuat MonitorTask!\r\n");
        Error_Handler();
    }

    printf("[INFO] Semua task berhasil dibuat\r\n");
    printf("[INFO] Total heap tersedia: %lu bytes\r\n",
           (unsigned long)xPortGetFreeHeapSize());
    printf("[INFO] Memulai FreeRTOS scheduler...\r\n\r\n");

    /* Mulai scheduler - tidak pernah return */
    vTaskStartScheduler();

    /* Seharusnya tidak pernah sampai sini */
    printf("[ERROR] Scheduler berhenti!\r\n");
    for (;;);
}
