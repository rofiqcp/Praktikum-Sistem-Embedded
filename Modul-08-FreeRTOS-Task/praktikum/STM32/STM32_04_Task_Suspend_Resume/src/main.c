/**
 * ============================================================================
 * FILE: main.c
 * PROJECT: 05-Task_Suspend_Resume
 * 
 * DESKRIPSI PROGRAM:
 * Program ini mendemonstrasikan mekanisme Suspend dan Resume task di FreeRTOS.
 * Suspend adalah cara untuk menghentikan sementara eksekusi task tanpa
 * menghapusnya dari sistem (task tetap ada di memori).
 * 
 * ============================================================================
 * KONSEP DASAR: TASK STATE DAN SUSPEND/RESUME
 * ============================================================================
 * 
 * TASK STATE DI FREERTOS:
 * ┌─────────────────────────────────────────────────────────────────┐
 * │                                                                 │
 * │    ┌─────────┐    vTaskSuspend()    ┌───────────┐               │
 * │    │  READY  │ ──────────────────── │ SUSPENDED │               │
 * │    └────┬────┘                      └─────┬─────┘               │
 * │         │                                 │                     │
 * │  Scheduler                         vTaskResume()                │
 * │  memilih                                  │                     │
 * │         ▼                                 │                     │
 * │    ┌─────────┐                            │                     │
 * │    │ RUNNING │ ◄──────────────────────────┘                     │
 * │    └────┬────┘                                                  │
 * │         │                                                       │
 * │  vTaskDelay()                                                   │
 * │  atau wait                                                      │
 * │         ▼                                                       │
 * │    ┌─────────┐                                                  │
 * │    │ BLOCKED │                                                  │
 * │    └─────────┘                                                  │
 * │                                                                 │
 * └─────────────────────────────────────────────────────────────────┘
 * 
 * SUSPENDED vs BLOCKED:
 * - BLOCKED: Task menunggu event/waktu, akan otomatis ready saat event terjadi
 * - SUSPENDED: Task dihentikan paksa, hanya bisa ready via vTaskResume()
 * 
 * SKENARIO DEMO:
 * 1. Processor Task memproses data terus-menerus
 * 2. Supervisor Task mengawasi dan mensimulasikan deteksi error
 * 3. Saat "error", Supervisor suspend Processor
 * 4. Setelah "error clear", Supervisor resume Processor
 * 5. Button bisa digunakan untuk manual suspend/resume
 * 
 * HARDWARE TARGET: STM32F103C8T6 (Blue Pill)
 * - CPU: 72MHz (HSE 8MHz + PLL x9)
 * - LED: PC13 (aktif LOW) - Toggle oleh Processor
 * - Button: PA0 (pull-up) - Manual kontrol
 * - UART: PA9 (TX), PA10 (RX) @ 115200 baud
 * ============================================================================
 */

/* ============================================================================
 * INCLUDE HEADERS
 * ============================================================================ */

#include "stm32f1xx_hal.h"      /* HAL library untuk STM32F1 series */
#include "FreeRTOS.h"          /* FreeRTOS kernel utama */
#include "task.h"              /* API untuk task management */
#include <string.h>            /* Library untuk strlen() */
#include <stdio.h>             /* Library untuk sprintf() */

/* ============================================================================
 * VARIABEL GLOBAL
 * ============================================================================ */

/* Handle UART untuk komunikasi serial debug */
UART_HandleTypeDef huart1;

/* ---------------------------------------------------------------------------
 * TASK HANDLES - KUNCI UNTUK SUSPEND/RESUME
 * ---------------------------------------------------------------------------
 * TaskHandle_t adalah pointer ke Task Control Block (TCB).
 * Digunakan untuk:
 * - vTaskSuspend(handle): Suspend task tertentu
 * - vTaskResume(handle): Resume task tertentu
 * - eTaskGetState(handle): Cek state task
 * 
 * Handle didapat saat xTaskCreate() dengan menyimpan ke parameter terakhir.
 * --------------------------------------------------------------------------- */
TaskHandle_t xDataProcessorHandle = NULL;  /* Handle untuk Processor Task */
TaskHandle_t xSupervisorHandle = NULL;     /* Handle untuk Supervisor Task */

/* Flag error untuk sinkronisasi */
volatile uint8_t errorFlag = 0;            /* 1 = ada error, 0 = normal */

/* ============================================================================
 * DEKLARASI FUNGSI (FUNCTION PROTOTYPES)
 * ============================================================================ */

