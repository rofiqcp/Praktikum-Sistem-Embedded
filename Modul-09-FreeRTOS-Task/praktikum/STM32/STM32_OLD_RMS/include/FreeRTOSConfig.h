/**
 * ================================================================================
 * KONFIGURASI FREERTOS UNTUK STM32F103C8T6 (Blue Pill)
 * ================================================================================
 * File: FreeRTOSConfig.h
 * Project: 02-Rate_Monotonic_Scheduling
 * 
 * Deskripsi:
 *   Rate Monotonic Scheduling (RMS) adalah algoritma penjadwalan dimana
 *   task dengan periode lebih pendek (frekuensi lebih tinggi) mendapat
 *   prioritas lebih tinggi. Ini adalah pendekatan optimal untuk sistem
 *   real-time dengan task periodik.
 *   
 *   Rumus RMS: Frekuensi tinggi = Prioritas tinggi
 *   - Task 100ms  -> Prioritas 3 (tertinggi)
 *   - Task 250ms  -> Prioritas 2 (menengah)
 *   - Task 500ms  -> Prioritas 1 (terendah)
 * 
 * ================================================================================
 */

#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/* ================================================================================
 * KONFIGURASI PROCESSOR CORTEX-M3
 * ================================================================================ */

/* Priority bits untuk NVIC (4 bits = 16 level prioritas: 0-15) */
#ifdef __NVIC_PRIO_BITS
    #define configPRIO_BITS __NVIC_PRIO_BITS
#else
    #define configPRIO_BITS 4
#endif

/* ================================================================================
 * KONFIGURASI KERNEL FREERTOS
 * ================================================================================ */

/* Gunakan preemptive scheduling (1) - PENTING untuk RMS! 
   Task prioritas tinggi akan interrupt task prioritas rendah */
#define configUSE_PREEMPTION                    1

/* Optimasi pemilihan task menggunakan instruksi CLZ (Count Leading Zeros) */
#define configUSE_PORT_OPTIMISED_TASK_SELECTION 0

/* Tickless idle untuk hemat daya (0=nonaktif untuk demo ini) */
#define configUSE_TICKLESS_IDLE                 0

/* Frekuensi CPU: 72MHz (8MHz HSE x 9 PLL) */
#define configCPU_CLOCK_HZ                      72000000

/* Frekuensi tick: 1000Hz = 1ms per tick (penting untuk timing akurat) */
#define configTICK_RATE_HZ                      1000

/* Jumlah level prioritas (0-6, 7 level total) */
#define configMAX_PRIORITIES                    7

/* Ukuran stack minimal dalam words (128 x 4 = 512 bytes) */
#define configMINIMAL_STACK_SIZE                128

/* Panjang maksimal nama task */
#define configMAX_TASK_NAME_LEN                 16

/* Gunakan tick counter 32-bit (0 = 32-bit, 1 = 16-bit) */
#define configUSE_16_BIT_TICKS                  0

/* Idle task yield ke task sama prioritas */
#define configIDLE_SHOULD_YIELD                 1

/* Aktifkan task notifications */
#define configUSE_TASK_NOTIFICATIONS            1
#define configTASK_NOTIFICATION_ARRAY_ENTRIES   3

/* Aktifkan mutex dan semaphore */
#define configUSE_MUTEXES                       1
#define configUSE_RECURSIVE_MUTEXES             1
#define configUSE_COUNTING_SEMAPHORES           1

/* Queue registry untuk debugging */
#define configQUEUE_REGISTRY_SIZE               8
#define configUSE_QUEUE_SETS                    1

/* Time slicing: task sama prioritas bergantian setiap tick */
#define configUSE_TIME_SLICING                  1

/* Konfigurasi library */
#define configUSE_NEWLIB_REENTRANT              0
#define configENABLE_BACKWARD_COMPATIBILITY     0
#define configNUM_THREAD_LOCAL_STORAGE_POINTERS 5

/* Tipe data */
#define configSTACK_DEPTH_TYPE                  uint16_t
#define configMESSAGE_BUFFER_LENGTH_TYPE        size_t

/* ================================================================================
 * KONFIGURASI MEMORY
 * ================================================================================ */

/* Static allocation: memory dialokasi saat compile */
#define configSUPPORT_STATIC_ALLOCATION         1

/* Dynamic allocation: memory dialokasi saat runtime */
#define configSUPPORT_DYNAMIC_ALLOCATION        1

/* Total heap size: 10KB untuk dynamic allocation */
#define configTOTAL_HEAP_SIZE                   10240

/* Heap dikelola oleh FreeRTOS (bukan aplikasi) */
#define configAPPLICATION_ALLOCATED_HEAP        0

/* ================================================================================
 * KONFIGURASI HOOK FUNCTIONS
 * ================================================================================ */

/* Idle hook: dipanggil saat tidak ada task yang ready */
#define configUSE_IDLE_HOOK                     0

/* Tick hook: dipanggil setiap tick interrupt */
#define configUSE_TICK_HOOK                     0

/* Deteksi stack overflow (method 2 = lebih thorough) */
#define configCHECK_FOR_STACK_OVERFLOW          2

/* Hook ketika malloc gagal */
#define configUSE_MALLOC_FAILED_HOOK            1

/* Hook saat daemon task startup */
#define configUSE_DAEMON_TASK_STARTUP_HOOK      0

/* ================================================================================
 * KONFIGURASI STATISTIK DAN DEBUG
 * ================================================================================ */

/* Runtime stats untuk mengukur CPU usage */
#define configGENERATE_RUN_TIME_STATS           0

