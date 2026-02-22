/* =========================================================================
 * STM32_10_Task_Communication
 * =========================================================================
 * JUDUL  : Demo Race Condition - Komunikasi Task via Global Variable
 *
 * DESKRIPSI:
 * Program ini SENGAJA mendemonstrasikan komunikasi antar-task yang TIDAK
 * AMAN menggunakan variabel global (shared struct) TANPA proteksi apapun
 * (tanpa mutex, tanpa queue, tanpa semaphore).
 *
 * TUJUAN PEMBELAJARAN:
 * Ini adalah setup untuk Modul 10 (Queue & Semaphore). Mahasiswa harus
 * melihat langsung MENGAPA kita butuh mekanisme sinkronisasi:
 *   1. Data corruption terjadi karena race condition
 *   2. Consumer membaca data yang sedang di-write oleh Producer
 *   3. Struct multi-field tidak atomik - bisa terbaca partial
 *   4. Solusinya ada di Modul 10: Queue atau Mutex
 *
 * =========================================================================
 * KONSEP RACE CONDITION
 * =========================================================================
 *
 *   MENGAPA TERJADI RACE CONDITION?
 *   ===============================
 *
 *   Pada ARM Cortex-M3, write ke variabel 32-bit ADALAH atomik.
 *   TAPI, write ke STRUCT yang berisi BANYAK field TIDAK atomik!
 *
 *   Producer menulis struct field-by-field:
 *
 *     shared.counter  = X;      <- instruksi 1
 *     shared.timestamp = Y;     <- instruksi 2
 *     shared.data[0]  = A;      <- instruksi 3
 *     shared.data[1]  = B;      <- instruksi 4
 *     ... (8 field data)
 *     shared.checksum = Z;      <- instruksi terakhir
 *
 *   MASALAH: Preemptive scheduler bisa INTERRUPT di antara instruksi!
 *
 *     Producer:  [counter=5] [timestamp=100] --PREEMPT-->
 *     Consumer:                              <-- reads all fields
 *                                            counter=5 (BARU)
 *                                            timestamp=100 (BARU)
 *                                            data[0]=old_A (LAMA!)  <- KORUPSI!
 *                                            data[1]=old_B (LAMA!)  <- KORUPSI!
 *                                            checksum=old_Z (LAMA!) <- KORUPSI!
 *     Producer:  -------------------- resumes -->
 *                [data[0]=A] [data[1]=B] [checksum=Z]
 *
 *   Consumer mendapat CAMPURAN data lama dan baru!
 *   Ini disebut TORN READ atau PARTIAL UPDATE.
 *
 * =========================================================================
 * ARSITEKTUR PROGRAM
 * =========================================================================
 *
 *    +-------------------------------------------------------------------+
 *    |                    SISTEM OVERVIEW (UNSAFE!)                       |
 *    +-------------------------------------------------------------------+
 *    |                                                                   |
 *    |   +------------------+          +------------------+              |
 *    |   |  vProducerTask   | WRITES   |  vConsumerTask   |              |
 *    |   |  (Priority 2)    |--------->|  (Priority 2)    |              |
 *    |   |                  |  SHARED  |                  |              |
 *    |   |  Menulis struct  |  GLOBAL  |  Membaca struct  |              |
 *    |   |  field-by-field  | VARIABLE |  & verifikasi    |              |
 *    |   +------------------+          +------------------+              |
 *    |              |                          |                         |
 *    |              |    +--------------+      |                         |
 *    |              +--->| SharedData_t |<-----+                         |
 *    |   TANPA MUTEX!    |  (GLOBAL)    |  TANPA LOCK!                   |
 *    |   TANPA QUEUE!    |              |  TANPA PROTEKSI!               |
 *    |                   | counter      |                                |
 *    |                   | timestamp    |                                |
 *    |   +----------+   | data[8]      |   +------------------+         |
 *    |   | CORRUPT! |   | checksum     |   |  vMonitorTask    |         |
 *    |   | TORN!    |   +--------------+   |  (Statistik)     |         |
 *    |   | PARTIAL! |                      +------------------+         |
 *    |   +----------+                                                    |
 *    |                                                                   |
 *    |   WARNING: INI CONTOH BURUK - JANGAN GUNAKAN DI PRODUKSI!         |
 *    |   Solusi benar: gunakan Queue atau Mutex (lihat Modul 10)         |
 *    |                                                                   |
 *    +-------------------------------------------------------------------+
 *
 * =========================================================================
 * DETEKSI KORUPSI DATA
 * =========================================================================
 *
 *   Kita menggunakan CHECKSUM sederhana untuk mendeteksi korupsi:
 *
 *     checksum = counter ^ timestamp ^ data[0] ^ data[1] ^ ... ^ data[7]
 *
 *   Producer menghitung checksum SETELAH menulis semua field.
 *   Consumer menghitung ulang checksum dan membandingkan.
 *
 *   Jika checksum TIDAK cocok -> data KORUP karena race condition!
 *
 *     Consistent (semua dari write ke-N):
 *       counter=N, timestamp=T, data=D, checksum = N^T^D  OK
 *
 *     Corrupt (campuran write ke-N dan ke-(N-1)):
 *       counter=N, timestamp=T_old, data=D_old, checksum=N^T^D  FAIL
 *                                   ^ dari write sebelumnya!
 *
 * =========================================================================
 * EXPECTED OUTPUT (UART 115200 baud)
 * =========================================================================
 *
 *   === Race Condition Demo (UNSAFE Communication) ===
 *
 *   [PROD] Write #1: cnt=1 ts=500 chk=0x12AB
 *   [CONS] Read  #1: cnt=1 ts=500 chk=0x12AB -> OK
 *
 *   [PROD] Write #5: cnt=5 ts=2500 chk=0x5678
 *   [CONS] Read  #5: cnt=5 ts=500 chk=0xABCD -> CORRUPT!
 *          Expected chk=0x5678, got=0xABCD (mismatch!)
 *
 *   [DATA]RACE cnt=5,corrupt=1,total=5,rate=20
 *
 * =========================================================================
 * Target     : STM32F103C8 (Blue Pill) - Cortex-M3 @ 72MHz
 * Framework  : STM32Cube HAL + FreeRTOS
 * UART       : 115200 baud, PA9=TX, PA10=RX
 * ========================================================================= */

