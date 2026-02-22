/**
 * ================================================================================
 * KONFIGURASI FREERTOS UNTUK STM32F103C8T6 (Blue Pill)
 * ================================================================================
 * File: FreeRTOSConfig.h
 * Deskripsi: File konfigurasi FreeRTOS kernel untuk STM32F103C8T6
 * Nilai-nilai di sini dapat disesuaikan sesuai kebutuhan aplikasi
 * ================================================================================
 */

#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/* ================================================================================
 * KONFIGURASI PROCESSOR CORTEX-M3
 * ================================================================================ */

/* Priority bits untuk NVIC (Nested Vectored Interrupt Controller) */
#ifdef __NVIC_PRIO_BITS
    /* Gunakan nilai dari NVIC jika tersedia */
    #define configPRIO_BITS __NVIC_PRIO_BITS
#else
    /* Default untuk STM32F1: 4 priority bits */
    #define configPRIO_BITS 4
#endif

/* ================================================================================
 * KONFIGURASI FREERTOS KERNEL DASAR
 * ================================================================================ */

/* Gunakan preemptive scheduling (1) atau cooperative scheduling (0) */
#define configUSE_PREEMPTION                    1

/* Optimalisasi pemilihan task (1=aktif, 0=nonaktif) */
#define configUSE_PORT_OPTIMISED_TASK_SELECTION 0

/* Gunakan tickless idle mode untuk hemat daya (1=aktif, 0=nonaktif) */
#define configUSE_TICKLESS_IDLE                 0

/* Frekuensi clock CPU dalam Hz (72MHz untuk STM32F103 dengan HSE 8MHz x PLL 9) */
#define configCPU_CLOCK_HZ                      72000000

/* Frekuensi tick timer FreeRTOS dalam Hz (1000 = 1ms per tick) */
#define configTICK_RATE_HZ                      1000

/* Jumlah prioritas task maksimal yang bisa dibuat (1-7) */
#define configMAX_PRIORITIES                    7

/* Ukuran stack minimal untuk task dalam unit StackType_t (bytes) */
#define configMINIMAL_STACK_SIZE                128

/* Panjang maksimal nama task (termasuk null terminator) */
#define configMAX_TASK_NAME_LEN                 16

/* Gunakan tick counter 16-bit (0) atau 32-bit (1) */
#define configUSE_16_BIT_TICKS                  0

/* Idle task harus yield jika task dengan prioritas sama siap (1=ya, 0=tidak) */
#define configIDLE_SHOULD_YIELD                 1

/* Aktifkan task notification feature (1=aktif, 0=nonaktif) */
#define configUSE_TASK_NOTIFICATIONS            1

/* Jumlah notifikasi per task yang dapat disimpan */
#define configTASK_NOTIFICATION_ARRAY_ENTRIES   3

/* Aktifkan mutex untuk mutual exclusion (1=aktif, 0=nonaktif) */
#define configUSE_MUTEXES                       1

/* Aktifkan recursive mutex untuk nested locking (1=aktif, 0=nonaktif) */
#define configUSE_RECURSIVE_MUTEXES             1

/* Aktifkan counting semaphore (1=aktif, 0=nonaktif) */
#define configUSE_COUNTING_SEMAPHORES           1

/* Ukuran queue registry untuk tracking queue */
#define configQUEUE_REGISTRY_SIZE               8

/* Aktifkan queue sets feature (1=aktif, 0=nonaktif) */
#define configUSE_QUEUE_SETS                    1

/* Gunakan time slicing untuk task dengan prioritas sama (1=aktif, 0=nonaktif) */
#define configUSE_TIME_SLICING                  1

/* Gunakan newlib reentrant untuk C standard library (1=aktif, 0=nonaktif) */
#define configUSE_NEWLIB_REENTRANT              0

/* Backward compatibility dengan FreeRTOS versi lama (1=aktif, 0=nonaktif) */
#define configENABLE_BACKWARD_COMPATIBILITY     0

/* Jumlah thread local storage pointers yang tersedia */
#define configNUM_THREAD_LOCAL_STORAGE_POINTERS 5

/* Tipe data untuk kedalaman stack task */
#define configSTACK_DEPTH_TYPE                  uint16_t

/* Tipe data untuk panjang message buffer */
#define configMESSAGE_BUFFER_LENGTH_TYPE        size_t

/* ================================================================================
 * KONFIGURASI MEMORY ALLOCATION
 * ================================================================================ */

/* Aktifkan static memory allocation (1=aktif, 0=nonaktif) */
#define configSUPPORT_STATIC_ALLOCATION         1

/* Aktifkan dynamic memory allocation (1=aktif, 0=nonaktif) */
#define configSUPPORT_DYNAMIC_ALLOCATION        1

