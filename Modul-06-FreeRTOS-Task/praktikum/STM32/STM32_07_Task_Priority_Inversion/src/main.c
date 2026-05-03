/* =========================================================================
 * STM32_07_Task_Priority_Inversion
 * =========================================================================
 * Deskripsi  : Demonstrasi fenomena Priority Inversion pada FreeRTOS
 *              Priority Inversion terjadi ketika task berprioritas tinggi
 *              terblokir oleh task berprioritas rendah yang memegang resource,
 *              sementara task berprioritas menengah tetap berjalan.
 *
 * Skenario   : 3 Task dengan prioritas berbeda:
 *              - HighTask  (prioritas 5): Butuh akses ke shared resource
 *              - MediumTask (prioritas 3): Tidak butuh resource, CPU-bound
 *              - LowTask   (prioritas 1): Memiliki shared resource
 *
 * Timeline   : 1. LowTask mengambil resource (flag)
 *              2. HighTask mencoba mengakses resource -> TERBLOKIR
 *              3. MediumTask berjalan (priority inversion!)
 *              4. LowTask akhirnya melepas resource
 *              5. HighTask baru bisa berjalan
 *
 * Solusi     : Disebutkan konsep Priority Inheritance Mutex (Modul 10)
 *
 * Target     : STM32F103C8 (Blue Pill) - Cortex-M3 @ 72MHz
 * Framework  : STM32Cube HAL + FreeRTOS
 * Peripheral : UART1 (PA9/PA10), LED (PC13), DWT Cycle Counter
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
extern void xPortSysTickHandler(void);

/* -- Definisi Konstanta -------------------------------------------------- */
/* Prioritas task */
#define HIGH_TASK_PRIORITY      5
#define MEDIUM_TASK_PRIORITY    3
#define LOW_TASK_PRIORITY       1
#define ORCHESTRATOR_PRIORITY   6

/* Ukuran stack (words) */
#define TASK_STACK_SIZE         256
#define ORCHESTRATOR_STACK_SIZE 256

/* Timing (ms) */
#define LOW_HOLD_TIME_MS        2000    /* Waktu LowTask menahan resource */
#define MEDIUM_WORK_TIME_MS     100     /* Interval kerja MediumTask */
#define HIGH_CHECK_INTERVAL_MS  50      /* Interval cek resource HighTask */
#define SCENARIO_PAUSE_MS       3000    /* Jeda antar skenario */

/* DWT (Data Watchpoint and Trace) untuk cycle counting */
#define DWT_CTRL_REG            (*(volatile uint32_t *)0xE0001000)
#define DWT_CYCCNT_REG          (*(volatile uint32_t *)0xE0001004)
#define DWT_DEMCR_REG           (*(volatile uint32_t *)0xE000EDFC)

/* LED dan Button */
#define LED_PORT                GPIOC
#define LED_PIN                 GPIO_PIN_13
#define BUTTON_PORT             GPIOA
#define BUTTON_PIN              GPIO_PIN_0

/* Jumlah maksimum event di timeline */
#define MAX_TIMELINE_EVENTS     64

/* -- Handle Peripheral --------------------------------------------------- */
static UART_HandleTypeDef huart1;

/* -- Handle Task --------------------------------------------------------- */
static TaskHandle_t xHighTaskHandle         = NULL;
static TaskHandle_t xMediumTaskHandle       = NULL;
static TaskHandle_t xLowTaskHandle          = NULL;
static TaskHandle_t xOrchestratorTaskHandle = NULL;

/* -- Shared Resource (Simulasi) ------------------------------------------ */
/* Flag yang merepresentasikan resource bersama */
static volatile uint8_t ucResourceLocked = 0;
static volatile uint8_t ucResourceOwner  = 0;  /* 0=none, 1=low, 2=med, 3=high */

/* -- Variabel Kontrol ---------------------------------------------------- */
static volatile uint32_t ulSystemTick      = 0;
static volatile uint32_t ulScenarioNumber  = 0;
static volatile uint8_t  ucScenarioActive  = 0;
static volatile uint8_t  ucHighTaskBlocked = 0;
static volatile uint8_t  ucInversionDetected = 0;