/* -- Header Files -------------------------------------------------------- */
#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

/* =========================================================================
 * DEFINISI KONSTANTA
 * =========================================================================
 * Konstanta yang mengontrol behavior program dan timing task.
 * ========================================================================= */

/* Jumlah elemen data dalam shared struct.
 * Semakin banyak field, semakin BESAR peluang race condition,
 * karena write membutuhkan lebih banyak instruksi. */
#define SHARED_DATA_SIZE            8

/* Timing Producer Task.
 * SENGAJA dibuat cepat agar sering menulis dan memperbesar
 * peluang race condition. Dalam aplikasi nyata, ini bisa
 * mewakili sensor yang di-sample cepat. */
#define PRODUCER_DELAY_MS           20

/* Timing Consumer Task.
 * Sedikit berbeda dari producer untuk membuat timing offset
 * yang bervariasi, meningkatkan peluang torn read. */
#define CONSUMER_DELAY_MS           23

/* Timing Monitor Task untuk statistik. */
#define MONITOR_DELAY_MS            3000

/* Delay simulasi "pekerjaan berat" di producer.
 * Ini memperlambat penulisan field sehingga scheduler punya
 * waktu lebih banyak untuk preempt di tengah-tengah write.
 * TANPA delay ini, race condition lebih jarang karena
 * write selesai terlalu cepat. */
#define PRODUCER_WORK_DELAY_US      50

/* Task priorities - SAMA agar bisa saling preempt via time-slicing.
 * Jika prioritas berbeda, task prioritas tinggi akan selalu
 * berjalan duluan, mengurangi race condition. */
#define PRODUCER_TASK_PRIORITY      2
#define CONSUMER_TASK_PRIORITY      2
#define MONITOR_TASK_PRIORITY       3

/* Task stack sizes (dalam words, bukan bytes) */
#define PRODUCER_STACK_SIZE         256
#define CONSUMER_STACK_SIZE         256
#define MONITOR_STACK_SIZE          384

/* LED dan pin */
#define LED_PORT                    GPIOC
#define LED_PIN                     GPIO_PIN_13

/* Pattern khusus untuk mendeteksi korupsi tambahan.
 * Producer menulis pattern ini di awal data array.
 * Jika consumer tidak menemukan pattern ini, data korup. */
#define DATA_MAGIC_PATTERN          0xA5

/* =========================================================================
 * DEFINISI STRUCT SHARED DATA
 * =========================================================================
 *
 * Struct ini diakses oleh KEDUA task tanpa proteksi!
 *
 * ANALISIS UKURAN STRUCT:
 *   counter   : 4 byte (uint32_t)
 *   timestamp : 4 byte (uint32_t)
 *   sequence  : 4 byte (uint32_t)
 *   data[8]   : 8 byte (uint8_t x 8)
 *   checksum  : 4 byte (uint32_t)
 *   --------------------
 *   Total     : 24 byte (dengan padding mungkin lebih)
 *
 * MENGAPA INI MASALAH:
 * Pada Cortex-M3, HANYA akses 32-bit yang ALIGNED adalah atomik.
 * Tapi menulis 24 byte membutuhkan MINIMAL 6 instruksi STR/STRB.
 * Scheduler bisa preempt di ANTARA instruksi-instruksi tersebut!
 *
 * Layout memori (tanpa padding):
 *   +----------+----------+----------+------------------+----------+
 *   | counter  |timestamp | sequence |  data[0..7]      | checksum |
 *   | (4 byte) |(4 byte)  | (4 byte) |  (8 byte)        | (4 byte) |
 *   +----------+----------+----------+------------------+----------+
 *   Offset: 0     4          8          12                20
 * ========================================================================= */
typedef struct {
    volatile uint32_t counter;              /* Nomor urut write (1, 2, 3, ...) */
    volatile uint32_t timestamp;            /* Tick count saat write */
    volatile uint32_t sequence;             /* Sequence number untuk verifikasi */
    volatile uint8_t  data[SHARED_DATA_SIZE]; /* Array data payload */
    volatile uint32_t checksum;             /* XOR checksum semua field */
} SharedData_t;

/* =========================================================================
 * VARIABEL GLOBAL (SHARED - TIDAK TERPROTEKSI!)
 * =========================================================================
 *
 * PERINGATAN: Variabel-variabel di bawah ini diakses oleh
 * multiple task TANPA mekanisme sinkronisasi apapun!
 *
 * Ini adalah CONTOH BURUK yang sengaja dibuat untuk demonstrasi.
 * Dalam kode produksi, SELALU gunakan:
 *   - Queue (xQueueSend/xQueueReceive) untuk passing data
 *   - Mutex (xSemaphoreTake/xSemaphoreGive) untuk shared resource
 *   - Critical section (taskENTER_CRITICAL) untuk operasi singkat
 * ========================================================================= */

/* Struct utama yang di-share antara Producer dan Consumer.
 * Producer MENULIS, Consumer MEMBACA - secara bersamaan! */
static SharedData_t g_sharedData = {0};

/* Flag yang memberitahu Consumer bahwa ada data baru.
 * MASALAH: Meskipun flag ini uint32_t (atomik), data yang
 * ditunjuknya BISA belum selesai ditulis! */
static volatile uint32_t g_dataReady = 0;

