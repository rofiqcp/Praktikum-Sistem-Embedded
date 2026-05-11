// File utama program STM32_03_Encoder_2Pin_Interrupt
// Rotary encoder dengan 2-pin external interrupt
// Encoder A pada PA0, B pada PA1
// Interrupt pada kedua pin untuk decode rotasi
// Task untuk menampilkan count

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
// Menginclude header FreeRTOS untuk queue
#include "queue.h"
// Menginclude header konfigurasi custom
#include "config.h"

// Mendeklarasikan variabel global untuk menyimpan count encoder
volatile int32_t encoder_count = 0;
// Mendeklarasikan variabel untuk menyimpan state sebelumnya dari encoder
volatile uint8_t encoder_prev_state = 0;
// Mendeklarasikan handle untuk queue yang mengirim data encoder
QueueHandle_t encoderQueue = NULL;
// Mendeklarasikan handle untuk task display
TaskHandle_t displayTaskHandle = NULL;

// Fungsi prototipe untuk inisialisasi HAL
void SystemClock_Config(void);
// Fungsi prototipe untuk inisialisasi GPIO dan interrupt
static void MX_GPIO_Init(void);
// Fungsi prototipe task display encoder count
void DisplayTask(void *argument);
// Fungsi prototipe callback interrupt EXTI
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin);
// Fungsi untuk membaca state encoder
uint8_t ReadEncoderState(void);

