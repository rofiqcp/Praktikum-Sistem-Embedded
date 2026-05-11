// Sertakan file header config.h yang berisi definisi dan deklarasi yang diperlukan
#include "FreeRTOSConfig.h"
#include "FreeRTOS.h"
#include "task.h"
#include "event_groups.h"
#include "semphr.h"

#if defined(STM32F103xB)
#include "stm32f1xx_hal.h"
#elif defined(STM32F401xC) || defined(STM32F411xE)
#include "stm32f4xx_hal.h"
#endif

#define LED_PIN GPIO_PIN_13
#define LED_PORT GPIOC
#define UART_BAUD_RATE 115200

#define EVENT_BIT_0 (1 << 0)
#define EVENT_BIT_1 (1 << 1)
#define EVENT_BIT_2 (1 << 2)
EventGroupHandle_t xEventGroup;

// Deklarasi handle untuk Task 1, digunakan untuk mengontrol task dari task lain
TaskHandle_t xTask1Handle = NULL;
// Deklarasi handle untuk Task 2, digunakan untuk mengontrol task dari task lain
TaskHandle_t xTask2Handle = NULL;
// Deklarasi handle untuk Task 3, digunakan untuk mengontrol task dari task lain
TaskHandle_t xTask3Handle = NULL;
// Deklarasi handle untuk Monitor Task, digunakan untuk mengontrol task dari task lain
TaskHandle_t xMonitorTaskHandle = NULL;

// Deklarasi prototipe fungsi Task 1 yang akan mengatur event bit 0
void vTask1(void *pvParameters);
// Deklarasi prototipe fungsi Task 2 yang akan mengatur event bit 1
void vTask2(void *pvParameters);
// Deklarasi prototipe fungsi Task 3 yang akan mengatur event bit 2
void vTask3(void *pvParameters);
// Deklarasi prototipe fungsi Monitor Task yang akan menunggu kombinasi event bits
void vMonitorTask(void *pvParameters);

// Fungsi utama program, titik masuk eksekusi mikrokontroler
int main(void)
{
    // Inisialisasi HAL (Hardware Abstraction Layer) Library untuk STM32
    HAL_Init();
    
    // Konfigurasi clock sistem mikrokontroler sesuai dengan kebutuhan
    SystemClock_Config();
    
    // Inisialisasi GPIO untuk LED (PC13) dan USART1 untuk komunikasi UART
    MX_GPIO_Init();
    // Inisialisasi module USART1 dengan baud rate 115200
    MX_USART1_UART_Init();
    
    // Kirim pesan awal ke terminal UART untuk menandakan program dimulai
    UART_SendString("=== Program Event Group Basic ===\r\n");
    
    // Buat Event Group baru menggunakan API FreeRTOS xEventGroupCreate
    xEventGroup = xEventGroupCreate();
    
    // Periksa apakah Event Group berhasil dibuat (tidak NULL)
    if(xEventGroup != NULL)
    {
        // Buat Task 1 dengan nama "Task1", stack minimal, prioritas 1, simpan handle
        xTaskCreate(vTask1, "Task1", configMINIMAL_STACK_SIZE, NULL, 1, &xTask1Handle);
        
        // Buat Task 2 dengan nama "Task2", stack minimal, prioritas 1, simpan handle
        xTaskCreate(vTask2, "Task2", configMINIMAL_STACK_SIZE, NULL, 1, &xTask2Handle);
        
        // Buat Task 3 dengan nama "Task3", stack minimal, prioritas 1, simpan handle
        xTaskCreate(vTask3, "Task3", configMINIMAL_STACK_SIZE, NULL, 1, &xTask3Handle);
        
        // Buat Monitor Task dengan nama "Monitor", stack minimal, prioritas 2 (lebih tinggi)
        xTaskCreate(vMonitorTask, "Monitor", configMINIMAL_STACK_SIZE, NULL, 2, &xMonitorTaskHandle);
        
        // Mulai scheduler FreeRTOS untuk menjadwalkan task-task yang telah dibuat
        vTaskStartScheduler();
    }
    
    // Loop tak terbatas jika scheduler gagal dimulai (error handling)
    while(1)
    {
        // Jika sampai sini, berarti ada error fatal pada sistem
    }
}

// Implementasi Task 1 - Set Event Bit 0 setiap 1000ms
void vTask1(void *pvParameters)
{
    // Variabel untuk menyimpan waktu delay dalam tick FreeRTOS (1000ms)
    TickType_t xDelay = pdMS_TO_TICKS(1000);
    
    // Loop utama task yang akan dieksekusi berulang kali
    while(1)
    {
        // Tunggu selama 1000ms menggunakan vTaskDelay
        vTaskDelay(xDelay);
        
        // Set Event Bit 0 pada Event Group menggunakan xEventGroupSetBits
        xEventGroupSetBits(xEventGroup, EVENT_BIT_0);
        
        // Kirim pesan ke UART bahwa Task 1 telah menyetel bit 0
        UART_SendString("Task 1: Set EVENT_BIT_0\r\n");
        
        // Toggle LED di pin PC13 untuk indikasi visual bahwa task berjalan
        HAL_GPIO_TogglePin(LED_PORT, LED_PIN);
    }
}