/* Statistik korupsi - diupdate oleh Consumer */
static volatile uint32_t g_totalReads    = 0;  /* Total pembacaan */
static volatile uint32_t g_corruptCount  = 0;  /* Jumlah data korup */
static volatile uint32_t g_goodCount     = 0;  /* Jumlah data OK */
static volatile uint32_t g_totalWrites   = 0;  /* Total penulisan oleh Producer */

/* Statistik detail korupsi */
static volatile uint32_t g_checksumFail  = 0;  /* Checksum mismatch */
static volatile uint32_t g_sequenceFail  = 0;  /* Sequence mismatch */
static volatile uint32_t g_patternFail   = 0;  /* Pattern mismatch */

/* Timestamp terakhir korupsi */
static volatile uint32_t g_lastCorruptTick = 0;

/* System tick untuk monitoring */
static volatile uint32_t g_systemTick = 0;

/* -- Handle Peripheral --------------------------------------------------- */
static UART_HandleTypeDef huart1;

/* -- Handle Task --------------------------------------------------------- */
static TaskHandle_t xProducerHandle = NULL;
static TaskHandle_t xConsumerHandle = NULL;
static TaskHandle_t xMonitorHandle  = NULL;

/* -- Static memory untuk Idle task (configSUPPORT_STATIC_ALLOCATION) ----- */
static StaticTask_t xIdleTaskTCB;
static StackType_t  uxIdleTaskStack[configMINIMAL_STACK_SIZE];

/* -- Prototipe Fungsi ---------------------------------------------------- */
static void SystemClock_Config(void);
static void GPIO_Init(void);
static void UART1_Init(void);
static void Error_Handler(void);

static void vProducerTask(void *pvParameters);
static void vConsumerTask(void *pvParameters);
static void vMonitorTask(void *pvParameters);

static void vPrintBanner(void);
static void vPrintRaceExplanation(void);
static uint32_t ulCalculateChecksum(SharedData_t *pData);
static void vBusyDelayUs(uint32_t us);

/* =========================================================================
 * _write: Redirect printf ke UART1
 * =========================================================================
 * Implementasi syscall _write() agar printf() mengirim output
 * ke UART1. Ini memudahkan debugging via serial monitor.
 * ========================================================================= */
int _write(int file, char *ptr, int len)
{
    (void)file;
    HAL_UART_Transmit(&huart1, (uint8_t *)ptr, len, HAL_MAX_DELAY);
    return len;
}

/* =========================================================================
 * SysTick_Handler
 * =========================================================================
 * Handler interrupt SysTick yang dipanggil setiap 1ms.
 * Harus memanggil HAL_IncTick() untuk HAL dan xPortSysTickHandler()
 * untuk FreeRTOS scheduler.
 * ========================================================================= */
void SysTick_Handler(void)
{
    HAL_IncTick();
    g_systemTick++;

    if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED)
    {
        xPortSysTickHandler();
    }
}

/* =========================================================================
 * FreeRTOS Hook Functions
 * ========================================================================= */

/* Dipanggil jika pvPortMalloc() gagal (heap penuh) */
void vApplicationMallocFailedHook(void)
{
    printf("[ERROR] MALLOC FAILED! Heap penuh!\r\n");
    printf("[DATA]MALLOC_FAIL,tick=%lu\r\n", (unsigned long)g_systemTick);
    for (;;)
    {
        HAL_GPIO_TogglePin(LED_PORT, LED_PIN);
        HAL_Delay(100);
    }
}

/* Dipanggil jika stack overflow terdeteksi */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    printf("[ERROR] STACK OVERFLOW pada task: %s\r\n", pcTaskName);
    printf("[DATA]STACK_OVERFLOW,task=%s,tick=%lu\r\n",
           pcTaskName, (unsigned long)g_systemTick);
    for (;;)
    {
        HAL_GPIO_TogglePin(LED_PORT, LED_PIN);
        HAL_Delay(200);
    }
}

/* Menyediakan memori statis untuk Idle Task */
void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer,
                                   StackType_t **ppxIdleTaskStackBuffer,
                                   uint32_t *pulIdleTaskStackSize)
{
    *ppxIdleTaskTCBBuffer   = &xIdleTaskTCB;
    *ppxIdleTaskStackBuffer = uxIdleTaskStack;
    *pulIdleTaskStackSize   = configMINIMAL_STACK_SIZE;
}

/* =========================================================================
 * SystemClock_Config: HSE 8MHz -> PLL x9 -> 72MHz
 * =========================================================================
 * Konfigurasi clock system STM32F103:
 *   - HSE (External crystal) = 8 MHz
 *   - PLL multiplier = x9
 *   - SYSCLK = 72 MHz
 *   - AHB = 72 MHz (div1)
 *   - APB1 = 36 MHz (div2, max 36MHz)
 *   - APB2 = 72 MHz (div1)
 *   - Flash latency = 2 wait states (untuk 72MHz)
 * ========================================================================= */
static void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    /* Konfigurasi HSE + PLL */
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
 * GPIO_Init: LED PC13
 * ========================================================================= */
static void GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    /* LED PC13 - output push-pull */
    GPIO_InitStruct.Pin   = LED_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_PORT, &GPIO_InitStruct);
    HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET); /* LED mati (active low) */
}

/* =========================================================================
 * UART1_Init: PA9=TX, PA10=RX, 115200 baud 8N1
 * ========================================================================= */
