// src/main.c untuk ESP32_03_Encoder_2Pin_Interrupt
// Program FreeRTOS dengan Rotary Encoder 2-pin interrupt

// Menginclude header FreeRTOS untuk task creation dan delay
#include "freertos/FreeRTOS.h"
// Menginclude header FreeRTOS untuk queue
#include "freertos/queue.h"
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

// Mendefinisikan enumerasi untuk arah rotasi encoder
typedef enum {
    ENCODER_CW = 0,      // Clockwise (putar kanan)
    ENCODER_CCW = 1,     // Counter-Clockwise (putar kiri)
    ENCODER_NONE = 2     // Tidak ada rotasi
} encoder_direction_t;

// Mendefinisikan struktur untuk event encoder
typedef struct {
    encoder_direction_t direction;  // Arah rotasi
    uint32_t timestamp;             // Timestamp event
} encoder_event_t;

// Mendefinisikan handle untuk queue encoder
QueueHandle_t xEncoderQueue = NULL;
// Mendefinisikan handle untuk task pemroses encoder
TaskHandle_t xTaskHandle = NULL;
// Mendefinisikan state sebelumnya dari encoder pin A
volatile uint8_t last_pin_a_state = GPIO_HIGH;
// Mendefinisikan counter untuk langkah encoder
volatile int32_t encoder_steps = 0;

// Fungsi ISR (Interrupt Service Routine) untuk Encoder Pin A
void IRAM_ATTR encoder_pin_a_isr(void *arg) {
    // Membaca state pin A saat ini
    uint8_t pin_a_state = gpio_get_level(ENCODER_PIN_A);
    // Membaca state pin B saat ini
    uint8_t pin_b_state = gpio_get_level(ENCODER_PIN_B);
    
    // Mengecek apakah ada perubahan pada pin A (falling atau rising edge)
    if (pin_a_state != last_pin_a_state) {
        // Membuat event encoder baru
        encoder_event_t event;
        
        // Decoding arah rotasi berdasarkan state pin A dan B
        if (pin_a_state == GPIO_LOW) {
            // Falling edge pada pin A
            if (pin_b_state == GPIO_HIGH) {
                // Rotasi Clockwise (CW)
                event.direction = ENCODER_CW;
                encoder_steps++;
            } else {
                // Rotasi Counter-Clockwise (CCW)
                event.direction = ENCODER_CCW;
                encoder_steps--;
            }
        }
        
        // Mengisi timestamp event
        event.timestamp = xTaskGetTickCountFromISR();
        
        // Mengirim event ke queue dari ISR
        xQueueSendFromISR(xEncoderQueue, &event, NULL);
        
        // Menyimpan state pin A untuk perbandingan berikutnya
        last_pin_a_state = pin_a_state;
    }
}

// Fungsi untuk inisialisasi GPIO encoder
void init_encoder_gpio(void) {
    // Mengatur mode GPIO encoder pin A sebagai input
    gpio_set_direction(ENCODER_PIN_A, GPIO_MODE_INPUT);
    // Mengatur resistor pull-up untuk pin A
    gpio_pullup_en(ENCODER_PIN_A);
    // Mengatur resistor pull-down tidak aktif untuk pin A
    gpio_pulldown_dis(ENCODER_PIN_A);
    
    // Mengatur mode GPIO encoder pin B sebagai input
    gpio_set_direction(ENCODER_PIN_B, GPIO_MODE_INPUT);
    // Mengatur resistor pull-up untuk pin B
    gpio_pullup_en(ENCODER_PIN_B);
    // Mengatur resistor pull-down tidak aktif untuk pin B
    gpio_pulldown_dis(ENCODER_PIN_B);
    
    // Mengatur interrupt type untuk pin A (rising dan falling edge)
    gpio_set_intr_type(ENCODER_PIN_A, GPIO_INTR_ANYEDGE);
    // Mengaktifkan interrupt untuk pin A
    gpio_intr_enable(ENCODER_PIN_A);
    // Mendaftarkan ISR handler untuk pin A
    gpio_isr_handler_add(ENCODER_PIN_A, encoder_pin_a_isr, NULL);
}

