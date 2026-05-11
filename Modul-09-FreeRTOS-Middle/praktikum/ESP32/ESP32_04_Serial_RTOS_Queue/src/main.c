// src/main.c untuk ESP32_04_Serial_RTOS_Queue
// Program FreeRTOS dengan UART dan RTOS queue

// Menginclude header FreeRTOS untuk task creation dan delay
#include "freertos/FreeRTOS.h"
// Menginclude header FreeRTOS untuk queue
#include "freertos/queue.h"
// Menginclude header FreeRTOS untuk task management
#include "freertos/task.h"
// Menginclude header UART ESP32
#include "driver/uart.h"
// Menginclude header GPIO ESP32
#include "driver/gpio.h"
// Menginclude header untuk konfigurasi aplikasi
#include "config.h"
// Menginclude header standard untuk printf
#include "stdio.h"
// Menginclude header standard untuk string manipulation
#include "string.h"

// Mendefinisikan struktur untuk command dari UART
typedef struct {
    char cmd_string[MAX_CMD_LEN];  // String command
    uint32_t timestamp;            // Timestamp command diterima
} uart_command_t;

// Mendefinisikan handle untuk queue command
QueueHandle_t xCommandQueue = NULL;
// Mendefinisikan handle untuk task read UART
TaskHandle_t xReadTaskHandle = NULL;
// Mendefinisikan handle untuk task process command
TaskHandle_t xProcessTaskHandle = NULL;

// Fungsi untuk inisialisasi UART
void init_uart(void) {
    // Mengatur konfigurasi UART
    uart_config_t uart_config = {
        .baud_rate = UART_BAUD_RATE,           // Baud rate 115200
        .data_bits = UART_DATA_8_BITS,         // 8 data bits
        .parity = UART_PARITY_DISABLE,         // No parity
        .stop_bits = UART_STOP_BITS_1,         // 1 stop bit
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE  // No flow control
    };
    
    // Mengatur parameter konfigurasi ke UART port
    uart_param_config(UART_PORT, &uart_config);
    // Mengatur pin UART (TX dan RX)
    uart_set_pin(UART_PORT, UART_TX_PIN, UART_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    // Menginstall driver UART dengan buffer RX dan TX
    uart_driver_install(UART_PORT, UART_BUF_SIZE, 0, 0, NULL, 0);
}

// Task 1: Read UART
void task_read_uart(void *pvParameters) {
    // Buffer untuk menyimpan data yang dibaca dari UART
    uint8_t data[UART_BUF_SIZE];
    // Variabel untuk menyimpan panjang data yang dibaca
    int len;
    // Variabel untuk command yang akan dikirim ke queue
    uart_command_t command;
    
    // Loop tak terhingga untuk task ini
    while (1) {
        // Membaca data dari UART dengan timeout tertent
        len = uart_read_bytes(UART_PORT, data, UART_BUF_SIZE, TASK_READ_DELAY);
        
        // Mengecek apakah ada data yang diterima
        if (len > 0) {
            // Null-terminate string data
            data[len] = 0;
            
            // Mengcopy data ke command string
            strncpy(command.cmd_string, (char *)data, MAX_CMD_LEN - 1);
            // Memastikan string diakhiri dengan null
            command.cmd_string[MAX_CMD_LEN - 1] = '\0';
            // Mengisi timestamp command
            command.timestamp = xTaskGetTickCount();
            
            // Mengirim command ke queue
            if (xQueueSend(xCommandQueue, &command, 0) == pdTRUE) {
                // Mencetak pesan bahwa command dikirim ke queue
                printf("UART Read: Command queued: %s\n", command.cmd_string);
            } else {
                // Mencetak pesan jika queue penuh
                printf("UART Read: Queue full!\n");
            }
        }
        
        // Delay task untuk memberikan waktu task lain berjalan
        vTaskDelay(TASK_READ_DELAY);
    }
}

// Task 2: Process commands from queue
void task_process_command(void *pvParameters) {
    // Variabel untuk menyimpan command yang diterima dari queue
    uart_command_t received_command;
    
    // Loop tak terhingga untuk task ini
    while (1) {
        // Menerima command dari queue (blocking dengan timeout portMAX_DELAY)
        if (xQueueReceive(xCommandQueue, &received_command, portMAX_DELAY) == pdTRUE) {
            // Mencetak pesan bahwa command diterima
            printf("Process: Command received: %s", received_command.cmd_string);
            // Mencetak timestamp command
            printf(" (Timestamp: %lu)\n", received_command.timestamp);
            
            // Memproses command berdasarkan string yang diterima
            if (strstr(received_command.cmd_string, "LED_ON") != NULL) {
                // Command untuk menyalakan LED
                printf("Process: Turning LED ON\n");
            } else if (strstr(received_command.cmd_string, "LED_OFF") != NULL) {
                // Command untuk mematikan LED
                printf("Process: Turning LED OFF\n");
            } else if (strstr(received_command.cmd_string, "STATUS") != NULL) {
                // Command untuk mengecek status
                printf("Process: System status OK\n");
            } else {
                // Command tidak dikenal
                printf("Process: Unknown command\n");
            }
        }
    }
}

// Fungsi utama program (app_main di ESP-IDF)
void app_main(void) {
    // Mencetak pesan bahwa program dimulai
    printf("ESP32 Serial RTOS Queue Demo Started\n");
    // Mencetak informasi tentang penggunaan queue
    printf("UART commands will be queued and processed\n");
    
    // Inisialisasi UART
    init_uart();
    
    // Membuat queue untuk command UART dengan ukuran 10
    xCommandQueue = xQueueCreate(CMD_QUEUE_SIZE, sizeof(uart_command_t));
    
    // Mengecek apakah queue berhasil dibuat
    if (xCommandQueue != NULL) {
        // Mencetak pesan bahwa queue berhasil dibuat
        printf("Command queue created successfully\n");
        
        // Membuat task read UART dengan stack 4096 bytes
        xTaskCreate(task_read_uart, "UARTRead", TASK_READ_STACK_SIZE, NULL, TASK_READ_PRIORITY, &xReadTaskHandle);
        // Membuat task process command dengan stack 3072 bytes
        xTaskCreate(task_process_command, "CmdProc", TASK_PROC_STACK_SIZE, NULL, TASK_PROC_PRIORITY, &xProcessTaskHandle);
        
        // Mencetak pesan bahwa task telah dibuat
        printf("UART read and command process tasks created\n");
        // Mencetak petunjuk penggunaan
        printf("Send commands via UART (e.g., LED_ON, LED_OFF, STATUS)\n");
    } else {
        // Mencetak pesan error jika queue gagal dibuat
        printf("Failed to create command queue!\n");
    }
    
    // app_main harus mengembalikan void atau memanggil vTaskDelete
    // Task tidak perlu dihapus karena akan berjalan terus menerus
}
