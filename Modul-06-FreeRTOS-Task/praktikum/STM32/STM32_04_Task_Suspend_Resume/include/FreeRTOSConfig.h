/**
 * ============================================================================
 * FILE: FreeRTOSConfig.h
 * PROJECT: 05-Task_Suspend_Resume
 * 
 * DESKRIPSI:
 * File konfigurasi FreeRTOS untuk demonstrasi Task Suspend dan Resume.
 * Program ini menunjukkan cara menghentikan sementara dan melanjutkan
 * eksekusi task tanpa menghapusnya dari memori.
 * 
 * KONSEP SUSPEND/RESUME:
 * - vTaskSuspend(handle): Menghentikan task secara paksa
 *   Task tidak akan dijadwalkan sampai di-resume
 *   Berbeda dengan BLOCKED - task tidak menunggu event
 * 
 * - vTaskResume(handle): Melanjutkan task yang di-suspend
 *   Task kembali ke state READY
 *   Akan dijadwalkan saat prioritasnya paling tinggi
 * 
 * KEGUNAAN SUSPEND/RESUME:
 * 1. Error Handling: Suspend task yang bermasalah
 * 2. Power Management: Suspend task saat tidak diperlukan
 * 3. Debug: Pause task untuk analisis
 * 4. Resource Management: Suspend saat resource tidak tersedia
 * 
 * HARDWARE TARGET: STM32F103C8T6 (Blue Pill)
 * - LED: PC13 (aktif LOW) - Toggle oleh Processor Task
 * - Button: PA0 (dengan pull-up) - Manual suspend/resume
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

#define configUSE_PREEMPTION                    1  /* Rentang: 0-1 */
#define configUSE_PORT_OPTIMISED_TASK_SELECTION 1  /* Rentang: 0-1 */
#define configUSE_TICKLESS_IDLE                 0  /* Rentang: 0-1 */
#if defined(STM32F103xB)
#define configCPU_CLOCK_HZ                      72000000
#elif defined(STM32F401xC)
#define configCPU_CLOCK_HZ                      84000000
#elif defined(STM32F411xE)
#define configCPU_CLOCK_HZ                      100000000
#else
#define configCPU_CLOCK_HZ                      72000000
#endif
#define configTICK_RATE_HZ                      ((TickType_t)1000)  /* Rentang: 1-10000 */
#define configMAX_PRIORITIES                    8  /* Rentang: 1-56 */
#define configMINIMAL_STACK_SIZE                ((uint16_t)128)  /* Rentang: 64-1024 */
#define configTOTAL_HEAP_SIZE                   ((size_t)(8 * 1024))  /* Rentang: 1024-16384 */
#define configMAX_TASK_NAME_LEN                 16  /* Rentang: 1-32 */
#define configUSE_16_BIT_TICKS                  0  /* Rentang: 0-1 */
#define configIDLE_SHOULD_YIELD                 1  /* Rentang: 0-1 */

/* ============================================================================
 * BAGIAN 2: FITUR SINKRONISASI
 * ============================================================================ */

#define configUSE_MUTEXES                       1  /* Rentang: 0-1 */
#define configUSE_RECURSIVE_MUTEXES             0  /* Rentang: 0-1 */
#define configUSE_COUNTING_SEMAPHORES           0  /* Rentang: 0-1 */

/* ============================================================================
 * BAGIAN 3: HOOK FUNCTIONS (CALLBACK)
 * ============================================================================ */

#define configUSE_IDLE_HOOK                     0  /* Rentang: 0-1 */
#define configUSE_TICK_HOOK                     0  /* Rentang: 0-1 */
#define configUSE_MALLOC_FAILED_HOOK            1  /* Aktif untuk debug */
#define configCHECK_FOR_STACK_OVERFLOW          2  /* Metode 2 - thorough */

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

#define configUSE_TIMERS                        0
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
 * ============================================================================
 * 
 * KRITIS UNTUK DEMO INI:
 * - INCLUDE_vTaskSuspend = 1 (WAJIB untuk suspend/resume)
 * - INCLUDE_eTaskGetState = 1 (untuk mengecek state task)
 * ============================================================================ */

#define INCLUDE_vTaskPrioritySet                0
#define INCLUDE_uxTaskPriorityGet               0
#define INCLUDE_vTaskDelete                     0
/* ---------------------------------------------------------------------------
 * INCLUDE_vTaskSuspend - KUNCI DARI DEMO INI!
 * ---------------------------------------------------------------------------
 * Mengaktifkan:
 * - vTaskSuspend(TaskHandle_t xTaskToSuspend)
 * - vTaskResume(TaskHandle_t xTaskToResume)
 * - xTaskResumeFromISR(TaskHandle_t xTaskToResume)
 * 
 * Juga mengaktifkan portMAX_DELAY yang berarti "block selamanya"
 * dalam fungsi seperti xQueueReceive, xSemaphoreTake, dll.
 */
