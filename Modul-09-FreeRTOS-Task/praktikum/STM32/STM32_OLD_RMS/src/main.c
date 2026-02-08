/**
 * ================================================================================
 * PROGRAM 2: RATE MONOTONIC SCHEDULING (RMS)
 * ================================================================================
 * 
 * DESKRIPSI:
 *   Rate Monotonic Scheduling adalah algoritma penjadwalan real-time dimana
 *   prioritas task ditentukan berdasarkan FREKUENSI eksekusinya.
 *   
 *   ATURAN: Task dengan periode LEBIH PENDEK mendapat prioritas LEBIH TINGGI
 *   
 *   Mengapa? Karena task yang sering dieksekusi (high frequency) biasanya
 *   memiliki deadline yang lebih ketat dan harus lebih responsif.
 * 
 * CONTOH PADA PROGRAM INI:
 *   - High Task:   100ms periode  -> Prioritas 3 (tertinggi)
 *   - Medium Task: 250ms periode  -> Prioritas 2 (menengah)
 *   - Low Task:    500ms periode  -> Prioritas 1 (terendah)
 * 
 * APA YANG TERJADI:
 *   1. Low Task sedang berjalan
 *   2. Medium Task siap -> Interrupt Low Task, Medium berjalan
 *   3. High Task siap -> Interrupt Medium Task, High berjalan
 *   4. High selesai -> Medium lanjut
 *   5. Medium selesai -> Low lanjut
 *   
 * HARDWARE:
 *   - STM32F103C8T6 (Blue Pill)
 *   - LED PC13 (toggle oleh Low Task)
 *   - UART1 PA9/PA10 (115200 baud) untuk output
 * 
 * ================================================================================
 */

/* ================================================================================
 * INCLUDE LIBRARIES
 * ================================================================================ */

/* STM32 HAL Library untuk akses hardware */
#include "stm32f1xx_hal.h"

/* FreeRTOS kernel untuk multi-tasking */
#include "FreeRTOS.h"

/* Task management dari FreeRTOS */
#include "task.h"

/* Library untuk manipulasi string */
#include <string.h>

/* Library untuk sprintf dan formatted output */
#include <stdio.h>

/* ================================================================================
 * VARIABEL GLOBAL
 * ================================================================================ */

/* Handle UART untuk komunikasi serial */
UART_HandleTypeDef huart1;

/* ================================================================================
 * FUNCTION PROTOTYPES (Deklarasi Fungsi)
 * ================================================================================ */

/* Konfigurasi system clock ke 72MHz */
void SystemClock_Config(void);

/* Inisialisasi GPIO untuk LED */
static void MX_GPIO_Init(void);

/* Inisialisasi UART1 untuk debug output */
static void MX_USART1_UART_Init(void);

/* Task dengan prioritas tinggi - periode 100ms */
void vHighPriorityTask(void *pvParameters);

/* Task dengan prioritas menengah - periode 250ms */
void vMediumPriorityTask(void *pvParameters);

/* Task dengan prioritas rendah - periode 500ms */
void vLowPriorityTask(void *pvParameters);

/* Fungsi helper untuk mengirim string via UART */
void UART_SendString(const char *str);

/* ================================================================================
 * IMPLEMENTASI FUNGSI UTILITY
 * ================================================================================ */

/**
 * UART_SendString - Mengirim string ke serial monitor
 * 
 * Fungsi ini mengirim string melalui UART1 ke terminal/serial monitor.
 * Digunakan untuk debugging dan melihat aktivitas task.
 * 
 * Parameter:
 *   str - pointer ke string yang akan dikirim (harus null-terminated)
 */
void UART_SendString(const char *str)
{
    /* HAL_UART_Transmit mengirim data secara blocking (menunggu selesai) */
    /* HAL_MAX_DELAY = tunggu sampai selesai tanpa timeout */
    HAL_UART_Transmit(&huart1, (uint8_t*)str, strlen(str), HAL_MAX_DELAY);
}

