// File utama program STM32_04_Serial_RTOS_Queue
// UART dengan RTOS queue
// Task 1: Read UART dan send ke queue
// Task 2: Receive dari queue dan process commands


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
#include <stdio.h>
#include <string.h>
#include "queue.h"
// Menginclude header konfigurasi custom
#include "config.h"

// Mendeklarasikan handle untuk UART
UART_HandleTypeDef huart1;
// Mendeklarasikan handle untuk queue command
QueueHandle_t cmdQueue = NULL;
// Mendeklarasikan handle untuk task UART RX
TaskHandle_t uartRxTaskHandle = NULL;
// Mendeklarasikan handle untuk task command process
TaskHandle_t cmdProcessTaskHandle = NULL;
// Mendeklarasikan buffer untuk UART receive
uint8_t uart_rx_buffer[UART_BUFFER_SIZE];
// Mendeklarasikan buffer untuk command yang sedang dibaca
uint8_t cmd_buffer[UART_BUFFER_SIZE];
// Mendeklarasikan index untuk command buffer
uint8_t cmd_index = 0;

// Fungsi prototipe untuk inisialisasi HAL
void SystemClock_Config(void);
// Fungsi prototipe untuk inisialisasi GPIO
static void MX_GPIO_Init(void);
// Fungsi prototipe untuk inisialisasi UART
static void MX_USART1_UART_Init(void);
// Fungsi prototipe task UART receive
void UartRxTask(void *argument);
// Fungsi prototipe task command process
void CmdProcessTask(void *argument);
// Fungsi untuk mengirim string ke UART
void UART_SendString(char *str);