/* Counter per task */
static volatile uint32_t ulHighTaskRuns     = 0;
static volatile uint32_t ulMediumTaskRuns   = 0;
static volatile uint32_t ulLowTaskRuns      = 0;

/* Timing DWT */
static volatile uint32_t ulInversionStartCycle  = 0;
static volatile uint32_t ulInversionEndCycle    = 0;
static volatile uint32_t ulInversionDuration_us = 0;
static volatile uint32_t ulHighBlockedTick      = 0;
static volatile uint32_t ulHighUnblockedTick    = 0;

/* -- Timeline Event Recording -------------------------------------------- */
typedef struct {
    uint32_t    ulTimestamp;     /* Tick saat event */
    uint32_t    ulCycles;       /* DWT cycle count */
    const char *pcTaskName;     /* Nama task */
    const char *pcEvent;        /* Deskripsi event */
    uint8_t     ucPriority;     /* Prioritas task */
} TimelineEvent_t;

static TimelineEvent_t xTimeline[MAX_TIMELINE_EVENTS];
static volatile uint32_t ulTimelineIndex = 0;

/* LED pattern untuk identifikasi task */
/* LowTask: LED nyala steady
 * MediumTask: LED kedip lambat
 * HighTask: LED kedip cepat
 * Idle: LED mati */

/* -- Static memory untuk Idle dan Timer task ----------------------------- */
static StaticTask_t xIdleTaskTCB;
static StackType_t  uxIdleTaskStack[configMINIMAL_STACK_SIZE];

static StaticTask_t xTimerTaskTCB;
static StackType_t  uxTimerTaskStack[configTIMER_TASK_STACK_DEPTH];

/* -- Prototipe Fungsi ---------------------------------------------------- */
static void SystemClock_Config(void);
static void GPIO_Init(void);
static void UART1_Init(void);
static void DWT_Init(void);
static void Error_Handler(void);

static void vHighPriorityTask(void *pvParameters);
static void vMediumPriorityTask(void *pvParameters);
static void vLowPriorityTask(void *pvParameters);
static void vOrchestratorTask(void *pvParameters);

static void vRecordEvent(const char *pcTask, const char *pcEvent, uint8_t ucPrio);
static void vPrintTimeline(void);
static void vPrintBanner(void);
static void vResetScenario(void);

static inline uint32_t ulGetDWTCycles(void);
static void vBusyWait_ms(uint32_t ulMs);

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
        Error_Handler();
    }

    RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                      | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
    {
        Error_Handler();
    }
#else
    #error "Unsupported STM32 target"
#endif
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
#if defined(STM32F401xC) || defined(STM32F411xE)
    GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
#endif
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin  = GPIO_PIN_10;
#if defined(STM32F103xB)
    GPIO_InitStruct.Mode = GPIO_MODE_AF_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
#elif defined(STM32F401xC) || defined(STM32F411xE)
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
#endif
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
 * Inisialisasi DWT Cycle Counter
 * Digunakan untuk pengukuran waktu presisi tinggi (cycle-accurate)
 * ========================================================================= */
static void DWT_Init(void)
{
    /* Aktifkan DWT via Debug Exception and Monitor Control Register */
    DWT_DEMCR_REG |= (1 << 24);  /* TRCENA bit */
    DWT_CYCCNT_REG = 0;           /* Reset counter */
    DWT_CTRL_REG  |= 1;           /* Aktifkan cycle counter */

    printf("[DWT] Cycle counter diinisialisasi\r\n");
    printf("[DWT] CPU Clock: %lu Hz, 1 cycle = %.2f ns\r\n",
           (unsigned long)configCPU_CLOCK_HZ,
           1000000000.0 / configCPU_CLOCK_HZ);
}

