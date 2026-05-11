// src/main.c untuk ESP32_09_GPIO_Interrupt_Combined
// Program FreeRTOS dengan multiple buttons dan interrupts

// Menginclude header FreeRTOS untuk task creation dan delay
#include "freertos/FreeRTOS.h"
// Menginclude header FreeRTOS untuk queue
#include "freertos/queue.h"
// Menginclude header FreeRTOS untuk semaphore
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

// Mendefinisikan struktur untuk button event
typedef struct {
    uint8_t button_num;          // Nomor button (1, 2, atau 3)
    button_event_type_t event;   // Tipe event (pressed/released)
    uint32_t timestamp;          // Timestamp event
} button_event_t;

// Mendefinisikan handle untuk queue button events
QueueHandle_t xButtonQueue = NULL;
// Mendefinisikan handle untuk semaphore binary button 1
SemaphoreHandle_t xButton1Semaphore = NULL;
// Mendefinisikan handle untuk task LED control
TaskHandle_t xLedTaskHandle = NULL;
// Mendefinisikan handle untuk task button monitor
TaskHandle_t xButtonTaskHandle = NULL;

// Fungsi ISR untuk Button 1 (menggunakan semaphore)
void IRAM_ATTR button1_isr_handler(void *arg) {
    // Memberikan semaphore dari ISR
    xSemaphoreGiveFromISR(xButton1Semaphore, NULL);
}

// Fungsi untuk inisialisasi GPIO buttons
void init_buttons(void) {
    // Mengatur mode GPIO button 1 sebagai input
    gpio_set_direction(BUTTON1_GPIO, GPIO_MODE_INPUT);
    // Mengatur resistor pull-up untuk button 1
    gpio_pullup_en(BUTTON1_GPIO);
    // Mengatur resistor pull-down tidak aktif untuk button 1
    gpio_pulldown_dis(BUTTON1_GPIO);
    // Mengatur interrupt type untuk falling edge (button 1 ditekan)
    gpio_set_intr_type(BUTTON1_GPIO, GPIO_INTR_NEGEDGE);
    // Mengaktifkan interrupt untuk button 1
    gpio_intr_enable(BUTTON1_GPIO);
    // Mendaftarkan ISR handler untuk button 1
    gpio_isr_handler_add(BUTTON1_GPIO, button1_isr_handler, NULL);
    
    // Mengatur mode GPIO button 2 sebagai input
    gpio_set_direction(BUTTON2_GPIO, GPIO_MODE_INPUT);
    // Mengatur resistor pull-up untuk button 2
    gpio_pullup_en(BUTTON2_GPIO);
    // Mengatur resistor pull-down tidak aktif untuk button 2
    gpio_pulldown_dis(BUTTON2_GPIO);
    
    // Mengatur mode GPIO button 3 sebagai input
    gpio_set_direction(BUTTON3_GPIO, GPIO_MODE_INPUT);
    // Mengatur resistor pull-up untuk button 3 (input only pin, pull-up eksternal)
    gpio_pullup_en(BUTTON3_GPIO);
    // Mengatur resistor pull-down tidak aktif untuk button 3
    gpio_pulldown_dis(BUTTON3_GPIO);
}

// Fungsi untuk inisialisasi GPIO LEDs
void init_leds(void) {
    // Mengatur mode GPIO LED 1 sebagai output
    gpio_set_direction(LED1_GPIO, GPIO_MODE_OUTPUT);
    // Mengatur level awal LED 1 ke LOW (mati)
    gpio_set_level(LED1_GPIO, 0);
    
    // Mengatur mode GPIO LED 2 sebagai output
    gpio_set_direction(LED2_GPIO, GPIO_MODE_OUTPUT);
    // Mengatur level awal LED 2 ke LOW (mati)
    gpio_set_level(LED2_GPIO, 0);
    
    // Mengatur mode GPIO LED 3 sebagai output
    gpio_set_direction(LED3_GPIO, GPIO_MODE_OUTPUT);
    // Mengatur level awal LED 3 ke LOW (mati)
    gpio_set_level(LED3_GPIO, 0);
}

// Task untuk LED control (menerima events dari queue)
void task_led_control(void *pvParameters) {
    // Variabel untuk menyimpan button event dari queue
    button_event_t received_event;
    // Variabel untuk state LED 1
    uint8_t led1_state = 0;
    // Variabel untuk state LED 2
    uint8_t led2_state = 0;
    // Variabel untuk state LED 3
    uint8_t led3_state = 0;
    
    // Loop tak terhingga untuk task ini
    while (1) {
        // Menerima event dari queue (blocking dengan timeout portMAX_DELAY)
        if (xQueueReceive(xButtonQueue, &received_event, portMAX_DELAY) == pdTRUE) {
            // Mengecek nomor button
            if (received_event.button_num == 1) {
                // Toggle LED 1 jika button 1 ditekan
                led1_state = !led1_state;
                // Mengatur level LED 1
                gpio_set_level(LED1_GPIO, led1_state);
                // Mencetak pesan
                printf("LED1: %d\n", led1_state);
            } else if (received_event.button_num == 2) {
                // Toggle LED 2 jika button 2 ditekan
                led2_state = !led2_state;
                // Mengatur level LED 2
                gpio_set_level(LED2_GPIO, led2_state);
                // Mencetak pesan
                printf("LED2: %d\n", led2_state);
            } else if (received_event.button_num == 3) {
                // Toggle LED 3 jika button 3 ditekan
                led3_state = !led3_state;
                // Mengatur level LED 3
                gpio_set_level(LED3_GPIO, led3_state);
                // Mencetak pesan
                printf("LED3: %d\n", led3_state);
            }
        }
    }
}

