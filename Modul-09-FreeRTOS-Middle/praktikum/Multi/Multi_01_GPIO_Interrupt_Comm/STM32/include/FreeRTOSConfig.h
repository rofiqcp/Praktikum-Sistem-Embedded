#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/* ARM CM3/CM4F configuration */

/* Ensure stdint is included */
#include <stdint.h>

/* Application specific configuration options */
#define configUSE_PREEMPTION                    1
#define configUSE_IDLE_HOOK                       0
#define configUSE_TICK_HOOK                       0
#define configCPU_CLOCK_HZ                        ( ( uint32_t ) 72000000 )  /* STM32F103 */
#define configTICK_RATE_HZ                          ( ( TickType_t ) 1000 )
#define configMAX_PRIORITIES                    ( 5 )
#define configMINIMAL_STACK_SIZE                ( ( uint16_t ) 128 )
#define configTOTAL_HEAP_SIZE                    ( ( size_t ) ( 4 * 1024 ) )
#define configMAX_TASK_NAME_LEN                  ( 16 )
#define configUSE_16_BIT_TICKS                  0
#define configIDLE_SHOULD_YIELD                 1

/* Co-routine definitions */
#define configUSE_CO_ROUTINES                    0
#define configMAX_CO_ROUTINE_PRIORITIES        ( 2 )

/* Set the following definitions to 1 to include the API function, or zero to exclude the API function */
#define INCLUDE_vTaskPrioritySet                1
#define INCLUDE_uxTaskPriorityGet                1
#define INCLUDE_vTaskDelete                        1
#define INCLUDE_vTaskCleanUpResources         0
#define INCLUDE_vTaskSuspend                   1
#define INCLUDE_vTaskDelayUntil                 1
#define INCLUDE_vTaskDelay                         1
#define INCLUDE_xTaskGetIdleTaskHandle       0
#define INCLUDE_xTaskAbortDelay                 0
#define INCLUDE_xQueueGetMutexHolder           1
#define INCLUDE_xSemaphoreGetMutexHolder      INCLUDE_xQueueGetMutexHolder
#define INCLUDE_xTaskGetHandle                    1
#define INCLUDE_eTaskGetState                      1
#define INCLUDE_xTimerPendFunctionCall          0
#define INCLUDE_xTaskResumeFromISR                 1
#define INCLUDE_xTaskGetSchedulerState          1

/* Cortex-M specific definitions */
#define configPRIO_BITS                             4        /* 15 priority levels */
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY         0xf
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY  5
#define configKERNEL_INTERRUPT_PRIORITY             ( configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8 - configPRIO_BITS) )
#define configMAX_SYSCALL_INTERRUPT_PRIORITY      ( configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - configPRIO_BITS) )

/* Optional functions - most linkers will remove unused functions anyway */
#define configUSE_MUTEXES                           1
#define configUSE_RECURSIVE_MUTEXES                1
#define configUSE_COUNTING_SEMAPHORES               1
#define configUSE_ALTERNATIVE_API                  0  /* Deprecated! */
#define configCHECK_FOR_STACK_OVERFLOW              0
#define configUSE_RECORD_STACK_HIGH_ADDRESS        0
#define configUSE_TRACING                        0
#define configUSE_STATS_FORMATTING_FUNCTIONS      0
#define configUSE_PORT_OPTIMISED_TASK_SELECTION  0

/* Software timer definitions */
#define configUSE_TIMERS                           1
#define configTIMER_TASK_PRIORITY                ( configMAX_PRIORITIES - 1 )
#define configTIMER_QUEUE_LENGTH                   5
#define configTIMER_TASK_STACK_DEPTH            ( configMINIMAL_STACK_SIZE * 2 )

/* Set to 1 to use queue sets */
#define configUSE_QUEUE_SETS                     0

/* Set to 1 to use event groups */
#define configUSE_EVENT_GROUPS                   1

/* Set to 1 to use task notifications */
#define configUSE_TASK_NOTIFICATIONS             1

/* Run time stats enable */
#define configGENERATE_RUN_TIME_STATS          0

/* Define to trap errors on xTaskCreate if you need */
#define configASSERT( x )                         
/* Use the system heap allocation */
#define configSUPPORT_DYNAMIC_ALLOCATION           1
#define configSUPPORT_STATIC_ALLOCATION             0

/* Hook function related definitions */
#define configUSE_MALLOC_FAILED_HOOK             0
#define configUSE_DAEMON_TASK_STARTUP_HOOK       0

/* Cortex-M3/4 crash debug config */
#define configTASK_RETURN_ADDRESS                     0

#endif /* FREERTOS_CONFIG_H */