static inline uint32_t ulGetDWTCycles(void)
{
    return DWT_CYCCNT_REG;
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
 * Busy-wait: simulasi kerja CPU tanpa melepas CPU ke scheduler
 * Berbeda dengan vTaskDelay() yang kooperatif
 * ========================================================================= */
static void vBusyWait_ms(uint32_t ulMs)
{
    uint32_t ulCyclesPerMs = configCPU_CLOCK_HZ / 1000;
    uint32_t ulStartCycle  = ulGetDWTCycles();
    uint32_t ulTargetCycles = ulMs * ulCyclesPerMs;

    while ((ulGetDWTCycles() - ulStartCycle) < ulTargetCycles)
    {
        /* Busy waiting - task TIDAK melepas CPU */
        __NOP();
    }
}

/* =========================================================================
 * Record event ke timeline untuk analisis
 * ========================================================================= */
static void vRecordEvent(const char *pcTask, const char *pcEvent, uint8_t ucPrio)
{
    if (ulTimelineIndex < MAX_TIMELINE_EVENTS)
    {
        xTimeline[ulTimelineIndex].ulTimestamp = xTaskGetTickCount();
        xTimeline[ulTimelineIndex].ulCycles    = ulGetDWTCycles();
        xTimeline[ulTimelineIndex].pcTaskName  = pcTask;
        xTimeline[ulTimelineIndex].pcEvent     = pcEvent;
        xTimeline[ulTimelineIndex].ucPriority  = ucPrio;
        ulTimelineIndex++;
    }

    printf("[DATA] EVENT,task=%s,event=%s,prio=%d,tick=%lu\r\n",
           pcTask, pcEvent, ucPrio,
           (unsigned long)xTaskGetTickCount());
}

/* =========================================================================
 * Cetak timeline event lengkap
 * ========================================================================= */
static void vPrintTimeline(void)
{
    printf("\r\n");
    printf("=========================================================\r\n");
    printf(" TIMELINE SKENARIO %lu\r\n", (unsigned long)ulScenarioNumber);
    printf("=========================================================\r\n");
    printf(" %-6s | %-12s | %-4s | %s\r\n",
           "Tick", "Task", "Prio", "Event");
    printf("---------+--------------+------+--------------------------\r\n");

    for (uint32_t i = 0; i < ulTimelineIndex && i < MAX_TIMELINE_EVENTS; i++)
    {
        printf(" %-6lu | %-12s | %-4d | %s\r\n",
               (unsigned long)xTimeline[i].ulTimestamp,
               xTimeline[i].pcTaskName,
               xTimeline[i].ucPriority,
               xTimeline[i].pcEvent);

        printf("[DATA] TIMELINE,idx=%lu,tick=%lu,task=%s,prio=%d,event=%s\r\n",
               (unsigned long)i,
               (unsigned long)xTimeline[i].ulTimestamp,
               xTimeline[i].pcTaskName,
               xTimeline[i].ucPriority,
               xTimeline[i].pcEvent);
    }

    printf("=========================================================\r\n");

    /* Analisis inversion */
    if (ucInversionDetected)
    {
        uint32_t ulBlockedMs = ulHighUnblockedTick - ulHighBlockedTick;
        printf("\r\n ANALISIS PRIORITY INVERSION:\r\n");
        printf("   HighTask terblokir selama: %lu ms\r\n",
               (unsigned long)ulBlockedMs);
        printf("   Durasi inversion (DWT): %lu us\r\n",
               (unsigned long)ulInversionDuration_us);
        printf("   MediumTask berjalan saat HighTask menunggu!\r\n");
        printf("   Ini adalah PRIORITY INVERSION klasik.\r\n");

        printf("\r\n DIAGRAM PRIORITY INVERSION:\r\n");
        printf("   Prioritas\r\n");
        printf("     High(5) : ---[BLOCKED]----->|RUN|\r\n");
        printf("     Med(3)  : ........[RUNNING].|...|\r\n");
        printf("     Low(1)  : [RESOURCE LOCK]...........[UNLOCK]\r\n");
        printf("                ^                ^\r\n");
        printf("                |                |\r\n");
        printf("           Low mengambil    Inversion terjadi:\r\n");
        printf("           resource         Med berjalan, High menunggu\r\n");

        printf("\r\n[DATA] INVERSION,blocked_ms=%lu,dwt_us=%lu,scenario=%lu\r\n",
               (unsigned long)ulBlockedMs,
               (unsigned long)ulInversionDuration_us,
               (unsigned long)ulScenarioNumber);
    }
}

/* =========================================================================
 * Reset skenario untuk iterasi berikutnya
 * ========================================================================= */
static void vResetScenario(void)
{
    ucResourceLocked    = 0;
    ucResourceOwner     = 0;
    ucHighTaskBlocked   = 0;
    ucInversionDetected = 0;
    ulTimelineIndex     = 0;
    ulHighTaskRuns      = 0;
    ulMediumTaskRuns    = 0;
    ulLowTaskRuns       = 0;
    ulInversionStartCycle  = 0;
    ulInversionEndCycle    = 0;
    ulInversionDuration_us = 0;
    ulHighBlockedTick      = 0;
    ulHighUnblockedTick    = 0;
}

/* =========================================================================
 * Banner
 * ========================================================================= */
static void vPrintBanner(void)
{
    printf("\r\n");
    printf("=========================================================\r\n");
    printf(" STM32 FreeRTOS - Priority Inversion Demo\r\n");
    printf(" Program 07: Demonstrasi Priority Inversion\r\n");
    printf("=========================================================\r\n");
    printf(" Target : STM32F103C8 (Blue Pill) @ 72MHz\r\n");
    printf(" RTOS   : FreeRTOS (Preemptive Scheduler)\r\n");
    printf(" DWT    : Cycle Counter untuk pengukuran presisi\r\n");
    printf("---------------------------------------------------------\r\n");
    printf(" Konfigurasi Task:\r\n");
    printf("   HighTask    : Prioritas %d - Butuh shared resource\r\n",
           HIGH_TASK_PRIORITY);
    printf("   MediumTask  : Prioritas %d - CPU-bound, tanpa resource\r\n",
           MEDIUM_TASK_PRIORITY);
    printf("   LowTask     : Prioritas %d - Memegang shared resource\r\n",
           LOW_TASK_PRIORITY);
    printf("---------------------------------------------------------\r\n");
    printf(" Fenomena Priority Inversion:\r\n");
    printf("   1. LowTask mengambil resource (lock)\r\n");
    printf("   2. HighTask membutuhkan resource -> terblokir\r\n");
    printf("   3. MediumTask berjalan (preempt LowTask)\r\n");
    printf("   4. Akibatnya: task prioritas MENENGAH menghalangi\r\n");
    printf("      task prioritas TINGGI (inversion!)\r\n");
    printf("   5. LowTask akhirnya melepas resource\r\n");
    printf("   6. HighTask baru bisa berjalan\r\n");
    printf("---------------------------------------------------------\r\n");
    printf(" Solusi (dibahas di Modul 10):\r\n");
    printf("   - Mutex dengan Priority Inheritance\r\n");
    printf("   - Priority Ceiling Protocol\r\n");
    printf("=========================================================\r\n");
    printf("[DATA] INIT,high_prio=%d,med_prio=%d,low_prio=%d\r\n",
           HIGH_TASK_PRIORITY, MEDIUM_TASK_PRIORITY, LOW_TASK_PRIORITY);
    printf("\r\n");
}

/* =========================================================================
 * Low Priority Task (Prioritas 1)
 * Mengambil shared resource dan menahannya selama beberapa waktu
 * ========================================================================= */
static void vLowPriorityTask(void *pvParameters)
{
    (void)pvParameters;

    for (;;)
    {
        /* Tunggu sampai skenario aktif */
        while (!ucScenarioActive)
        {
            vTaskDelay(pdMS_TO_TICKS(10));
        }

        ulLowTaskRuns++;

        /* LED pattern: nyala steady (active low) */
        HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_RESET);

        /* ---- LANGKAH 1: Ambil resource ---- */
        vRecordEvent("LowTask", "ACQUIRE_RESOURCE", LOW_TASK_PRIORITY);
        printf("[LowTask] Mengambil shared resource (lock)\r\n");

        ucResourceLocked = 1;
        ucResourceOwner  = 1;  /* Low task owns it */

        /* Simulasi kerja dengan resource */
        printf("[LowTask] Bekerja dengan resource selama %d ms...\r\n",
               LOW_HOLD_TIME_MS);

        /* Busy-wait yang TIDAK melepas CPU (kritis!) */
        /* Tapi karena prioritas rendah, bisa di-preempt oleh task lain */
        uint32_t ulWorkStart = xTaskGetTickCount();
        uint32_t ulWorkTicks = pdMS_TO_TICKS(LOW_HOLD_TIME_MS);
        uint32_t ulChunkMs = 200; /* Kerja per chunk */

        while ((xTaskGetTickCount() - ulWorkStart) < ulWorkTicks)
        {
            /* Simulasi kerja - busy wait singkat lalu yield point */
            vBusyWait_ms(ulChunkMs);

            /* Cetak progres */
            uint32_t ulElapsed = xTaskGetTickCount() - ulWorkStart;
            uint32_t ulPercent = (ulElapsed * 100) / ulWorkTicks;
            printf("[LowTask] Kerja dengan resource: %lu%%\r\n",
                   (unsigned long)ulPercent);

            printf("[DATA] LOW_PROGRESS,pct=%lu,tick=%lu\r\n",
                   (unsigned long)ulPercent,
                   (unsigned long)xTaskGetTickCount());

            /* TaskYield memberi kesempatan task lain */
            taskYIELD();
        }

        /* ---- LANGKAH 5: Lepas resource ---- */
        vRecordEvent("LowTask", "RELEASE_RESOURCE", LOW_TASK_PRIORITY);
        printf("[LowTask] Melepas shared resource (unlock)\r\n");

        ucResourceLocked = 0;
        ucResourceOwner  = 0;

        HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET); /* LED mati */

        /* Tunggu skenario berikutnya */
        while (ucScenarioActive)
        {
            vTaskDelay(pdMS_TO_TICKS(50));
        }
    }
}

