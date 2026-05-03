/**
 * ============================================================================
 * FILE: main.c
 * PROJECT: 04-Idle_Task_Hook
 * 
 * DESKRIPSI PROGRAM:
 * Program ini mendemonstrasikan penggunaan Idle Task Hook di FreeRTOS.
 * Idle Hook adalah callback yang dipanggil setiap kali tidak ada task yang
 * perlu dijalankan (semua task dalam state BLOCKED atau SUSPENDED).
 * 
 * ============================================================================
 * KONSEP DASAR: IDLE TASK DAN IDLE HOOK
 * ============================================================================
 * 
 * IDLE TASK:
 * FreeRTOS secara otomatis membuat task khusus bernama "Idle Task" dengan
 * prioritas terendah (tskIDLE_PRIORITY = 0). Idle task berjalan ketika
 * tidak ada task lain yang ready.
 * 
 * KAPAN IDLE TASK BERJALAN:
 * ┌──────────┐     ┌──────────┐     ┌──────────┐
 * │  Worker  │     │  Worker  │     │  Worker  │
 * │  Running │     │  Running │     │  Running │
 * └────┬─────┘     └────┬─────┘     └────┬─────┘
 *      │vTaskDelay()    │vTaskDelay()    │vTaskDelay()
 *      ▼                ▼                ▼
 * ┌─────────────────────────────────────────────┐
 * │        IDLE TASK BERJALAN (hook dipanggil)   │
 * └─────────────────────────────────────────────┘
 * 
 * KEGUNAAN IDLE HOOK:
 * 1. Power Saving - Masuk ke mode sleep menggunakan __WFI()
 * 2. Background Processing - Garbage collection, cleanup
 * 3. Monitoring - Menghitung idle time untuk estimasi CPU usage
 * 
 * HARDWARE TARGET: STM32F103C8T6 (Blue Pill)
 * - CPU: 72MHz (HSE 8MHz + PLL x9)
 * - LED: PC13 (aktif LOW) - Toggle oleh Worker Task
 * - UART: PA9 (TX), PA10 (RX) @ 115200 baud
 * ============================================================================
 */

/* ============================================================================
 * INCLUDE HEADERS
 * ============================================================================ */

#if defined(STM32F103xB)
#include "stm32f1xx_hal.h"
#elif defined(STM32F401xC) || defined(STM32F411xE)
#include "stm32f4xx_hal.h"
#endif
#include "FreeRTOS.h"          /* FreeRTOS kernel utama */
#include "task.h"              /* API untuk task management */
#include <string.h>            /* Library untuk strlen() */
#include <stdio.h>             /* Library untuk sprintf() */
extern void xPortSysTickHandler(void);

/* ============================================================================
 * VARIABEL GLOBAL
 * ============================================================================
 * Variabel volatile karena diakses dari beberapa context:
 * - Idle hook (dari idle task)
 * - Monitor task (untuk membaca statistik)
 * ============================================================================ */

/* Handle UART untuk komunikasi serial debug */
UART_HandleTypeDef huart1;

/* ---------------------------------------------------------------------------
 * COUNTER STATISTIK IDLE
 * ---------------------------------------------------------------------------
 * idleCounter: Menghitung berapa kali idle hook dipanggil
 * sleepCounter: Menghitung berapa kali CPU masuk mode sleep
 * 
 * volatile diperlukan karena:
 * - Dimodifikasi oleh idle hook (context berbeda)
 * - Dibaca oleh monitor task
 * - Compiler tidak boleh mengoptimasi akses
 * --------------------------------------------------------------------------- */
volatile uint32_t idleCounter = 0;   /* Total panggilan idle hook */
volatile uint32_t sleepCounter = 0;  /* Total masuk sleep mode */

/* ============================================================================
 * DEKLARASI FUNGSI (FUNCTION PROTOTYPES)
 * ============================================================================ */

/* Fungsi konfigurasi sistem dan peripheral */
void SystemClock_Config(void);               /* Konfigurasi clock system 72MHz */
static void MX_GPIO_Init(void);              /* Inisialisasi GPIO (LED) */
static void MX_USART1_UART_Init(void);       /* Inisialisasi UART debug */

/* Fungsi utilitas */
void UART_SendString(const char *str);       /* Kirim string via UART */