/* Fungsi konfigurasi sistem dan peripheral */
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART1_UART_Init(void);

/* Fungsi utilitas */
void UART_SendString(const char *str);
const char* GetTaskStateString(eTaskState state);

/* Task FreeRTOS */
void vDataProcessorTask(void *pvParameters);  /* Task yang diproses datanya */
void vSupervisorTask(void *pvParameters);     /* Task pengawas */
void vButtonMonitorTask(void *pvParameters);  /* Task monitor tombol */

/* FreeRTOS hook functions */
void vApplicationMallocFailedHook(void);
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName);

/* ============================================================================
 * FUNGSI UTILITAS: UART_SendString
 * ============================================================================ */
void UART_SendString(const char *str) {
    HAL_UART_Transmit(&huart1, (uint8_t*)str, strlen(str), HAL_MAX_DELAY);
}

/* ============================================================================
 * FUNGSI UTILITAS: GetTaskStateString
 * ============================================================================
 * Mengkonversi eTaskState enum ke string yang bisa dibaca.
 * 
 * eTaskState adalah enum yang berisi:
 * - eRunning: Task sedang eksekusi di CPU
 * - eReady: Task siap dijalankan, menunggu giliran
 * - eBlocked: Task menunggu event (delay, semaphore, queue, dll)
 * - eSuspended: Task di-suspend secara paksa
 * - eDeleted: Task sudah dihapus (handle masih ada untuk sementara)
 * ============================================================================ */
const char* GetTaskStateString(eTaskState state) {
    switch(state) {
        case eRunning:   return "Running";    /* Sedang berjalan */
        case eReady:     return "Ready";      /* Siap dijalankan */
        case eBlocked:   return "Blocked";    /* Menunggu event */
        case eSuspended: return "SUSPENDED";  /* Di-suspend */
        case eDeleted:   return "Deleted";    /* Sudah dihapus */
        default:         return "Unknown";    /* State tidak dikenal */
    }
}

/* ============================================================================
 * TASK 1: vDataProcessorTask - TASK YANG BISA DI-SUSPEND
 * ============================================================================
 * Task ini mensimulasikan pemrosesan data yang berjalan terus-menerus.
 * Task ini bisa di-suspend oleh Supervisor atau secara manual via button.
 * 
 * KARAKTERISTIK:
 * - Prioritas: 2 (medium)
 * - LED toggle setiap iterasi untuk indikator aktivitas
 * - Saat di-suspend, LED berhenti berkedip
 * - Saat di-resume, melanjutkan dari titik terakhir
 * 
 * PENTING:
 * Saat task di-suspend di tengah eksekusi, state (variabel lokal, register)
 * tetap tersimpan. Saat di-resume, eksekusi dilanjutkan dari titik suspend.
 * ============================================================================ */
void vDataProcessorTask(void *pvParameters) {
    /* ---------------------------------------------------------------------------
     * VARIABEL LOKAL
     * --------------------------------------------------------------------------- */
    uint32_t dataCounter = 0;      /* Counter untuk tracking data yang diproses */
    char buffer[80];               /* Buffer untuk output UART */
    
    /* Cast parameter yang tidak digunakan */
    (void)pvParameters;
    
    UART_SendString("[PROCESSOR] Task dimulai\r\n");
    UART_SendString("[PROCESSOR] LED berkedip = task aktif\r\n");
    UART_SendString("[PROCESSOR] LED diam = task SUSPENDED\r\n\r\n");
    
    /* ---------------------------------------------------------------------------
     * INFINITE LOOP
     * --------------------------------------------------------------------------- */
    for(;;) {
        dataCounter++;
        
        /* -----------------------------------------------------------------------
         * SIMULASI PEMROSESAN DATA
         * -----------------------------------------------------------------------
         * Dalam aplikasi nyata, ini bisa berupa:
         * - Parsing data dari sensor
         * - Encoding/decoding
         * - Komputasi matematis
         * - Protocol handling
         * ----------------------------------------------------------------------- */
        volatile uint32_t i;
        for(i = 0; i < PROCESSING_SIMULATION_LOOPS; i++) {
            /* Simulasi kerja CPU */
        }
        
        /* Cetak status pemrosesan */
        sprintf(buffer, "[PROCESSOR] Memproses data paket #%lu\r\n", dataCounter);
        UART_SendString(buffer);
        
        /* -----------------------------------------------------------------------
         * TOGGLE LED - INDIKATOR TASK AKTIF
         * -----------------------------------------------------------------------
         * LED berkedip menunjukkan task sedang berjalan.
         * Saat task di-suspend, LED akan berhenti berkedip.
         * Ini berguna untuk visual debugging - jika LED diam, task suspended.
         * ----------------------------------------------------------------------- */
        HAL_GPIO_TogglePin(LED_GPIO_PORT, LED_GPIO_PIN);
        
        /* Delay sebelum iterasi berikutnya */
        vTaskDelay(pdMS_TO_TICKS(PROCESSOR_TASK_PERIOD_MS));
        
        /* -----------------------------------------------------------------------
         * CATATAN TENTANG SUSPEND SAAT DELAY:
         * -----------------------------------------------------------------------
         * Task bisa di-suspend saat:
         * 1. Sedang berjalan (di loop for di atas)
         * 2. Sedang blocked di vTaskDelay()
         * 
         * Jika di-suspend saat blocked, task tetap suspended setelah delay
         * selesai. Task hanya akan ready kembali setelah di-resume.
         * ----------------------------------------------------------------------- */
    }
}