/* =========================================================================
 * Medium Priority Task (Prioritas 3)
 * CPU-bound task yang tidak membutuhkan shared resource
 * Berjalan saat HighTask terblokir (priority inversion!)
 * ========================================================================= */
static void vMediumPriorityTask(void *pvParameters)
{
    (void)pvParameters;

    for (;;)
    {
        /* Tunggu sampai skenario aktif */
        while (!ucScenarioActive)
        {
            vTaskDelay(pdMS_TO_TICKS(10));
        }

        /* Tunggu sebentar agar LowTask sempat mengambil resource */
        vTaskDelay(pdMS_TO_TICKS(300));

        ulMediumTaskRuns++;

        /* LED pattern: kedip lambat */
        vRecordEvent("MediumTask", "START_WORK", MEDIUM_TASK_PRIORITY);
        printf("[MediumTask] Mulai bekerja (CPU-bound, tanpa resource)\r\n");

        if (ucHighTaskBlocked)
        {
            printf("[MediumTask] !!! PRIORITY INVERSION TERJADI !!!\r\n");
            printf("[MediumTask] HighTask(prio=%d) sedang menunggu,\r\n",
                   HIGH_TASK_PRIORITY);
            printf("[MediumTask] tapi saya(prio=%d) yang berjalan!\r\n",
                   MEDIUM_TASK_PRIORITY);

            ucInversionDetected = 1;
            ulInversionStartCycle = ulGetDWTCycles();
        }

        /* Simulasi CPU-bound work */
        uint32_t ulWorkIterations = 8;
        for (uint32_t i = 0; i < ulWorkIterations; i++)
        {
            /* LED kedip lambat */
            HAL_GPIO_TogglePin(LED_PORT, LED_PIN);

            /* Busy work */
            vBusyWait_ms(150);

            printf("[MediumTask] Iterasi kerja %lu/%lu (HighTask %s)\r\n",
                   (unsigned long)(i + 1),
                   (unsigned long)ulWorkIterations,
                   ucHighTaskBlocked ? "BLOCKED" : "free");

            printf("[DATA] MEDIUM_WORK,iter=%lu,high_blocked=%d,tick=%lu\r\n",
                   (unsigned long)(i + 1),
                   ucHighTaskBlocked,
                   (unsigned long)xTaskGetTickCount());

            taskYIELD();
        }

        if (ucInversionDetected)
        {
            ulInversionEndCycle = ulGetDWTCycles();
            ulInversionDuration_us = (ulInversionEndCycle - ulInversionStartCycle)
                                     / (configCPU_CLOCK_HZ / 1000000);
        }

        vRecordEvent("MediumTask", "END_WORK", MEDIUM_TASK_PRIORITY);
        printf("[MediumTask] Selesai bekerja\r\n");

        HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET); /* LED mati */

        /* Tunggu skenario berikutnya */
        while (ucScenarioActive)
        {
            vTaskDelay(pdMS_TO_TICKS(50));
        }
    }
}

