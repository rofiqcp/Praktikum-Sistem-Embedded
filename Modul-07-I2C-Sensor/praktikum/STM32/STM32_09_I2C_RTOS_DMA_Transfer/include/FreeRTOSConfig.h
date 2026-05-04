#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#include "config.h"

// Basic configuration
#define configUSE_PREEMPTION                    1
#define configUSE_IDLE_HOOK                     0
#define configUSE_TICK_HOOK                     0
#define configCPU_CLOCK_HZ                      (SystemCoreClock)
#define configTICK_RATE_HZ                      ((TickType_t)1000)
#define configMAX_PRIORITIES                    32
#define configMINIMAL_STACK_SIZE                ((uint16_t)128)
#define configTOTAL_HEAP_SIZE                   ((size_t)8192)
#define configMAX_TASK_NAME_LEN                 16
#define configUSE_TRACE_FACILITY                0
#define configUSE_16_BIT_TICKS                  0
#define configIDLE_SHOULD_YIELD                1
#define configUSE_MUTEXES                       1
#define configQUEUE_REGISTRY_SIZE               0
#define configCHECK_FOR_STACK_OVERFLOW          0
#define configUSE_RECURSIVE_MUTEXES             1
#define configUSE_MALLOC_FAILED_HOOK            0
#define configUSE_APPLICATION_TASK_TAG          0
#define configUSE_COUNTING_SEMAPHORES           1
#define configUSE_QUEUE_SETS                    0
#define configUSE_TASK_NOTIFICATIONS            1
#define configUSE_TIME_SLICING                  1
#define configUSE_NEWLIB_REENTRANT              0
#define configENABLE_BACKWARD_COMPATIBILITY     0
#define configNUM_THREAD_LOCAL_STORAGE_POINTERS  0

// Cortex-M specific settings
#define configKERNEL_INTERRUPT_PRIORITY         (255)
#define configMAX_SYSCALL_INTERRUPT_PRIORITY    (191)
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY   15
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY 5

// Memory allocation
#define configSUPPORT_STATIC_ALLOCATION         0
#define configSUPPORT_DYNAMIC_ALLOCATION       1
#define configUSE_PORT_OPTIMISED_TASK_SELECTION 0

// Tickless idle (optional, disabled for simplicity)
#define configUSE_TICKLESS_IDLE                0

// Hook function related definitions
#define configSTACK_DEPTH_TYPE                  uint16_t
#define configMESSAGE_BUFFER_LENGTH_TYPE       size_t

#endif // FREERTOS_CONFIG_H