static void UART1_Init(void)
{
    __HAL_RCC_USART1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* PA9 = TX (Alternate Function Push-Pull) */
    GPIO_InitStruct.Pin   = GPIO_PIN_9;
    GPIO_InitStruct.Mode  = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* PA10 = RX (Input floating) */
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
 * Error_Handler
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
 * vBusyDelayUs: Delay busy-wait dalam microseconds
 * =========================================================================
 * Delay sederhana menggunakan loop kosong. TIDAK presisi, tapi cukup
 * untuk memperlambat penulisan struct sehingga race condition lebih
 * sering terjadi. Pada 72MHz, ~9 iterasi = ~1us.
 * ========================================================================= */
static void vBusyDelayUs(uint32_t us)
{
    volatile uint32_t count = us * 9;
    while (count--)
    {
        __NOP();
    }
}

/* =========================================================================
 * ulCalculateChecksum: Hitung XOR checksum dari SharedData_t
 * =========================================================================
 * Menghitung checksum XOR dari semua field data (kecuali checksum itu
 * sendiri). Digunakan untuk mendeteksi apakah data konsisten.
 *
 * Jika Producer menulis semua field secara atomik, checksum akan cocok.
 * Jika Consumer membaca saat Producer masih menulis (race condition),
 * checksum TIDAK akan cocok karena campuran data lama dan baru.
 * ========================================================================= */
static uint32_t ulCalculateChecksum(SharedData_t *pData)
{
    uint32_t chk = 0;

    /* XOR counter */
    chk ^= pData->counter;

    /* XOR timestamp */
    chk ^= pData->timestamp;

    /* XOR sequence */
    chk ^= pData->sequence;

    /* XOR setiap byte data array */
    for (int i = 0; i < SHARED_DATA_SIZE; i++)
    {
        chk ^= ((uint32_t)pData->data[i] << ((i % 4) * 8));
    }

    return chk;
}

/* =========================================================================
 * vPrintBanner: Informasi program saat startup
 * ========================================================================= */
static void vPrintBanner(void)
{
    printf("\r\n");
    printf("=========================================================\r\n");
    printf(" STM32 FreeRTOS - Race Condition Demo\r\n");
    printf(" Program 10: Unsafe Task Communication (Global Variable)\r\n");
    printf("=========================================================\r\n");
    printf(" Target  : STM32F103C8 (Blue Pill) @ 72MHz\r\n");
    printf(" RTOS    : FreeRTOS (Preemptive, Time-Slicing)\r\n");
    printf(" UART    : 115200 baud (PA9=TX)\r\n");
    printf("---------------------------------------------------------\r\n");
    printf(" Konfigurasi Task:\r\n");
    printf("   ProducerTask : Prioritas %d - Menulis shared struct\r\n",
           PRODUCER_TASK_PRIORITY);
    printf("   ConsumerTask : Prioritas %d - Membaca & verifikasi\r\n",
           CONSUMER_TASK_PRIORITY);
    printf("   MonitorTask  : Prioritas %d - Statistik korupsi\r\n",
           MONITOR_TASK_PRIORITY);
    printf("---------------------------------------------------------\r\n");
    printf(" PERINGATAN: Program ini SENGAJA TIDAK AMAN!\r\n");
    printf(" Shared struct diakses TANPA mutex/queue!\r\n");
    printf(" Ini untuk demonstrasi race condition.\r\n");
    printf(" Solusi benar ada di Modul 10.\r\n");
    printf("---------------------------------------------------------\r\n");
    printf(" Shared Data: %u bytes, %d fields\r\n",
           (unsigned int)sizeof(SharedData_t), SHARED_DATA_SIZE + 3);
    printf(" Producer delay: %d ms, Consumer delay: %d ms\r\n",
           PRODUCER_DELAY_MS, CONSUMER_DELAY_MS);
    printf(" Write-delay per field: %d us (memperlambat write)\r\n",
           PRODUCER_WORK_DELAY_US);
    printf("=========================================================\r\n");
    printf("[DATA]INIT,prod_ms=%d,cons_ms=%d,data_size=%d,work_us=%d\r\n",
           PRODUCER_DELAY_MS, CONSUMER_DELAY_MS,
           SHARED_DATA_SIZE, PRODUCER_WORK_DELAY_US);
    printf("\r\n");
}

/* =========================================================================
 * vPrintRaceExplanation: Cetak penjelasan race condition
 * =========================================================================
 * Mencetak diagram dan penjelasan mengapa race condition terjadi.
 * Berguna untuk mahasiswa yang melihat output serial.
 * ========================================================================= */
static void vPrintRaceExplanation(void)
{
    printf("\r\n");
    printf("=========================================================\r\n");
    printf(" MENGAPA TERJADI DATA CORRUPTION?\r\n");
    printf("=========================================================\r\n");
    printf("\r\n");
    printf(" Producer menulis struct FIELD-BY-FIELD:\r\n");
    printf("   1. counter  = N\r\n");
    printf("   2. timestamp = T\r\n");
    printf("   3. sequence = S\r\n");
    printf("   4. data[0] = D0\r\n");
    printf("   5. data[1] = D1\r\n");
    printf("   ... (delay antar field!)\r\n");
    printf("   9. checksum = XOR(semua)\r\n");
    printf("\r\n");
    printf(" MASALAH:\r\n");
    printf(" Scheduler bisa PREEMPT di antara langkah 1-9!\r\n");
    printf(" Consumer bisa membaca saat hanya SEBAGIAN yang\r\n");
    printf(" sudah ditulis. Hasilnya: DATA CAMPURAN!\r\n");
    printf("\r\n");
    printf(" Timeline race condition:\r\n");
    printf("  Producer: [cnt=5][ts=T]--PREEMPT!-->[data][chk]\r\n");
    printf("  Consumer:              [reads all fields now!]\r\n");
    printf("            -> cnt=5(baru), data=old(lama) -> KORUP!\r\n");
    printf("\r\n");
    printf(" SOLUSI (Modul 10):\r\n");
    printf("   Queue: xQueueSend copies entire struct atomically\r\n");
    printf("   Mutex: Lock sebelum write, unlock setelah selesai\r\n");
    printf("   Critical Section: Disable interrupt sementara\r\n");
    printf("=========================================================\r\n");
    printf("[DATA]EXPLAIN,printed=1\r\n");
    printf("\r\n");
}

/* =========================================================================
 * PRODUCER TASK
 * =========================================================================
 * Task ini menulis data ke shared struct secara periodik.
 *
 * POIN KRITIS - MENGAPA INI UNSAFE:
 * 1. Struct ditulis FIELD-BY-FIELD (bukan satu operasi atomik)
 * 2. Ada delay SENGAJA antara penulisan field (vBusyDelayUs)
 *    untuk memperbesar "window of vulnerability"
 * 3. Tidak ada mutex, semaphore, atau critical section
 * 4. Consumer bisa preempt kapan saja dan membaca data parsial
 *
 * Dalam sistem real:
 * - Menulis struct bisa memakan 6-20 instruksi assembly
 * - Meskipun tanpa delay buatan, preemption TETAP bisa terjadi
 * - Ini terutama masalah pada struct besar atau CPU cepat dengan
 *   banyak task pada prioritas yang sama (time-slicing)
 * ========================================================================= */
static void vProducerTask(void *pvParameters)
{
    (void)pvParameters;

    uint32_t writeCount = 0;
    TickType_t xLastWakeTime = xTaskGetTickCount();

    printf("[PROD] Producer task dimulai\r\n");
    printf("[PROD] Delay: %d ms, Work delay: %d us/field\r\n",
           PRODUCER_DELAY_MS, PRODUCER_WORK_DELAY_US);

    for (;;)
    {
        writeCount++;

        /* ---------------------------------------------------------------
         * MULAI MENULIS KE SHARED STRUCT (TANPA PROTEKSI!)
         * ---------------------------------------------------------------
         * Setiap write di bawah ini adalah satu instruksi STR pada ARM.
         * Tapi KUMPULAN write ini BUKAN atomik!
         *
         * Di antara setiap write, scheduler BISA preempt task ini
         * dan memberikan CPU ke Consumer, yang akan membaca data
         * yang baru SETENGAH ditulis!
         * --------------------------------------------------------------- */

        /* Field 1: Counter - nomor urut write */
        g_sharedData.counter = writeCount;
        vBusyDelayUs(PRODUCER_WORK_DELAY_US);
        /* ^^^ Delay di sini memperbesar window untuk preemption.
         *     Pada hardware asli, delay ini tidak perlu ada, tapi
         *     race condition tetap bisa terjadi meskipun jarang. */

        /* Field 2: Timestamp - waktu saat write dimulai */
        g_sharedData.timestamp = (uint32_t)xTaskGetTickCount();
        vBusyDelayUs(PRODUCER_WORK_DELAY_US);

        /* Field 3: Sequence - harus sama dengan counter jika konsisten */
        g_sharedData.sequence = writeCount;
        vBusyDelayUs(PRODUCER_WORK_DELAY_US);

        /* Field 4-11: Data array - diisi pattern berdasarkan counter.
         * Pattern: setiap byte = (counter + index) & 0xFF
         * Consumer akan memverifikasi pattern ini. */
        for (int i = 0; i < SHARED_DATA_SIZE; i++)
        {
            g_sharedData.data[i] = (uint8_t)((writeCount + i) & 0xFF);
            /* Delay di antara setiap byte!
             * Ini membuat "jendela kerentanan" sangat besar. */
            if (i % 2 == 0)
            {
                vBusyDelayUs(PRODUCER_WORK_DELAY_US);
            }
        }

        /* Field terakhir: Checksum - dihitung dari semua field di atas.
         * Jika consumer membaca checksum yang cocok dengan data,
         * maka data MUNGKIN konsisten. Jika tidak cocok -> KORUP! */
        g_sharedData.checksum = ulCalculateChecksum((SharedData_t *)&g_sharedData);

        /* Update statistik total writes */
        g_totalWrites = writeCount;

        /* Set flag data ready.
         * MASALAH: Flag ini BISA di-set sebelum checksum selesai ditulis
         * ke memori (tergantung compiler optimization dan memory ordering).
         * Pada Cortex-M3 ini BIASANYA aman karena strongly-ordered memory,
         * tapi secara konsep tetap salah. */
        g_dataReady = writeCount;

        /* Cetak info setiap 50 write agar tidak membanjiri UART */
        if (writeCount % 50 == 0)
        {
            printf("[PROD] Write #%lu: cnt=%lu ts=%lu seq=%lu chk=0x%08lX\r\n",
                   (unsigned long)writeCount,
                   (unsigned long)g_sharedData.counter,
                   (unsigned long)g_sharedData.timestamp,
                   (unsigned long)g_sharedData.sequence,
                   (unsigned long)g_sharedData.checksum);
        }

        /* Toggle LED sebagai heartbeat producer */
        if (writeCount % 25 == 0)
        {
            HAL_GPIO_TogglePin(LED_PORT, LED_PIN);
        }

        /* Delay sampai waktu write berikutnya */
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(PRODUCER_DELAY_MS));
    }
}