/* ================================================================================
 * IMPLEMENTASI TASK - HIGH PRIORITY (100ms)
 * ================================================================================
 * 
 * Task ini memiliki PRIORITAS TERTINGGI karena periode TERPENDEK.
 * Sesuai aturan RMS: Frekuensi tinggi = Prioritas tinggi
 * 
 * Periode: 100ms (10x per detik)
 * Prioritas: 3 (tertinggi dari 3 task)
 * 
 * Apa yang terjadi:
 * - Task ini akan SELALU bisa interrupt task lain
 * - Tidak ada task lain yang bisa interrupt task ini (kecuali ISR)
 * - Paling responsif terhadap event
 * 
 * ================================================================================ */
void vHighPriorityTask(void *pvParameters)
{
    /* xLastWakeTime menyimpan waktu terakhir task bangun
       Digunakan untuk perhitungan delay yang akurat */
    TickType_t xLastWakeTime = xTaskGetTickCount();
    
    /* Counter untuk tracking berapa kali task dieksekusi */
    uint32_t counter = 0;
    
    /* Buffer untuk format output string */
    char buffer[60];
    
    /* Infinite loop - task berjalan selamanya */
    for(;;)
    {
        /* Increment counter setiap eksekusi */
        counter++;
        
        /* Format output: tampilkan tick count dan execution count */
        sprintf(buffer, "[HIGH] Tick: %lu, Count: %lu\r\n", 
                (unsigned long)xTaskGetTickCount(), counter);
        
        /* Kirim ke serial monitor */
        UART_SendString(buffer);
        
        /* ================================================================
         * SIMULASI BEBAN KERJA
         * ================================================================
         * Loop ini mensimulasikan pekerjaan yang dilakukan task.
         * Dalam aplikasi real, ini bisa berupa:
         * - Membaca sensor dengan sampling rate tinggi
         * - Processing data real-time
         * - Control loop (PID, dll)
         * ================================================================ */
        volatile uint32_t i;
        for(i = 0; i < HIGH_TASK_WORK_ITERATIONS; i++);
        
        /* ================================================================
         * vTaskDelayUntil - PENTING UNTUK RMS!
         * ================================================================
         * 
         * Berbeda dengan vTaskDelay yang delay DARI SAAT INI,
         * vTaskDelayUntil delay SAMPAI WAKTU ABSOLUT tertentu.
         * 
         * Ini memastikan task berjalan dengan periode TETAP,
         * tidak tergantung berapa lama eksekusi sebelumnya.
         * 
         * Contoh:
         * - vTaskDelay(100): Delay 100ms DARI SEKARANG
         * - vTaskDelayUntil: Delay SAMPAI 100ms SEJAK EKSEKUSI TERAKHIR
         * 
         * Dengan vTaskDelayUntil, jika eksekusi memakan 10ms,
         * delay hanya 90ms, sehingga total tetap 100ms.
         * 
         * ================================================================ */
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(HIGH_TASK_PERIOD_MS));
    }
}

/* ================================================================================
 * IMPLEMENTASI TASK - MEDIUM PRIORITY (250ms)
 * ================================================================================
 * 
 * Task ini memiliki PRIORITAS MENENGAH.
 * Periode lebih panjang dari High, lebih pendek dari Low.
 * 
 * Periode: 250ms (4x per detik)
 * Prioritas: 2 (menengah)
 * 
 * Apa yang terjadi:
 * - Bisa di-interrupt oleh High Task
 * - Bisa interrupt Low Task
 * - Balance antara responsivitas dan resource usage
 * 
 * ================================================================================ */
void vMediumPriorityTask(void *pvParameters)
{
    /* Waktu terakhir task bangun untuk delay absolut */
    TickType_t xLastWakeTime = xTaskGetTickCount();
    
    /* Counter eksekusi */
    uint32_t counter = 0;
    
    /* Buffer output */
    char buffer[60];
    
    /* Infinite loop */
    for(;;)
    {
        counter++;
        
        /* Format dan kirim output */
        /* [MED] menandakan ini Medium priority task */
        sprintf(buffer, "[MED]  Tick: %lu, Count: %lu\r\n", 
                (unsigned long)xTaskGetTickCount(), counter);
        UART_SendString(buffer);
        
        /* Simulasi beban kerja - lebih berat dari High Task */
        volatile uint32_t i;
        for(i = 0; i < MEDIUM_TASK_WORK_ITERATIONS; i++);
        
        /* Delay sampai periode berikutnya */
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(MEDIUM_TASK_PERIOD_MS));
    }
}

