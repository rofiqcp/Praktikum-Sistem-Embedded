/**
 * ============================================================================
 * config.h - Konfigurasi Pin dan Parameter
 * STM32_03_Task_Delay_Periodic
 * 
 * Definisi pin, parameter task, dan konfigurasi hardware
 * untuk program perbandingan vTaskDelay vs vTaskDelayUntil
 * ============================================================================
 */

#ifndef CONFIG_H
#define CONFIG_H

/* ========================== Konfigurasi LED ============================== */
/* LED Built-in PC13 - Indikator vTaskDelay */
#define LED_DELAY_PORT          GPIOC
#define LED_DELAY_PIN           GPIO_PIN_13
#define LED_DELAY_CLK_EN()      __HAL_RCC_GPIOC_CLK_ENABLE()

/* LED Eksternal PB0 - Indikator vTaskDelayUntil */
#define LED_UNTIL_PORT          GPIOB
#define LED_UNTIL_PIN           GPIO_PIN_0
#define LED_UNTIL_CLK_EN()      __HAL_RCC_GPIOB_CLK_ENABLE()

/* LED Eksternal PB1 - Indikator sistem aktif */
#define LED_STATUS_PORT         GPIOB
#define LED_STATUS_PIN          GPIO_PIN_1
#define LED_STATUS_CLK_EN()     __HAL_RCC_GPIOB_CLK_ENABLE()

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

/* =================== Konfigurasi Task Periodik =========================== */
/* Task 1: Menggunakan vTaskDelay (akumulasi drift) */
#define TASK_DELAY_NAME         "Task_Delay"
#define TASK_DELAY_STACK        384
#define TASK_DELAY_PRIORITY     2
#define TASK_DELAY_PERIOD_MS    100

/* Task 2: Menggunakan vTaskDelayUntil (presisi) */
#define TASK_UNTIL_NAME         "Task_Until"
#define TASK_UNTIL_STACK        384
#define TASK_UNTIL_PRIORITY     2
#define TASK_UNTIL_PERIOD_MS    100

/* Task Monitor: Cetak statistik */
#define MONITOR_TASK_NAME       "Task_Monitor"
#define MONITOR_STACK_SIZE      512
#define MONITOR_PRIORITY        3
#define MONITOR_PERIOD_MS       5000

/* =================== Konfigurasi Pengukuran ============================== */
/* Jumlah sampel untuk statistik */
#define MAX_SAMPLES             100

/* Simulasi beban kerja (loop iterasi) */
#define WORKLOAD_ITERATIONS     50000UL

/* Batas deviasi yang dianggap signifikan (dalam us) */
#define DEVIATION_THRESHOLD_US  500

/* ===================== DWT Cycle Counter ================================= */
/* DWT registers untuk pengukuran presisi waktu */
#define DWT_CONTROL             (*((volatile uint32_t*)0xE0001000))
#define DWT_CYCCNT              (*((volatile uint32_t*)0xE0001004))
#define DWT_LAR                 (*((volatile uint32_t*)0xE0001FB0))
#define SCB_DEMCR               (*((volatile uint32_t*)0xE000EDFC))
#define TRCENA_BIT              (1UL << 24)
#define DWT_CTRL_ENABLE_BIT     (1UL << 0)
#define DWT_LAR_UNLOCK          0xC5ACCE55

/* Konversi siklus ke mikrodetik (72 MHz → 1 siklus = ~13.9 ns) */
#define CYCLES_TO_US(cycles)    ((uint32_t)((cycles) / (SYSTEM_CLOCK_MHZ)))

/* ======================= Konfigurasi Sistem ============================== */
#define SYSTEM_CLOCK_MHZ        72
#define PRINT_BUFFER_SIZE       256

/* ======================== Macro Utility ================================== */
#define LED_DELAY_TOGGLE()      HAL_GPIO_TogglePin(LED_DELAY_PORT, LED_DELAY_PIN)
#define LED_UNTIL_TOGGLE()      HAL_GPIO_TogglePin(LED_UNTIL_PORT, LED_UNTIL_PIN)
#define LED_STATUS_TOGGLE()     HAL_GPIO_TogglePin(LED_STATUS_PORT, LED_STATUS_PIN)

#endif /* CONFIG_H */
