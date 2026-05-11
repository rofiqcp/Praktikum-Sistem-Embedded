// File utama program STM32_01_GPIO_RTOS_Task
// Menggunakan FreeRTOS dengan STM32Cube HAL
// Mendemonstrasikan multiple GPIO tasks dengan pola LED

// Menginclude header FreeRTOS untuk task management
#include "FreeRTOS.h"
// Menginclude header FreeRTOS untuk task creation dan management
#include "task.h"
// Menginclude header konfigurasi custom
#include "config.h"

// Mendeklarasikan handle untuk task LED1
TaskHandle_t led1TaskHandle = NULL;
// Mendeklarasikan handle untuk task LED2
TaskHandle_t led2TaskHandle = NULL;
// Mendeklarasikan handle untuk task tombol
TaskHandle_t buttonTaskHandle = NULL;

// Fungsi prototipe untuk inisialisasi HAL
void SystemClock_Config(void);
// Fungsi prototipe untuk inisialisasi GPIO
static void MX_GPIO_Init(void);
// Fungsi prototipe task LED1 - blink 500ms
void Led1Task(void *argument);
// Fungsi prototipe task LED2 - blink 1000ms
void Led2Task(void *argument);
// Fungsi prototipe task tombol - read dengan debounce
void ButtonTask(void *argument);

// Fungsi utama program
int main(void)
{
    // Menginisialisasi HAL Library
    HAL_Init();
    // Mengkonfigurasi system clock
    SystemClock_Config();
    // Menginisialisasi GPIO (LED dan Button)
    MX_GPIO_Init();

    // Membuat task LED1 dengan delay 500ms
    xTaskCreate(Led1Task, "LED1_Task", TASK_STACK_SIZE, NULL, LED1_TASK_PRIORITY, &led1TaskHandle);
    // Membuat task LED2 dengan delay 1000ms
    xTaskCreate(Led2Task, "LED2_Task", TASK_STACK_SIZE, NULL, LED2_TASK_PRIORITY, &led2TaskHandle);
    // Membuat task tombol dengan debounce
    xTaskCreate(ButtonTask, "Button_Task", TASK_STACK_SIZE, NULL, BUTTON_TASK_PRIORITY, &buttonTaskHandle);

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

    // Mengkonfigurasi clock bus: HCLK, SYSCLK, PCLK1, PCLK2
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK|RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
    // Sumber clock adalah HSI
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
    // Pembagi HCLK = 1 (HCLK = SYSCLK)
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    // Pembagi PCLK1 = 1
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    // Pembagi PCLK2 = 1
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    // Menginisialisasi konfigurasi clock bus
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0);
}

// Implementasi fungsi inisialisasi GPIO
static void MX_GPIO_Init(void)
{
    // Mendeklarasikan struktur konfigurasi pin GPIO
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // Mengaktifkan clock untuk GPIOC (untuk LED)
    __HAL_RCC_GPIOC_CLK_ENABLE();
    // Mengaktifkan clock untuk GPIOA (untuk Button)
    __HAL_RCC_GPIOA_CLK_ENABLE();

    // Mengkonfigurasi pin PC13 sebagai output push-pull untuk LED
    GPIO_InitStruct.Pin = LED_PIN;
    // Mode output push-pull
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    // Kecepatan rendah
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    // Menginisialisasi pin GPIOC
    HAL_GPIO_Init(LED_PORT, &GPIO_InitStruct);

    // Mengkonfigurasi pin PA0 sebagai input dengan pull-up untuk Button
    GPIO_InitStruct.Pin = BUTTON_PIN;
    // Mode input
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    // Menggunakan pull-up internal
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    // Menginisialisasi pin GPIOA
    HAL_GPIO_Init(BUTTON_PORT, &GPIO_InitStruct);
}

// Implementasi task LED1 - blink setiap 500ms
void Led1Task(void *argument)
{
    // Loop tak terbatas untuk task
    while (1)
    {
        // Toggle LED (PC13)
        HAL_GPIO_TogglePin(LED_PORT, LED_PIN);
        // Delay 500ms menggunakan FreeRTOS delay
        vTaskDelay(pdMS_TO_TICKS(LED1_DELAY_MS));
    }
}

// Implementasi task LED2 - blink setiap 1000ms
void Led2Task(void *argument)
{
    // Mendeklarasikan variabel untuk menyimpan state LED
    uint8_t led_state = 0;

    // Loop tak terbatas untuk task
    while (1)
    {
        // Mengecek state LED saat ini
        if (led_state == 0)
        {
            // Mengatur LED menyala (PC13 low karena LED active low)
            HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_RESET);
            // Mengubah state ke 1
            led_state = 1;
        }
        else
        {
            // Mengatur LED mati (PC13 high)
            HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET);
            // Mengubah state ke 0
            led_state = 0;
        }
        // Delay 1000ms menggunakan FreeRTOS delay
        vTaskDelay(pdMS_TO_TICKS(LED2_DELAY_MS));
    }
}

// Implementasi task Button - read dengan debounce
void ButtonTask(void *argument)
{
    // Mendeklarasikan variabel untuk menyimpan state tombol sebelumnya
    GPIO_PinState button_state_prev = GPIO_PIN_SET;
    // Mendeklarasikan variabel untuk menyimpan state tombol saat ini
    GPIO_PinState button_state_current = GPIO_PIN_SET;
    // Mendeklarasikan variabel untuk menyimpan waktu terakhir tombol ditekan
    TickType_t last_press_time = 0;

    // Loop tak terbatas untuk task
    while (1)
    {
        // Membaca state tombol saat ini (PA0)
        button_state_current = HAL_GPIO_ReadPin(BUTTON_PORT, BUTTON_PIN);

        // Mengecek apakah tombol ditekan (active low: 0 = pressed)
        if (button_state_current == GPIO_PIN_RESET && button_state_prev == GPIO_PIN_SET)
        {
            // Mendapatkan waktu saat ini
            TickType_t current_time = xTaskGetTickCount();
            // Mengecek debounce
            if ((current_time - last_press_time) > pdMS_TO_TICKS(DEBOUNCE_DELAY_MS))
            {
                // Update waktu penekanan terakhir
                last_press_time = current_time;
                // Tombol ditekan - bisa tambahkan aksi di sini
                // Toggle LED sebagai indikasi tombol ditekan
                HAL_GPIO_TogglePin(LED_PORT, LED_PIN);
            }
        }

        // Menyimpan state tombol untuk iterasi berikutnya
        button_state_prev = button_state_current;
        // Delay 10ms untuk polling tombol
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