/* ================================================================================
 * IMPLEMENTASI TASK - LOW PRIORITY (500ms)
 * ================================================================================
 * 
 * Task ini memiliki PRIORITAS TERENDAH karena periode TERPANJANG.
 * 
 * Periode: 500ms (2x per detik)
 * Prioritas: 1 (terendah dari 3 task)
 * 
 * Apa yang terjadi:
 * - Bisa di-interrupt oleh High dan Medium Task
 * - Hanya berjalan ketika tidak ada task lain yang ready
 * - Cocok untuk task background yang tidak time-critical
 * 
 * Contoh penggunaan di dunia nyata:
 * - Logging data
 * - Update display
 * - Housekeeping tasks
 * 
 * ================================================================================ */
void vLowPriorityTask(void *pvParameters)
{
    /* Waktu terakhir task bangun */
    TickType_t xLastWakeTime = xTaskGetTickCount();
    
    /* Counter eksekusi */
    uint32_t counter = 0;
    
    /* Buffer output */
    char buffer[60];
    
    /* Infinite loop */
    for(;;)
    {
        counter++;
        
        /* Format dan kirim output */
        sprintf(buffer, "[LOW]  Tick: %lu, Count: %lu\r\n", 
                (unsigned long)xTaskGetTickCount(), counter);
        UART_SendString(buffer);
        
        /* Toggle LED untuk indikasi visual aktivitas Low Task
           LED di Blue Pill active low, jadi toggle membuatnya berkedip */
        HAL_GPIO_TogglePin(LED_PORT, LED_PIN);
        
        /* Simulasi beban kerja - paling berat karena punya waktu paling banyak */
        volatile uint32_t i;
        for(i = 0; i < LOW_TASK_WORK_ITERATIONS; i++);
        
        /* Delay sampai periode berikutnya */
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(LOW_TASK_PERIOD_MS));
    }
}

/* ================================================================================
 * KONFIGURASI SYSTEM CLOCK
 * ================================================================================
 * 
 * Mengkonfigurasi clock system STM32F103 ke 72MHz menggunakan:
 * - HSE (High Speed External): Crystal 8MHz
 * - PLL (Phase Locked Loop): Multiply by 9
 * - Hasil: 8MHz x 9 = 72MHz
 * 
 * ================================================================================ */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    /* Konfigurasi oscillator menggunakan HSE dan PLL */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;              /* Aktifkan HSE */
    RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1; /* Tidak dibagi */
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;          /* Aktifkan PLL */
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;  /* Sumber dari HSE */
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;          /* Multiply 9x */
    HAL_RCC_OscConfig(&RCC_OscInitStruct);

    /* Konfigurasi clock buses */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK; /* SYSCLK dari PLL */
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;    /* AHB = 72MHz */
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;     /* APB1 = 36MHz (max) */
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;     /* APB2 = 72MHz */
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2);
}

/* ================================================================================
 * INISIALISASI GPIO
 * ================================================================================
 * 
 * Mengkonfigurasi GPIO untuk LED pada PC13.
 * LED Blue Pill adalah active-low (nyala saat pin LOW).
 * 
 * ================================================================================ */
static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* Enable clock untuk GPIOC dan GPIOA */
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    /* Konfigurasi LED PC13 sebagai output push-pull */
    HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET);  /* Mulai dengan LED off */
    GPIO_InitStruct.Pin = LED_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;          /* Push-pull output */
    GPIO_InitStruct.Pull = GPIO_NOPULL;                  /* Tidak ada pull-up/down */
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;         /* Kecepatan rendah cukup */
    HAL_GPIO_Init(LED_PORT, &GPIO_InitStruct);
}

/* ================================================================================
 * INISIALISASI UART1
 * ================================================================================
 * 
 * Mengkonfigurasi UART1 untuk komunikasi serial:
 * - TX: PA9
 * - RX: PA10
 * - Baud rate: 115200
 * - 8 data bits, no parity, 1 stop bit
 * 
 * ================================================================================ */