// Fungsi utama program
int main(void)
{
    // Menginisialisasi HAL Library
    HAL_Init();
    // Mengkonfigurasi system clock
    SystemClock_Config();
    // Menginisialisasi GPIO
    MX_GPIO_Init();
    // Menginisialisasi UART1
    MX_USART1_UART_Init();

    // Membuat queue untuk command (ukuran 10 item, tipe char pointer)
    cmdQueue = xQueueCreate(CMD_QUEUE_SIZE, sizeof(char *));

    // Membuat task UART receive
    xTaskCreate(UartRxTask, "UART_RX_Task", TASK_STACK_SIZE, NULL, UART_RX_TASK_PRIORITY, &uartRxTaskHandle);
    // Membuat task command process
    xTaskCreate(CmdProcessTask, "CMD_Process_Task", TASK_STACK_SIZE, NULL, CMD_TASK_PRIORITY, &cmdProcessTaskHandle);

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

// Implementasi fungsi inisialisasi GPIO
static void MX_GPIO_Init(void)
{
    // Mendeklarasikan struktur konfigurasi pin GPIO
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // Mengaktifkan clock untuk GPIOC (LED)
    __HAL_RCC_GPIOC_CLK_ENABLE();

    // Mengkonfigurasi pin PC13 sebagai output push-pull untuk LED
    GPIO_InitStruct.Pin = LED_PIN;
    // Mode output push-pull
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    // Kecepatan rendah
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    // Menginisialisasi pin GPIOC
    HAL_GPIO_Init(LED_PORT, &GPIO_InitStruct);
}

// Implementasi fungsi inisialisasi UART1
static void MX_USART1_UART_Init(void)
{
    // Mengaktifkan clock untuk USART1
    __HAL_RCC_USART1_CLK_ENABLE();
    // Mengaktifkan clock untuk GPIOA (TX/RX pins)
    __HAL_RCC_GPIOA_CLK_ENABLE();

    // Mengkonfigurasi pin PA9 sebagai USART1_TX
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_9;
    // Mode alternate function push-pull
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    // Kecepatan medium
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
    // Menginisialisasi pin PA9
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    // Mengkonfigurasi pin PA10 sebagai USART1_RX
    GPIO_InitStruct.Pin = GPIO_PIN_10;
    // Mode input floating
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    // Menggunakan pull-up
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    // Menginisialisasi pin PA10
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    // Mengkonfigurasi parameter UART
    huart1.Instance = USART1;
    // Baud rate 115200
    huart1.Init.BaudRate = UART_BAUDRATE;
    // Word length 8 bit
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    // 1 stop bit
    huart1.Init.StopBits = UART_STOPBITS_1;
    // No parity
    huart1.Init.Parity = UART_PARITY_NONE;
    // Mode TX dan RX
    huart1.Init.Mode = UART_MODE_TX_RX;
    // No hardware flow control
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    // Over sampling 16
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    // Menginisialisasi UART
    HAL_UART_Init(&huart1);

    // Mengirim pesan startup ke UART
    UART_SendString("STM32 UART RTOS Queue Ready\r\n");
}

// Fungsi untuk mengirim string ke UART
void UART_SendString(char *str)
{
    // Mengirim string melalui UART
    HAL_UART_Transmit(&huart1, (uint8_t *)str, strlen(str), HAL_MAX_DELAY);
}

// Implementasi task UART receive - membaca UART dan mengirim ke queue
void UartRxTask(void *argument)
{
    // Mendeklarasikan variabel untuk menyimpan karakter yang diterima
    uint8_t rx_char;
    // Mendeklarasikan pointer untuk command
    char *cmd_ptr;

    // Loop tak terbatas untuk task
    while (1)
    {
        // Menerima 1 karakter dari UART (blocking)
        if (HAL_UART_Receive(&huart1, &rx_char, 1, 100) == HAL_OK)
        {
            // Mengecek apakah karakter adalah newline atau carriage return
            if (rx_char == '\n' || rx_char == '\r')
            {
                // End of command - tambahkan null terminator
                cmd_buffer[cmd_index] = '\0';
                // Mengecek apakah ada command
                if (cmd_index > 0)
                {
                    // Mengirim command ke queue
                    cmd_ptr = (char *)cmd_buffer;
                    // Send ke queue
                    xQueueSend(cmdQueue, &cmd_ptr, 0);
                    // Reset index buffer
                    cmd_index = 0;
                }
            }
            else
            {
                // Karakter biasa - simpan ke buffer
                cmd_buffer[cmd_index++] = rx_char;
                // Mengecek apakah buffer penuh
                if (cmd_index >= UART_BUFFER_SIZE - 1)
                {
                    // Buffer penuh - reset
                    cmd_index = 0;
                }
            }
        }
        // Delay singkat
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

// Implementasi task command process - menerima dari queue dan memproses
void CmdProcessTask(void *argument)
{
    // Mendeklarasikan pointer untuk command yang diterima
    char *received_cmd;
    // Mendeklarasikan buffer untuk response
    char response[64];

    // Loop tak terbatas untuk task
    while (1)
    {
        // Menunggu command dari queue (blocking dengan timeout 100ms)
        if (xQueueReceive(cmdQueue, &received_cmd, pdMS_TO_TICKS(100)) == pdTRUE)
        {
            // Command diterima - proses command
            // Mengecek command "LED ON"
            if (strcmp(received_cmd, "LED ON") == 0)
            {
                // Menyalakan LED
                HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_RESET);
                // Mengirim response
                UART_SendString("LED ON\r\n");
            }
            // Mengecek command "LED OFF"
            else if (strcmp(received_cmd, "LED OFF") == 0)
            {
                // Mematikan LED
                HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET);
                // Mengirim response
                UART_SendString("LED OFF\r\n");
            }
            // Mengecek command "LED TOGGLE"
            else if (strcmp(received_cmd, "LED TOGGLE") == 0)
            {
                // Toggle LED
                HAL_GPIO_TogglePin(LED_PORT, LED_PIN);
                // Mengirim response
                UART_SendString("LED TOGGLED\r\n");
            }
            // Mengecek command "HELLO"
            else if (strcmp(received_cmd, "HELLO") == 0)
            {
                // Mengirim hello response
                UART_SendString("HELLO FROM STM32\r\n");
            }
            // Command tidak dikenal
            else
            {
                // Mengirim pesan unknown command
                snprintf(response, sizeof(response), "UNKNOWN: %s\r\n", received_cmd);
                // Mengirim response
                UART_SendString(response);
            }
        }
    }
}