/* ============================================================================
 * TASK 2: vSupervisorTask - TASK PENGAWAS
 * ============================================================================
 * Task ini mengawasi sistem dan mengontrol Processor Task.
 * Mensimulasikan deteksi error dan recovery.
 * 
 * ALUR SIMULASI:
 * 1. Setiap ERROR_TRIGGER_CYCLE iterasi: simulate error, suspend processor
 * 2. Setelah ERROR_CLEAR_OFFSET iterasi lagi: clear error, resume processor
 * 3. Report status processor setiap iterasi
 * 
 * PRIORITAS:
 * Lebih tinggi dari Processor (3 vs 2) agar selalu bisa preempt
 * dan melakukan suspend kapan saja.
 * ============================================================================ */
void vSupervisorTask(void *pvParameters) {
    /* ---------------------------------------------------------------------------
     * VARIABEL LOKAL
     * --------------------------------------------------------------------------- */
    uint32_t cycleCount = 0;       /* Counter cycle untuk timing error */
    char buffer[100];              /* Buffer untuk output UART */
    eTaskState processorState;     /* State dari Processor Task */
    
    /* Cast parameter yang tidak digunakan */
    (void)pvParameters;
    
    UART_SendString("[SUPERVISOR] Task dimulai\r\n");
    sprintf(buffer, "[SUPERVISOR] Simulasi error setiap %d cycle\r\n", ERROR_TRIGGER_CYCLE);
    UART_SendString(buffer);
    sprintf(buffer, "[SUPERVISOR] Auto-resume setelah %d cycle dari error\r\n\r\n", 
            ERROR_CLEAR_OFFSET);
    UART_SendString(buffer);
    
    /* ---------------------------------------------------------------------------
     * INFINITE LOOP
     * --------------------------------------------------------------------------- */
    for(;;) {
        cycleCount++;
        
        /* -----------------------------------------------------------------------
         * CEK STATE PROCESSOR TASK
         * -----------------------------------------------------------------------
         * eTaskGetState() mengembalikan state task saat ini.
         * 
         * PENTING: Fungsi ini memerlukan INCLUDE_eTaskGetState = 1
         * 
         * Kegunaan:
         * - Monitoring status task
         * - Menentukan apakah perlu resume atau suspend
         * - Debugging dan logging
         * ----------------------------------------------------------------------- */
        processorState = eTaskGetState(xDataProcessorHandle);
        
        /* Format dan cetak status */
        sprintf(buffer, "[SUPERVISOR] Cycle: %lu, Processor: %s\r\n", 
                cycleCount, 
                GetTaskStateString(processorState));
        UART_SendString(buffer);
        
        /* -----------------------------------------------------------------------
         * SIMULASI DETEKSI ERROR - TRIGGER SUSPEND
         * -----------------------------------------------------------------------
         * Setiap ERROR_TRIGGER_CYCLE iterasi, kita "detect error" dan
         * suspend Processor Task.
         * 
         * vTaskSuspend(TaskHandle_t xTaskToSuspend):
         * - Memindahkan task ke state SUSPENDED
         * - Task tidak akan dijadwalkan sampai di-resume
         * - Jika parameter NULL, suspend task yang memanggil (diri sendiri)
         * - Bisa dipanggil dari task manapun dengan prioritas apapun
         * 
         * CATATAN:
         * Kita cek processorState != eSuspended untuk mencegah
         * suspend task yang sudah suspended (tidak ada efek, tapi tidak perlu).
         * ----------------------------------------------------------------------- */
        if((cycleCount % ERROR_TRIGGER_CYCLE == 0) && (processorState != eSuspended)) {
            /* Set flag error */
            errorFlag = 1;
            
            /* Cetak pesan error */
            UART_SendString("\r\n");
            UART_SendString("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\r\n");
            UART_SendString("!!! ERROR TERDETEKSI - SUSPEND PROCESSOR !!!\r\n");
            UART_SendString("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\r\n\r\n");
            
            /* ===================================================================
             * SUSPEND PROCESSOR TASK
             * ===================================================================
             * Ini adalah fungsi kunci dari demo!
             * 
             * Setelah dipanggil:
             * - Processor Task berhenti eksekusi
             * - LED berhenti berkedip
             * - State processor menjadi eSuspended
             * - Task tetap ada di memori, tidak dihapus
             * =================================================================== */
            vTaskSuspend(xDataProcessorHandle);
        }
        
        /* -----------------------------------------------------------------------
         * SIMULASI CLEAR ERROR - TRIGGER RESUME
         * -----------------------------------------------------------------------
         * Setelah ERROR_CLEAR_OFFSET cycle dari trigger error,
         * kita "clear error" dan resume Processor Task.
         * 
         * vTaskResume(TaskHandle_t xTaskToResume):
         * - Memindahkan task dari SUSPENDED ke READY
         * - Task akan dijadwalkan saat prioritasnya tertinggi
         * - Eksekusi dilanjutkan dari titik suspend
         * - Tidak ada efek jika task tidak dalam state SUSPENDED
         * ----------------------------------------------------------------------- */
        if((cycleCount % ERROR_TRIGGER_CYCLE == ERROR_CLEAR_OFFSET) && 
           (processorState == eSuspended)) {
            /* Clear flag error */
            errorFlag = 0;
            
            /* Cetak pesan recovery */
            UART_SendString("\r\n");
            UART_SendString(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\r\n");
            UART_SendString(">>> ERROR CLEARED - RESUME PROCESSOR >>>\r\n");
            UART_SendString(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\r\n\r\n");
            
            /* ===================================================================
             * RESUME PROCESSOR TASK
             * ===================================================================
             * Setelah dipanggil:
             * - Processor Task menjadi READY
             * - Scheduler akan menjalankannya saat giliran
             * - LED mulai berkedip lagi
             * - Eksekusi dilanjutkan dari titik terakhir (setelah vTaskDelay)
             * =================================================================== */
            vTaskResume(xDataProcessorHandle);
        }
        
        /* Delay sampai iterasi berikutnya */
        vTaskDelay(pdMS_TO_TICKS(SUPERVISOR_TASK_PERIOD_MS));
    }
}

/* ============================================================================
 * TASK 3: vButtonMonitorTask - KONTROL MANUAL
 * ============================================================================
 * Task ini memonitor tombol PA0 untuk kontrol manual suspend/resume.
 * 
 * FUNGSI:
 * - Tekan tombol saat Processor running/ready -> Suspend
 * - Tekan tombol saat Processor suspended -> Resume
 * 
 * DEBOUNCING:
 * Tombol mekanik menghasilkan bouncing (osilasi) saat ditekan/dilepas.
 * Kita gunakan delay untuk menunggu bouncing selesai.
 * ============================================================================ */
void vButtonMonitorTask(void *pvParameters) {
    /* ---------------------------------------------------------------------------
     * VARIABEL LOKAL
     * --------------------------------------------------------------------------- */
    uint8_t lastButtonState = 1;   /* State tombol terakhir (1 = tidak ditekan) */
    uint8_t currentButtonState;    /* State tombol saat ini */
    eTaskState processorState;     /* State Processor Task */
    
    /* Cast parameter yang tidak digunakan */
    (void)pvParameters;
    
    /* ---------------------------------------------------------------------------
     * INFINITE LOOP
     * --------------------------------------------------------------------------- */
    for(;;) {
        /* Baca state tombol saat ini */
        currentButtonState = HAL_GPIO_ReadPin(BUTTON_GPIO_PORT, BUTTON_GPIO_PIN);
        
        /* -----------------------------------------------------------------------
         * DETEKSI FALLING EDGE (TOMBOL DITEKAN)
         * -----------------------------------------------------------------------
         * Kita deteksi transisi dari HIGH ke LOW (falling edge).
         * lastButtonState = 1 (tidak ditekan)
         * currentButtonState = 0 (ditekan)
         * ----------------------------------------------------------------------- */
        if(lastButtonState == 1 && currentButtonState == 0) {
            /* Debounce delay - tunggu bouncing selesai */
            vTaskDelay(pdMS_TO_TICKS(BUTTON_DEBOUNCE_MS));
            
            /* Baca ulang untuk konfirmasi */
            currentButtonState = HAL_GPIO_ReadPin(BUTTON_GPIO_PORT, BUTTON_GPIO_PIN);
            
            /* Jika masih ditekan (LOW), proses toggle */
            if(currentButtonState == 0) {
                /* Cek state Processor Task */
                processorState = eTaskGetState(xDataProcessorHandle);
                
                if(processorState == eSuspended) {
                    /* Processor suspended -> Resume */
                    UART_SendString("\r\n[BUTTON] Manual Resume! Processor dilanjutkan\r\n\r\n");
                    vTaskResume(xDataProcessorHandle);
                } else {
                    /* Processor running/ready/blocked -> Suspend */
                    UART_SendString("\r\n[BUTTON] Manual Suspend! Processor dihentikan\r\n\r\n");
                    vTaskSuspend(xDataProcessorHandle);
                }
            }
        }
        
        /* Simpan state untuk deteksi edge berikutnya */
        lastButtonState = currentButtonState;
        
        /* Polling rate - cukup cepat untuk responsif, tidak terlalu cepat untuk hemat CPU */
        vTaskDelay(pdMS_TO_TICKS(BUTTON_TASK_PERIOD_MS));
    }
}

/* ============================================================================
 * FUNGSI: SystemClock_Config
 * ============================================================================
 * Mengkonfigurasi system clock STM32F103 ke 72MHz.
 * ============================================================================ */
void SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    /* Konfigurasi HSE dan PLL */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
    HAL_RCC_OscConfig(&RCC_OscInitStruct);

    /* Konfigurasi clock system */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK |
                                  RCC_CLOCKTYPE_SYSCLK |
                                  RCC_CLOCKTYPE_PCLK1 |
                                  RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2);
}

