// src/main.c untuk ESP32_05_DAC_RTOS_Output
// Program FreeRTOS dengan DAC (PWM) dan RTOS queue

// Menginclude header FreeRTOS untuk task creation dan delay
#include "freertos/FreeRTOS.h"
// Menginclude header FreeRTOS untuk queue
#include "freertos/queue.h"
// Menginclude header FreeRTOS untuk task management
#include "freertos/task.h"
// Menginclude header LEDC (PWM) ESP32 untuk mensimulasikan DAC
#include "driver/ledc.h"
// Menginclude header UART ESP32
#include "driver/uart.h"
// Menginclude header untuk konfigurasi aplikasi
#include "config.h"
// Menginclude header standard untuk printf
#include "stdio.h"
// Menginclude header standard untuk math (sin)
#include "math.h"

// Mendefinisikan handle untuk queue waveform selection
QueueHandle_t xWaveformQueue = NULL;
// Mendefinisikan handle untuk task generate waveform
TaskHandle_t xGenTaskHandle = NULL;
// Mendefinisikan handle untuk task output DAC
TaskHandle_t xOutTaskHandle = NULL;
// Mendefinisikan tipe waveform saat ini
waveform_type_t current_waveform = WAVEFORM_SINE;
// Mendefinisikan counter untuk posisi gelombang
uint32_t waveform_counter = 0;

// Fungsi untuk inisialisasi PWM (mensimulasikan DAC)
void init_pwm_dac(void) {
    // Mengatur konfigurasi timer LEDC
    ledc_timer_config_t ledc_timer = {
        .duty_resolution = PWM_RESOLUTION,  // Resolusi 8 bit
        .freq_hz = PWM_FREQUENCY,           // Frekuensi 1 kHz
        .speed_mode = LEDC_HIGH_SPEED_MODE, // Mode high speed
        .timer_num = LEDC_TIMER,           // Timer 0
        .clk_cfg = LEDC_AUTO_CLK           // Clock otomatis
    };
    // Mengatur timer LEDC dengan konfigurasi di atas
    ledc_timer_config(&ledc_timer);
    
    // Mengatur konfigurasi channel LEDC
    ledc_channel_config_t ledc_channel = {
        .channel = LEDC_CHANNEL,           // Channel 0
        .duty = 0,                         // Duty cycle awal 0
        .gpio_num = PWM_GPIO,              // GPIO 2
        .speed_mode = LEDC_HIGH_SPEED_MODE, // Mode high speed
        .timer_sel = LEDC_TIMER,           // Gunakan timer 0
        .hpoint = 0                        // Hpoint 0
    };
    // Mengatur channel LEDC dengan konfigurasi di atas
    ledc_channel_config(&ledc_channel);
}

// Fungsi untuk menghitung nilai gelombang sinus
uint8_t calculate_sine_wave(uint32_t position) {
    // Menghitung nilai sinus (0-255) berdasarkan posisi
    return (uint8_t)((sinf(position * 2.0f * M_PI / 256.0f) + 1.0f) * 127.5f);
}

// Fungsi untuk menghitung nilai gelombang sawtooth
uint8_t calculate_sawtooth_wave(uint32_t position) {
    // Menghitung nilai gigi gergaji (0-255) berdasarkan posisi
    return (uint8_t)(position % 256);
}

// Fungsi untuk menghitung nilai gelombang kotak
uint8_t calculate_square_wave(uint32_t position) {
    // Menghitung nilai kotak (0 atau 255) berdasarkan posisi
    return (position % 256) < 128 ? 0 : 255;
}

// Fungsi untuk menghitung nilai gelombang segitiga
uint8_t calculate_triangle_wave(uint32_t position) {
    // Menghitung nilai segitiga (0-255) berdasarkan posisi
    uint32_t pos = position % 512;
    if (pos < 256) {
        return (uint8_t)pos;
    } else {
        return (uint8_t)(511 - pos);
    }
}

