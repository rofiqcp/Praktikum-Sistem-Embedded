// include/config.h untuk ESP32_05_DAC_RTOS_Output
// Header guard untuk mencegah include berulang
#ifndef CONFIG_H
#define CONFIG_H

// Mendefinisikan GPIO untuk PWM output (mensimulasikan DAC)
#define PWM_GPIO 2

// Mendefinisikan channel LEDC untuk PWM
#define LEDC_CHANNEL LEDC_CHANNEL_0

// Mendefinisikan timer LEDC untuk PWM
#define LEDC_TIMER LEDC_TIMER_0

// Mendefinisikan frekuensi PWM (1 kHz)
#define PWM_FREQUENCY 1000

// Mendefinisikan resolusi PWM (8 bit = 0-255)
#define PWM_RESOLUTION LEDC_TIMER_8_BIT

// Mendefinisikan baud rate UART untuk komunikasi serial
#define UART_BAUD_RATE 115200

// Mendefinisikan ukuran queue untuk waveform selection
#define WAVEFORM_QUEUE_SIZE 5

// Mendefinisikan delay untuk task generate waveform dalam tick FreeRTOS
#define TASK_GEN_DELAY pdMS_TO_TICKS(10)

// Mendefinisikan delay untuk task output DAC dalam tick FreeRTOS
#define TASK_OUT_DELAY pdMS_TO_TICKS(5)

// Mendefinisikan ukuran stack untuk task generate waveform
#define TASK_GEN_STACK_SIZE 3072

// Mendefinisikan ukuran stack untuk task output DAC
#define TASK_OUT_STACK_SIZE 2048

// Mendefinisikan prioritas untuk task generate waveform
#define TASK_GEN_PRIORITY 2

// Mendefinisikan prioritas untuk task output DAC
#define TASK_OUT_PRIORITY 2

// Mendefinisikan tipe waveform
typedef enum {
    WAVEFORM_SINE = 0,      // Gelombang sinus
    WAVEFORM_SAWTOOTH = 1,  // Gelombang gigi gergaji
    WAVEFORM_SQUARE = 2,    // Gelombang kotak
    WAVEFORM_TRIANGLE = 3   // Gelombang segitiga
} waveform_type_t;

// Mengakhiri header guard
#endif
