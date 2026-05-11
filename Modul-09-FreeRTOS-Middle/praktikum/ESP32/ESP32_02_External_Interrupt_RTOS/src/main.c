// src/main.c untuk ESP32_02_External_Interrupt_RTOS
// Program FreeRTOS dengan GPIO interrupt dan semaphore

// Menginclude header FreeRTOS untuk task creation dan delay
#include "freertos/FreeRTOS.h"
// Menginclude header FreeRTOS untuk semaphore dan queue
#include "freertos/semphr.h"
// Menginclude header FreeRTOS untuk task management
#include "freertos/task.h"
// Menginclude header GPIO ESP32
#include "driver/gpio.h"
// Menginclude header UART ESP32
#include "driver/uart.h"
// Menginclude header untuk konfigurasi aplikasi
#include "config.h"
// Menginclude header standard untuk printf
#include "stdio.h"

// Mendefinisikan handle untuk semaphore binary
SemaphoreHandle_t xBinarySemaphore = NULL;
// Mendefinisikan handle untuk task pemroses interrupt
TaskHandle_t xTaskHandle = NULL;
// Mendefinisikan counter untuk interrupt events
volatile uint32_t interrupt_count = 0;

// Fungsi ISR (Interrupt Service Routine) untuk GPIO
void IRAM_ATTR gpio_isr_handler(void *arg) {
    // Memberikan semaphore dari ISR untuk membangunkan task
    xSemaphoreGiveFromISR(xBinarySemaphore, NULL);
    // Menambah counter interrupt
    interrupt_count++;
}

// Fungsi untuk inisialisasi GPIO interrupt
void init_gpio_interrupt(void) {
    // Mengatur mode GPIO interrupt sebagai input
    gpio_set_direction(INT_GPIO, GPIO_MODE_INPUT);
    // Mengatur resistor pull-up untuk GPIO interrupt
    gpio_pullup_en(INT_GPIO);
    // Mengatur resistor pull-down tidak aktif
    gpio_pulldown_dis(INT_GPIO);
    // Mengatur interrupt type untuk falling edge (ketika sinyal HIGH ke LOW)
    gpio_set_intr_type(INT_GPIO, GPIO_INTR_NEGEDGE);
    // Mengaktifkan interrupt untuk GPIO tersebut
    gpio_intr_enable(INT_GPIO);
    // Mendaftarkan ISR handler untuk GPIO interrupt
    gpio_isr_handler_add(INT_GPIO, gpio_isr_handler, NULL);
}

// Fungsi untuk inisialisasi GPIO LED
void init_led(void) {
    // Mengatur mode GPIO LED sebagai output
    gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);
    // Mengatur level awal LED ke LOW (mati)
    gpio_set_level(LED_GPIO, GPIO_LOW);
}

// Task untuk memproses event interrupt
void task_interrupt_processor(void *pvParameters) {
    // Variabel untuk menyimpan state LED saat ini
    uint8_t led_state = GPIO_LOW;
    
    // Loop tak terhingga untuk task ini
    while (1) {
        // Menunggu semaphore diberikan dari ISR (blocking dengan timeout portMAX_DELAY)
        if (xSemaphoreTake(xBinarySemaphore, portMAX_DELAY) == pdTRUE) {
            // Membalikkan state LED untuk indikasi interrupt diterima
            led_state = !led_state;
            // Mengatur level GPIO LED sesuai state baru
            gpio_set_level(LED_GPIO, led_state);
            // Mencetak pesan bahwa interrupt diproses
            printf("Interrupt processed! Count: %lu\n", interrupt_count);
        }
    }
}

// Fungsi utama program (app_main di ESP-IDF)
void app_main(void) {
    // Mencetak pesan bahwa program dimulai
    printf("ESP32 External Interrupt RTOS Demo Started\n");
    // Mencetak informasi tentang penggunaan semaphore
    printf("Using binary semaphore for ISR-task synchronization\n");
    
    // Inisialisasi LED
    init_led();
    // Inisialisasi GPIO interrupt
    init_gpio_interrupt();
    
    // Membuat binary semaphore untuk sinkronisasi ISR dan task
    xBinarySemaphore = xSemaphoreCreateBinary();
    
    // Mengecek apakah semaphore berhasil dibuat
    if (xBinarySemaphore != NULL) {
        // Mencetak pesan bahwa semaphore berhasil dibuat
        printf("Binary semaphore created successfully\n");
        
        // Membuat task untuk memproses interrupt dengan stack 2048 bytes
        xTaskCreate(task_interrupt_processor, "IntProcessor", TASK_STACK_SIZE, NULL, TASK_PRIORITY, &xTaskHandle);
        
        // Mencetak pesan bahwa task telah dibuat
        printf("Interrupt processor task created\n");
        // Mencetak petunjuk penggunaan
        printf("Press button on GPIO 0 to trigger interrupt\n");
    } else {
        // Mencetak pesan error jika semaphore gagal dibuat
        printf("Failed to create binary semaphore!\n");
    }
    
    // app_main harus mengembalikan void atau memanggil vTaskDelete
    // Task tidak perlu dihapus karena akan berjalan terus menerus
}
