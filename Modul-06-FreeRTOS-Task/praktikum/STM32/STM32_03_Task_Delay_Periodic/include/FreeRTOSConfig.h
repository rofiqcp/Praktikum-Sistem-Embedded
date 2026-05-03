/**
 * ============================================================================
 * FreeRTOSConfig.h - Konfigurasi Kernel FreeRTOS
 * Target: STM32F103C8 (Cortex-M3) @ 72MHz
 * 
 * Konfigurasi untuk program perbandingan vTaskDelay vs vTaskDelayUntil
 * ============================================================================
 */

#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/* ======================== Konfigurasi Dasar ============================== */
#define configUSE_PREEMPTION                    1
#if defined(STM32F103xB)
#define configCPU_CLOCK_HZ                      72000000
#elif defined(STM32F401xC)
#define configCPU_CLOCK_HZ                      84000000
#elif defined(STM32F411xE)
#define configCPU_CLOCK_HZ                      100000000
#else
#define configCPU_CLOCK_HZ                      72000000
#endif
#define configTICK_RATE_HZ                      1000
#define configMAX_PRIORITIES                    7
#define configMINIMAL_STACK_SIZE                128
#define configTOTAL_HEAP_SIZE                   10240
#define configMAX_TASK_NAME_LEN                 16
#define configUSE_16_BIT_TICKS                  0
#define configIDLE_SHOULD_YIELD                 1

/* ====================== Fitur Sinkronisasi =============================== */
#define configUSE_MUTEXES                       1
#define configUSE_RECURSIVE_MUTEXES             1
#define configUSE_COUNTING_SEMAPHORES           1

/* ======================= Alokasi Memori ================================== */
#define configSUPPORT_STATIC_ALLOCATION         1
#define configSUPPORT_DYNAMIC_ALLOCATION        1

/* ======================== Hook Functions ================================= */
#define configUSE_IDLE_HOOK                     0
#define configUSE_TICK_HOOK                     0
#define configCHECK_FOR_STACK_OVERFLOW          2
#define configUSE_MALLOC_FAILED_HOOK            1

/* ======================== Software Timer ================================= */
#define configUSE_TIMERS                        1
#define configTIMER_TASK_PRIORITY               3
#define configTIMER_QUEUE_LENGTH                10
#define configTIMER_TASK_STACK_DEPTH            configMINIMAL_STACK_SIZE

/* ====================== Trace & Statistics =============================== */
#define configUSE_TRACE_FACILITY                1
#define configUSE_STATS_FORMATTING_FUNCTIONS    1

/* =================== Konfigurasi Interrupt NVIC ========================== */
#define configPRIO_BITS                         4
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY         15
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY    5
#define configKERNEL_INTERRUPT_PRIORITY         (configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8 - configPRIO_BITS))
#define configMAX_SYSCALL_INTERRUPT_PRIORITY    (configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - configPRIO_BITS))

/* ========================= Assert Macro ================================== */
#define configASSERT(x) if ((x) == 0) { taskDISABLE_INTERRUPTS(); for(;;); }

/* ==================== INCLUDE Function APIs ============================== */
#define INCLUDE_vTaskPrioritySet                1
#define INCLUDE_uxTaskPriorityGet               1
#define INCLUDE_vTaskDelete                     1
#define INCLUDE_vTaskSuspend                    1
#define INCLUDE_xResumeFromISR                  1
#define INCLUDE_vTaskDelayUntil                 1
#define INCLUDE_vTaskDelay                      1
#define INCLUDE_xTaskGetSchedulerState          1
#define INCLUDE_uxTaskGetStackHighWaterMark     1
#define INCLUDE_eTaskGetState                   1

/* ============== Pemetaan Handler ke Port FreeRTOS ======================== */
#define vPortSVCHandler     SVC_Handler
#define xPortPendSVHandler  PendSV_Handler

#endif /* FREERTOS_CONFIG_H */