// Fungsi utama program
int main(void)
{
    // Menginisialisasi HAL Library
    HAL_Init();
    // Mengkonfigurasi system clock
    SystemClock_Config();
    // Menginisialisasi GPIO dan interrupt encoder
    MX_GPIO_Init();

    // Membuat queue untuk mengirim data encoder count (ukuran 10 item, tipe int32_t)
    encoderQueue = xQueueCreate(10, sizeof(int32_t));

    // Membuat task display untuk menampilkan encoder count
    xTaskCreate(DisplayTask, "Display_Task", TASK_STACK_SIZE, NULL, DISPLAY_TASK_PRIORITY, &displayTaskHandle);

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

    // Mengkonfigurasi oscillator HSI 8MHz
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

// Fungsi untuk membaca state encoder (gabungan pin A dan B)
uint8_t ReadEncoderState(void)
{
    // Mendeklarasikan variabel untuk menyimpan state
    uint8_t state = 0;
    // Membaca pin Encoder A (PA0) - bit 0
    if (HAL_GPIO_ReadPin(ENCODER_A_PORT, ENCODER_A_PIN) == GPIO_PIN_SET)
    {
        state |= 0x01;
    }
    // Membaca pin Encoder B (PA1) - bit 1
    if (HAL_GPIO_ReadPin(ENCODER_B_PORT, ENCODER_B_PIN) == GPIO_PIN_SET)
    {
        state |= 0x02;
    }
    // Mengembalikan state encoder
    return state;
}

// Implementasi fungsi inisialisasi GPIO dan interrupt encoder
static void MX_GPIO_Init(void)
{
    // Mendeklarasikan struktur konfigurasi pin GPIO
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // Mengaktifkan clock untuk GPIOC (LED)
    __HAL_RCC_GPIOC_CLK_ENABLE();
    // Mengaktifkan clock untuk GPIOA (Encoder A dan B)
    __HAL_RCC_GPIOA_CLK_ENABLE();

    // Mengkonfigurasi pin PC13 sebagai output untuk LED indikasi
    GPIO_InitStruct.Pin = LED_PIN;
    // Mode output push-pull
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    // Kecepatan rendah
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    // Menginisialisasi pin GPIOC
    HAL_GPIO_Init(LED_PORT, &GPIO_InitStruct);

    // Mengkonfigurasi pin PA0 (Encoder A) sebagai interrupt pada kedua edge
    GPIO_InitStruct.Pin = ENCODER_A_PIN;
    // Mode interrupt pada rising dan falling edge
    GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
    // Menggunakan pull-up internal
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    // Menginisialisasi pin PA0
    HAL_GPIO_Init(ENCODER_A_PORT, &GPIO_InitStruct);

    // Mengkonfigurasi pin PA1 (Encoder B) sebagai interrupt pada kedua edge
    GPIO_InitStruct.Pin = ENCODER_B_PIN;
    // Mode interrupt pada rising dan falling edge
    GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
    // Menggunakan pull-up internal
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    // Menginisialisasi pin PA1
    HAL_GPIO_Init(ENCODER_B_PORT, &GPIO_InitStruct);

    // Mengaktifkan interrupt EXTI0 (PA0) di NVIC
    HAL_NVIC_SetPriority(EXTI0_IRQn, 5, 0);
    // Mengaktifkan EXTI0 interrupt
    HAL_NVIC_EnableIRQ(EXTI0_IRQn);

    // Mengaktifkan interrupt EXTI1 (PA1) di NVIC
    HAL_NVIC_SetPriority(EXTI1_IRQn, 5, 0);
    // Mengaktifkan EXTI1 interrupt
    HAL_NVIC_EnableIRQ(EXTI1_IRQn);

    // Membaca state awal encoder
    encoder_prev_state = ReadEncoderState();
}

// Implementasi task display encoder count
void DisplayTask(void *argument)
{
    // Mendeklarasikan variabel untuk menerima data dari queue
    int32_t received_count = 0;
    // Mendeklarasikan variabel untuk menyimpan count sebelumnya
    int32_t last_count = 0;

    // Loop tak terbatas untuk task
    while (1)
    {
        // Mengecek apakah ada data di queue
        if (xQueueReceive(encoderQueue, &received_count, 0) == pdTRUE)
        {
            // Data diterima dari queue
            // Mengecek apakah count berubah
            if (received_count != last_count)
            {
                // Count berubah - bisa tampilkan ke serial/UART di sini
                // Untuk sekarang, toggle LED sebagai indikasi
                HAL_GPIO_TogglePin(LED_PORT, LED_PIN);
                // Update last count
                last_count = received_count;
            }
        }
        // Delay 10ms untuk polling queue
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// Implementasi callback interrupt EXTI untuk encoder
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    // Mengecek apakah interrupt dari pin encoder A atau B
    if (GPIO_Pin == ENCODER_A_PIN || GPIO_Pin == ENCODER_B_PIN)
    {
        // Membaca state encoder saat ini
        uint8_t current_state = ReadEncoderState();
        // Mendeklarasikan variabel untuk menyimpan direction
        int8_t direction = 0;

        // Decode rotasi encoder berdasarkan state transition
        // Tabel transisi: prev_state -> current_state
        if (encoder_prev_state == 0x00 && current_state == 0x01) direction = 1;
        else if (encoder_prev_state == 0x01 && current_state == 0x03) direction = 1;
        else if (encoder_prev_state == 0x03 && current_state == 0x02) direction = 1;
        else if (encoder_prev_state == 0x02 && current_state == 0x00) direction = 1;
        else if (encoder_prev_state == 0x00 && current_state == 0x02) direction = -1;
        else if (encoder_prev_state == 0x02 && current_state == 0x03) direction = -1;
        else if (encoder_prev_state == 0x03 && current_state == 0x01) direction = -1;
        else if (encoder_prev_state == 0x01 && current_state == 0x00) direction = -1;

        // Mengecek apakah ada rotasi terdeteksi
        if (direction != 0)
        {
            // Update encoder count
            encoder_count += direction;
            // Mengirim count ke queue dari ISR
            BaseType_t xHigherPriorityTaskWoken = pdFALSE;
            // Mengirim data ke queue
            int32_t count = encoder_count;
            xQueueSendFromISR(encoderQueue, &count, &xHigherPriorityTaskWoken);
            // Melakukan context switch jika diperlukan
            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        }

        // Update state sebelumnya
        encoder_prev_state = current_state;
    }
}

// Handler interrupt EXTI0 (PA0 - Encoder A)
void EXTI0_IRQHandler(void)
{
    // Memanggil HAL GPIO EXTI handler
    HAL_GPIO_EXTI_IRQHandler(ENCODER_A_PIN);
}

// Handler interrupt EXTI1 (PA1 - Encoder B)
void EXTI1_IRQHandler(void)
{
    // Memanggil HAL GPIO EXTI handler
    HAL_GPIO_EXTI_IRQHandler(ENCODER_B_PIN);
}