static void MX_USART1_UART_Init(void)
{
    /* Enable clock untuk USART1 dan GPIOA */
    __HAL_RCC_USART1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    /* Konfigurasi TX pin (PA9) sebagai alternate function push-pull */
    GPIO_InitStruct.Pin = GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    /* Konfigurasi RX pin (PA10) sebagai input */
    GPIO_InitStruct.Pin = GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* Konfigurasi parameter UART */
    huart1.Instance = USART1;
    huart1.Init.BaudRate = UART_BAUDRATE;           /* 115200 baud */
    huart1.Init.WordLength = UART_WORDLENGTH_8B;    /* 8 data bits */
    huart1.Init.StopBits = UART_STOPBITS_1;         /* 1 stop bit */
    huart1.Init.Parity = UART_PARITY_NONE;          /* Tanpa parity */
    huart1.Init.Mode = UART_MODE_TX_RX;             /* Mode TX dan RX */
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;    /* Tanpa flow control */
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(&huart1);
}

/* ================================================================================
 * INTERRUPT HANDLERS
 * ================================================================================ */

/* Non-Maskable Interrupt Handler */
void NMI_Handler(void)
{
    while (1) {}
}

/* Hard Fault Handler - dipanggil saat ada error fatal */
void HardFault_Handler(void)
{
    while (1) {}
}

/* Memory Management Fault Handler */
void MemManage_Handler(void)
{
    while (1) {}
}

/* Bus Fault Handler */
void BusFault_Handler(void)
{
    while (1) {}
}

/* Usage Fault Handler */
void UsageFault_Handler(void)
{
    while (1) {}
}

/* Debug Monitor Handler */
void DebugMon_Handler(void)
{
}

/* SysTick Handler - dipanggil setiap tick untuk FreeRTOS scheduler */
void SysTick_Handler(void)
{
    xPortSysTickHandler();
}

/* ================================================================================
 * FREERTOS HOOK FUNCTIONS
 * ================================================================================ */

/**
 * vApplicationMallocFailedHook - Dipanggil ketika pvPortMalloc gagal
 * 
 * Ini terjadi ketika heap FreeRTOS penuh dan tidak bisa alokasi memory baru.
 * Solusi: perbesar configTOTAL_HEAP_SIZE atau kurangi penggunaan memory.
 */
void vApplicationMallocFailedHook(void)
{
    /* Disable interrupt dan loop selamanya untuk debugging */
    configASSERT(0);
}

/**
 * vApplicationStackOverflowHook - Dipanggil ketika stack overflow terdeteksi
 * 
 * Stack overflow terjadi ketika task menggunakan lebih banyak stack
 * daripada yang dialokasikan. Solusi: perbesar stack size task.
 */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    (void)pcTaskName;
    configASSERT(0);
}

/* ================================================================================
 * STATIC ALLOCATION FUNCTIONS
 * ================================================================================ */

#if configSUPPORT_STATIC_ALLOCATION == 1

/* Buffer static untuk Idle Task */
static StaticTask_t xIdleTaskTCB;
static StackType_t uxIdleTaskStack[configMINIMAL_STACK_SIZE];

/**
 * vApplicationGetIdleTaskMemory - Menyediakan memory untuk Idle Task
 * 
 * FreeRTOS memanggil fungsi ini untuk mendapatkan memory static
 * untuk Idle Task ketika configSUPPORT_STATIC_ALLOCATION = 1.
 */
void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer,
                                   StackType_t **ppxIdleTaskStackBuffer,
                                   configSTACK_DEPTH_TYPE *pulIdleTaskStackSize)
{
    *ppxIdleTaskTCBBuffer = &xIdleTaskTCB;
    *ppxIdleTaskStackBuffer = uxIdleTaskStack;
    *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}

#if configUSE_TIMERS == 1

/* Buffer static untuk Timer Task */
static StaticTask_t xTimerTaskTCB;
static StackType_t uxTimerTaskStack[configTIMER_TASK_STACK_DEPTH];

/**
 * vApplicationGetTimerTaskMemory - Menyediakan memory untuk Timer Task
 * 
 * Sama seperti Idle Task, Timer Task juga membutuhkan memory static
 * ketika menggunakan static allocation.
 */
void vApplicationGetTimerTaskMemory(StaticTask_t **ppxTimerTaskTCBBuffer,
                                    StackType_t **ppxTimerTaskStackBuffer,
                                    configSTACK_DEPTH_TYPE *pulTimerTaskStackSize)
{
    *ppxTimerTaskTCBBuffer = &xTimerTaskTCB;
    *ppxTimerTaskStackBuffer = uxTimerTaskStack;
    *pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
}

