/**
 * ================================================================================
 * KONFIGURASI FREERTOS UNTUK STM32F103C8T6 (Blue Pill)
 * ================================================================================
 * File: FreeRTOSConfig.h
 * Deskripsi: File konfigurasi FreeRTOS kernel untuk STM32F103C8T6
 * ================================================================================
 */

#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/* ================================================================================
 * KONFIGURASI PROCESSOR CORTEX-M3
 * ================================================================================ */

#ifdef __NVIC_PRIO_BITS
    #define configPRIO_BITS __NVIC_PRIO_BITS
#else
    #define configPRIO_BITS 4
#endif

/* ================================================================================
 * KONFIGURASI FREERTOS KERNEL DASAR
 * ================================================================================ */

#define configUSE_PREEMPTION                    1
#define configUSE_PORT_OPTIMISED_TASK_SELECTION 0
#define configUSE_TICKLESS_IDLE                 0
#define configCPU_CLOCK_HZ                      72000000
#define configTICK_RATE_HZ                      1000
#define configMAX_PRIORITIES                    7
#define configMINIMAL_STACK_SIZE                128
#define configMAX_TASK_NAME_LEN                 16
#define configUSE_16_BIT_TICKS                  0
#define configIDLE_SHOULD_YIELD                 1
#define configUSE_TASK_NOTIFICATIONS            1
#define configTASK_NOTIFICATION_ARRAY_ENTRIES   3
#define configUSE_MUTEXES                       1
#define configUSE_RECURSIVE_MUTEXES             1
#define configUSE_COUNTING_SEMAPHORES           1
#define configQUEUE_REGISTRY_SIZE               8
#define configUSE_QUEUE_SETS                    1
#define configUSE_TIME_SLICING                  1
#define configUSE_NEWLIB_REENTRANT              0
#define configENABLE_BACKWARD_COMPATIBILITY     0
#define configNUM_THREAD_LOCAL_STORAGE_POINTERS 5
#define configSTACK_DEPTH_TYPE                  uint16_t
#define configMESSAGE_BUFFER_LENGTH_TYPE        size_t

/* ================================================================================
 * KONFIGURASI MEMORY ALLOCATION
 * ================================================================================ */

#define configSUPPORT_STATIC_ALLOCATION         1
#define configSUPPORT_DYNAMIC_ALLOCATION        1
#define configTOTAL_HEAP_SIZE                   10240
#define configAPPLICATION_ALLOCATED_HEAP        0

/* ================================================================================
 * KONFIGURASI HOOK FUNCTIONS
 * ================================================================================ */

#define configUSE_IDLE_HOOK                     0
#define configUSE_TICK_HOOK                     0
#define configCHECK_FOR_STACK_OVERFLOW          2
#define configUSE_MALLOC_FAILED_HOOK            1
#define configUSE_DAEMON_TASK_STARTUP_HOOK      0

/* ================================================================================
 * KONFIGURASI STATISTIK DAN TRACING
 * ================================================================================ */

#define configGENERATE_RUN_TIME_STATS           0
#define configUSE_TRACE_FACILITY                1
#define configUSE_STATS_FORMATTING_FUNCTIONS    1

/* ================================================================================
 * KONFIGURASI CO-ROUTINE
 * ================================================================================ */

#define configUSE_CO_ROUTINES                   0
#define configMAX_CO_ROUTINE_PRIORITIES         1

/* ================================================================================
 * KONFIGURASI SOFTWARE TIMER
 * ================================================================================ */

#define configUSE_TIMERS                        1
#define configTIMER_TASK_PRIORITY               3
#define configTIMER_QUEUE_LENGTH                10
#define configTIMER_TASK_STACK_DEPTH            configMINIMAL_STACK_SIZE

/* ================================================================================
 * KONFIGURASI INTERRUPT NESTING
 * ================================================================================ */

#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY         15
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY    5
#define configKERNEL_INTERRUPT_PRIORITY         ( configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8 - configPRIO_BITS) )
#define configMAX_SYSCALL_INTERRUPT_PRIORITY    ( configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - configPRIO_BITS) )

/* ================================================================================
 * KONFIGURASI ASSERTION
 * ================================================================================ */

#define configASSERT( x ) if ((x) == 0) {taskDISABLE_INTERRUPTS(); for( ;; );}

/* ================================================================================
 * KONFIGURASI MPU
 * ================================================================================ */

#define configINCLUDE_APPLICATION_DEFINED_PRIVILEGED_FUNCTIONS 0
#define configTOTAL_MPU_REGIONS                 8
#define configTEX_S_C_B_FLASH                   0x07UL
#define configTEX_S_C_B_SRAM                    0x07UL
#define configENFORCE_SYSTEM_CALLS_FROM_KERNEL_ONLY 1
#define configALLOW_UNPRIVILEGED_CRITICAL_SECTIONS 1

/* ================================================================================
 * KONFIGURASI OPTIONAL FUNCTIONS
 * ================================================================================ */

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
#define INCLUDE_xTaskGetIdleTaskHandle          1
#define INCLUDE_eTaskGetState                   1
#define INCLUDE_xEventGroupSetBitFromISR        1
#define INCLUDE_xTimerPendFunctionCall          1
#define INCLUDE_xTaskAbortDelay                 1
#define INCLUDE_xSemaphoreGetMutexHolder        1

/* ================================================================================
 * KONFIGURASI HARDWARE SPESIFIK STM32F103C8T6
 * ================================================================================ */

#define UART_BAUDRATE                           115200
#define LED_PORT                                GPIOC
#define LED_PIN                                 GPIO_PIN_13
#define BUTTON_PORT                             GPIOA
#define BUTTON_PIN                              GPIO_PIN_0
#define DEBOUNCE_DELAY_MS                       50

/* ================================================================================
 * KONFIGURASI TASK
 * ================================================================================ */

#define MAIN_TASK_STACK_SIZE                    256
#define MAIN_TASK_PRIORITY                      (tskIDLE_PRIORITY + 2)
#define MAIN_TASK_DELAY_MS                      10

#endif /* FREERTOS_CONFIG_H */