/* ============================================================================
 * FUNGSI: MX_GPIO_Init
 * ============================================================================
 * Menginisialisasi GPIO untuk LED dan Button.
 * ============================================================================ */
static void MX_GPIO_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* Enable clock untuk GPIOC dan GPIOA */
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    /* -------------------------------------------------------------------------
     * KONFIGURASI LED PC13
     * ------------------------------------------------------------------------- */
    HAL_GPIO_WritePin(LED_GPIO_PORT, LED_GPIO_PIN, GPIO_PIN_SET);  /* LED mati awal */
    
    GPIO_InitStruct.Pin = LED_GPIO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_GPIO_PORT, &GPIO_InitStruct);

    /* -------------------------------------------------------------------------
     * KONFIGURASI BUTTON PA0
     * -------------------------------------------------------------------------
     * Mode: Input dengan internal pull-up
     * Saat tidak ditekan: HIGH (karena pull-up)
     * Saat ditekan: LOW (terhubung ke GND)
     * ------------------------------------------------------------------------- */
    GPIO_InitStruct.Pin = BUTTON_GPIO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;         /* Internal pull-up aktif */
    HAL_GPIO_Init(BUTTON_GPIO_PORT, &GPIO_InitStruct);
}

/* ============================================================================
 * FUNGSI: MX_USART1_UART_Init
 * ============================================================================
 * Menginisialisasi USART1 untuk komunikasi serial debugging.
 * ============================================================================ */