/* =========================================================================
 * High Priority Task (Prioritas 5)
 * Membutuhkan akses ke shared resource
 * TERBLOKIR jika resource dipegang LowTask
 * ========================================================================= */
static void vHighPriorityTask(void *pvParameters)
{
    (void)pvParameters;

    for (;;)
    {
        /* Tunggu sampai skenario aktif */
        while (!ucScenarioActive)
        {
            vTaskDelay(pdMS_TO_TICKS(10));
        }

        /* Tunggu agar LowTask sudah mengambil resource */
        vTaskDelay(pdMS_TO_TICKS(500));

        ulHighTaskRuns++;

        /* ---- LANGKAH 2: Coba akses resource ---- */
        vRecordEvent("HighTask", "REQUEST_RESOURCE", HIGH_TASK_PRIORITY);
        printf("[HighTask] Mencoba mengakses shared resource...\r\n");

        if (ucResourceLocked)
        {
            printf("[HighTask] RESOURCE TERKUNCI oleh LowTask!\r\n");
            printf("[HighTask] Harus menunggu... (TERBLOKIR)\r\n");

            ucHighTaskBlocked = 1;
            ulHighBlockedTick = xTaskGetTickCount();

            vRecordEvent("HighTask", "BLOCKED_BY_LOW", HIGH_TASK_PRIORITY);
            printf("[DATA] HIGH_BLOCKED,owner=%d,tick=%lu\r\n",
                   ucResourceOwner,
                   (unsigned long)xTaskGetTickCount());

            /* LED kedip cepat saat blocked */
            /* Polling-wait untuk resource (simulasi blocking) */
            uint32_t ulPollCount = 0;
            while (ucResourceLocked)
            {
                ulPollCount++;
                /* LED kedip cepat */
                if (ulPollCount % 5 == 0)
                {
                    HAL_GPIO_TogglePin(LED_PORT, LED_PIN);
                }

                if (ulPollCount % 20 == 0)
                {
                    printf("[HighTask] Masih menunggu resource... "
                           "(poll #%lu, %lu ms terblokir)\r\n",
                           (unsigned long)ulPollCount,
                           (unsigned long)(xTaskGetTickCount() - ulHighBlockedTick));
                }

                /* Delay singkat lalu cek lagi */
                vTaskDelay(pdMS_TO_TICKS(HIGH_CHECK_INTERVAL_MS));
            }

            ucHighTaskBlocked = 0;
            ulHighUnblockedTick = xTaskGetTickCount();

            printf("[HighTask] Resource tersedia setelah %lu ms!\r\n",
                   (unsigned long)(ulHighUnblockedTick - ulHighBlockedTick));

            vRecordEvent("HighTask", "UNBLOCKED", HIGH_TASK_PRIORITY);
        }

        /* ---- LANGKAH 6: Gunakan resource ---- */
        vRecordEvent("HighTask", "ACQUIRE_RESOURCE", HIGH_TASK_PRIORITY);
        printf("[HighTask] Mengambil dan menggunakan resource\r\n");

        ucResourceLocked = 1;
        ucResourceOwner  = 3;  /* High task owns it */

        /* LED nyala penuh saat bekerja */
        HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_RESET);

        /* Kerja cepat dengan resource */
        vBusyWait_ms(200);
        printf("[HighTask] Kerja dengan resource selesai\r\n");

        ucResourceLocked = 0;
        ucResourceOwner  = 0;

        vRecordEvent("HighTask", "RELEASE_RESOURCE", HIGH_TASK_PRIORITY);
        printf("[HighTask] Melepas resource\r\n");

        HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET);

        /* Tandai skenario selesai */
        printf("[DATA] HIGH_DONE,blocked_ms=%lu,tick=%lu\r\n",
               (unsigned long)(ulHighUnblockedTick - ulHighBlockedTick),
               (unsigned long)xTaskGetTickCount());

        /* Tunggu skenario berikutnya */
        while (ucScenarioActive)
        {
            vTaskDelay(pdMS_TO_TICKS(50));
        }
    }
}