/* =========================================================================
 * CONSUMER TASK
 * =========================================================================
 * Task ini membaca shared struct dan MEMVERIFIKASI konsistensi data.
 *
 * CARA DETEKSI KORUPSI:
 * 1. CHECKSUM: Hitung ulang XOR checksum, bandingkan dengan yang
 *    tersimpan. Jika tidak cocok -> data korup.
 *
 * 2. SEQUENCE: counter dan sequence harus sama (keduanya diisi
 *    dengan writeCount). Jika berbeda -> data korup.
 *
 * 3. PATTERN: data[i] harus == (counter + i) & 0xFF.
 *    Jika tidak -> data korup (terbaca dari write sebelumnya).
 *
 * MENGAPA CONSUMER BISA BACA DATA KORUP:
 * Consumer dan Producer punya prioritas SAMA (time-slicing).
 * FreeRTOS scheduler bisa memberikan time-slice ke Consumer
 * saat Producer baru menulis SEBAGIAN field struct.
 * ========================================================================= */
static void vConsumerTask(void *pvParameters)
{
    (void)pvParameters;

    uint32_t readCount      = 0;
    uint32_t lastDataReady  = 0;
    uint32_t localCorrupt   = 0;
    uint32_t localGood      = 0;
    uint32_t consecutiveOK  = 0;

    /* Variabel lokal untuk menyimpan salinan data yang dibaca */
    uint32_t readCounter;
    uint32_t readTimestamp;
    uint32_t readSequence;
    uint8_t  readData[SHARED_DATA_SIZE];
    uint32_t readChecksum;
    uint32_t calcChecksum;

    printf("[CONS] Consumer task dimulai\r\n");
    printf("[CONS] Delay: %d ms\r\n", CONSUMER_DELAY_MS);
    printf("[CONS] Verifikasi: checksum + sequence + pattern\r\n\r\n");

    for (;;)
    {
        /* Cek apakah ada data baru dari producer */
        if (g_dataReady != lastDataReady)
        {
            lastDataReady = g_dataReady;
            readCount++;

            /* -----------------------------------------------------------
             * MEMBACA SHARED STRUCT (TANPA LOCK!)
             * -----------------------------------------------------------
             * Kita membaca field satu per satu. Selama pembacaan ini,
             * Producer BISA menulis dan mengubah field yang BELUM
             * kita baca. Hasilnya: kita mendapat campuran data dari
             * dua write yang berbeda!
             *
             * Bahkan meng-copy ke variabel lokal TIDAK membantu
             * jika copy dilakukan field-by-field (seperti di bawah).
             * Yang dibutuhkan adalah PROTEKSI selama SELURUH copy.
             *
             * Solusi benar: taskENTER_CRITICAL() / taskEXIT_CRITICAL()
             * atau gunakan queue yang meng-copy atomik.
             * ----------------------------------------------------------- */
            readCounter   = g_sharedData.counter;
            readTimestamp  = g_sharedData.timestamp;
            readSequence  = g_sharedData.sequence;

            for (int i = 0; i < SHARED_DATA_SIZE; i++)
            {
                readData[i] = g_sharedData.data[i];
            }

            readChecksum = g_sharedData.checksum;

            /* -----------------------------------------------------------
             * VERIFIKASI KONSISTENSI DATA
             * -----------------------------------------------------------
             * Tiga level pengecekan:
             * ----------------------------------------------------------- */

            uint8_t isCorrupt = 0;
            char    corruptReason[64] = {0};

            /* Cek 1: Checksum verification */
            {
                /* Buat struct sementara untuk hitung checksum */
                SharedData_t tmpData;
                tmpData.counter   = readCounter;
                tmpData.timestamp = readTimestamp;
                tmpData.sequence  = readSequence;
                memcpy((void *)tmpData.data, readData, SHARED_DATA_SIZE);
                tmpData.checksum  = 0; /* Tidak digunakan dalam kalkulasi */

                calcChecksum = ulCalculateChecksum(&tmpData);
            }

            if (calcChecksum != readChecksum)
            {
                isCorrupt = 1;
                g_checksumFail++;
                snprintf(corruptReason, sizeof(corruptReason),
                         "CHECKSUM exp=0x%08lX got=0x%08lX",
                         (unsigned long)readChecksum,
                         (unsigned long)calcChecksum);
            }

            /* Cek 2: Sequence consistency
             * counter dan sequence diisi dari writeCount yang sama.
             * Jika berbeda, Producer ter-preempt di antara kedua write. */
            if (readCounter != readSequence)
            {
                isCorrupt = 1;
                g_sequenceFail++;
                if (corruptReason[0] == '\0')
                {
                    snprintf(corruptReason, sizeof(corruptReason),
                             "SEQUENCE cnt=%lu seq=%lu",
                             (unsigned long)readCounter,
                             (unsigned long)readSequence);
                }
            }

            /* Cek 3: Data pattern verification
             * data[i] harus == (counter + i) & 0xFF */
            for (int i = 0; i < SHARED_DATA_SIZE; i++)
            {
                uint8_t expected = (uint8_t)((readCounter + i) & 0xFF);
                if (readData[i] != expected)
                {
                    isCorrupt = 1;
                    g_patternFail++;
                    if (corruptReason[0] == '\0')
                    {
                        snprintf(corruptReason, sizeof(corruptReason),
                                 "PATTERN data[%d] exp=%u got=%u",
                                 i, expected, readData[i]);
                    }
                    break; /* Satu saja cukup untuk tahu ada korupsi */
                }
            }

            /* -----------------------------------------------------------
             * UPDATE STATISTIK & CETAK HASIL
             * ----------------------------------------------------------- */
            if (isCorrupt)
            {
                localCorrupt++;
                g_corruptCount = localCorrupt;
                g_lastCorruptTick = (uint32_t)xTaskGetTickCount();
                consecutiveOK = 0;

                /* Cetak detail korupsi */
                printf("[CONS] Read #%lu: cnt=%lu ts=%lu seq=%lu -> CORRUPT!\r\n",
                       (unsigned long)readCount,
                       (unsigned long)readCounter,
                       (unsigned long)readTimestamp,
                       (unsigned long)readSequence);
                printf("       Reason: %s\r\n", corruptReason);

                /* Cetak [DATA] tagged output untuk parser */
                uint32_t rate = 0;
                if (readCount > 0)
                {
                    rate = (localCorrupt * 100) / readCount;
                }
                printf("[DATA]RACE cnt=%lu,corrupt=%lu,total=%lu,rate=%lu\r\n",
                       (unsigned long)readCounter,
                       (unsigned long)localCorrupt,
                       (unsigned long)readCount,
                       (unsigned long)rate);

                printf("[DATA]CORRUPT_DETAIL,type=%s,chk_fail=%lu,seq_fail=%lu,pat_fail=%lu\r\n",
                       (calcChecksum != readChecksum) ? "checksum" :
                       (readCounter != readSequence) ? "sequence" : "pattern",
                       (unsigned long)g_checksumFail,
                       (unsigned long)g_sequenceFail,
                       (unsigned long)g_patternFail);
            }
            else
            {
                localGood++;
                g_goodCount = localGood;
                consecutiveOK++;

                /* Cetak setiap 100 reads OK atau saat baru pulih dari korupsi */
                if (readCount % 100 == 0 || consecutiveOK == 1)
                {
                    printf("[CONS] Read #%lu: cnt=%lu ts=%lu -> OK",
                           (unsigned long)readCount,
                           (unsigned long)readCounter,
                           (unsigned long)readTimestamp);
                    if (consecutiveOK == 1 && localCorrupt > 0)
                    {
                        printf(" (recovered after corruption)");
                    }
                    printf("\r\n");
                }
            }

            /* Update total reads */
            g_totalReads = readCount;
        }

        /* Delay sebelum membaca lagi.
         * Delay SENGAJA berbeda dari Producer (23 vs 20 ms)
         * agar timing offset bervariasi dan race condition
         * terjadi pada titik yang berbeda-beda dalam struct. */
        vTaskDelay(pdMS_TO_TICKS(CONSUMER_DELAY_MS));
    }
}

