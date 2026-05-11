// src/main.c untuk ESP32_01_GPIO_RTOS_Task
// Program FreeRTOS dengan multiple GPIO tasks

// Menginclude header FreeRTOS untuk task creation dan delay
#include "freertos/FreeRTOS.h"
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

// Mendefinisikan handle untuk Task 1 (LED blink 500ms)
TaskHandle_t xTask1Handle = NULL;
// Mendefinisikan handle untuk Task 2 (LED blink 1000ms)
TaskHandle_t xTask2Handle = NULL;
// Mendefinisikan handle untuk Task 3 (Button monitor)
TaskHandle_t xTask3Handle = NULL;

// Fungsi untuk inisialisasi GPIO LED 1
void init_led_1(void) {
    // Mengatur mode GPIO untuk LED 1 sebagai output
    gpio_set_direction(LED_1_GPIO, GPIO_MODE_OUTPUT);
    // Mengatur level awal LED 1 ke LOW (mati)
    gpio_set_level(LED_1_GPIO, GPIO_LOW);
}

// Fungsi untuk inisialisasi GPIO LED 2
void init_led_2(void) {
    // Mengatur mode GPIO untuk LED 2 sebagai output
    gpio_set_direction(LED_2_GPIO, GPIO_MODE_OUTPUT);
    // Mengatur level awal LED 2 ke LOW (mati)
    gpio_set_level(LED_2_GPIO, GPIO_LOW);
}

// Fungsi untuk inisialisasi GPIO Button
void init_button(void) {
    // Mengatur mode GPIO untuk button sebagai input
    gpio_set_direction(BUTTON_GPIO, GPIO_MODE_INPUT);
    // Mengatur resistor pull-up untuk button
    gpio_pullup_en(BUTTON_GPIO);
    // Mengatur resistor pull-down tidak aktif
    gpio_pulldown_dis(BUTTON_GPIO);
}

// Task 1: LED blink dengan delay 500ms
void task_1_led_blink_500ms(void *pvParameters) {
    // Variabel untuk menyimpan state LED saat ini
    uint8_t led_state = GPIO_LOW;
    // Loop tak terhingga untuk task ini
    while (1) {
        // Membalikkan state LED (dari LOW ke HIGH atau sebaliknya)
        led_state = !led_state;
        // Mengatur level GPIO LED 1 sesuai state baru
        gpio_set_level(LED_1_GPIO, led_state);
        // Mencetak pesan ke UART bahwa LED 1 telah diupdate
        printf("Task 1: LED 1 state = %d\n", led_state);
        // Delay task selama 500ms menggunakan FreeRTOS delay
        vTaskDelay(TASK_1_DELAY);
    }
}

// Task 2: LED blink dengan delay 1000ms
void task_2_led_blink_1000ms(void *pvParameters) {
    // Variabel untuk menyimpan state LED saat ini
    uint8_t led_state = GPIO_LOW;
    // Loop tak terhingga untuk task ini
    while (1) {
        // Membalikkan state LED (dari LOW ke HIGH atau sebaliknya)
        led_state = !led_state;
        // Mengatur level GPIO LED 2 sesuai state baru
        gpio_set_level(LED_2_GPIO, led_state);
        // Mencetak pesan ke UART bahwa LED 2 telah diupdate
        printf("Task 2: LED 2 state = %d\n", led_state);
        // Delay task selama 1000ms menggunakan FreeRTOS delay
        vTaskDelay(TASK_2_DELAY);
    }
}

// Task 3: Button monitor
void task_3_button_monitor(void *pvParameters) {
    // Variabel untuk menyimpan state tombol sebelumnya
    uint8_t prev_button_state = GPIO_HIGH;
    // Variabel untuk menyimpan state tombol saat ini
    uint8_t current_button_state;
    // Loop tak terhingga untuk task ini
    while (1) {
        // Membaca state tombol saat ini dari GPIO
        current_button_state = gpio_get_level(BUTTON_GPIO);
        // Mengecek apakah tombol baru saja ditekan (falling edge)
        if (prev_button_state == GPIO_HIGH && current_button_state == GPIO_LOW) {
            // Mencetak pesan bahwa tombol ditekan
            printf("Task 3: Button pressed!\n");
        }
        // Mengecek apakah tombol baru saja dilepas (rising edge)
        if (prev_button_state == GPIO_LOW && current_button_state == GPIO_HIGH) {
            // Mencetak pesan bahwa tombol dilepas
            printf("Task 3: Button released!\n");
        }
        // Menyimpan state tombol saat ini untuk perbandingan berikutnya
        prev_button_state = current_button_state;
        // Delay task selama 50ms untuk debouncing
        vTaskDelay(TASK_3_DELAY);
    }
}

// Fungsi utama program (app_main di ESP-IDF)
void app_main(void) {
    // Mencetak pesan bahwa program dimulai
    printf("ESP32 GPIO RTOS Task Demo Started\n");
    // Mencetak informasi tentang task yang akan dibuat
    printf("Creating tasks: LED1(500ms), LED2(1000ms), Button Monitor\n");
    // Inisialisasi LED 1
    init_led_1();
    // Inisialisasi LED 2
    init_led_2();
    // Inisialisasi Button
    init_button();
    // Membuat Task 1: LED blink 500ms dengan stack 2048 bytes
    xTaskCreate(task_1_led_blink_500ms, "LED1_500ms", TASK_STACK_SIZE, NULL, TASK_1_PRIORITY, &xTask1Handle);
    // Membuat Task 2: LED blink 1000ms dengan stack 2048 bytes
    xTaskCreate(task_2_led_blink_1000ms, "LED2_1000ms", TASK_STACK_SIZE, NULL, TASK_2_PRIORITY, &xTask2Handle);
    // Membuat Task 3: Button monitor dengan stack 2048 bytes
    xTaskCreate(task_3_button_monitor, "ButtonMon", TASK_STACK_SIZE, NULL, TASK_3_PRIORITY, &xTask3Handle);
    // Mencetak pesan bahwa semua task telah dibuat
    printf("All tasks created successfully!\n");
}