/* Task FreeRTOS */
void vWorkerTask(void *pvParameters);        /* Task yang melakukan kerja periodik */
void vMonitorTask(void *pvParameters);       /* Task yang melaporkan statistik idle */

/* FreeRTOS hook functions - WAJIB diimplementasikan jika config aktif */
void vApplicationIdleHook(void);             /* HOOK UTAMA - dipanggil saat idle */
void vApplicationMallocFailedHook(void);     /* Dipanggil jika malloc gagal */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName);

/* ============================================================================
 * FUNGSI: vApplicationIdleHook - INTI DARI DEMO INI
 * ============================================================================
 * Callback yang dipanggil FreeRTOS setiap kali idle task berjalan.
 * 
 * KARAKTERISTIK:
 * - Dipanggil dari context idle task (prioritas terendah)
 * - Tidak boleh memanggil fungsi blocking (vTaskDelay, xQueueReceive, dll)
 * - Harus return cepat agar scheduler bisa berjalan normal
 * - Bisa dipanggil ribuan kali per detik tergantung workload
 * 
 * IMPLEMENTASI DI SINI:
 * 1. Increment counter untuk statistik
 * 2. Masuk ke sleep mode menggunakan __WFI()
 * 
 * __WFI() (Wait For Interrupt):
 * - Instruksi ARM yang menghentikan eksekusi CPU
 * - CPU tidur sampai ada interrupt (termasuk SysTick)
 * - Sangat hemat daya karena CPU tidak clock saat tidur
 * - Bangun otomatis saat ada interrupt apa saja
 * 
 * ESTIMASI POWER SAVING:
 * - STM32F103 @ 72MHz: ~36mA aktif
 * - Sleep mode: ~14mA (hemat ~60%)
 * - Stop mode: ~20µA (perlu konfigurasi lebih lanjut)
 * ============================================================================ */
void vApplicationIdleHook(void) {
    /* ---------------------------------------------------------------------------
     * INCREMENT COUNTER STATISTIK
     * ---------------------------------------------------------------------------
     * idleCounter: Menghitung total panggilan
     * sleepCounter: Menghitung masuk sleep (sama dalam kasus ini)
     * 
     * Dalam aplikasi nyata, bisa berbeda jika ada kondisi dimana
     * tidak selalu masuk sleep (misalnya ada flag yang harus dicek).
     * --------------------------------------------------------------------------- */
    idleCounter++;      /* Hitung panggilan idle hook */
    sleepCounter++;     /* Hitung masuk sleep mode */
    
    /* ---------------------------------------------------------------------------
     * MASUK SLEEP MODE - HEMAT DAYA
     * ---------------------------------------------------------------------------
     * __WFI() adalah intrinsic function untuk instruksi WFI (Wait For Interrupt).
     * 
     * CARA KERJA:
     * 1. CPU berhenti mengeksekusi instruksi
     * 2. Clock CPU dihentikan (tapi peripheral clock tetap jalan)
     * 3. CPU bangun saat ada interrupt (SysTick, UART, GPIO, dll)
     * 4. Eksekusi dilanjutkan dari instruksi setelah WFI
     * 
     * KEUNTUNGAN:
     * - Hemat daya signifikan
     * - Wake-up instant (< 6 clock cycles)
     * - Tidak perlu konfigurasi khusus
     * 
     * ALTERNATIF:
     * __WFE() - Wait For Event (lebih flexible, bisa bangun dari event flag)
     * --------------------------------------------------------------------------- */
    __WFI();  /* Wait For Interrupt - CPU tidur sampai ada interrupt */
    
    /* ---------------------------------------------------------------------------
     * CATATAN PENTING:
     * ---------------------------------------------------------------------------
     * 1. JANGAN panggil vTaskDelay() atau fungsi blocking lainnya!
     *    Idle hook berjalan di context idle task, tidak boleh block.
     * 
     * 2. JANGAN lakukan operasi yang terlalu lama!
     *    Ini akan menunda respons terhadap event lain.
     * 
     * 3. JANGAN alokasi memori dinamis!
     *    Bisa menyebabkan fragmentation atau failure.
     * 
     * 4. BISA lakukan:
     *    - Increment counter
     *    - Set flag
     *    - Toggle GPIO (untuk debugging)
     *    - Masuk sleep mode
     *    - Background cleanup yang cepat
     * --------------------------------------------------------------------------- */
}