// Task 1: Generate waveforms
void task_generate_waveform(void *pvParameters) {
    // Variabel untuk menyimpan waveform yang akan dihasilkan
    waveform_type_t waveform_to_generate;
    // Variabel untuk nilai output waveform
    uint8_t waveform_value;
    
    // Loop tak terhingga untuk task ini
    while (1) {
        // Mengecek apakah ada waveform baru di queue (non-blocking)
        if (xQueueReceive(xWaveformQueue, &waveform_to_generate, 0) == pdTRUE) {
            // Update waveform saat ini
            current_waveform = waveform_to_generate;
            // Mencetak pesan perubahan waveform
            printf("Generate: Waveform changed to %d\n", current_waveform);
        }
        
        // Menghitung nilai waveform berdasarkan tipe saat ini
        switch (current_waveform) {
            case WAVEFORM_SINE:
                // Menghitung nilai sinus
                waveform_value = calculate_sine_wave(waveform_counter);
                break;
            case WAVEFORM_SAWTOOTH:
                // Menghitung nilai gigi gergaji
                waveform_value = calculate_sawtooth_wave(waveform_counter);
                break;
            case WAVEFORM_SQUARE:
                // Menghitung nilai kotak
                waveform_value = calculate_square_wave(waveform_counter);
                break;
            case WAVEFORM_TRIANGLE:
                // Menghitung nilai segitiga
                waveform_value = calculate_triangle_wave(waveform_counter);
                break;
            default:
                // Default ke sinus
                waveform_value = calculate_sine_wave(waveform_counter);
                break;
        }
        
        // Mengatur duty cycle PWM sesuai nilai waveform
        ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL, waveform_value);
        // Mengupdate duty cycle PWM
        ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL);
        
        // Menambah counter waveform
        waveform_counter++;
        
        // Delay task untuk kecepatan gelombang
        vTaskDelay(TASK_GEN_DELAY);
    }
}

// Task 2: Output to DAC (monitoring)
void task_output_dac(void *pvParameters) {
    // Variabel untuk menyimpan waveform saat ini
    waveform_type_t last_waveform = WAVEFORM_SINE;
    // Counter untuk cetak status periodik
    uint32_t print_counter = 0;
    
    // Loop tak terhingga untuk task ini
    while (1) {
        // Mengecek apakah waveform berubah
        if (last_waveform != current_waveform) {
            // Mencetak pesan waveform baru
            printf("Output: Now outputting waveform type %d\n", current_waveform);
            // Update waveform terakhir
            last_waveform = current_waveform;
        }
        
        // Mencetak status setiap 100 iterasi
        if (print_counter++ >= 100) {
            // Mencetak counter waveform saat ini
            printf("Output: Waveform counter = %lu\n", waveform_counter);
            // Reset counter cetak
            print_counter = 0;
        }
        
        // Delay task
        vTaskDelay(TASK_OUT_DELAY);
    }
}

// Fungsi utama program (app_main di ESP-IDF)
void app_main(void) {
    // Mencetak pesan bahwa program dimulai
    printf("ESP32 DAC (PWM) RTOS Output Demo Started\n");
    // Mencetak informasi tentang penggunaan queue
    printf("Using PWM to simulate DAC output\n");
    
    // Inisialisasi PWM DAC
    init_pwm_dac();
    
    // Membuat queue untuk waveform selection dengan ukuran 5
    xWaveformQueue = xQueueCreate(WAVEFORM_QUEUE_SIZE, sizeof(waveform_type_t));
    
    // Mengecek apakah queue berhasil dibuat
    if (xWaveformQueue != NULL) {
        // Mencetak pesan bahwa queue berhasil dibuat
        printf("Waveform queue created successfully\n");
        
        // Mengirim waveform awal ke queue
        xQueueSend(xWaveformQueue, &current_waveform, 0);
        
        // Membuat task generate waveform dengan stack 3072 bytes
        xTaskCreate(task_generate_waveform, "WaveGen", TASK_GEN_STACK_SIZE, NULL, TASK_GEN_PRIORITY, &xGenTaskHandle);
        // Membuat task output DAC dengan stack 2048 bytes
        xTaskCreate(task_output_dac, "DACOut", TASK_OUT_STACK_SIZE, NULL, TASK_OUT_PRIORITY, &xOutTaskHandle);
        
        // Mencetak pesan bahwa task telah dibuat
        printf("Waveform generator and DAC output tasks created\n");
        // Mencetak petunjuk penggunaan
        printf("PWM output on GPIO 2\n");
    } else {
        // Mencetak pesan error jika queue gagal dibuat
        printf("Failed to create waveform queue!\n");
    }
    
    // app_main harus mengembalikan void atau memanggil vTaskDelete
    // Task tidak perlu dihapus karena akan berjalan terus menerus
}
