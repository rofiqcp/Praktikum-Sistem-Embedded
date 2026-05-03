/**
 * ============================================================================
 * FILE: FreeRTOSConfig.h
 * PROJECT: 04-Idle_Task_Hook
 * 
 * DESKRIPSI:
 * File konfigurasi FreeRTOS untuk demonstrasi Idle Task Hook.
 * Program ini menunjukkan cara memanfaatkan waktu idle CPU untuk:
 * - Mode hemat daya (power saving)
 * - Background processing
 * - Monitoring sistem
 * 
 * KONSEP IDLE TASK:
 * FreeRTOS secara otomatis membuat "Idle Task" yang berjalan ketika
 * tidak ada task lain yang ready (semua task sedang BLOCKED atau SUSPENDED).
 * 
 * Idle Task Hook adalah callback yang dipanggil setiap kali idle task berjalan.
 * Ini memberikan kesempatan untuk:
 * 1. Masuk ke mode low-power (__WFI = Wait For Interrupt)
 * 2. Menghitung statistik idle time
 * 3. Melakukan background housekeeping
 * 
 * HARDWARE TARGET: STM32F103C8T6 (Blue Pill)
 * - LED: PC13 (aktif LOW) - Toggle oleh worker task
 * - UART: PA9 (TX), PA10 (RX) @ 115200 baud
 * ============================================================================
 */

#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/* ============================================================================
 * INCLUDE HEADER UNTUK DEFINISI STM32
 * ============================================================================ */
#if defined(STM32F103xB)
#include "stm32f1xx.h"
#elif defined(STM32F401xC) || defined(STM32F411xE)
#include "stm32f4xx.h"
#endif

/* ============================================================================
 * BAGIAN 1: KONFIGURASI FUNDAMENTAL KERNEL
 * ============================================================================ */

/* ---------------------------------------------------------------------------
 * configUSE_PREEMPTION
 * ---------------------------------------------------------------------------
 * Mengaktifkan preemptive scheduling.
 * NILAI: 0 = Cooperative, 1 = Preemptive
 */
#define configUSE_PREEMPTION                    1  /* Rentang: 0-1 */

/* ---------------------------------------------------------------------------
 * configUSE_PORT_OPTIMISED_TASK_SELECTION
 * ---------------------------------------------------------------------------
 * Menggunakan instruksi CLZ untuk seleksi task yang lebih cepat.
 */
#define configUSE_PORT_OPTIMISED_TASK_SELECTION 1  /* Rentang: 0-1 */

/* ---------------------------------------------------------------------------
 * configUSE_TICKLESS_IDLE
 * ---------------------------------------------------------------------------
 * Mode tickless untuk hemat daya lebih lanjut.
 * 
 * NILAI: 0 = Tick terus berjalan
 *        1 = Tick dihentikan saat idle (lebih hemat daya)
 * 
 * Catatan: Untuk demo ini kita gunakan 0 agar idle counter akurat.
 * Jika 1, idle hook tidak dipanggil sesering karena CPU benar-benar tidur.
 */
#define configUSE_TICKLESS_IDLE                 0  /* Rentang: 0-1 */

/* ---------------------------------------------------------------------------
 * configCPU_CLOCK_HZ
 * ---------------------------------------------------------------------------
 * Frekuensi clock CPU: 72MHz
 */
#if defined(STM32F103xB)
#define configCPU_CLOCK_HZ                      ((uint32_t)72000000)
#elif defined(STM32F401xC)
#define configCPU_CLOCK_HZ                      ((uint32_t)84000000)
#elif defined(STM32F411xE)
#define configCPU_CLOCK_HZ                      ((uint32_t)100000000)
#else
#define configCPU_CLOCK_HZ                      ((uint32_t)72000000)
#endif

/* ---------------------------------------------------------------------------
 * configTICK_RATE_HZ
 * ---------------------------------------------------------------------------
 * Frekuensi tick interrupt: 1000 Hz = 1ms per tick
 */
#define configTICK_RATE_HZ                      ((TickType_t)1000)  /* Rentang: 1-10000 */

/* ---------------------------------------------------------------------------
 * configMAX_PRIORITIES
 * ---------------------------------------------------------------------------
 * Jumlah level prioritas yang tersedia.
 */
#define configMAX_PRIORITIES                    8  /* Rentang: 1-56 */

/* ---------------------------------------------------------------------------
 * configMINIMAL_STACK_SIZE
 * ---------------------------------------------------------------------------
 * Ukuran stack minimum dalam WORDS (128 words = 512 bytes)
 */
#define configMINIMAL_STACK_SIZE                ((uint16_t)128)  /* Rentang: 64-1024 */

/* ---------------------------------------------------------------------------
 * configTOTAL_HEAP_SIZE
 * ---------------------------------------------------------------------------
 * Total heap untuk FreeRTOS: 8KB
 */
