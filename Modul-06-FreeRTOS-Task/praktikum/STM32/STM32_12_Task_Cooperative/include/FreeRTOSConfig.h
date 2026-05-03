/**
 * ============================================================================
 * FreeRTOSConfig.h — Konfigurasi Kernel FreeRTOS untuk STM32F103 (Cortex-M3)
 * ============================================================================
 * Target  : Blue Pill STM32F103C8T6
 * Clock   : HSE 8MHz → PLL x9 → 72MHz
 * Program : STM32_12_Task_Cooperative
 * ============================================================================
 */

#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/* --------------------------------------------------------------------------
 * Deteksi otomatis jumlah bit prioritas NVIC dari CMSIS
 * STM32F103 (Cortex-M3) memiliki 4 bit prioritas
 * -------------------------------------------------------------------------- */
#ifdef __NVIC_PRIO_BITS
  #define configPRIO_BITS __NVIC_PRIO_BITS
#else
  #define configPRIO_BITS 4
#endif

/* --------------------------------------------------------------------------
 * Konfigurasi Dasar Kernel
 * -------------------------------------------------------------------------- */
#define configUSE_PREEMPTION                    1       /* Gunakan preemptive scheduling */
#if defined(STM32F103xB)
#define configCPU_CLOCK_HZ                      ((uint32_t)72000000)
#elif defined(STM32F401xC)
#define configCPU_CLOCK_HZ                      ((uint32_t)84000000)
#elif defined(STM32F411xE)
#define configCPU_CLOCK_HZ                      ((uint32_t)100000000)
#else
#define configCPU_CLOCK_HZ                      ((uint32_t)72000000)
#endif
#define configTICK_RATE_HZ                      1000    /* Tick rate 1ms */
#define configMAX_PRIORITIES                    7       /* Jumlah level prioritas */
#define configMINIMAL_STACK_SIZE                128     /* Stack minimum dalam words */
#define configMAX_TASK_NAME_LEN                 16      /* Panjang maksimum nama task */
#define configUSE_16_BIT_TICKS                  0       /* Gunakan 32-bit tick counter */
#define configTOTAL_HEAP_SIZE                   10240   /* Total heap FreeRTOS 10KB */

/* --------------------------------------------------------------------------
 * Konfigurasi Alokasi Memori
 * -------------------------------------------------------------------------- */
#define configSUPPORT_STATIC_ALLOCATION         1       /* Dukung alokasi statis */
#define configSUPPORT_DYNAMIC_ALLOCATION        1       /* Dukung alokasi dinamis */

/* --------------------------------------------------------------------------
 * Konfigurasi Fitur Sinkronisasi
 * -------------------------------------------------------------------------- */
#define configUSE_MUTEXES                       1       /* Aktifkan mutex */
#define configUSE_COUNTING_SEMAPHORES           1       /* Aktifkan counting semaphore */
#define configUSE_TASK_NOTIFICATIONS            1       /* Aktifkan task notification */

/* --------------------------------------------------------------------------
 * Konfigurasi Hook Functions
 * -------------------------------------------------------------------------- */
#define configUSE_IDLE_HOOK                     0       /* Tidak gunakan idle hook */
#define configUSE_TICK_HOOK                     0       /* Tidak gunakan tick hook */
#define configCHECK_FOR_STACK_OVERFLOW          2       /* Deteksi stack overflow metode 2 */
#define configUSE_MALLOC_FAILED_HOOK            1       /* Hook saat malloc gagal */

/* --------------------------------------------------------------------------
 * Konfigurasi Trace & Statistik Runtime
 * -------------------------------------------------------------------------- */
#define configUSE_TRACE_FACILITY                1       /* Aktifkan trace facility */
#define configUSE_STATS_FORMATTING_FUNCTIONS    1       /* Aktifkan vTaskList(), vTaskGetRunTimeStats() */
#define configGENERATE_RUN_TIME_STATS           0       /* Nonaktifkan run-time stats */

/* --------------------------------------------------------------------------
 * Konfigurasi Software Timer
 * -------------------------------------------------------------------------- */
#define configUSE_TIMERS                        1       /* Aktifkan software timer */
#define configTIMER_TASK_PRIORITY               3       /* Prioritas timer task */
#define configTIMER_QUEUE_LENGTH                10      /* Panjang antrian timer */
#define configTIMER_TASK_STACK_DEPTH            configMINIMAL_STACK_SIZE

/* --------------------------------------------------------------------------
 * Konfigurasi Prioritas Interrupt (Cortex-M3 Specific)
 * -------------------------------------------------------------------------- */
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY         15
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY    5
#define configKERNEL_INTERRUPT_PRIORITY         (configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8 - configPRIO_BITS))
#define configMAX_SYSCALL_INTERRUPT_PRIORITY    (configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - configPRIO_BITS))

/* --------------------------------------------------------------------------
 * Macro Assert — Loop tak terbatas jika assertion gagal
 * -------------------------------------------------------------------------- */
#define configASSERT(x) if ((x) == 0) {taskDISABLE_INTERRUPTS(); for(;;);}

/* --------------------------------------------------------------------------
 * Fungsi API Opsional yang Diaktifkan
 * -------------------------------------------------------------------------- */
#define INCLUDE_vTaskPrioritySet                1
#define INCLUDE_uxTaskPriorityGet               1
#define INCLUDE_vTaskDelete                     1
#define INCLUDE_vTaskSuspend                    1
#define INCLUDE_xResumeFromISR                  1
#define INCLUDE_vTaskDelayUntil                 1
#define INCLUDE_vTaskDelay                      1
#define INCLUDE_xTaskGetSchedulerState          1
#define INCLUDE_xTaskGetCurrentTaskHandle       1
#define INCLUDE_uxTaskGetStackHighWaterMark     1
#define INCLUDE_eTaskGetState                   1
#define INCLUDE_xTimerPendFunctionCall          1

#endif /* FREERTOS_CONFIG_H */