/* ============================================================================
 * FUNGSI UTILITAS: UART_SendString
 * ============================================================================
 * Mengirim string melalui UART1 untuk debugging.
 * ============================================================================ */
void UART_SendString(const char *str) {
    HAL_UART_Transmit(&huart1,                /* Handle UART */
                      (uint8_t*)str,          /* Data yang dikirim */
                      strlen(str),            /* Panjang data */
                      HAL_MAX_DELAY);         /* Timeout (blocking) */
}

/* ============================================================================
 * TASK 1: vWorkerTask - TASK YANG MELAKUKAN KERJA PERIODIK
 * ============================================================================
 * Task ini mensimulasikan workload yang berjalan periodik.
 * Saat task ini delay (vTaskDelay), idle task akan berjalan.
 * 
 * ALUR:
 * 1. Lakukan "kerja" (simulasi dengan loop)
 * 2. Toggle LED sebagai indikator
 * 3. Delay 500ms -> idle task berjalan selama ini
 * 4. Repeat
 * 
 * HUBUNGAN DENGAN IDLE HOOK:
 * Semakin lama delay, semakin banyak idle hook dipanggil.
 * Semakin lama "kerja", semakin sedikit idle hook dipanggil.
 * ============================================================================ */
void vWorkerTask(void *pvParameters) {
    /* ---------------------------------------------------------------------------
     * VARIABEL LOKAL
     * --------------------------------------------------------------------------- */
    uint32_t counter = 0;          /* Counter iterasi */
    char buffer[60];               /* Buffer untuk output UART */
    
    /* Cast parameter yang tidak digunakan untuk menghindari warning */
    (void)pvParameters;
    
    UART_SendString("[Worker] Task dimulai\r\n");
    UART_SendString("[Worker] Periode kerja: ");
    char tmpBuf[20];
    sprintf(tmpBuf, "%d ms\r\n\r\n", WORKER_TASK_PERIOD_MS);
    UART_SendString(tmpBuf);
    
    /* ---------------------------------------------------------------------------
     * INFINITE LOOP
     * --------------------------------------------------------------------------- */
    for(;;) {
        counter++;
        
        /* -----------------------------------------------------------------------
         * SIMULASI WORKLOAD
         * -----------------------------------------------------------------------
         * Loop kosong untuk mensimulasikan CPU bekerja.
         * Selama loop ini berjalan, idle task TIDAK berjalan.
         * ----------------------------------------------------------------------- */
        volatile uint32_t i;
        for(i = 0; i < WORKER_SIMULATION_LOOPS; i++) {
            /* CPU sibuk di sini - idle hook tidak dipanggil */
        }
        
        /* -----------------------------------------------------------------------
         * TOGGLE LED - INDIKATOR KERJA
         * -----------------------------------------------------------------------
         * LED berkedip setiap iterasi menunjukkan task aktif.
         * Frekuensi kedip = 1/(2*WORKER_TASK_PERIOD_MS) Hz
         * Dengan periode 500ms: frekuensi = 1 Hz
         * ----------------------------------------------------------------------- */
        HAL_GPIO_TogglePin(LED_GPIO_PORT, LED_GPIO_PIN);
        
        /* -----------------------------------------------------------------------
         * LAPORAN PERIODIK
         * ----------------------------------------------------------------------- */
        if(counter % WORKER_TASK_REPORT_INTERVAL == 0) {
            sprintf(buffer, "[Worker] Iterasi: %lu selesai\r\n", counter);
            UART_SendString(buffer);
        }
        
        /* -----------------------------------------------------------------------
         * DELAY - SAAT INI IDLE TASK BERJALAN!
         * -----------------------------------------------------------------------
         * Saat vTaskDelay() dipanggil:
         * 1. Worker task masuk state BLOCKED
         * 2. Scheduler mencari task lain yang ready
         * 3. Jika tidak ada task ready, idle task berjalan
         * 4. Idle hook dipanggil berulang kali
         * 5. CPU masuk sleep mode setiap panggilan idle hook
         * 6. Setelah delay selesai, worker task menjadi ready lagi
         * ----------------------------------------------------------------------- */
        vTaskDelay(pdMS_TO_TICKS(WORKER_TASK_PERIOD_MS));
    }
}