#define configTOTAL_HEAP_SIZE                   ((size_t)(8 * 1024))  /* Rentang: 1024-16384 */

/* ---------------------------------------------------------------------------
 * configMAX_TASK_NAME_LEN
 * ---------------------------------------------------------------------------
 * Panjang maksimum nama task untuk debugging.
 */
#define configMAX_TASK_NAME_LEN                 16  /* Rentang: 1-32 */

/* ---------------------------------------------------------------------------
 * configUSE_16_BIT_TICKS
 * ---------------------------------------------------------------------------
 * Tipe data tick counter: 0 = 32-bit, 1 = 16-bit
 */
#define configUSE_16_BIT_TICKS                  0  /* Rentang: 0-1 */

/* ---------------------------------------------------------------------------
 * configIDLE_SHOULD_YIELD
 * ---------------------------------------------------------------------------
 * Apakah idle task harus yield ke task dengan prioritas sama.
 * 
 * NILAI: 1 = Ya, idle task yield
 * 
 * Penting untuk demo ini: idle task akan berjalan saat worker sleep.
 */
#define configIDLE_SHOULD_YIELD                 1  /* Rentang: 0-1 */

/* ============================================================================
 * BAGIAN 2: FITUR SINKRONISASI
 * ============================================================================ */

#define configUSE_MUTEXES                       1  /* Rentang: 0-1 */
#define configUSE_RECURSIVE_MUTEXES             0  /* Tidak digunakan */
#define configUSE_COUNTING_SEMAPHORES           0  /* Tidak digunakan */

/* ============================================================================
 * BAGIAN 3: HOOK FUNCTIONS (CALLBACK)
 * ============================================================================
 * 
 * PENTING UNTUK DEMO INI:
 * configUSE_IDLE_HOOK = 1 untuk mengaktifkan vApplicationIdleHook()
 * ============================================================================ */

/* ---------------------------------------------------------------------------
 * configUSE_IDLE_HOOK - KUNCI DARI DEMO INI!
 * ---------------------------------------------------------------------------
 * NILAI: 1 = Aktifkan idle hook
 * 
 * Ketika aktif, FreeRTOS akan memanggil vApplicationIdleHook() setiap kali
 * idle task berjalan. Ini adalah tempat untuk:
 * 
 * 1. POWER SAVING:
 *    __WFI();  // Wait For Interrupt - CPU tidur sampai ada interrupt
 *    
 * 2. BACKGROUND PROCESSING:
 *    - Garbage collection
 *    - Statistics collection
 *    - Buffer cleanup
 *    
 * 3. MONITORING:
 *    - Menghitung berapa kali idle dipanggil
 *    - Estimasi CPU utilization
 * 
 * PERINGATAN:
 * - Jangan panggil fungsi blocking dalam idle hook!
 * - Jangan gunakan vTaskDelay() atau fungsi yang bisa block
 * - Idle hook harus return cepat
 */
#define configUSE_IDLE_HOOK                     1  /* WAJIB 1 untuk demo! */

/* ---------------------------------------------------------------------------
 * configUSE_TICK_HOOK
 * ---------------------------------------------------------------------------
 * Hook yang dipanggil setiap tick interrupt.
 * Tidak digunakan dalam demo ini.
 */
#define configUSE_TICK_HOOK                     0  /* Rentang: 0-1 */

/* ---------------------------------------------------------------------------
 * Hook untuk error handling
 * ---------------------------------------------------------------------------
 */
#define configUSE_MALLOC_FAILED_HOOK            1  /* Aktif - penting untuk debug! */
#define configCHECK_FOR_STACK_OVERFLOW          2  /* Metode 2 - lebih thorough */

/* ============================================================================
 * BAGIAN 4: RUNTIME STATS DAN DEBUG
 * ============================================================================ */

#define configGENERATE_RUN_TIME_STATS           0
#define configUSE_TRACE_FACILITY                0
#define configUSE_STATS_FORMATTING_FUNCTIONS    0

/* ============================================================================
 * BAGIAN 5: CO-ROUTINE (DEPRECATED)
 * ============================================================================ */
#define configUSE_CO_ROUTINES                   0
#define configMAX_CO_ROUTINE_PRIORITIES         2

/* ============================================================================
 * BAGIAN 6: SOFTWARE TIMER
 * ============================================================================ */

#define configUSE_TIMERS                        0  /* Tidak digunakan dalam demo */
#define configTIMER_TASK_PRIORITY               3
#define configTIMER_QUEUE_LENGTH                10
#define configTIMER_TASK_STACK_DEPTH            configMINIMAL_STACK_SIZE

/* ============================================================================
 * BAGIAN 7: KONFIGURASI INTERRUPT CORTEX-M3
 * ============================================================================ */

#ifdef __NVIC_PRIO_BITS
    #define configPRIO_BITS                     __NVIC_PRIO_BITS