/* =========================================================================
 * Orchestrator Task - Mengatur alur skenario demonstrasi
 * Prioritas tertinggi untuk kontrol penuh
 * ========================================================================= */
static void vOrchestratorTask(void *pvParameters)
{
    (void)pvParameters;

    vTaskDelay(pdMS_TO_TICKS(1000)); /* Tunggu stabilisasi */

    for (;;)
    {
        ulScenarioNumber++;

        printf("\r\n");
        printf("*********************************************************\r\n");
        printf("* SKENARIO %lu: Priority Inversion Demo\r\n",
               (unsigned long)ulScenarioNumber);
        printf("*********************************************************\r\n");
        printf("\r\n");
        printf("[DATA] SCENARIO_START,%lu,tick=%lu\r\n",
               (unsigned long)ulScenarioNumber,
               (unsigned long)xTaskGetTickCount());

        /* Reset variabel */
        vResetScenario();

        printf("[Orchestrator] Memulai skenario...\r\n");
        printf("[Orchestrator] Urutan yang diharapkan:\r\n");
        printf("  1. LowTask(prio=%d) mengambil resource\r\n", LOW_TASK_PRIORITY);
        printf("  2. HighTask(prio=%d) butuh resource -> blocked\r\n", HIGH_TASK_PRIORITY);
        printf("  3. MediumTask(prio=%d) berjalan (INVERSION!)\r\n", MEDIUM_TASK_PRIORITY);
        printf("  4. LowTask melepas resource\r\n");
        printf("  5. HighTask berjalan\r\n");
        printf("\r\n");

        /* Aktifkan skenario - semua task mulai bekerja */
        ucScenarioActive = 1;

        /* Tunggu skenario selesai (HighTask menyelesaikan pekerjaannya) */
        /* Timeout 15 detik */
        uint32_t ulTimeout = 15000;
        uint32_t ulStart = xTaskGetTickCount();

        while (ucScenarioActive)
        {
            vTaskDelay(pdMS_TO_TICKS(200));

            /* Cek apakah HighTask sudah selesai */
            if (ulHighTaskRuns > 0 && ucResourceOwner == 0 && !ucHighTaskBlocked)
            {
                /* Beri waktu untuk cleanup output */
                vTaskDelay(pdMS_TO_TICKS(500));
                ucScenarioActive = 0;
            }

            /* Timeout check */
            if ((xTaskGetTickCount() - ulStart) > pdMS_TO_TICKS(ulTimeout))
            {
                printf("[Orchestrator] Timeout! Memaksa akhir skenario\r\n");
                ucScenarioActive = 0;
            }
        }

        /* Cetak timeline */
        vPrintTimeline();

        /* Solusi: Priority Inheritance */
        printf("\r\n");
        printf("=========================================================\r\n");
        printf(" FASE 2: Solusi - Priority Inheritance (Konsep)\r\n");
        printf("=========================================================\r\n");
        printf(" Pada Modul 10 (Queue & Semaphore), kita akan belajar:\r\n");
        printf("\r\n");
        printf(" xSemaphoreCreateMutex() vs xSemaphoreCreateBinary():\r\n");
        printf("   - Mutex memiliki fitur Priority Inheritance\r\n");
        printf("   - Binary Semaphore TIDAK memilikinya\r\n");
        printf("\r\n");
        printf(" Cara kerja Priority Inheritance:\r\n");
        printf("   1. LowTask(prio=1) mengambil mutex\r\n");
        printf("   2. HighTask(prio=5) mencoba mengambil mutex\r\n");
        printf("   3. Kernel MENAIKKAN prioritas LowTask ke 5\r\n");
        printf("   4. LowTask berjalan dengan prioritas 5\r\n");
        printf("   5. MediumTask(prio=3) TIDAK bisa preempt\r\n");
        printf("   6. LowTask selesai, prioritas dikembalikan ke 1\r\n");
        printf("   7. HighTask langsung berjalan\r\n");
        printf("\r\n");
        printf(" Hasilnya: TIDAK ADA priority inversion!\r\n");
        printf("=========================================================\r\n");

        printf("[DATA] SCENARIO_END,%lu,inversion=%d,tick=%lu\r\n",
               (unsigned long)ulScenarioNumber,
               ucInversionDetected,
               (unsigned long)xTaskGetTickCount());

        /* Jeda antar skenario */
        printf("\r\n[Orchestrator] Jeda %d detik sebelum skenario berikutnya...\r\n",
               SCENARIO_PAUSE_MS / 1000);

        /* Statistik skenario */
        printf("[DATA] STATS,scenario=%lu,high_runs=%lu,med_runs=%lu,low_runs=%lu\r\n",
               (unsigned long)ulScenarioNumber,
               (unsigned long)ulHighTaskRuns,
               (unsigned long)ulMediumTaskRuns,
               (unsigned long)ulLowTaskRuns);

        vTaskDelay(pdMS_TO_TICKS(SCENARIO_PAUSE_MS));
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
    DWT_Init();

    /* Banner */
    vPrintBanner();

    /* Buat task */
    BaseType_t xResult;

    xResult = xTaskCreate(vLowPriorityTask, "LowTask",
                          TASK_STACK_SIZE, NULL,
                          LOW_TASK_PRIORITY, &xLowTaskHandle);
    if (xResult != pdPASS) { printf("[ERROR] Gagal membuat LowTask!\r\n"); Error_Handler(); }

    xResult = xTaskCreate(vMediumPriorityTask, "MediumTask",
                          TASK_STACK_SIZE, NULL,
                          MEDIUM_TASK_PRIORITY, &xMediumTaskHandle);
    if (xResult != pdPASS) { printf("[ERROR] Gagal membuat MediumTask!\r\n"); Error_Handler(); }

    xResult = xTaskCreate(vHighPriorityTask, "HighTask",
                          TASK_STACK_SIZE, NULL,
                          HIGH_TASK_PRIORITY, &xHighTaskHandle);
    if (xResult != pdPASS) { printf("[ERROR] Gagal membuat HighTask!\r\n"); Error_Handler(); }

    xResult = xTaskCreate(vOrchestratorTask, "Orchestrator",
                          ORCHESTRATOR_STACK_SIZE, NULL,
                          ORCHESTRATOR_PRIORITY, &xOrchestratorTaskHandle);
    if (xResult != pdPASS) { printf("[ERROR] Gagal membuat Orchestrator!\r\n"); Error_Handler(); }

    printf("[INFO] Semua task berhasil dibuat\r\n");
    printf("[INFO] Free Heap: %lu bytes\r\n",
           (unsigned long)xPortGetFreeHeapSize());
    printf("[INFO] Memulai scheduler...\r\n\r\n");

    /* Mulai scheduler */
    vTaskStartScheduler();

    printf("[ERROR] Scheduler berhenti!\r\n");
    for (;;);
}