#define INCLUDE_vTaskSuspend                    1  /* WAJIB untuk suspend/resume! */
#define INCLUDE_xResumeFromISR                  1  /* Resume dari ISR jika perlu */
#define INCLUDE_vTaskDelayUntil                 1
#define INCLUDE_vTaskDelay                      1
#define INCLUDE_xTaskGetSchedulerState          1
#define INCLUDE_xTaskGetCurrentTaskHandle       1  /* Untuk mendapat handle task sendiri */
#define INCLUDE_uxTaskGetStackHighWaterMark     1
#define INCLUDE_xTaskGetIdleTaskHandle          0
/* ---------------------------------------------------------------------------
 * INCLUDE_eTaskGetState - PENTING UNTUK MONITORING
 * ---------------------------------------------------------------------------
 * Mengaktifkan eTaskGetState(TaskHandle_t xTask) yang return:
 * - eRunning: Task sedang berjalan
 * - eReady: Task ready tapi tidak sedang berjalan
 * - eBlocked: Task sedang blocked (waiting event/delay)
 * - eSuspended: Task di-suspend
 * - eDeleted: Task sudah dihapus
 */
#define INCLUDE_eTaskGetState                   1  /* WAJIB untuk cek state task */
#define INCLUDE_xEventGroupSetBitFromISR        0
#define INCLUDE_xTimerPendFunctionCall          0
#define INCLUDE_xTaskAbortDelay                 0
#define INCLUDE_xTaskGetHandle                  0
#define INCLUDE_xTaskResumeFromISR              1  /* Resume dari ISR */

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
 */
#define LED_GPIO_PORT                           GPIOC
#define LED_GPIO_PIN                            GPIO_PIN_13
#define LED_ACTIVE_LOW                          1

/* ---------------------------------------------------------------------------
 * KONFIGURASI BUTTON
 * ---------------------------------------------------------------------------
 * Button pada PA0 dengan internal pull-up.
 * Tekan button untuk toggle suspend/resume secara manual.
 */
#define BUTTON_GPIO_PORT                        GPIOA
#define BUTTON_GPIO_PIN                         GPIO_PIN_0
#define BUTTON_ACTIVE_LOW                       1  /* Ditekan = LOW */
#define BUTTON_DEBOUNCE_MS                      50  /* Rentang: 10-100 ms */

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
 * PROCESSOR_TASK: Task yang memproses data
 * - Prioritas: 2
 * - Bisa di-suspend saat error
 * 
 * SUPERVISOR_TASK: Task yang mengawasi dan kontrol
 * - Prioritas: 3 (lebih tinggi - selalu bisa suspend processor)
 * - Mensimulasikan deteksi error
 * 
 * BUTTON_TASK: Task untuk monitor tombol
 * - Prioritas: 1 (terendah)
 * - Kontrol manual suspend/resume
 */
#define PROCESSOR_TASK_STACK_SIZE               256  /* Rentang: 128-512 words */
#define PROCESSOR_TASK_PRIORITY                 (tskIDLE_PRIORITY + 2)
#define PROCESSOR_TASK_NAME                     "Processor"
#define PROCESSOR_TASK_PERIOD_MS                300  /* Rentang: 100-1000 ms */

#define SUPERVISOR_TASK_STACK_SIZE              256  /* Rentang: 128-512 words */
#define SUPERVISOR_TASK_PRIORITY                (tskIDLE_PRIORITY + 3)
#define SUPERVISOR_TASK_NAME                    "Supervisor"
#define SUPERVISOR_TASK_PERIOD_MS               500  /* Rentang: 200-1000 ms */
#define ERROR_TRIGGER_CYCLE                     10   /* Error setiap N cycle */
#define ERROR_CLEAR_OFFSET                      5    /* Clear setelah N cycle dari trigger */

#define BUTTON_TASK_STACK_SIZE                  128  /* Rentang: 64-256 words */
#define BUTTON_TASK_PRIORITY                    (tskIDLE_PRIORITY + 1)
#define BUTTON_TASK_NAME                        "Button"
#define BUTTON_TASK_PERIOD_MS                   10   /* Polling setiap 10ms */

/* ---------------------------------------------------------------------------
 * KONFIGURASI SIMULASI
 * ---------------------------------------------------------------------------
 */
#define PROCESSING_SIMULATION_LOOPS             20000  /* Rentang: 5000-50000 */

#endif /* FREERTOS_CONFIG_H */