/* Ukuran total heap untuk dynamic allocation dalam bytes (dapat diubah) */
#define configTOTAL_HEAP_SIZE                   10240

/* Heap dialokasikan oleh aplikasi (1) atau FreeRTOS (0) */
#define configAPPLICATION_ALLOCATED_HEAP        0

/* ================================================================================
 * KONFIGURASI HOOK FUNCTIONS
 * ================================================================================ */

/* Gunakan idle hook function (dipanggil saat idle task berjalan) */
#define configUSE_IDLE_HOOK                     0

/* Gunakan tick hook function (dipanggil setiap tick kernel) */
#define configUSE_TICK_HOOK                     0

/* Level deteksi stack overflow (0=nonaktif, 1=method1, 2=method2) */
#define configCHECK_FOR_STACK_OVERFLOW          2

/* Hook function ketika malloc gagal (1=aktif, 0=nonaktif) */
#define configUSE_MALLOC_FAILED_HOOK            1

/* Daemon task startup hook function (1=aktif, 0=nonaktif) */
#define configUSE_DAEMON_TASK_STARTUP_HOOK      0

/* ================================================================================
 * KONFIGURASI STATISTIK DAN TRACING
 * ================================================================================ */

/* Generate runtime statistics untuk measuring task CPU usage (1=aktif, 0=nonaktif) */
#define configGENERATE_RUN_TIME_STATS           0

/* Gunakan trace facility untuk debugging (1=aktif, 0=nonaktif) */
#define configUSE_TRACE_FACILITY                1

/* Gunakan stats formatting functions (1=aktif, 0=nonaktif) */
#define configUSE_STATS_FORMATTING_FUNCTIONS    1

/* ================================================================================
 * KONFIGURASI CO-ROUTINE (jarang digunakan)
 * ================================================================================ */

/* Aktifkan co-routine support (1=aktif, 0=nonaktif) */
#define configUSE_CO_ROUTINES                   0

/* Jumlah prioritas co-routine maksimal */
#define configMAX_CO_ROUTINE_PRIORITIES         1

/* ================================================================================
 * KONFIGURASI SOFTWARE TIMER
 * ================================================================================ */

/* Aktifkan software timer (1=aktif, 0=nonaktif) - dapat diubah sesuai kebutuhan */
#define configUSE_TIMERS                        1

/* Prioritas timer task dalam sistem (0-6, nilai lebih tinggi = prioritas lebih tinggi) */
#define configTIMER_TASK_PRIORITY               3

/* Panjang queue untuk perintah timer (jumlah perintah timer yang tertunda) */
#define configTIMER_QUEUE_LENGTH                10

/* Kedalaman stack untuk timer task dalam unit StackType_t */
#define configTIMER_TASK_STACK_DEPTH            configMINIMAL_STACK_SIZE

/* ================================================================================
 * KONFIGURASI INTERRUPT NESTING (untuk STM32 dengan NVIC)
 * ================================================================================ */

/* Priority level terendah yang digunakan kernel FreeRTOS (0-15) */
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY         15

/* Priority level maksimal untuk syscall interrupt */
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY    5

/* Priority kernel interrupt (shifted ke posisi yang benar untuk NVIC) */
#define configKERNEL_INTERRUPT_PRIORITY         ( configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8 - configPRIO_BITS) )

/* Priority maksimal syscall interrupt (shifted ke posisi yang benar untuk NVIC) */
#define configMAX_SYSCALL_INTERRUPT_PRIORITY    ( configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - configPRIO_BITS) )

/* ================================================================================
 * KONFIGURASI ASSERTION
 * ================================================================================ */

/* Macro assertion: jika kondisi gagal, matikan interrupt dan loop infinite */
#define configASSERT( x ) if ((x) == 0) {taskDISABLE_INTERRUPTS(); for( ;; );}

/* ================================================================================
 * KONFIGURASI MPU (Memory Protection Unit - jarang digunakan di STM32F103)
 * ================================================================================ */

/* Aktifkan privileged functions yang didefinisikan aplikasi (1=aktif, 0=nonaktif) */
#define configINCLUDE_APPLICATION_DEFINED_PRIVILEGED_FUNCTIONS 0

/* Jumlah region MPU yang tersedia (1-16, biasanya 8) */
#define configTOTAL_MPU_REGIONS                 8

/* Atribut TEX/S/C/B untuk FLASH memory region */
#define configTEX_S_C_B_FLASH                   0x07UL

/* Atribut TEX/S/C/B untuk SRAM memory region */
#define configTEX_S_C_B_SRAM                    0x07UL

/* Enforce syscalls dari kernel saja (1=aktif, 0=nonaktif) */
#define configENFORCE_SYSTEM_CALLS_FROM_KERNEL_ONLY 1