/* =========================================================================
 * MONITOR TASK
 * =========================================================================
 * Task ini mencetak statistik race condition secara periodik.
 * Memberikan ringkasan berapa banyak data yang korup vs OK,
 * corruption rate, dan detail jenis korupsi.
 * ========================================================================= */
static void vMonitorTask(void *pvParameters)
{
    (void)pvParameters;

    uint32_t monitorCycle = 0;
    uint32_t prevCorrupt  = 0;
    uint32_t prevTotal    = 0;

    printf("[MON] Monitor task dimulai (interval: %d ms)\r\n\r\n",
           MONITOR_DELAY_MS);

    /* Tunggu sebentar agar Producer dan Consumer mulai dulu */
    vTaskDelay(pdMS_TO_TICKS(2000));

    for (;;)
    {
        monitorCycle++;

        /* Baca statistik saat ini */
        uint32_t writes  = g_totalWrites;
        uint32_t reads   = g_totalReads;
        uint32_t corrupt = g_corruptCount;
        uint32_t good    = g_goodCount;
        uint32_t chkFail = g_checksumFail;
        uint32_t seqFail = g_sequenceFail;
        uint32_t patFail = g_patternFail;

        /* Hitung delta dari monitor sebelumnya */
        uint32_t deltaCorrupt = corrupt - prevCorrupt;
        uint32_t deltaTotal   = reads - prevTotal;
        prevCorrupt = corrupt;
        prevTotal   = reads;

        /* Hitung rates */
        uint32_t overallRate = 0;
        uint32_t recentRate  = 0;

        if (reads > 0)
        {
            overallRate = (corrupt * 100) / reads;
        }
        if (deltaTotal > 0)
        {
            recentRate = (deltaCorrupt * 100) / deltaTotal;
        }

        /* Cetak header statistik */
        printf("\r\n");
        printf("=========================================================\r\n");
        printf("  RACE CONDITION MONITOR #%lu\r\n",
               (unsigned long)monitorCycle);
        printf("=========================================================\r\n");
        printf("  Writes total    : %lu\r\n", (unsigned long)writes);
        printf("  Reads total     : %lu\r\n", (unsigned long)reads);
        printf("  Data OK         : %lu\r\n", (unsigned long)good);
        printf("  Data CORRUPT    : %lu\r\n", (unsigned long)corrupt);
        printf("  Corruption rate : %lu%% (overall)\r\n",
               (unsigned long)overallRate);
        printf("  Recent rate     : %lu%% (last %lu reads)\r\n",
               (unsigned long)recentRate, (unsigned long)deltaTotal);
        printf("---------------------------------------------------------\r\n");
        printf("  Detail korupsi:\r\n");
        printf("    Checksum fail : %lu\r\n", (unsigned long)chkFail);
        printf("    Sequence fail : %lu\r\n", (unsigned long)seqFail);
        printf("    Pattern fail  : %lu\r\n", (unsigned long)patFail);
        printf("---------------------------------------------------------\r\n");

        /* Heap info */
        printf("  Free heap       : %lu bytes\r\n",
               (unsigned long)xPortGetFreeHeapSize());
        printf("  Min heap ever   : %lu bytes\r\n",
               (unsigned long)xPortGetMinimumEverFreeHeapSize());

        /* Stack high water mark */
        if (xProducerHandle != NULL)
        {
            printf("  Producer stack  : %lu words free\r\n",
                   (unsigned long)uxTaskGetStackHighWaterMark(xProducerHandle));
        }
        if (xConsumerHandle != NULL)
        {
            printf("  Consumer stack  : %lu words free\r\n",
                   (unsigned long)uxTaskGetStackHighWaterMark(xConsumerHandle));
        }

        printf("=========================================================\r\n");

        /* [DATA] tagged output untuk parser Python */
        printf("[DATA]RACE cnt=%lu,corrupt=%lu,total=%lu,rate=%lu\r\n",
               (unsigned long)writes,
               (unsigned long)corrupt,
               (unsigned long)reads,
               (unsigned long)overallRate);

        printf("[DATA]MONITOR,cycle=%lu,writes=%lu,reads=%lu,"
               "corrupt=%lu,good=%lu,overall=%lu,recent=%lu,"
               "chk=%lu,seq=%lu,pat=%lu,heap=%lu\r\n",
               (unsigned long)monitorCycle,
               (unsigned long)writes,
               (unsigned long)reads,
               (unsigned long)corrupt,
               (unsigned long)good,
               (unsigned long)overallRate,
               (unsigned long)recentRate,
               (unsigned long)chkFail,
               (unsigned long)seqFail,
               (unsigned long)patFail,
               (unsigned long)xPortGetFreeHeapSize());

        /* Pesan interpretasi untuk mahasiswa */
        if (corrupt == 0 && reads > 50)
        {
            printf("\r\n");
            printf("[MON] Belum ada korupsi terdeteksi.\r\n");
            printf("[MON] Ini TIDAK berarti program aman! Race condition\r\n");
            printf("[MON] bisa terjadi kapan saja secara non-deterministik.\r\n");
            printf("[MON] Tunggu lebih lama atau kurangi delay.\r\n");
        }
        else if (corrupt > 0)
        {
            printf("\r\n");
            printf("[MON] KORUPSI TERDETEKSI! %lu dari %lu reads (%lu%%)\r\n",
                   (unsigned long)corrupt, (unsigned long)reads,
                   (unsigned long)overallRate);
            printf("[MON] Ini BUKTI bahwa shared variable tanpa proteksi\r\n");
            printf("[MON] menghasilkan data yang TIDAK KONSISTEN.\r\n");
            printf("[MON] Lihat Modul 10 untuk solusi: Queue & Mutex.\r\n");
        }

        printf("\r\n");

        vTaskDelay(pdMS_TO_TICKS(MONITOR_DELAY_MS));
    }
}