/* ============================================================================
 * TASK 2: vMonitorTask - TASK YANG MELAPORKAN STATISTIK IDLE
 * ============================================================================
 * Task ini membaca counter statistik dan melaporkan ke UART.
 * Berguna untuk:
 * - Memantau seberapa sering idle hook dipanggil
 * - Estimasi CPU utilization (lebih banyak idle = CPU lebih santai)
 * 
 * CARA HITUNG CPU UTILIZATION:
 * Jika dalam 1 detik idle hook dipanggil N kali, dan dalam kondisi
 * benar-benar idle (tidak ada task) dipanggil M kali, maka:
 * CPU Utilization = (1 - N/M) * 100%
 * ============================================================================ */
void vMonitorTask(void *pvParameters) {
    /* ---------------------------------------------------------------------------
     * VARIABEL LOKAL
     * --------------------------------------------------------------------------- */
    char buffer[120];              /* Buffer untuk output UART */
    uint32_t lastIdle = 0;         /* Nilai idle counter terakhir */
    uint32_t lastSleep = 0;        /* Nilai sleep counter terakhir */
    uint32_t currentIdle;          /* Nilai idle counter saat ini */
    uint32_t currentSleep;         /* Nilai sleep counter saat ini */
    uint32_t deltaIdle;            /* Perubahan idle counter */
    uint32_t deltaSleep;           /* Perubahan sleep counter */
    
    /* Cast parameter yang tidak digunakan untuk menghindari warning */
    (void)pvParameters;
    
    UART_SendString("[Monitor] Task dimulai\r\n");
    UART_SendString("[Monitor] Periode laporan: ");
    char tmpBuf[20];
    sprintf(tmpBuf, "%d ms\r\n\r\n", MONITOR_TASK_PERIOD_MS);
    UART_SendString(tmpBuf);
    
    /* ---------------------------------------------------------------------------
     * INFINITE LOOP
     * --------------------------------------------------------------------------- */
    for(;;) {
        /* -----------------------------------------------------------------------
         * BACA COUNTER STATISTIK
         * -----------------------------------------------------------------------
         * Counter ini di-update oleh idle hook.
         * Kita hitung delta (perubahan) sejak pembacaan terakhir.
         * ----------------------------------------------------------------------- */
        currentIdle = idleCounter;    /* Baca nilai saat ini */
        currentSleep = sleepCounter;
        
        deltaIdle = currentIdle - lastIdle;    /* Hitung perubahan */
        deltaSleep = currentSleep - lastSleep;
        
        /* -----------------------------------------------------------------------
         * FORMAT DAN KIRIM LAPORAN
         * -----------------------------------------------------------------------
         * Output:
         * - Total panggilan idle hook
         * - Perubahan sejak laporan terakhir
         * - Total masuk sleep mode
         * - Perubahan sleep sejak laporan terakhir
         * ----------------------------------------------------------------------- */
        sprintf(buffer, 
                "[Monitor] Idle: %lu (+%lu), Sleep: %lu (+%lu) dalam %d ms\r\n",
                currentIdle, deltaIdle,
                currentSleep, deltaSleep,
                MONITOR_TASK_PERIOD_MS);
        UART_SendString(buffer);
        
        /* Simpan nilai untuk perhitungan delta berikutnya */
        lastIdle = currentIdle;
        lastSleep = currentSleep;
        
        /* -----------------------------------------------------------------------
         * DELAY SAMPAI LAPORAN BERIKUTNYA
         * ----------------------------------------------------------------------- */
        vTaskDelay(pdMS_TO_TICKS(MONITOR_TASK_PERIOD_MS));
    }
}

/* ============================================================================
 * FUNGSI: SystemClock_Config
 * ============================================================================
 * Mengkonfigurasi system clock STM32F103 ke 72MHz.
 * ============================================================================ */
void SystemClock_Config(void) {
#if defined(STM32F103xB)
    /* F1: HSE 8MHz -> PLL x9 =72MHz */
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
    HAL_RCC_OscConfig(&RCC_OscInitStruct);

    RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                      | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2);
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
    HAL_RCC_OscConfig(&RCC_OscInitStruct);

    RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                      | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2);
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
    HAL_RCC_OscConfig(&RCC_OscInitStruct);

    RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                      | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3);
#else
    #error "Unsupported STM32 target"
#endif
}

/* ============================================================================
 * FUNGSI: MX_GPIO_Init
 * ============================================================================
 * Menginisialisasi GPIO untuk LED indicator.
 * ============================================================================ */