// Fungsi untuk inisialisasi LED indikator
void init_leds(void) {
    // Mengatur mode GPIO LED CW sebagai output
    gpio_set_direction(LED_CW_GPIO, GPIO_MODE_OUTPUT);
    // Mengatur level awal LED CW ke LOW (mati)
    gpio_set_level(LED_CW_GPIO, GPIO_LOW);
    
    // Mengatur mode GPIO LED CCW sebagai output
    gpio_set_direction(LED_CCW_GPIO, GPIO_MODE_OUTPUT);
    // Mengatur level awal LED CCW ke LOW (mati)
    gpio_set_level(LED_CCW_GPIO, GPIO_LOW);
}

// Task untuk memproses event encoder
void task_encoder_processor(void *pvParameters) {
    // Variabel untuk menyimpan event encoder yang diterima
    encoder_event_t received_event;
    // Variabel untuk blink LED CW
    uint8_t cw_led_state = GPIO_LOW;
    // Variabel untuk blink LED CCW
    uint8_t ccw_led_state = GPIO_LOW;
    
    // Loop tak terhingga untuk task ini
    while (1) {
        // Menerima event dari queue (blocking dengan timeout portMAX_DELAY)
        if (xQueueReceive(xEncoderQueue, &received_event, portMAX_DELAY) == pdTRUE) {
            // Mengecek arah rotasi
            if (received_event.direction == ENCODER_CW) {
                // Rotasi Clockwise - nyalakan LED CW
                cw_led_state = !cw_led_state;
                // Mengatur level GPIO LED CW sesuai state baru
                gpio_set_level(LED_CW_GPIO, cw_led_state);
                // Mencetak pesan rotasi CW
                printf("Encoder: CW (Steps: %ld)\n", encoder_steps);
                // Mematikan LED CCW
                gpio_set_level(LED_CCW_GPIO, GPIO_LOW);
            } else if (received_event.direction == ENCODER_CCW) {
                // Rotasi Counter-Clockwise - nyalakan LED CCW
                ccw_led_state = !ccw_led_state;
                // Mengatur level GPIO LED CCW sesuai state baru
                gpio_set_level(LED_CCW_GPIO, ccw_led_state);
                // Mencetak pesan rotasi CCW
                printf("Encoder: CCW (Steps: %ld)\n", encoder_steps);
                // Mematikan LED CW
                gpio_set_level(LED_CW_GPIO, GPIO_LOW);
            }
        }
    }
}

// Fungsi utama program (app_main di ESP-IDF)
void app_main(void) {
    // Mencetak pesan bahwa program dimulai
    printf("ESP32 Encoder 2-Pin Interrupt RTOS Demo Started\n");
    // Mencetak informasi tentang penggunaan queue
    printf("Using queue for encoder events\n");
    
    // Inisialisasi LED indikator
    init_leds();
    // Inisialisasi GPIO encoder
    init_encoder_gpio();
    
    // Membaca state awal pin A
    last_pin_a_state = gpio_get_level(ENCODER_PIN_A);
    
    // Membuat queue untuk event encoder dengan ukuran 10
    xEncoderQueue = xQueueCreate(QUEUE_SIZE, sizeof(encoder_event_t));
    
    // Mengecek apakah queue berhasil dibuat
    if (xEncoderQueue != NULL) {
        // Mencetak pesan bahwa queue berhasil dibuat
        printf("Encoder queue created successfully\n");
        
        // Membuat task untuk memproses encoder dengan stack 2048 bytes
        xTaskCreate(task_encoder_processor, "EncProc", TASK_STACK_SIZE, NULL, TASK_PRIORITY, &xTaskHandle);
        
        // Mencetak pesan bahwa task telah dibuat
        printf("Encoder processor task created\n");
        // Mencetak petunjuk penggunaan
        printf("Rotate encoder to see CW/CCW detection\n");
    } else {
        // Mencetak pesan error jika queue gagal dibuat
        printf("Failed to create encoder queue!\n");
    }
    
    // app_main harus mengembalikan void atau memanggil vTaskDelete
    // Task tidak perlu dihapus karena akan berjalan terus menerus
}