// Implementasi Task 2 - Set Event Bit 1 setiap 1500ms
void vTask2(void *pvParameters)
{
    // Variabel untuk menyimpan waktu delay dalam tick FreeRTOS (1500ms)
    TickType_t xDelay = pdMS_TO_TICKS(1500);
    
    // Loop utama task yang akan dieksekusi berulang kali
    while(1)
    {
        // Tunggu selama 1500ms menggunakan vTaskDelay
        vTaskDelay(xDelay);
        
        // Set Event Bit 1 pada Event Group menggunakan xEventGroupSetBits
        xEventGroupSetBits(xEventGroup, EVENT_BIT_1);
        
        // Kirim pesan ke UART bahwa Task 2 telah menyetel bit 1
        UART_SendString("Task 2: Set EVENT_BIT_1\r\n");
        
        // Toggle LED di pin PC13 untuk indikasi visual bahwa task berjalan
        HAL_GPIO_TogglePin(LED_PORT, LED_PIN);
    }
}

// Implementasi Task 3 - Set Event Bit 2 setiap 2000ms
void vTask3(void *pvParameters)
{
    // Variabel untuk menyimpan waktu delay dalam tick FreeRTOS (2000ms)
    TickType_t xDelay = pdMS_TO_TICKS(2000);
    
    // Loop utama task yang akan dieksekusi berulang kali
    while(1)
    {
        // Tunggu selama 2000ms menggunakan vTaskDelay
        vTaskDelay(xDelay);
        
        // Set Event Bit 2 pada Event Group menggunakan xEventGroupSetBits
        xEventGroupSetBits(xEventGroup, EVENT_BIT_2);
        
        // Kirim pesan ke UART bahwa Task 3 telah menyetel bit 2
        UART_SendString("Task 3: Set EVENT_BIT_2\r\n");
        
        // Toggle LED di pin PC13 untuk indikasi visual bahwa task berjalan
        HAL_GPIO_TogglePin(LED_PORT, LED_PIN);
    }
}

// Implementasi Monitor Task - Menunggu kombinasi event bits dengan AND/OR logic
void vMonitorTask(void *pvParameters)
{
    // Variabel untuk menyimpan event bits yang diterima dari Event Group
    EventBits_t uxBits;
    
    // Loop utama task yang akan dieksekusi berulang kali
    while(1)
    {
        // Tunggu hingga EVENT_BIT_0 DAN EVENT_BIT_1 ter-set (AND logic)
        uxBits = xEventGroupWaitBits(
            xEventGroup,                    // Handle Event Group yang akan ditunggu
            EVENT_BIT_0 | EVENT_BIT_1,      // Bit yang ingin ditunggu (bit 0 dan 1)
            pdTRUE,                         // Clear bit setelah dibaca (TRUE = clear)
            pdTRUE,                         // Tunggu SEMUA bit (AND logic = TRUE)
            portMAX_DELAY                   // Tunggu selamanya (tidak ada timeout)
        );
        
        // Periksa apakah EVENT_BIT_0 dan EVENT_BIT_1 sudah ter-set (AND logic terpenuhi)
        if((uxBits & (EVENT_BIT_0 | EVENT_BIT_1)) == (EVENT_BIT_0 | EVENT_BIT_1))
        {
            // Kirim pesan bahwa kombinasi AND logic terpenuhi
            UART_SendString("Monitor: AND Logic terpenuhi (Bit0 & Bit1)\r\n");
        }
        
        // Tunggu hingga EVENT_BIT_0 ATAU EVENT_BIT_2 ter-set (OR logic)
        uxBits = xEventGroupWaitBits(
            xEventGroup,                    // Handle Event Group yang akan ditunggu
            EVENT_BIT_0 | EVENT_BIT_2,      // Bit yang ingin ditunggu (bit 0 atau 2)
            pdTRUE,                         // Clear bit setelah dibaca (TRUE = clear)
            pdFALSE,                        // Tunggu SALAH SATU bit (OR logic = FALSE)
            portMAX_DELAY                   // Tunggu selamanya (tidak ada timeout)
        );
        
        // Periksa apakah EVENT_BIT_0 atau EVENT_BIT_2 sudah ter-set (OR logic terpenuhi)
        if((uxBits & (EVENT_BIT_0 | EVENT_BIT_2)) != 0)
        {
            // Kirim pesan bahwa logic OR terpenuhi
            UART_SendString("Monitor: OR Logic terpenuhi (Bit0 | Bit2)\r\n");
        }
        
        // Tunggu kombinasi ketiga bit (semua bit harus ter-set)
        uxBits = xEventGroupWaitBits(
            xEventGroup,                            // Handle Event Group yang akan ditunggu
            EVENT_BIT_0 | EVENT_BIT_1 | EVENT_BIT_2,  // Bit yang ingin ditunggu (semua bit)
            pdTRUE,                                 // Clear bit setelah dibaca (TRUE = clear)
            pdTRUE,                                  // Tunggu SEMUA bit (AND logic = TRUE)
            pdMS_TO_TICKS(5000)                     // Timeout 5 detik
        );
        
        // Periksa apakah ketiga bit sudah ter-set (semua bit terpenuhi)
        if((uxBits & (EVENT_BIT_0 | EVENT_BIT_1 | EVENT_BIT_2)) == (EVENT_BIT_0 | EVENT_BIT_1 | EVENT_BIT_2))
        {
            // Kirim pesan bahwa semua bit terpenuhi
            UART_SendString("Monitor: Semua bit terpenuhi!\r\n");
        }
        else
        {
            // Kirim pesan timeout karena tidak semua bit ter-set dalam 5 detik
            UART_SendString("Monitor: Timeout menunggu semua bit\r\n");
        }
    }
}