static void MX_USART1_UART_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* Enable clock */
    __HAL_RCC_USART1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    /* PA9 = TX */
    GPIO_InitStruct.Pin = DEBUG_UART_TX_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(DEBUG_UART_TX_PORT, &GPIO_InitStruct);
    
    /* PA10 = RX */
    GPIO_InitStruct.Pin = DEBUG_UART_RX_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(DEBUG_UART_RX_PORT, &GPIO_InitStruct);

    /* Konfigurasi USART1 */
    huart1.Instance = DEBUG_UART_INSTANCE;
    huart1.Init.BaudRate = DEBUG_UART_BAUDRATE;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(&huart1);
}

/* ============================================================================
 * FUNGSI: vApplicationMallocFailedHook
 * ============================================================================
 */
void vApplicationMallocFailedHook(void) {
    UART_SendString("\r\n!!! FATAL: Malloc gagal! !!!\r\n");
    taskDISABLE_INTERRUPTS();
    for(;;) {
        HAL_GPIO_TogglePin(LED_GPIO_PORT, LED_GPIO_PIN);
        for(volatile uint32_t i = 0; i < 100000; i++);
    }
}

/* ============================================================================
 * FUNGSI: vApplicationStackOverflowHook
 * ============================================================================
 */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
    (void)xTask;
    UART_SendString("\r\n!!! FATAL: Stack Overflow pada: ");
    UART_SendString(pcTaskName);
    UART_SendString(" !!!\r\n");
    taskDISABLE_INTERRUPTS();
    for(;;) {
        HAL_GPIO_TogglePin(LED_GPIO_PORT, LED_GPIO_PIN);
        for(volatile uint32_t i = 0; i < 50000; i++);
    }
}