/* Trace facility untuk debugging */
#define configUSE_TRACE_FACILITY                1

/* Stats formatting functions */
#define configUSE_STATS_FORMATTING_FUNCTIONS    1

/* ================================================================================
 * KONFIGURASI CO-ROUTINE (tidak digunakan)
 * ================================================================================ */

#define configUSE_CO_ROUTINES                   0
#define configMAX_CO_ROUTINE_PRIORITIES         1

/* ================================================================================
 * KONFIGURASI SOFTWARE TIMER
 * ================================================================================ */

/* Aktifkan software timer */
#define configUSE_TIMERS                        1

/* Prioritas timer task (3 = medium-high) */
#define configTIMER_TASK_PRIORITY               3

/* Panjang command queue timer */
#define configTIMER_QUEUE_LENGTH                10

/* Stack size timer task */
#define configTIMER_TASK_STACK_DEPTH            configMINIMAL_STACK_SIZE

/* ================================================================================
 * KONFIGURASI INTERRUPT
 * ================================================================================ */

/* Prioritas interrupt terendah (15 = paling rendah di STM32) */
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY         15

/* Prioritas maksimal untuk API FreeRTOS dari ISR */
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY    5

/* Konversi ke format NVIC */
#define configKERNEL_INTERRUPT_PRIORITY         ( configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8 - configPRIO_BITS) )
#define configMAX_SYSCALL_INTERRUPT_PRIORITY    ( configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - configPRIO_BITS) )

/* ================================================================================
 * KONFIGURASI ASSERT
 * ================================================================================ */

/* Assert: jika gagal, disable interrupt dan infinite loop */
#define configASSERT( x ) if ((x) == 0) {taskDISABLE_INTERRUPTS(); for( ;; );}

/* ================================================================================
 * KONFIGURASI MPU (tidak digunakan)
 * ================================================================================ */

#define configINCLUDE_APPLICATION_DEFINED_PRIVILEGED_FUNCTIONS 0
#define configTOTAL_MPU_REGIONS                 8
#define configTEX_S_C_B_FLASH                   0x07UL
#define configTEX_S_C_B_SRAM                    0x07UL
#define configENFORCE_SYSTEM_CALLS_FROM_KERNEL_ONLY 1
#define configALLOW_UNPRIVILEGED_CRITICAL_SECTIONS 1

/* ================================================================================
 * OPTIONAL FUNCTIONS
 * ================================================================================ */

#define INCLUDE_vTaskPrioritySet                1
#define INCLUDE_uxTaskPriorityGet               1
#define INCLUDE_vTaskDelete                     1
#define INCLUDE_vTaskSuspend                    1
#define INCLUDE_xResumeFromISR                  1
#define INCLUDE_vTaskDelayUntil                 1  /* PENTING untuk RMS! */
#define INCLUDE_vTaskDelay                      1
#define INCLUDE_xTaskGetSchedulerState          1
#define INCLUDE_xTaskGetCurrentTaskHandle       1
#define INCLUDE_uxTaskGetStackHighWaterMark     1
#define INCLUDE_xTaskGetIdleTaskHandle          1
#define INCLUDE_eTaskGetState                   1
#define INCLUDE_xEventGroupSetBitFromISR        1
#define INCLUDE_xTimerPendFunctionCall          1
#define INCLUDE_xTaskAbortDelay                 1
#define INCLUDE_xSemaphoreGetMutexHolder        1

/* ================================================================================
 * KONFIGURASI HARDWARE - STM32F103C8T6
 * ================================================================================ */

/* LED pin (PC13 = LED built-in Blue Pill, active low) */
#define LED_PORT                                GPIOC
#define LED_PIN                                 GPIO_PIN_13

/* UART untuk debug output */
#define UART_BAUDRATE                           115200

/* ================================================================================
 * KONFIGURASI TASK - RATE MONOTONIC SCHEDULING
 * ================================================================================
 * 
 * KONSEP RMS:
 * Task dengan periode LEBIH PENDEK mendapat prioritas LEBIH TINGGI
 * Karena task yang lebih sering dieksekusi harus lebih responsif
 * 
 * RUMUS: U = Σ(Ci/Ti) <= n(2^(1/n) - 1)
 * Dimana:
 *   U  = CPU utilization
 *   Ci = Waktu eksekusi task i
 *   Ti = Periode task i
 *   n  = Jumlah task
 * 
 * Untuk 3 task: Batas utilization = 0.78 (78%)
 * 
 * ================================================================================ */

/* High Priority Task - Periode 100ms (frekuensi tertinggi) */
#define HIGH_TASK_STACK_SIZE                    256
#define HIGH_TASK_PRIORITY                      (tskIDLE_PRIORITY + 3)
#define HIGH_TASK_PERIOD_MS                     100

/* Medium Priority Task - Periode 250ms */
#define MEDIUM_TASK_STACK_SIZE                  256
#define MEDIUM_TASK_PRIORITY                    (tskIDLE_PRIORITY + 2)
#define MEDIUM_TASK_PERIOD_MS                   250

/* Low Priority Task - Periode 500ms (frekuensi terendah) */
#define LOW_TASK_STACK_SIZE                     256
#define LOW_TASK_PRIORITY                       (tskIDLE_PRIORITY + 1)
#define LOW_TASK_PERIOD_MS                      500

/* Simulasi beban kerja (loop iterations) */
#define HIGH_TASK_WORK_ITERATIONS               10000
#define MEDIUM_TASK_WORK_ITERATIONS             20000
#define LOW_TASK_WORK_ITERATIONS                50000

#endif /* FREERTOS_CONFIG_H */