static void MX_GPIO_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* Enable clock untuk GPIOC dan GPIOA */
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    /* Konfigurasi LED PC13 */
    HAL_GPIO_WritePin(LED_GPIO_PORT, LED_GPIO_PIN, GPIO_PIN_SET);  /* LED mati awal */
    
    GPIO_InitStruct.Pin = LED_GPIO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_GPIO_PORT, &GPIO_InitStruct);
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

    /* PA9 = TX (Alternate Function Push-Pull) */
    GPIO_InitStruct.Pin = DEBUG_UART_TX_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
#if defined(STM32F401xC) || defined(STM32F411xE)
    GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
#endif
    HAL_GPIO_Init(DEBUG_UART_TX_PORT, &GPIO_InitStruct);
    
    /* PA10 = RX */
    GPIO_InitStruct.Pin = DEBUG_UART_RX_PIN;
#if defined(STM32F103xB)
    GPIO_InitStruct.Mode = GPIO_MODE_AF_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
#elif defined(STM32F401xC) || defined(STM32F411xE)
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
#endif
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
 * Callback ketika alokasi memori FreeRTOS gagal.
 * ============================================================================ */
void vApplicationMallocFailedHook(void) {
    UART_SendString("\r\n!!! FATAL: Malloc gagal! Heap penuh. !!!\r\n");
    taskDISABLE_INTERRUPTS();
    for(;;) {
        HAL_GPIO_TogglePin(LED_GPIO_PORT, LED_GPIO_PIN);
        for(volatile uint32_t i = 0; i < 100000; i++);
    }
}

/* ============================================================================
 * FUNGSI: vApplicationStackOverflowHook
 * ============================================================================
 * Callback ketika stack overflow terdeteksi.
 * ============================================================================ */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
    (void)xTask;
    UART_SendString("\r\n!!! FATAL: Stack Overflow pada task: ");
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
 * Interrupt handler untuk SysTick timer.
 * ============================================================================ */
void SysTick_Handler(void) {
    HAL_IncTick();  /* Untuk HAL timing */
    
    if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED) {
        xPortSysTickHandler();  /* FreeRTOS tick */
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
    UART_SendString("  Program 4: Idle Task Hook Utilization\r\n");
    UART_SendString("============================================================\r\n");
    UART_SendString("\r\n");
    UART_SendString("DESKRIPSI:\r\n");
    UART_SendString("Program ini mendemonstrasikan penggunaan Idle Hook untuk:\r\n");
    UART_SendString("1. Mode hemat daya (CPU sleep dengan __WFI)\r\n");
    UART_SendString("2. Monitoring CPU utilization\r\n");
    UART_SendString("\r\n");
    UART_SendString("CARA KERJA:\r\n");
    UART_SendString("- Worker task bekerja lalu delay (idle hook aktif)\r\n");
    UART_SendString("- Monitor task melaporkan statistik idle\r\n");
    UART_SendString("- Semakin besar idle count = CPU semakin santai\r\n");
    UART_SendString("\r\n");
    UART_SendString("============================================================\r\n\r\n");

    /* ---------------------------------------------------------------------------
     * MEMBUAT TASK FREERTOS
     * ---------------------------------------------------------------------------
     * Worker Task: Prioritas 2 (lebih tinggi)
     * - Melakukan kerja simulasi
     * - Delay 500ms setiap iterasi
     * 
     * Monitor Task: Prioritas 1 (lebih rendah)
     * - Melaporkan statistik setiap 2 detik
     * --------------------------------------------------------------------------- */
    BaseType_t result1 = xTaskCreate(
        vWorkerTask,                /* Fungsi task */
        WORKER_TASK_NAME,           /* Nama task */
        WORKER_TASK_STACK_SIZE,     /* Stack size */
        NULL,                       /* Parameter */
        WORKER_TASK_PRIORITY,       /* Prioritas (2) */
        NULL                        /* Handle */
    );
    
    BaseType_t result2 = xTaskCreate(
        vMonitorTask,               /* Fungsi task */
        MONITOR_TASK_NAME,          /* Nama task */
        MONITOR_TASK_STACK_SIZE,    /* Stack size */
        NULL,                       /* Parameter */
        MONITOR_TASK_PRIORITY,      /* Prioritas (1) */
        NULL                        /* Handle */
    );
    
    /* Cek hasil pembuatan task */
    if(result1 != pdPASS || result2 != pdPASS) {
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
