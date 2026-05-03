/**
 * ============================================================================
 * config.h - Konfigurasi Pin dan Parameter
 * STM32_02_Task_Priority
 * 
 * Definisi pin, parameter task, dan konfigurasi hardware
 * untuk program demonstrasi prioritas task FreeRTOS
 * ============================================================================
 */

#ifndef CONFIG_H
#define CONFIG_H

/* ========================== Konfigurasi LED ============================== */
/* LED Built-in pada Blue Pill (PC13, active LOW) */
#define LED_BUILTIN_PORT        GPIOC
#define LED_BUILTIN_PIN         GPIO_PIN_13
#define LED_BUILTIN_CLK_EN()    __HAL_RCC_GPIOC_CLK_ENABLE()

/* LED Eksternal 1 - Indikator Task LOW (PB0) */
#define LED_LOW_PORT            GPIOB
#define LED_LOW_PIN             GPIO_PIN_0
#define LED_LOW_CLK_EN()        __HAL_RCC_GPIOB_CLK_ENABLE()

/* LED Eksternal 2 - Indikator Task HIGH (PB1) */
#define LED_HIGH_PORT           GPIOB
#define LED_HIGH_PIN            GPIO_PIN_1
#define LED_HIGH_CLK_EN()       __HAL_RCC_GPIOB_CLK_ENABLE()

/* ========================== Konfigurasi Tombol =========================== */
#define BTN_PORT                GPIOA
#define BTN_PIN                 GPIO_PIN_0
#define BTN_CLK_EN()            __HAL_RCC_GPIOA_CLK_ENABLE()

/* ========================== Konfigurasi UART ============================= */
#define UART_INSTANCE           USART1
#define UART_BAUDRATE           115200
#define UART_TX_PORT            GPIOA
#define UART_TX_PIN             GPIO_PIN_9
#define UART_RX_PORT            GPIOA
#define UART_RX_PIN             GPIO_PIN_10
#define UART_CLK_EN()           __HAL_RCC_USART1_CLK_ENABLE()
#define UART_GPIO_CLK_EN()      __HAL_RCC_GPIOA_CLK_ENABLE()

/* ===================== Konfigurasi Task Prioritas ======================== */
/* Task Low Priority */
#define TASK_LOW_NAME           "Task_Low"
#define TASK_LOW_STACK          256
#define TASK_LOW_PRIORITY       1
#define TASK_LOW_LED_PORT       LED_LOW_PORT
#define TASK_LOW_LED_PIN        LED_LOW_PIN

/* Task Medium Priority */
#define TASK_MED_NAME           "Task_Med"
#define TASK_MED_STACK          256
#define TASK_MED_PRIORITY       3
#define TASK_MED_LED_PORT       LED_BUILTIN_PORT
#define TASK_MED_LED_PIN        LED_BUILTIN_PIN

/* Task High Priority */
#define TASK_HIGH_NAME          "Task_High"
#define TASK_HIGH_STACK         256
#define TASK_HIGH_PRIORITY      5
#define TASK_HIGH_LED_PORT      LED_HIGH_PORT
#define TASK_HIGH_LED_PIN       LED_HIGH_PIN

/* Task Monitor */
#define MONITOR_TASK_NAME       "Task_Monitor"
#define MONITOR_STACK_SIZE      512
#define MONITOR_PRIORITY        6
#define MONITOR_PERIOD_MS       3000

/* ===================== Parameter CPU Work ================================ */
/* Jumlah iterasi loop untuk simulasi beban CPU */
#define CPU_WORK_ITERATIONS     500000UL

/* Durasi antar fase dalam ms */
#define PHASE_DURATION_MS       8000
#define TASK_WORK_PERIOD_MS     1000

/* ======================= Konfigurasi Sistem ============================== */
#if defined(STM32F103xB)
#define SYSTEM_CLOCK_MHZ        72
#elif defined(STM32F401xC)
#define SYSTEM_CLOCK_MHZ        84
#elif defined(STM32F411xE)
#define SYSTEM_CLOCK_MHZ        100
#else
#define SYSTEM_CLOCK_MHZ        72
#endif
#define PRINT_BUFFER_SIZE       256
#define TOTAL_PHASES            3

/* ======================== Macro Utility ================================== */
#define LED_BUILTIN_ON()        HAL_GPIO_WritePin(LED_BUILTIN_PORT, LED_BUILTIN_PIN, GPIO_PIN_RESET)
#define LED_BUILTIN_OFF()       HAL_GPIO_WritePin(LED_BUILTIN_PORT, LED_BUILTIN_PIN, GPIO_PIN_SET)
#define LED_BUILTIN_TOGGLE()    HAL_GPIO_TogglePin(LED_BUILTIN_PORT, LED_BUILTIN_PIN)

#endif /* CONFIG_H */