#else
    #define configPRIO_BITS                     4
#endif

#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY         15
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY    5

#define configKERNEL_INTERRUPT_PRIORITY         (configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8 - configPRIO_BITS))
#define configMAX_SYSCALL_INTERRUPT_PRIORITY    (configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - configPRIO_BITS))

/* ============================================================================
 * BAGIAN 8: DEFINISI HANDLER INTERRUPT
 * ============================================================================ */

#define xPortPendSVHandler                      PendSV_Handler
#define vPortSVCHandler                         SVC_Handler

/* ============================================================================
 * BAGIAN 9: FUNGSI API YANG DIAKTIFKAN
 * ============================================================================ */

#define INCLUDE_vTaskPrioritySet                0
#define INCLUDE_uxTaskPriorityGet               0
#define INCLUDE_vTaskDelete                     0
#define INCLUDE_vTaskSuspend                    0
#define INCLUDE_xResumeFromISR                  0
#define INCLUDE_vTaskDelayUntil                 1
#define INCLUDE_vTaskDelay                      1  /* WAJIB - digunakan oleh task */
#define INCLUDE_xTaskGetSchedulerState          1
#define INCLUDE_xTaskGetCurrentTaskHandle       0
#define INCLUDE_uxTaskGetStackHighWaterMark     1
#define INCLUDE_xTaskGetIdleTaskHandle          0
#define INCLUDE_eTaskGetState                   0
#define INCLUDE_xEventGroupSetBitFromISR        0
#define INCLUDE_xTimerPendFunctionCall          0
#define INCLUDE_xTaskAbortDelay                 0
#define INCLUDE_xTaskGetHandle                  0
#define INCLUDE_xTaskResumeFromISR              0

/* ============================================================================
 * BAGIAN 10: ASSERTIONS DAN ERROR HANDLING
 * ============================================================================ */

#define configASSERT(x) if((x) == 0) { taskDISABLE_INTERRUPTS(); for(;;); }

/* ============================================================================
 * BAGIAN 11: KONFIGURASI HARDWARE APLIKASI
 * ============================================================================ */

/* ---------------------------------------------------------------------------
 * KONFIGURASI LED INDICATOR
 * ---------------------------------------------------------------------------
 * LED pada PC13 (aktif LOW) - toggle oleh Worker Task
 */
#define LED_GPIO_PORT                           GPIOC
#define LED_GPIO_PIN                            GPIO_PIN_13
#define LED_ACTIVE_LOW                          1

/* ---------------------------------------------------------------------------
 * KONFIGURASI UART DEBUG
 * ---------------------------------------------------------------------------
 */
#define DEBUG_UART_INSTANCE                     USART1
#define DEBUG_UART_BAUDRATE                     115200
#define DEBUG_UART_TX_PORT                      GPIOA
#define DEBUG_UART_TX_PIN                       GPIO_PIN_9
#define DEBUG_UART_RX_PORT                      GPIOA
#define DEBUG_UART_RX_PIN                       GPIO_PIN_10

/* ---------------------------------------------------------------------------
 * KONFIGURASI TASK
 * ---------------------------------------------------------------------------
 * 
 * WORKER_TASK: Task yang melakukan kerja periodik
 * - Prioritas: 2 (lebih tinggi dari Monitor)
 * - Periode: 500ms
 * - Saat delay, idle task berjalan dan memanggil idle hook
 * 
 * MONITOR_TASK: Task yang melaporkan statistik idle
 * - Prioritas: 1 (lebih rendah)
 * - Periode: 2000ms (setiap 2 detik)
 */
#define WORKER_TASK_STACK_SIZE                  256  /* Rentang: 128-512 words */
#define WORKER_TASK_PRIORITY                    (tskIDLE_PRIORITY + 2)  /* Prioritas 2 */
#define WORKER_TASK_NAME                        "Worker"
#define WORKER_TASK_PERIOD_MS                   500  /* Rentang: 100-5000 ms */
#define WORKER_TASK_REPORT_INTERVAL             10   /* Lapor setiap 10 iterasi */

#define MONITOR_TASK_STACK_SIZE                 256  /* Rentang: 128-512 words */
#define MONITOR_TASK_PRIORITY                   (tskIDLE_PRIORITY + 1)  /* Prioritas 1 */
#define MONITOR_TASK_NAME                       "Monitor"
#define MONITOR_TASK_PERIOD_MS                  2000 /* Rentang: 500-10000 ms */

/* ---------------------------------------------------------------------------
 * KONFIGURASI SIMULASI WORKLOAD
 * ---------------------------------------------------------------------------
 * Jumlah iterasi loop untuk simulasi kerja CPU
 */
#define WORKER_SIMULATION_LOOPS                 50000  /* Rentang: 10000-100000 */

#endif /* FREERTOS_CONFIG_H */