// Task untuk button monitor (polling button 2 dan 3)
void task_button_monitor(void *pvParameters) {
    // Variabel untuk state button 1 sebelumnya
    uint8_t btn1_prev = 1;
    // Variabel untuk state button 2 sebelumnya
    uint8_t btn2_prev = 1;
    // Variabel untuk state button 3 sebelumnya
    uint8_t btn3_prev = 1;
    // Variabel untuk state button saat ini
    uint8_t btn_state;
    // Variabel untuk button event
    button_event_t event;
    
    // Loop tak terhingga untuk task ini
    while (1) {
        // Mengecek semaphore button 1 (dari ISR)
        if (xSemaphoreTake(xButton1Semaphore, 0) == pdTRUE) {
            // Button 1 ditekan (falling edge)
            event.button_num = 1;
            // Mengisi tipe event
            event.event = BUTTON_PRESSED;
            // Mengisi timestamp
            event.timestamp = xTaskGetTickCount();
            // Mengirim event ke queue
            xQueueSend(xButtonQueue, &event, 0);
            // Mencetak pesan
            printf("Button 1 pressed (ISR)\n");
        }
        
        // Membaca state button 2
        btn_state = gpio_get_level(BUTTON2_GPIO);
        // Mengecek falling edge button 2
        if (btn2_prev == 1 && btn_state == 0) {
            // Button 2 ditekan
            event.button_num = 2;
            // Mengisi tipe event
            event.event = BUTTON_PRESSED;
            // Mengisi timestamp
            event.timestamp = xTaskGetTickCount();
            // Mengirim event ke queue
            xQueueSend(xButtonQueue, &event, 0);
            // Mencetak pesan
            printf("Button 2 pressed (polling)\n");
        }
        // Menyimpan state button 2
        btn2_prev = btn_state;
        
        // Membaca state button 3
        btn_state = gpio_get_level(BUTTON3_GPIO);
        // Mengecek falling edge button 3
        if (btn3_prev == 1 && btn_state == 0) {
            // Button 3 ditekan
            event.button_num = 3;
            // Mengisi tipe event
            event.event = BUTTON_PRESSED;
            // Mengisi timestamp
            event.timestamp = xTaskGetTickCount();
            // Mengirim event ke queue
            xQueueSend(xButtonQueue, &event, 0);
            // Mencetak pesan
            printf("Button 3 pressed (polling)\n");
        }
        // Menyimpan state button 3
        btn3_prev = btn_state;
        
        // Delay task selama 20ms
        vTaskDelay(TASK_BTN_DELAY);
    }
}

// Fungsi utama program (app_main di ESP-IDF)
void app_main(void) {
    // Mencetak pesan bahwa program dimulai
    printf("ESP32 GPIO Interrupt Combined RTOS Demo Started\n");
    // Mencetak informasi tentang penggunaan queue dan semaphore
    printf("Using queue for button events, semaphore for ISR\n");
    
    // Inisialisasi buttons
    init_buttons();
    // Inisialisasi LEDs
    init_leds();
    
    // Membuat queue untuk button events dengan ukuran 10
    xButtonQueue = xQueueCreate(BUTTON_QUEUE_SIZE, sizeof(button_event_t));
    // Membuat binary semaphore untuk button 1 ISR
    xButton1Semaphore = xSemaphoreCreateBinary();
    
    // Mengecek apakah queue dan semaphore berhasil dibuat
    if (xButtonQueue != NULL && xButton1Semaphore != NULL) {
        // Mencetak pesan bahwa queue dan semaphore berhasil dibuat
        printf("Button queue and semaphore created successfully\n");
        
        // Membuat task LED control dengan stack 2048 bytes
        xTaskCreate(task_led_control, "LEDCtrl", TASK_LED_STACK_SIZE, NULL, TASK_LED_PRIORITY, &xLedTaskHandle);
        // Membuat task button monitor dengan stack 3072 bytes
        xTaskCreate(task_button_monitor, "BtnMon", TASK_BTN_STACK_SIZE, NULL, TASK_BTN_PRIORITY, &xButtonTaskHandle);
        
        // Mencetak pesan bahwa task telah dibuat
        printf("LED control and button monitor tasks created\n");
        // Mencetak petunjuk penggunaan
        printf("Buttons: 1(INT), 2(POLL), 3(POLL) -> LEDs: 2, 4, 16\n");
    } else {
        // Mencetak pesan error jika gagal
        printf("Failed to create queue or semaphore!\n");
    }
    
    // app_main harus mengembalikan void atau memanggil vTaskDelete
    // Task tidak perlu dihapus karena akan berjalan terus menerus
}
