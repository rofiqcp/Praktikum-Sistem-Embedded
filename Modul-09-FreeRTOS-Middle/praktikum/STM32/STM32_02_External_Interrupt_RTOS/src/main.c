// File utama program STM32_02_External_Interrupt_RTOS
// Menggunakan EXTI interrupt dengan semaphore FreeRTOS
// Button PA0 interrupt -> give semaphore
// Task mengambil semaphore dan toggle LED


#if defined(STM32F103xB)
#include "stm32f1xx_hal.h"
#elif defined(STM32F401xC) || defined(STM32F411xE)
#include "stm32f4xx_hal.h"
#endif
// Menginclude header file utama STM32 HAL
// Menginclude header FreeRTOS untuk task management
#include "FreeRTOS.h"
// Menginclude header FreeRTOS untuk task creation
#include "task.h"
// Menginclude header FreeRTOS untuk semaphore
#include "semphr.h"
// Menginclude header konfigurasi custom
#include "config.h"

// Mendeklarasikan handle untuk semaphore binary
SemaphoreHandle_t buttonSemaphore = NULL;
// Mendeklarasikan handle untuk task LED
TaskHandle_t ledTaskHandle = NULL;

// Fungsi prototipe untuk inisialisasi HAL
void SystemClock_Config(void);
// Fungsi prototipe untuk inisialisasi GPIO dan interrupt
static void MX_GPIO_Init(void);
// Fungsi prototipe task LED yang menunggu semaphore
void LedTask(void *argument);
// Fungsi prototipe callback interrupt EXTI
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin);

// Fungsi utama program
int main(void)
{
    // Menginisialisasi HAL Library
    HAL_Init();
    // Mengkonfigurasi system clock
    SystemClock_Config();
    // Menginisialisasi GPIO dan interrupt
    MX_GPIO_Init();

    // Membuat binary semaphore untuk sinkronisasi interrupt dan task
    buttonSemaphore = xSemaphoreCreateBinary();

    // Membuat task LED yang menunggu semaphore
    xTaskCreate(LedTask, "LED_Task", TASK_STACK_SIZE, NULL, LED_TASK_PRIORITY, &ledTaskHandle);

    // Memulai scheduler FreeRTOS
    vTaskStartScheduler();

    // Loop tak terbatas (seharusnya tidak pernah sampai sini)
    while (1)
    {
    }
}

// Implementasi fungsi konfigurasi system clock
void SystemClock_Config(void)
{
    // Mendeklarasikan struktur konfigurasi clock
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    // Mendeklarasikan struktur konfigurasi clock bus
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    // Mengkonfigurasi oscillator HSI (High Speed Internal) 8MHz
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    // Mengaktifkan HSI
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    // Tidak menggunakan PLL
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
    // Menginisialisasi konfigurasi oscillator
    HAL_RCC_OscConfig(&RCC_OscInitStruct);

    // Mengkonfigurasi clock bus
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK|RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
    // Sumber clock adalah HSI
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
    // Pembagi HCLK = 1
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    // Pembagi PCLK1 = 1
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    // Pembagi PCLK2 = 1
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    // Menginisialisasi konfigurasi clock bus
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0);
}

// Implementasi fungsi inisialisasi GPIO dan interrupt
static void MX_GPIO_Init(void)
{
    // Mendeklarasikan struktur konfigurasi pin GPIO
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // Mengaktifkan clock untuk GPIOC (LED)
    __HAL_RCC_GPIOC_CLK_ENABLE();
    // Mengaktifkan clock untuk GPIOA (Button)
    __HAL_RCC_GPIOA_CLK_ENABLE();

    // Mengkonfigurasi pin PC13 sebagai output push-pull untuk LED
    GPIO_InitStruct.Pin = LED_PIN;
    // Mode output push-pull
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    // Kecepatan rendah
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    // Menginisialisasi pin GPIOC
    HAL_GPIO_Init(LED_PORT, &GPIO_InitStruct);

    // Mengkonfigurasi pin PA0 sebagai interrupt falling edge untuk Button
    GPIO_InitStruct.Pin = BUTTON_PIN;
    // Mode interrupt pada falling edge (tombol ditekan)
    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
    // Menggunakan pull-up internal
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    // Menginisialisasi pin GPIOA dengan interrupt
    HAL_GPIO_Init(BUTTON_PORT, &GPIO_InitStruct);

    // Mengaktifkan interrupt EXTI0 di NVIC
    HAL_NVIC_SetPriority(EXTI0_IRQn, 5, 0);
    // Mengaktifkan EXTI0 interrupt
    HAL_NVIC_EnableIRQ(EXTI0_IRQn);
}

// Implementasi task LED yang menunggu semaphore dari interrupt
void LedTask(void *argument)
{
    // Loop tak terbatas untuk task
    while (1)
    {
        // Menunggu semaphore dari interrupt (blocking dengan timeout portMAX_DELAY)
        if (xSemaphoreTake(buttonSemaphore, portMAX_DELAY) == pdTRUE)
        {
            // Semaphore diterima - toggle LED
            HAL_GPIO_TogglePin(LED_PORT, LED_PIN);
        }
    }
}

// Implementasi callback interrupt EXTI
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    // Mengecek apakah interrupt dari pin PA0 (BUTTON_PIN = GPIO_PIN_0)
    if (GPIO_Pin == BUTTON_PIN)
    {
        // Memberikan semaphore dari context interrupt
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        // Give semaphore untuk membangunkan task LED
        xSemaphoreGiveFromISR(buttonSemaphore, &xHigherPriorityTaskWoken);
        // Melakukan context switch jika diperlukan
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

// Handler interrupt EXTI0 (PA0)
void EXTI0_IRQHandler(void)
{
    // Memanggil HAL GPIO EXTI handler
    HAL_GPIO_EXTI_IRQHandler(BUTTON_PIN);
}