// Fungsi untuk mengonfigurasi clock sistem STM32F103C8 (72MHz)
void SystemClock_Config(void)
{
    // Deklarasi struktur untuk konfigurasi clock
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    // Deklarasi struktur untuk konfigurasi clock bus
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    
    // Inisialisasi oscillator dengan konfigurasi berikut
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    // Gunakan High Speed External (HSE) 8MHz crystal
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    // Matikan PLL (Phase Locked Loop) untuk konfigurasi
    RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    // Konfigurasi PLL: source HSE, multiply by 9 (8MHz x 9 = 72MHz)
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
    // Terapkan konfigurasi oscillator
    HAL_RCC_OscConfig(&RCC_OscInitStruct);
    
    // Konfigurasi clock bus sistem
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                                |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
    // Source clock sistem dari PLL (72MHz)
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    // HCLK = SYSCLK = 72MHz (AHB prescaler = 1)
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    // PCLK1 = HCLK/2 = 36MHz (APB1 prescaler = 2)
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    // PCLK2 = HCLK = 72MHz (APB2 prescaler = 1)
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    // Terapkan konfigurasi clock bus
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2);
}

// Fungsi untuk menginisialisasi GPIO (LED PC13 dan UART pins)
void MX_GPIO_Init(void)
{
    // Deklarasi struktur konfigurasi GPIO
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    // Aktifkan clock untuk port GPIOC
    __HAL_RCC_GPIOC_CLK_ENABLE();
    // Aktifkan clock untuk port GPIOA (untuk UART)
    __HAL_RCC_GPIOA_CLK_ENABLE();
    
    // Konfigurasi pin PC13 sebagai output push-pull untuk LED
    GPIO_InitStruct.Pin = GPIO_PIN_13;
    // Mode output push-pull
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    // Kecepatan medium
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
    // Inisialisasi pin PC13
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
    
    // Konfigurasi pin PA9 (USART1_TX) sebagai alternate function push-pull
    GPIO_InitStruct.Pin = GPIO_PIN_9;
    // Mode alternate function push-pull
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    // Kecepatan tinggi
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    // Inisialisasi pin PA9
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    // Konfigurasi pin PA10 (USART1_RX) sebagai input floating
    GPIO_InitStruct.Pin = GPIO_PIN_10;
    // Mode input floating
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    // No pull-up or pull-down
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    // Inisialisasi pin PA10
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

// Handle untuk UART (digunakan oleh MX_USART1_UART_Init)
UART_HandleTypeDef huart1;

// Fungsi untuk menginisialisasi USART1 dengan baud rate 115200
void MX_USART1_UART_Init(void)
{
    // Aktifkan clock untuk USART1
    __HAL_RCC_USART1_CLK_ENABLE();
    
    // Konfigurasi handle UART1
    huart1.Instance = USART1;
    // Baud rate 115200
    huart1.Init.BaudRate = 115200;
    // Panjang data 8 bit
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    // 1 bit stop
    huart1.Init.StopBits = UART_STOPBITS_1;
    // Tanpa parity
    huart1.Init.Parity = UART_PARITY_NONE;
    // Mode: transmit dan receive
    huart1.Init.Mode = UART_MODE_TX_RX;
    // Hardware flow control: none
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    // Over-sampling 16x
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    // Inisialisasi UART1
    HAL_UART_Init(&huart1);
}

// Fungsi untuk mengirim string melalui UART1
void UART_SendString(char *str)
{
    // Kirim string menggunakan HAL UART transmit dalam blocking mode
    HAL_UART_Transmit(&huart1, (uint8_t*)str, strlen(str), HAL_MAX_DELAY);
}

// Callback untuk interrupt SysTick (diperlukan oleh FreeRTOS untuk tick timer)
void xPortSysTickHandler(void) __attribute__((weak));
// Handler interrupt SysTick untuk STM32
void SysTick_Handler(void)
{
    // Panggil handler HAL untuk increment tick HAL
    HAL_IncTick();
    
    // Panggil handler FreeRTOS untuk increment tick RTOS jika scheduler sudah mulai
    if(xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED)
    {
        // Panggil handler SysTick FreeRTOS
        xPortSysTickHandler();
    }
}