/* Izinkan unprivileged code masuk critical section (1=aktif, 0=nonaktif) */
#define configALLOW_UNPRIVILEGED_CRITICAL_SECTIONS 1

/* ================================================================================
 * KONFIGURASI OPTIONAL FUNCTIONS (diinclude atau tidak dalam build)
 * ================================================================================ */

/* Include vTaskPrioritySet function (ubah prioritas task) */
#define INCLUDE_vTaskPrioritySet                1

/* Include uxTaskPriorityGet function (dapatkan prioritas task saat ini) */
#define INCLUDE_uxTaskPriorityGet               1

/* Include vTaskDelete function (hapus/delete task - PENTING untuk project ini) */
#define INCLUDE_vTaskDelete                     1

/* Include vTaskSuspend function (suspend task sementara) */
#define INCLUDE_vTaskSuspend                    1

/* Include xResumeFromISR function (resume task dari ISR) */
#define INCLUDE_xResumeFromISR                  1

/* Include vTaskDelayUntil function (delay task hingga waktu absolut) */
#define INCLUDE_vTaskDelayUntil                 1

/* Include vTaskDelay function (delay task relatif) */
#define INCLUDE_vTaskDelay                      1

/* Include xTaskGetSchedulerState function (status scheduler) */
#define INCLUDE_xTaskGetSchedulerState          1

/* Include xTaskGetCurrentTaskHandle function (dapatkan handle task saat ini) */
#define INCLUDE_xTaskGetCurrentTaskHandle       1

/* Include uxTaskGetStackHighWaterMark function (monitor stack usage) */
#define INCLUDE_uxTaskGetStackHighWaterMark     1

/* Include xTaskGetIdleTaskHandle function (dapatkan handle idle task) */
#define INCLUDE_xTaskGetIdleTaskHandle          1

/* Include eTaskGetState function (dapatkan state task) */
#define INCLUDE_eTaskGetState                   1

/* Include xEventGroupSetBitFromISR function (set event bits dari ISR) */
#define INCLUDE_xEventGroupSetBitFromISR        1

/* Include xTimerPendFunctionCall function (call function dari timer task) */
#define INCLUDE_xTimerPendFunctionCall          1

/* Include xTaskAbortDelay function (abort delay task) */
#define INCLUDE_xTaskAbortDelay                 1

/* CMSIS RTOS V2 API support */
#define INCLUDE_xSemaphoreGetMutexHolder        1

/* ================================================================================
 * KONFIGURASI HARDWARE SPESIFIK STM32F103C8T6
 * ================================================================================ */

/* Baud rate UART untuk debug output (dapat diubah: 115200, 9600, 230400, dll) */
#define UART_BAUDRATE                           115200

/* GPIO pin untuk LED (PC13 adalah LED bawaan Blue Pill) */
#define LED_PORT                                GPIOC
#define LED_PIN                                 GPIO_PIN_13

/* GPIO pin untuk button (PA0 adalah button bawaan Blue Pill) */
#define BUTTON_PORT                             GPIOA
#define BUTTON_PIN                              GPIO_PIN_0

/* Debounce delay dalam millisecond untuk button (nilai dapat diubah: 10-100ms) */
#define DEBOUNCE_DELAY_MS                       50

/* ================================================================================
 * KONFIGURASI TASK - MAIN TASK (Main Task - Memantau Tombol)
 * ================================================================================ */

/* Stack size untuk main task dalam unit StackType_t (bytes, dapat diubah) */
#define MAIN_TASK_STACK_SIZE                    256

/* Prioritas main task (0=terendah, 6=tertinggi, gunakan tskIDLE_PRIORITY + offset) */
#define MAIN_TASK_PRIORITY                      (tskIDLE_PRIORITY + 2)

/* Task delay/period dalam millisecond (dapat diubah sesuai kebutuhan) */
#define MAIN_TASK_DELAY_MS                      10

/* ================================================================================
 * KONFIGURASI TASK - DYNAMIC TASK (Task Dinamis - Mengedip LED)
 * ================================================================================ */

/* Stack size untuk dynamic task yang dibuat runtime (bytes, dapat diubah: 96-256) */
#define DYNAMIC_TASK_STACK_SIZE                 128

/* Prioritas dynamic task yang dibuat (dapat diubah: 0-5, harus < MAIN_TASK_PRIORITY) */
#define DYNAMIC_TASK_PRIORITY                   (tskIDLE_PRIORITY + 1)

/* LED toggle delay dalam millisecond untuk dynamic task (dapat diubah: 100-500ms) */
#define DYNAMIC_TASK_LED_DELAY_MS               200

/* Counter yang dipantau setiap N iterasi (dapat diubah: 5-20) */
#define DYNAMIC_TASK_PRINT_INTERVAL             10

#endif /* FREERTOS_CONFIG_H */