#endif /* configUSE_TIMERS */
#endif /* configSUPPORT_STATIC_ALLOCATION */

/* ================================================================================
 * MAIN FUNCTION
 * ================================================================================
 * 
 * Entry point program. Urutan eksekusi:
 * 1. Inisialisasi HAL
 * 2. Konfigurasi clock ke 72MHz
 * 3. Inisialisasi GPIO dan UART
 * 4. Kirim pesan welcome
 * 5. Buat 3 task dengan prioritas berbeda (RMS)
 * 6. Mulai scheduler FreeRTOS
 * 
 * Setelah scheduler dimulai, fungsi main() tidak pernah return.
 * 
 * ================================================================================ */
int main(void)
{
    /* Inisialisasi HAL Library */
    HAL_Init();
    
    /* Konfigurasi system clock ke 72MHz */
    SystemClock_Config();
    
    /* Inisialisasi peripheral */
    MX_GPIO_Init();
    MX_USART1_UART_Init();

    /* Kirim header informasi program ke serial monitor */
    UART_SendString("\r\n");
    UART_SendString("================================================================================\r\n");
    UART_SendString("PROGRAM 2: RATE MONOTONIC SCHEDULING (RMS)\r\n");
    UART_SendString("================================================================================\r\n");
    UART_SendString("\r\n");
    UART_SendString("KONSEP RMS:\r\n");
    UART_SendString("  Task dengan periode LEBIH PENDEK = Prioritas LEBIH TINGGI\r\n");
    UART_SendString("\r\n");
    UART_SendString("KONFIGURASI TASK:\r\n");
    UART_SendString("  [HIGH] Periode: 100ms -> Prioritas: 3 (tertinggi)\r\n");
    UART_SendString("  [MED]  Periode: 250ms -> Prioritas: 2 (menengah)\r\n");
    UART_SendString("  [LOW]  Periode: 500ms -> Prioritas: 1 (terendah)\r\n");
    UART_SendString("\r\n");
    UART_SendString("PERHATIKAN:\r\n");
    UART_SendString("  - HIGH Task selalu berjalan tepat waktu\r\n");
    UART_SendString("  - MED Task bisa di-interrupt oleh HIGH\r\n");
    UART_SendString("  - LOW Task hanya berjalan saat yang lain idle\r\n");
    UART_SendString("\r\n");
    UART_SendString("================================================================================\r\n");
    UART_SendString("\r\n");

    /* ================================================================
     * MEMBUAT TASK DENGAN PRIORITAS RMS
     * ================================================================
     * 
     * Sesuai aturan RMS:
     * - Periode terpendek (100ms) = Prioritas tertinggi (3)
     * - Periode menengah (250ms)  = Prioritas menengah (2)
     * - Periode terpanjang (500ms) = Prioritas terendah (1)
     * 
     * ================================================================ */
    
    /* Buat High Priority Task - periode 100ms, prioritas 3 */
    xTaskCreate(vHighPriorityTask,      /* Fungsi task */
                "HighTask",             /* Nama task (untuk debug) */
                HIGH_TASK_STACK_SIZE,   /* Stack size */
                NULL,                   /* Parameter (tidak ada) */
                HIGH_TASK_PRIORITY,     /* Prioritas */
                NULL);                  /* Handle (tidak disimpan) */
    
    /* Buat Medium Priority Task - periode 250ms, prioritas 2 */
    xTaskCreate(vMediumPriorityTask,
                "MedTask",
                MEDIUM_TASK_STACK_SIZE,
                NULL,
                MEDIUM_TASK_PRIORITY,
                NULL);
    
    /* Buat Low Priority Task - periode 500ms, prioritas 1 */
    xTaskCreate(vLowPriorityTask,
                "LowTask",
                LOW_TASK_STACK_SIZE,
                NULL,
                LOW_TASK_PRIORITY,
                NULL);

    /* Mulai FreeRTOS scheduler - tidak pernah return */
    vTaskStartScheduler();

    /* Kode di bawah tidak akan pernah dieksekusi */
    while(1) {}
    
    return 0;
}

/* ================================================================================
 * END OF FILE
 * ================================================================================ */
