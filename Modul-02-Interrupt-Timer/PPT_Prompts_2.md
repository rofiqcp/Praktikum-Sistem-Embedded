# Prompt untuk Pembuatan PPT - Bagian 2 (Praktikum & Aplikasi)
## Modul 02: Interrupt dan Timer

> **Instruksi Umum untuk AI Image Generator:**
> - Style: Technical illustration, clean, professional
> - Color scheme: Biru tua (#1a365d), Hijau (#38a169), Abu-abu (#718096)
> - Resolusi: 1920x1080 (16:9)
> - Font style: Modern sans-serif
> - **Framework: STM32Cube HAL (STM32) dan ESP-IDF (ESP32) — BUKAN Arduino**

---

## Slide 1: Cover Bagian 2

**Prompt:**
```
Buat slide cover untuk PPT Bagian 2 dengan judul "BAB 02: INTERRUPT DAN TIMER",
subtitle "Bagian 2: Implementasi Praktis & Hands-on Lab". Background menampilkan
foto workbench dengan breadboard, STM32 Blue Pill, ESP32, dan oscilloscope
menampilkan timing waveform. Style: modern tech aesthetic dengan overlay gradient.
```

---

## Slide 2: Agenda Praktikum

**Prompt:**
```
Buat slide "Agenda Praktikum" dengan timeline horizontal:

SESI 1 (50 menit):
├─ Setup Hardware (15 min)
├─ External Interrupt Demo (20 min)
└─ Hands-on Task 1 (15 min)

SESI 2 (50 menit):
├─ Timer Configuration (15 min)
├─ Timer Interrupt Demo (20 min)
└─ Hands-on Task 2 (15 min)

SESI 3 (50 menit):
├─ Watchdog & Cascade Demo (20 min)
├─ Integration Challenge (15 min)
└─ Q&A + Assessment (15 min)

Gunakan timeline dengan progress bar dan ikon untuk setiap kegiatan.
```

---

## Slide 3: Hardware Setup Overview

**Prompt:**
```
Buat slide "Hardware Setup" dengan foto-foto komponen:

Komponen Utama:
┌─────────────────┬─────────────────┬─────────────────┐
│  STM32 Blue Pill│    ESP32        │   Breadboard    │
│  [foto board]   │   DevKitC       │   + Jumpers     │
│                 │  [foto board]   │   [foto]        │
└─────────────────┴─────────────────┴─────────────────┘

Komponen Pendukung:
• 4x Push Button (Tactile Switch)
• 4x LED 5mm (R, G, Y, B)
• 4x Resistor 330Ω
• 4x Resistor 10kΩ
• ST-Link V2 Programmer
• USB Cables (2x)

Checklist box di samping setiap item.
```

---

## Slide 4: Skema Rangkaian STM32

**Prompt:**
```
Buat slide "Skema Rangkaian STM32" dengan diagram wiring:

                 STM32F103C8T6
              ┌────────────────────┐
    BTN1 ─────┤PA0 (EXTI0)    PC13├───[LED_BUILTIN]
    BTN2 ─────┤PA1 (EXTI1)    PB3 ├───[R 330Ω]──LED1
    BTN3 ─────┤PB0 (EXTI0)    PB4 ├───[R 330Ω]──LED2
    BTN4 ─────┤PB1 (EXTI1)    PB5 ├───[R 330Ω]──LED3
              │                    │
      GND ────┤GND           3.3V ├────
      3.3V ───┤3.3V          GND  ├────
              └────────────────────┘

Button Wiring Detail:
3.3V ──[10kΩ]──┬── GPIO (INPUT)
               │
              ─┴─ Button ── GND

Catatan: PC13 = Active LOW (internal LED)
```

---

## Slide 5: Skema Rangkaian ESP32

**Prompt:**
```
Buat slide "Skema Rangkaian ESP32" dengan diagram wiring:

                   ESP32 DevKitC
              ┌────────────────────┐
    BTN1 ─────┤GPIO0 (BOOT)  GPIO2├───[LED_BUILTIN]
    BTN2 ─────┤GPIO13        GPIO4├───[R 330Ω]──LED1
    BTN3 ─────┤GPIO15        GPIO5├───[R 330Ω]──LED2
    BTN4 ─────┤GPIO14        GPIO18├──[R 330Ω]──LED3
              │                    │
      GND ────┤GND            3.3V├────
      5V  ────┤5V (USB)       GND ├────
              └────────────────────┘

⚠️ CATATAN:
• GPIO0 = Boot button (sudah ada di board)
• GPIO2 = Built-in LED (beberapa board)
• Input-only: GPIO34-39 (tidak ada pull-up)

Gunakan warna kabel: Merah=VCC, Hitam=GND, Lainnya=Signal
```

---

## Slide 6: Demo 1 - External Interrupt STM32 (HAL)

**Prompt:**
```
Buat slide "Demo 1: External Interrupt STM32 (HAL)" dengan:

Tujuan: Menyalakan LED saat tombol ditekan menggunakan interrupt

Konfigurasi:
• Button pada PA0 (EXTI0) — via STM32CubeMX
• LED pada PB3 — GPIO_Output
• Trigger: Falling Edge (GPIO_MODE_IT_FALLING)

Expected Behavior:
[Timeline diagram]
Button: ────┐___________┌────────
            ↓ EXTI0_IRQ → HAL_GPIO_EXTI_Callback()
LED:    ────┐XXXXXXXXX┌──────────
            Toggle     Toggle

Serial Output (via UART printf):
> Button pressed! Count: 1
> Button pressed! Count: 2
> Button pressed! Count: 3

Status box: "✓ No polling required - CPU efficient!"
```

---

## Slide 7: Code Demo 1 - STM32 EXTI (HAL)

**Prompt:**
```
Buat slide code untuk STM32 External Interrupt dengan HAL, syntax highlighting:

#include "main.h"
#include <stdio.h>

volatile uint32_t press_count = 0;
volatile uint8_t button_flag = 0;

/* Callback dari HAL — dipanggil otomatis saat EXTI terjadi */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    if (GPIO_Pin == GPIO_PIN_0) {
        button_flag = 1;
        press_count++;
    }
}

int main(void) {
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();      // CubeMX auto-generated
    MX_USART1_UART_Init();
    
    printf("STM32 EXTI Interrupt Demo Ready!\r\n");
    
    while (1) {
        if (button_flag) {
            button_flag = 0;
            HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_3);
            printf("Button pressed! Count: %lu\r\n", press_count);
        }
    }
}

Highlight: volatile keyword, HAL_GPIO_EXTI_Callback, flag pattern
Catatan: CubeMX menggenerate MX_GPIO_Init() dengan EXTI config
```

---

## Slide 8: Demo 2 - External Interrupt ESP32 (ESP-IDF)

**Prompt:**
```
Buat slide "Demo 2: External Interrupt ESP32 (ESP-IDF)" dengan code:

#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#define BTN_PIN  GPIO_NUM_4
#define LED_PIN  GPIO_NUM_2
static const char *TAG = "EXTI";
static QueueHandle_t gpio_evt_queue = NULL;

static void IRAM_ATTR gpio_isr_handler(void *arg) {
    uint32_t gpio_num = (uint32_t)arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

static void gpio_task(void *arg) {
    uint32_t io_num;
    uint32_t count = 0;
    while (1) {
        if (xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
            count++;
            int level = gpio_get_level(LED_PIN);
            gpio_set_level(LED_PIN, !level);
            ESP_LOGI(TAG, "Button GPIO%lu pressed! Count: %lu",
                     io_num, count);
        }
    }
}

Perbedaan dari STM32:
⚠️ IRAM_ATTR wajib untuk ISR
⚠️ Gunakan Queue untuk komunikasi ISR → Task
⚠️ gpio_config() bukan pinMode()
```

---

## Slide 9: Hands-on Task 1

**Prompt:**
```
Buat slide "Hands-on Task 1: Multiple Button Handler" dengan:

TUGAS:
Buat program yang mendeteksi 3 tombol dengan interrupt berbeda
dan menyalakan LED yang sesuai.

Requirements:
┌────────────────────────────────────────────────────┐
│ Button 1 → Toggle LED 1 (Hijau)                    │
│ Button 2 → Toggle LED 2 (Kuning)                   │
│ Button 3 → Toggle LED 3 (Merah)                    │
│ All buttons → Print status ke Serial/UART          │
└────────────────────────────────────────────────────┘

Checklist:
□ Semua button menggunakan interrupt (bukan polling)
□ STM32: HAL_GPIO_EXTI_Callback() dengan GPIO_Pin check
□ ESP32: gpio_isr_handler_add() + IRAM_ATTR
□ Implementasi debouncing (timestamp-based)
□ Output UART/ESP_LOGI menampilkan status

Waktu: 15 menit
Tingkat: ⭐⭐ (Menengah)
```

---

## Slide 10: Demo 3 - Hardware Timer STM32 (HAL)

**Prompt:**
```
Buat slide "Demo 3: Hardware Timer STM32 (HAL)" dengan:

Tujuan: LED blink 1 Hz menggunakan timer interrupt (tanpa delay)

Timer Configuration via CubeMX:
┌────────────────────────────────────────────────────┐
│ Clock APB1 = 72 MHz                                │
│ Target: 1 Hz (1 second period)                     │
│                                                    │
│ Perhitungan:                                       │
│ 72,000,000 / 1 Hz = 72,000,000 counts             │
│                                                    │
│ Pilih: PSC = 7199, ARR = 9999                      │
│ Timer_Freq = 72MHz / 7200 = 10 kHz                │
│ Period = 10000 / 10 kHz = 1 second ✓              │
└────────────────────────────────────────────────────┘

Timing Diagram:
TIM2_CNT: 0...1000...5000...9999 | 0...1000...
                               ↓ Update IRQ
LED:      ─────────┐___________┌───────────
                   Toggle      Toggle
```

---

## Slide 11: Code Demo 3 - STM32 Timer (HAL)

**Prompt:**
```
Buat slide code Timer STM32 dengan HAL dan penjelasan:

#include "main.h"
#include <stdio.h>

extern TIM_HandleTypeDef htim2;  // CubeMX generated
volatile uint32_t tick_count = 0;

/* Timer period elapsed callback */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    if (htim->Instance == TIM2) {
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
        tick_count++;
    }
}

int main(void) {
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_TIM2_Init();          // CubeMX: PSC=7199, ARR=9999
    MX_USART1_UART_Init();
    
    /* Start timer dengan interrupt */
    HAL_TIM_Base_Start_IT(&htim2);
    
    printf("Timer Demo Started!\r\n");
    
    while (1) {
        // Main loop bebas untuk tugas lain!
        printf("Tick count: %lu\r\n", tick_count);
        HAL_Delay(2000);
    }
}

Callout boxes:
• "CubeMX menghitung PSC/ARR dari target frequency"
• "HAL_TIM_Base_Start_IT() = start + enable interrupt"
• "Main loop TIDAK terblokir oleh LED blink!"
```

---

## Slide 12: Demo 4 - Hardware Timer ESP32 (ESP-IDF gptimer)

**Prompt:**
```
Buat slide "Demo 4: Hardware Timer ESP32 (ESP-IDF gptimer)" dengan code:

#include "driver/gptimer.h"
#include "driver/gpio.h"
#include "esp_log.h"

#define LED_PIN GPIO_NUM_2
static const char *TAG = "TIMER";
static volatile uint32_t isr_count = 0;

static bool IRAM_ATTR timer_alarm_cb(
    gptimer_handle_t timer,
    const gptimer_alarm_event_data_t *edata,
    void *user_ctx)
{
    isr_count++;
    int level = gpio_get_level(LED_PIN);
    gpio_set_level(LED_PIN, !level);
    return false;  // no high-priority task woken
}

void app_main(void) {
    // GPIO output
    gpio_reset_pin(LED_PIN);
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
    
    // Timer config: 1MHz resolution
    gptimer_handle_t gptimer = NULL;
    gptimer_config_t timer_config = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = 1000000,  // 1MHz
    };
    gptimer_new_timer(&timer_config, &gptimer);
    
    // Alarm setiap 500ms (500,000 µs), auto-reload
    gptimer_alarm_config_t alarm_config = {
        .alarm_count = 500000,
        .reload_count = 0,
        .flags.auto_reload_on_alarm = true,
    };
    gptimer_set_alarm_action(gptimer, &alarm_config);
    
    // Register callback
    gptimer_event_callbacks_t cbs = {
        .on_alarm = timer_alarm_cb,
    };
    gptimer_register_event_callbacks(gptimer, &cbs, NULL);
    gptimer_enable(gptimer);
    gptimer_start(gptimer);
    
    while (1) {
        ESP_LOGI(TAG, "ISR count: %lu", isr_count);
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

Highlight: gptimer API (bukan timerBegin), IRAM_ATTR callback
```

---

## Slide 13: Hands-on Task 2

**Prompt:**
```
Buat slide "Hands-on Task 2: Stopwatch" dengan spesifikasi:

TUGAS:
Buat stopwatch sederhana menggunakan timer dan button interrupt

Fitur:
┌────────────────────────────────────────────────────┐
│ • Button 1: Start/Stop stopwatch                   │
│ • Button 2: Reset stopwatch                        │
│ • Timer: Update counter setiap 100ms               │
│ • Serial: Display waktu dalam format MM:SS.S       │
└────────────────────────────────────────────────────┘

STM32 (HAL):
• Button via HAL_GPIO_EXTI_Callback()
• Timer via HAL_TIM_PeriodElapsedCallback()
• Output via printf() + UART

ESP32 (ESP-IDF):
• Button via gpio_isr_handler_add()
• Timer via gptimer alarm callback
• Output via ESP_LOGI()

Expected Output:
> Stopwatch Ready!
> [START] 00:00.0
> [RUNNING] 00:01.5
> [STOP] 00:04.8
> [RESET] 00:00.0

Waktu: 20 menit
Tingkat: ⭐⭐⭐ (Menengah-Lanjut)
```

---

## Slide 14: Demo 5 - Watchdog Timer

**Prompt:**
```
Buat slide "Demo 5: Watchdog Timer" dengan side-by-side code:

STM32 HAL - IWDG:
┌────────────────────────────────────────────────────┐
│ IWDG_HandleTypeDef hiwdg;                          │
│                                                    │
│ void init_watchdog(void) {                         │
│     hiwdg.Instance = IWDG;                         │
│     hiwdg.Init.Prescaler = IWDG_PRESCALER_64;     │
│     hiwdg.Init.Reload = 625; // ~1s timeout       │
│     HAL_IWDG_Init(&hiwdg);                        │
│ }                                                  │
│                                                    │
│ while (1) {                                        │
│     process_sensor();                              │
│     HAL_IWDG_Refresh(&hiwdg); // Feed!             │
│     HAL_Delay(100);                                │
│ }                                                  │
└────────────────────────────────────────────────────┘

ESP32 ESP-IDF - Task WDT:
┌────────────────────────────────────────────────────┐
│ #include "esp_task_wdt.h"                          │
│                                                    │
│ esp_task_wdt_config_t cfg = {                      │
│     .timeout_ms = 5000,                            │
│     .trigger_panic = true,                         │
│ };                                                 │
│ esp_task_wdt_reconfigure(&cfg);                    │
│ esp_task_wdt_add(NULL);                            │
│                                                    │
│ while (1) {                                        │
│     process_sensor();                              │
│     esp_task_wdt_reset(); // Feed!                 │
│     vTaskDelay(pdMS_TO_TICKS(100));                │
│ }                                                  │
└────────────────────────────────────────────────────┘

Test: Comment out refresh → observe system reset!
```

---

## Slide 15: Demo 6 - Timer Cascade

**Prompt:**
```
Buat slide "Demo 6: Timer Cascade (Master-Slave)" dengan:

STM32 HAL - Hardware Cascade:
┌────────────────────────────────────────────────────┐
│ /* TIM2 sebagai Master */                          │
│ TIM_MasterConfigTypeDef sMasterConfig = {0};       │
│ sMasterConfig.MasterOutputTrigger =                │
│     TIM_TRGO_UPDATE;                               │
│ sMasterConfig.MasterSlaveMode =                    │
│     TIM_MASTERSLAVEMODE_ENABLE;                    │
│ HAL_TIMEx_MasterConfigSynchronization(             │
│     &htim2, &sMasterConfig);                       │
│                                                    │
│ /* TIM3 sebagai Slave */                           │
│ TIM_SlaveConfigTypeDef sSlaveConfig = {0};         │
│ sSlaveConfig.SlaveMode =                           │
│     TIM_SLAVEMODE_EXTERNAL1;                       │
│ sSlaveConfig.InputTrigger = TIM_TS_ITR1;           │
│ HAL_TIM_SlaveConfigSynchronization(                │
│     &htim3, &sSlaveConfig);                        │
└────────────────────────────────────────────────────┘

Diagram:
TIM2 overflow (1kHz) ──TRGO──► TIM3 clock input
TIM2: 16-bit counter   ×      TIM3: 16-bit counter
         = Effective 32-bit counting!

Use case: Precise long-duration timing, event counting
```

---

## Slide 16: Advanced Application - Frequency Counter

**Prompt:**
```
Buat slide "Advanced: Frequency Counter" dengan diagram sistem:

Konsep:
┌─────────────────────────────────────────────────────┐
│ Hitung jumlah pulse input dalam periode waktu tetap│
│ (Gate Time) menggunakan 2 interrupt sources        │
└─────────────────────────────────────────────────────┘

System Diagram:
                    ┌─────────────┐
  Signal Input ────►│ Input Capture│────► Pulse Count
  (GPIO + INT)      │  Interrupt   │
                    └─────────────┘
                          │
                    ┌─────────────┐
  Gate Timer ──────►│ Timer       │────► Gate = 1 second
  (1 Hz)            │ Interrupt   │
                    └─────────────┘
                          │
                          ▼
               Frequency = Pulse Count / Gate Time

STM32: TIM Input Capture + TIM Gate
ESP32: GPIO ISR + gptimer alarm
```

---

## Slide 17: Code - Frequency Counter (ESP-IDF)

**Prompt:**
```
Buat slide code Frequency Counter ESP-IDF dengan penjelasan:

#include "driver/gpio.h"
#include "driver/gptimer.h"
#include "esp_log.h"

static volatile uint32_t pulse_count = 0;
static volatile bool measure_ready = false;

/* Pulse input ISR */
static void IRAM_ATTR pulse_isr(void *arg) {
    pulse_count++;
}

/* Gate timer callback (1 second) */
static bool IRAM_ATTR gate_cb(gptimer_handle_t timer,
    const gptimer_alarm_event_data_t *edata,
    void *ctx)
{
    measure_ready = true;
    return false;
}

void app_main(void) {
    // GPIO input for signal
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << GPIO_NUM_4),
        .mode = GPIO_MODE_INPUT,
        .intr_type = GPIO_INTR_POSEDGE,
    };
    gpio_config(&io_conf);
    gpio_install_isr_service(0);
    gpio_isr_handler_add(GPIO_NUM_4, pulse_isr, NULL);
    
    // Gate timer 1 detik (gptimer)
    gptimer_handle_t gate_timer = NULL;
    gptimer_config_t tcfg = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = 1000000,
    };
    gptimer_new_timer(&tcfg, &gate_timer);
    gptimer_alarm_config_t acfg = {
        .alarm_count = 1000000, // 1s
        .flags.auto_reload_on_alarm = true,
    };
    gptimer_set_alarm_action(gate_timer, &acfg);
    // ... register callback, enable, start
    
    while (1) {
        if (measure_ready) {
            measure_ready = false;
            uint32_t freq = pulse_count;
            pulse_count = 0;
            ESP_LOGI(TAG, "Frequency: %lu Hz", freq);
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

Use case: Mengukur RPM motor, frekuensi sinyal
```

---

## Slide 18: Interrupt Priority Demonstration

**Prompt:**
```
Buat slide "Demo: Interrupt Priority" dengan:

Skenario Test:
┌─────────────────────────────────────────────────────┐
│ BTN1 (Priority HIGH): HAL_NVIC_SetPriority(x, 0,0) │
│ BTN2 (Priority LOW):  HAL_NVIC_SetPriority(x, 3,0) │
│                                                     │
│ Test: Tekan BTN2, lalu tekan BTN1 saat BTN2 handler │
│       masih running                                 │
└─────────────────────────────────────────────────────┘

Expected Output (Nested Interrupt):
LOW handler start
    HIGH handler start    ◄─── HIGH interrupt LOW
    HIGH handler end
LOW handler end           ◄─── LOW continues

Timeline:
BTN2:    ┌──────────────────┐
         │   LOW Handler    │
BTN1:    │  ┌──────────┐    │
         │  │HIGH Hndlr│    │
         ▼  ▼          ▼    ▼
Time:    t0 t1         t2   t3

⚠️ Ini adalah "Nested Interrupt" - fitur NVIC!
Konfigurasi: HAL_NVIC_SetPriority() + HAL_NVIC_EnableIRQ()
```

---

## Slide 19: Common Mistakes & Debugging

**Prompt:**
```
Buat slide "Common Mistakes & Debugging" dengan:

❌ MISTAKE 1: Tidak clear flag (tanpa HAL)
void EXTI0_IRQHandler(void) {
    // Handler code...
    // Lupa: __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_0);
}
Result: ISR dipanggil terus-menerus!
✅ Fix: Gunakan HAL_GPIO_EXTI_IRQHandler() — auto clear

❌ MISTAKE 2: Lupa IRAM_ATTR (ESP32)
void gpio_isr_handler(void *arg) {  // Missing IRAM_ATTR!
    ...
}
Result: Guru panic crash!

❌ MISTAKE 3: Tidak pakai volatile
uint8_t button_pressed = 0;  // Missing volatile!
void HAL_GPIO_EXTI_Callback(uint16_t pin) {
    button_pressed = 1;
}
Result: Compiler optimize → flag tidak pernah update!

❌ MISTAKE 4: Blocking dalam ISR
void HAL_GPIO_EXTI_Callback(uint16_t pin) {
    printf("Test\r\n");       // BLOCKING!
    HAL_Delay(100);           // BLOCKING!
}
Result: System hang atau timing issues!

Gunakan ❌ merah untuk kesalahan dan ✓ hijau untuk koreksi.
```

---

## Slide 20: Debugging Techniques

**Prompt:**
```
Buat slide "Debugging Techniques" dengan tips:

1. GPIO Toggle Method (Oscilloscope):
┌────────────────────────────────────────────────────┐
│ void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {   │
│     HAL_GPIO_WritePin(DEBUG_PORT, DEBUG_PIN,        │
│                       GPIO_PIN_SET);                │
│     // ISR code                                    │
│     HAL_GPIO_WritePin(DEBUG_PORT, DEBUG_PIN,        │
│                       GPIO_PIN_RESET);              │
│ }                                                  │
│                                                    │
│ → Ukur pulse width dengan oscilloscope            │
│ → Mengetahui berapa lama ISR berjalan             │
└────────────────────────────────────────────────────┘

2. Counter Verification:
┌────────────────────────────────────────────────────┐
│ volatile uint32_t isr_counter = 0;                 │
│ void HAL_TIM_PeriodElapsedCallback(...) {           │
│     isr_counter++;                                 │
│ }                                                  │
│                                                    │
│ while (1) {                                        │
│     printf("ISR: %lu\r\n", isr_counter);           │
│     HAL_Delay(1000);                               │
│ }                                                  │
└────────────────────────────────────────────────────┘

3. LED Status Indicator:
• ISR entry: LED ON
• ISR exit: LED OFF
• Blink rate = ISR frequency
```

---

## Slide 21: Performance Comparison

**Prompt:**
```
Buat slide "Performance Comparison" dengan tabel benchmark:

INTERRUPT LATENCY TEST (Button → LED Toggle):

┌─────────────────┬────────────────┬────────────────┐
│ Method          │ STM32F103      │ ESP32          │
├─────────────────┼────────────────┼────────────────┤
│ Polling (1ms)   │ ~1000 µs       │ ~1000 µs       │
│ Polling (100µs) │ ~100 µs        │ ~100 µs        │
│ Interrupt       │ ~2 µs          │ ~5 µs          │
├─────────────────┼────────────────┼────────────────┤
│ CPU Usage Poll  │ ~90%           │ ~90%           │
│ CPU Usage INT   │ ~0.1%          │ ~0.1%          │
└─────────────────┴────────────────┴────────────────┘

TIMER ACCURACY TEST (1 second interval):

┌─────────────────┬────────────────┬────────────────┐
│ Method          │ STM32 Error    │ ESP32 Error    │
├─────────────────┼────────────────┼────────────────┤
│ HAL_Delay/vTask │ ±5ms           │ ±10ms          │
│ Hardware Timer  │ ±0.1ms         │ ±0.1ms         │
└─────────────────┴────────────────┴────────────────┘

Highlight: "Hardware Timer = 100x more accurate!"
```

---

## Slide 22: Hands-on Task 3 - Integration

**Prompt:**
```
Buat slide "Hands-on Task 3: Integration Challenge" dengan:

FINAL CHALLENGE:
Buat sistem "Event Logger" yang menggabungkan semua konsep

Requirements:
┌────────────────────────────────────────────────────┐
│ 1. 3 Button dengan External Interrupt              │
│ 2. Timer untuk timestamp (microsecond precision)   │
│ 3. Heartbeat LED (blink 1 Hz via timer)            │
│ 4. Watchdog timer untuk safety                     │
│ 5. Output format:                                  │
│    [TIMESTAMP] EVENT: Button_X pressed             │
└────────────────────────────────────────────────────┘

Expected Output:
[0.000000] System Started
[0.000000] Watchdog enabled (5s timeout)
[0.000000] Heartbeat LED: ON
[1.000000] Heartbeat LED: OFF
[1.523456] EVENT: Button_1 pressed
[2.000000] Heartbeat LED: ON
[2.891234] EVENT: Button_2 pressed
...

Waktu: 25 menit
Tingkat: ⭐⭐⭐⭐ (Lanjut)
```

---

## Slide 23: Daftar Program Praktikum

**Prompt:**
```
Buat slide "Daftar 12 Program Praktikum" dengan tabel lengkap:

┌────┬──────────────────────┬──────────────────────────────────────┐
│ No │ Nama Program         │ Konsep yang Dipelajari               │
├────┼──────────────────────┼──────────────────────────────────────┤
│ 01 │ EXTI_Interrupt       │ External interrupt dasar             │
│ 02 │ EXTI_Debounce        │ Debouncing dengan timestamp          │
│ 03 │ Timer_Periodic       │ Timer interrupt periodik             │
│ 04 │ Timer_One_Shot       │ Timer satu kali trigger              │
│ 05 │ Timer_PWM_Basic      │ PWM generation via timer             │
│ 06 │ Watchdog_Timer       │ IWDG/WWDG & esp_task_wdt            │
│ 07 │ Timer_Cascade        │ Master-slave timer chaining          │
│ 08 │ Output_Compare_Toggle│ OC toggle mode                       │
│ 09 │ Input_Capture        │ Mengukur pulse width                 │
│ 10 │ Encoder_Interface    │ Quadrature encoder reading           │
│ 11 │ Multiple_Timers      │ Beberapa timer bersamaan             │
│ 12 │ NVIC_Priority        │ Priority levels & nesting            │
└────┴──────────────────────┴──────────────────────────────────────┘

Setiap program ada versi STM32 (HAL) dan ESP32 (ESP-IDF)
Direktori: praktikum/STM32_XX dan praktikum/ESP32_XX
```

---

## Slide 24: Project Preview

**Prompt:**
```
Buat slide "Project Preview: Security Monitoring System" dengan:

System Overview:
┌─────────────────────────────────────────────────────┐
│                                                     │
│  [PIR Sensor] ──┐                     ┌─► [Alert]   │
│  [Door Switch]──┼──► STM32 ◄──UART──► ESP32 ──► [Log]│
│  [Window Sens]──┤    (Edge)           (Hub)  └─► [UART]│
│  [Panic Button]─┘                                   │
│                                                     │
└─────────────────────────────────────────────────────┘

Key Features:
• NVIC Priority untuk different sensors
• Watchdog timer untuk reliability
• Timer-based timestamp dengan precision
• Interrupt-driven response (no polling)
• UART communication dengan checksum

Framework: STM32Cube HAL + ESP-IDF (bukan Arduino!)
Deadline: 2 minggu
Deliverables: Code + Documentation + Video Demo
```

---

## Slide 25: Assessment Criteria

**Prompt:**
```
Buat slide "Assessment Criteria" dengan rubrik:

PRAKTIKUM (40%):
┌────────────────────────────────────────────────────┐
│ ✓ Task 1: Multiple Button Handler    [10 points]   │
│ ✓ Task 2: Stopwatch                  [15 points]   │
│ ✓ Task 3: Event Logger + Watchdog    [15 points]   │
└────────────────────────────────────────────────────┘

LAPORAN (30%):
┌────────────────────────────────────────────────────┐
│ ✓ Dokumentasi lengkap                [10 points]   │
│ ✓ Analisis hasil                     [10 points]   │
│ ✓ Jawaban pertanyaan                 [10 points]   │
└────────────────────────────────────────────────────┘

PEMAHAMAN (20%):
┌────────────────────────────────────────────────────┐
│ ✓ Bisa menjelaskan konsep interrupt  [10 points]   │
│ ✓ Bisa menghitung timer              [10 points]   │
└────────────────────────────────────────────────────┘

KEAKTIFAN (10%):
┌────────────────────────────────────────────────────┐
│ ✓ Partisipasi dan inisiatif          [10 points]   │
└────────────────────────────────────────────────────┘
```

---

## Slide 26: Quick Reference Card

**Prompt:**
```
Buat slide "Quick Reference Card" untuk di-print:

STM32 INTERRUPT (HAL):
HAL_GPIO_Init(GPIOx, &GPIO_InitStruct); // Mode=IT_FALLING
HAL_NVIC_SetPriority(EXTIx_IRQn, pre, sub);
HAL_NVIC_EnableIRQ(EXTIx_IRQn);
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) { ... }

STM32 TIMER (HAL):
HAL_TIM_Base_Init(&htimx);    // PSC, ARR, CounterMode
HAL_TIM_Base_Start_IT(&htimx);
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) { ... }

STM32 WATCHDOG (HAL):
HAL_IWDG_Init(&hiwdg);        // Prescaler, Reload
HAL_IWDG_Refresh(&hiwdg);     // Feed watchdog

ESP32 INTERRUPT (ESP-IDF):
gpio_config(&io_conf);         // .intr_type = GPIO_INTR_NEGEDGE
gpio_install_isr_service(0);
gpio_isr_handler_add(pin, isr_func, arg);

ESP32 TIMER (ESP-IDF):
gptimer_new_timer(&config, &handle);
gptimer_set_alarm_action(handle, &alarm);
gptimer_register_event_callbacks(handle, &cbs, NULL);
gptimer_enable(handle); gptimer_start(handle);

ESP32 WATCHDOG (ESP-IDF):
esp_task_wdt_reconfigure(&cfg);
esp_task_wdt_add(NULL); esp_task_wdt_reset();

TIMER CALCULATION:
Timer_Freq = Clock / (PSC + 1)
Period = (ARR + 1) / Timer_Freq

Format: business card style, bisa di-print 4 per halaman.
```

---

## Slide 27: FAQ

**Prompt:**
```
Buat slide "Frequently Asked Questions" dengan:

Q1: Mengapa ISR harus singkat?
A: ISR memblokir interrupt lain. Jika terlalu lama,
   sistem menjadi tidak responsif dan bisa miss event.

Q2: Kapan pakai polling vs interrupt?
A: Polling: event sangat sering (>10kHz), sederhana
   Interrupt: event sporadis, perlu response cepat

Q3: Apa beda IWDG dan WWDG pada STM32?
A: IWDG: independent clock, selalu running, refresh kapan saja
   WWDG: window-based, refresh hanya di window tertentu

Q4: Bagaimana debug ISR tanpa printf?
A: Gunakan GPIO toggle + oscilloscope, atau
   set flag dan print di main loop (HAL/ESP-IDF).

Q5: Timer tidak akurat, kenapa?
A: Cek: clock source, prescaler calculation,
   interrupt priority, ISR execution time.

Format Q&A dengan warna berbeda untuk question dan answer.
```

---

## Slide 28: Resources & Next Steps

**Prompt:**
```
Buat slide "Resources & Next Steps" dengan:

📚 RESOURCES:
• GitHub Repository: [QR Code]
• Video Tutorial: [QR Code]
• Dokumentasi Online: [QR Code]

🔗 USEFUL LINKS:
• Mastering STM32 - Ch7 Interrupts, Ch11 Timers
• Kolban's ESP32 Book - p267 ISR, p300 Timers
• ESP-IDF Official Examples (gpio, gptimer, wdt)
• STM32 Timer Cookbook (AN4776)

📅 NEXT MODULE:
BAB 03: Serial Communication (UART)
• Mengirim data antar MCU
• Protokol komunikasi
• Integration dengan interrupt

💡 PREPARATION:
• Review konsep serial communication
• Pastikan hardware ready
• Install serial terminal (HTerm/Realterm)

Gunakan QR code placeholder untuk setiap link.
```

---

## Slide 29: Closing

**Prompt:**
```
Buat slide "Terima Kasih" dengan:

🎯 KEY TAKEAWAYS:
1. Interrupt = Responsif & Efisien
2. Timer = Presisi & Non-blocking
3. Watchdog = Safety & Reliability
4. Best Practice = Short ISR + Flag Pattern
5. Framework = STM32Cube HAL + ESP-IDF

📧 CONTACT:
[Email instruktur]
[Office hours]

🙋 QUESTIONS?

Background: gradient dengan circuit pattern subtle
Footer: "BAB 02: Interrupt dan Timer | Praktikum Sistem Embedded"

Tambahkan ikon questions/discussion untuk Q&A session.
```

---

## Catatan untuk Pembuatan PPT

1. **Konsistensi dengan Bagian 1:**
   - Gunakan color scheme yang sama
   - Font dan style harus seragam
   - Transition antar slide konsisten

2. **Framework yang digunakan:**
   - STM32: STM32Cube HAL (BUKAN Arduino/HardwareTimer)
   - ESP32: ESP-IDF native (BUKAN Arduino framework)
   - Semua code menggunakan file .c (bukan .cpp)
   - Fungsi output: printf+UART (STM32), ESP_LOGI (ESP32)

3. **12 Program Praktikum:**
   - 01_EXTI_Interrupt, 02_EXTI_Debounce
   - 03_Timer_Periodic, 04_Timer_One_Shot
   - 05_Timer_PWM_Basic, 06_Watchdog_Timer
   - 07_Timer_Cascade, 08_Output_Compare_Toggle
   - 09_Input_Capture, 10_Encoder_Interface
   - 11_Multiple_Timers, 12_NVIC_Priority

4. **Interaktivitas:**
   - Slide demo bisa ditambah video recording
   - Hands-on task dengan timer countdown
   - Quick reference bisa di-print sebagai handout

5. **Timing Presentasi:**
   - Bagian 2 total: ~90 menit
   - Sisakan waktu untuk hands-on (45 menit)
   - Demo live coding jika memungkinkan