/* ============================================================================
 * FUNGSI: SysTick_Handler
 * ============================================================================
 */
void SysTick_Handler(void) {
    HAL_IncTick();
    if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED) {
        xPortSysTickHandler();
    }
}

/* ============================================================================
 * FUNGSI: main
 * ============================================================================
 * Entry point program.
 * ============================================================================ */
int main(void) {
    /* Inisialisasi HAL dan peripheral */
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USART1_UART_Init();

    /* Pesan startup */
    UART_SendString("\r\n");
    UART_SendString("============================================================\r\n");
    UART_SendString("  Program 5: Task Suspend & Resume\r\n");
    UART_SendString("============================================================\r\n");
    UART_SendString("\r\n");
    UART_SendString("DESKRIPSI:\r\n");
    UART_SendString("Demonstrasi mekanisme suspend dan resume task.\r\n");
    UART_SendString("\r\n");
    UART_SendString("KONTROL:\r\n");
    UART_SendString("- Otomatis: Supervisor simulasi error setiap 10 cycle\r\n");
    UART_SendString("- Manual: Tekan tombol PA0 untuk toggle suspend/resume\r\n");
    UART_SendString("\r\n");
    UART_SendString("INDIKATOR:\r\n");
    UART_SendString("- LED berkedip = Processor Task aktif\r\n");
    UART_SendString("- LED diam = Processor Task SUSPENDED\r\n");
    UART_SendString("\r\n");
    UART_SendString("============================================================\r\n\r\n");

    /* ---------------------------------------------------------------------------
     * MEMBUAT TASK FREERTOS
     * ---------------------------------------------------------------------------
     * PENTING: Simpan handle untuk bisa suspend/resume task tersebut.
     * Parameter terakhir xTaskCreate adalah pointer untuk menyimpan handle.
     * --------------------------------------------------------------------------- */
    
    /* Processor Task - bisa di-suspend */
    BaseType_t result1 = xTaskCreate(
        vDataProcessorTask,
        PROCESSOR_TASK_NAME,
        PROCESSOR_TASK_STACK_SIZE,
        NULL,
        PROCESSOR_TASK_PRIORITY,
        &xDataProcessorHandle         /* SIMPAN HANDLE! */
    );
    
    /* Supervisor Task - mengontrol Processor */
    BaseType_t result2 = xTaskCreate(
        vSupervisorTask,
        SUPERVISOR_TASK_NAME,
        SUPERVISOR_TASK_STACK_SIZE,
        NULL,
        SUPERVISOR_TASK_PRIORITY,
        &xSupervisorHandle            /* Simpan handle (opsional) */
    );
    
    /* Button Task - kontrol manual */
    BaseType_t result3 = xTaskCreate(
        vButtonMonitorTask,
        BUTTON_TASK_NAME,
        BUTTON_TASK_STACK_SIZE,
        NULL,
        BUTTON_TASK_PRIORITY,
        NULL                          /* Handle tidak disimpan */
    );
    
    /* Cek hasil pembuatan task */
    if(result1 != pdPASS || result2 != pdPASS || result3 != pdPASS) {
        UART_SendString("ERROR: Gagal membuat task!\r\n");
        while(1) {
            HAL_GPIO_TogglePin(LED_GPIO_PORT, LED_GPIO_PIN);
            HAL_Delay(100);
        }
    }
    
    UART_SendString("Task berhasil dibuat. Memulai scheduler...\r\n\r\n");

    /* Start FreeRTOS scheduler */
    vTaskStartScheduler();

    /* Seharusnya tidak sampai sini */
    UART_SendString("ERROR: Scheduler gagal start!\r\n");
    while(1) {
        HAL_GPIO_TogglePin(LED_GPIO_PORT, LED_GPIO_PIN);
        HAL_Delay(500);
    }
}