/* =========================================================================
 * MAIN
 * =========================================================================
 * Fungsi utama: inisialisasi hardware, cetak banner, buat tasks,
 * dan mulai scheduler FreeRTOS.
 * ========================================================================= */
int main(void)
{
    /* Inisialisasi HAL (SysTick, Flash, dll) */
    HAL_Init();

    /* Konfigurasi system clock ke 72MHz */
    SystemClock_Config();

    /* Inisialisasi GPIO (LED) */
    GPIO_Init();

    /* Inisialisasi UART1 untuk serial output */
    UART1_Init();

    /* Cetak banner program */
    vPrintBanner();

    /* Cetak penjelasan race condition */
    vPrintRaceExplanation();

    /* Inisialisasi shared data ke nol */
    memset((void *)&g_sharedData, 0, sizeof(SharedData_t));

    printf("[MAIN] Membuat tasks...\r\n\r\n");

    /* -----------------------------------------------------------------
     * Buat Producer Task
     * -----------------------------------------------------------------
     * Producer menulis ke shared struct dengan prioritas 2.
     * ----------------------------------------------------------------- */
    BaseType_t xResult;

    xResult = xTaskCreate(
        vProducerTask,              /* Fungsi task */
        "Producer",                 /* Nama task */
        PRODUCER_STACK_SIZE,        /* Stack size (words) */
        NULL,                       /* Parameter */
        PRODUCER_TASK_PRIORITY,     /* Prioritas */
        &xProducerHandle            /* Handle */
    );

    if (xResult != pdPASS)
    {
        printf("[ERROR] Gagal membuat Producer task!\r\n");
        Error_Handler();
    }
    printf("[MAIN] Producer task dibuat (prio=%d, stack=%d)\r\n",
           PRODUCER_TASK_PRIORITY, PRODUCER_STACK_SIZE);

    /* -----------------------------------------------------------------
     * Buat Consumer Task
     * -----------------------------------------------------------------
     * Consumer membaca shared struct dengan prioritas SAMA (2).
     * Prioritas sama = time-slicing aktif = race condition lebih sering.
     * ----------------------------------------------------------------- */
    xResult = xTaskCreate(
        vConsumerTask,
        "Consumer",
        CONSUMER_STACK_SIZE,
        NULL,
        CONSUMER_TASK_PRIORITY,
        &xConsumerHandle
    );

    if (xResult != pdPASS)
    {
        printf("[ERROR] Gagal membuat Consumer task!\r\n");
        Error_Handler();
    }
    printf("[MAIN] Consumer task dibuat (prio=%d, stack=%d)\r\n",
           CONSUMER_TASK_PRIORITY, CONSUMER_STACK_SIZE);

    /* -----------------------------------------------------------------
     * Buat Monitor Task
     * -----------------------------------------------------------------
     * Monitor punya prioritas lebih tinggi (3) agar selalu bisa
     * mencetak statistik meskipun Producer/Consumer sibuk.
     * ----------------------------------------------------------------- */
    xResult = xTaskCreate(
        vMonitorTask,
        "Monitor",
        MONITOR_STACK_SIZE,
        NULL,
        MONITOR_TASK_PRIORITY,
        &xMonitorHandle
    );

    if (xResult != pdPASS)
    {
        printf("[ERROR] Gagal membuat Monitor task!\r\n");
        Error_Handler();
    }
    printf("[MAIN] Monitor task dibuat (prio=%d, stack=%d)\r\n",
           MONITOR_TASK_PRIORITY, MONITOR_STACK_SIZE);

    printf("\r\n");
    printf("[MAIN] Semua task siap. Memulai scheduler...\r\n");
    printf("[MAIN] Race condition akan terlihat di output CONS.\r\n");
    printf("[MAIN] Perhatikan tag [DATA]RACE untuk statistik.\r\n");
    printf("\r\n");

    /* Mulai FreeRTOS scheduler - fungsi ini tidak pernah return */
    vTaskStartScheduler();

    /* Jika sampai sini, berarti heap tidak cukup untuk idle task */
    printf("[ERROR] Scheduler gagal dimulai! Heap tidak cukup?\r\n");

    for (;;)
    {
        Error_Handler();
    }
}
