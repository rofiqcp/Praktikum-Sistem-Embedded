# Prompt untuk Pembuatan PPT - Bagian 1 (Teori)
## Modul 02: Interrupt dan Timer

> **Instruksi Umum untuk AI Image Generator:**
> - Style: Technical illustration, clean, professional
> - Color scheme: Biru tua (#1a365d), Hijau (#38a169), Abu-abu (#718096)
> - Resolusi: 1920x1080 (16:9)
> - Font style: Modern sans-serif
> - **Framework: STM32Cube HAL (STM32) dan ESP-IDF (ESP32) — BUKAN Arduino**

---

## Slide 1: Cover

**Prompt:**
```
Buat slide cover PPT dengan judul "BAB 02: INTERRUPT DAN TIMER", subjudul 
"Pemrograman Sistem Embedded - STM32 & ESP32". Latar belakang menampilkan 
ilustrasi minimalis timing diagram dengan interrupt signals, gelombang 
clock, dan mikrokontroler. Gunakan warna gradien biru tua ke biru terang. 
Tambahkan logo universitas di pojok kanan atas dan nama institusi di bawah.
```

---

## Slide 2: Capaian Pembelajaran

**Prompt:**
```
Buat slide "Capaian Pembelajaran" dengan 6 poin dalam format numbered list:
1. Memahami konsep interrupt dan perbedaan dengan polling
2. Menguasai konfigurasi NVIC dan EXTI pada STM32 menggunakan HAL
3. Mengimplementasikan GPIO Interrupt pada ESP32 menggunakan ESP-IDF
4. Mengkonfigurasi Hardware Timer pada kedua platform (HAL & gptimer)
5. Menerapkan teknik debouncing berbasis interrupt/timer
6. Memahami Watchdog Timer dan Timer Cascade

Gunakan ikon target/crosshair di samping judul. Background dengan pattern 
subtle circuit board.
```

---

## Slide 3: Outline Materi

**Prompt:**
```
Buat slide "Outline Materi" dengan diagram mind-map sederhana:
- Pusat: "Interrupt & Timer"
- Cabang 1: "Konsep Interrupt" → Polling vs Interrupt, ISR, Priority
- Cabang 2: "External Interrupt" → NVIC/EXTI STM32, GPIO ISR ESP32
- Cabang 3: "Hardware Timer" → Counter, Prescaler, Auto-reload
- Cabang 4: "Watchdog Timer" → IWDG, WWDG, esp_task_wdt
- Cabang 5: "Timer Cascade" → Master-Slave, Software Chaining
- Cabang 6: "Aplikasi" → Debouncing, Timing, Event counting

Gunakan warna berbeda untuk setiap cabang. Style: clean infographic.
```

---

## Slide 4: Apa itu Interrupt?

**Prompt:**
```
Buat slide penjelasan "Apa itu Interrupt?" dengan:
- Definisi dalam box highlight: "Interrupt adalah mekanisme hardware yang 
  memungkinkan CPU menghentikan sementara program utama untuk menangani 
  event prioritas tinggi"
- Ilustrasi analogi: seseorang sedang bekerja di meja, telepon berdering, 
  dia mengangkat telepon, lalu kembali bekerja
- Poin-poin: "Responsif", "Efisien", "Real-time capable"

Gunakan ikon lightning bolt untuk menggambarkan event interrupt.
```

---

## Slide 5: Polling vs Interrupt

**Prompt:**
```
Buat slide perbandingan "Polling vs Interrupt" dengan layout dua kolom:

Kolom Kiri - POLLING:
- Ilustrasi: loop arrow checking status repeatedly
- CPU terus-menerus memeriksa
- Membuang cycle CPU
- Simple to implement
- ❌ Tidak efisien

Kolom Tengah: VS dengan panah perbandingan

Kolom Kanan - INTERRUPT:
- Ilustrasi: bell notification icon
- CPU diberitahu saat event terjadi
- CPU bebas melakukan hal lain
- More complex
- ✓ Sangat efisien

Tambahkan timeline diagram di bawah menunjukkan CPU usage comparison.
```

---

## Slide 6: Alur Proses Interrupt

**Prompt:**
```
Buat slide "Alur Proses Interrupt" dengan flowchart vertikal:

1. [Normal Execution] - Program utama berjalan
   ↓ ⚡ Interrupt Signal
2. [Save Context] - CPU menyimpan state (PC, registers)
   ↓
3. [Vector Table] - CPU mencari alamat ISR
   ↓
4. [Execute ISR] - Interrupt Service Routine dijalankan
   ↓
5. [Clear Flag] - Flag interrupt dibersihkan
   ↓
6. [Restore Context] - CPU mengembalikan state
   ↓
7. [Resume] - Program utama dilanjutkan

Gunakan warna hijau untuk normal flow, merah untuk interrupt event.
```

---

## Slide 7: Jenis-jenis Interrupt

**Prompt:**
```
Buat slide "Jenis-jenis Interrupt" dengan kategori:

1. BERDASARKAN SUMBER:
   - External Interrupt: dari pin GPIO (tombol, sensor)
   - Internal Interrupt: dari peripheral (timer, ADC, UART)
   - Software Interrupt: dari instruksi program

2. BERDASARKAN PRIORITY:
   - Non-Maskable (NMI): tidak bisa di-disable
   - Maskable: bisa di-enable/disable

3. BERDASARKAN TRIGGER:
   - Edge-triggered: Rising, Falling, Both
   - Level-triggered: High, Low

Ilustrasi dengan sinyal waveform untuk setiap trigger type.
```

---

## Slide 8: NVIC pada ARM Cortex-M

**Prompt:**
```
Buat slide "NVIC - Nested Vectored Interrupt Controller" dengan diagram blok:

┌───────────────────────────────────────┐
│           NVIC Architecture            │
├───────────────────────────────────────┤
│                                       │
│  [External Sources] ──┐               │
│  [Peripheral IRQ] ────┼──► [NVIC] ──► [Cortex-M Core]
│  [Software IRQ] ──────┘     │              │
│                             │              │
│                      ┌──────┴──────┐       │
│                      │ • Priority  │       │
│                      │ • Pending   │       │
│                      │ • Enable    │       │
│                      └─────────────┘       │
└───────────────────────────────────────────┘

Fitur NVIC:
• Nested: ISR prioritas tinggi bisa interrupt ISR prioritas rendah
• Vectored: Setiap interrupt punya alamat handler sendiri
• Low Latency: 12 clock cycles entry time
```

---

## Slide 9: External Interrupt STM32 (EXTI)

**Prompt:**
```
Buat slide "External Interrupt STM32 (EXTI)" dengan:

1. Diagram EXTI Lines:
   - 16 EXTI lines (EXTI0-EXTI15)
   - Mapping dari GPIO: PA0→EXTI0, PB0→EXTI0, etc
   - Catatan: Hanya satu pin per nomor yang bisa aktif

2. Trigger Modes (dengan waveform):
   - Rising Edge: LOW → HIGH transition
   - Falling Edge: HIGH → LOW transition
   - Both Edges: Both transitions

3. Block diagram EXTI:
   GPIO Pin → Edge Detector → Pending Register → NVIC → CPU

Warna: gunakan warna berbeda untuk setiap GPIO port.
```

---

## Slide 10: Konfigurasi EXTI STM32 (HAL)

**Prompt:**
```
Buat slide "Konfigurasi EXTI STM32 dengan HAL" dengan code dan diagram:

Langkah Konfigurasi menggunakan STM32Cube HAL:
┌────────────────────────────────────────────────────┐
│ 1. CubeMX: Konfigurasi pin sebagai GPIO_EXTI       │
│    Pilih PA0 → GPIO_EXTI0, Trigger = Falling Edge  │
├────────────────────────────────────────────────────┤
│ 2. HAL Init (auto-generated):                      │
│    GPIO_InitTypeDef GPIO_InitStruct = {0};          │
│    GPIO_InitStruct.Pin = GPIO_PIN_0;               │
│    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;    │
│    GPIO_InitStruct.Pull = GPIO_PULLUP;             │
│    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);         │
├────────────────────────────────────────────────────┤
│ 3. Enable NVIC:                                    │
│    HAL_NVIC_SetPriority(EXTI0_IRQn, 0, 0);        │
│    HAL_NVIC_EnableIRQ(EXTI0_IRQn);                 │
├────────────────────────────────────────────────────┤
│ 4. Callback (user code):                           │
│    void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin){ │
│        if (GPIO_Pin == GPIO_PIN_0) {               │
│            button_flag = 1;                        │
│        }                                           │
│    }                                               │
└────────────────────────────────────────────────────┘

Highlight: HAL menangani clear flag secara otomatis!
```

---

## Slide 11: Interrupt Handler STM32 (HAL)

**Prompt:**
```
Buat slide "Interrupt Handler STM32 (HAL)" dengan code box:

/* stm32f1xx_it.c - auto-generated */
void EXTI0_IRQHandler(void) {
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_0);
}

/* main.c - user callback */
volatile uint8_t button_flag = 0;
volatile uint32_t press_count = 0;

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    if (GPIO_Pin == GPIO_PIN_0) {
        button_flag = 1;
        press_count++;
    }
}

int main(void) {
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    
    while (1) {
        if (button_flag) {
            button_flag = 0;
            HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_3);
        }
    }
}

PENTING:
⚠️ HAL_GPIO_EXTI_IRQHandler() otomatis clear pending flag
⚠️ ISR harus singkat (< 1ms)
⚠️ Gunakan volatile untuk shared variables
```

---

## Slide 12: Interrupt ESP32 Architecture

**Prompt:**
```
Buat slide "Interrupt ESP32 Architecture" dengan diagram:

┌─────────────────────────────────────────────────────┐
│              ESP32 Interrupt System                  │
├─────────────────────────────────────────────────────┤
│                                                     │
│   ┌───────────┐         ┌───────────┐              │
│   │ PRO_CPU   │         │ APP_CPU   │              │
│   │ (Core 0)  │         │ (Core 1)  │              │
│   │ 32 slots  │         │ 32 slots  │              │
│   └─────┬─────┘         └─────┬─────┘              │
│         │                     │                     │
│         └────────┬────────────┘                     │
│                  │                                  │
│         ┌────────▼────────┐                         │
│         │ Interrupt Matrix│ (Crossbar Switch)       │
│         └────────┬────────┘                         │
│                  │                                  │
│    ┌─────────────┼─────────────┐                    │
│   GPIO         Timer         UART    ...            │
│   (71 interrupt sources total)                      │
└─────────────────────────────────────────────────────┘

Highlight: Dual-core = interrupt bisa di-route ke core manapun
```

---

## Slide 13: GPIO Interrupt ESP32 (ESP-IDF)

**Prompt:**
```
Buat slide "GPIO Interrupt ESP32 (ESP-IDF)" dengan:

1. Available Pins:
   ✓ GPIO 0-33: Support interrupt
   ✗ GPIO 34-39: Input-only, TIDAK bisa output

2. Interrupt Modes (dengan waveform):
   - GPIO_INTR_POSEDGE: Rising Edge
   - GPIO_INTR_NEGEDGE: Falling Edge
   - GPIO_INTR_ANYEDGE: Both Edges
   - GPIO_INTR_LOW_LEVEL: Level LOW
   - GPIO_INTR_HIGH_LEVEL: Level HIGH

3. Code snippet ESP-IDF:
   gpio_config_t io_conf = {
       .pin_bit_mask = (1ULL << GPIO_NUM_4),
       .mode = GPIO_MODE_INPUT,
       .pull_up_en = GPIO_PULLUP_ENABLE,
       .intr_type = GPIO_INTR_NEGEDGE,
   };
   gpio_config(&io_conf);
   gpio_install_isr_service(0);
   gpio_isr_handler_add(GPIO_NUM_4, button_isr, NULL);

Catatan: ESP-IDF menggunakan gpio_config() bukan pinMode()
```

---

## Slide 14: IRAM_ATTR pada ESP32

**Prompt:**
```
Buat slide "IRAM_ATTR - Mengapa Penting?" dengan:

Diagram memory:
┌─────────────────┐
│   IRAM (Fast)   │ ◄── ISR dengan IRAM_ATTR
│   Internal RAM  │     Access: ~1 clock cycle
├─────────────────┤
│   DRAM (Data)   │     Variables
├─────────────────┤
│   Flash (Slow)  │ ◄── Normal code
│   External SPI  │     Access: Variable (Cache)
└─────────────────┘

TANPA IRAM_ATTR:
❌ ISR di Flash → Cache miss → Unpredictable delay → CRASH!

DENGAN IRAM_ATTR:
✓ ISR di IRAM → Selalu tersedia → Consistent fast access

Code ESP-IDF:
static void IRAM_ATTR button_isr_handler(void *arg) {
    uint32_t gpio_num = (uint32_t)arg;
    // Set flag atau kirim ke queue
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

⚠️ IRAM terbatas (~200KB) - gunakan bijak!
```

---

## Slide 15: Hardware Timer - Konsep Dasar

**Prompt:**
```
Buat slide "Hardware Timer - Konsep Dasar" dengan:

Definisi: "Timer adalah peripheral yang menghitung clock cycles untuk 
menghasilkan event berbasis waktu yang presisi"

Block Diagram Timer:
                    
Clock Source ──► Prescaler ──► Counter ──► Compare/Capture
   (APB)          (÷N)        (CNT)         (CCR)
                                │
                                ▼
                          Auto-Reload
                            (ARR)

Kegunaan:
• ⏱️ Delay presisi (tanpa blocking)
• 📊 Pengukuran waktu/frekuensi
• 🔄 Generasi event periodik
• 🎵 PWM generation
• 📥 Input capture (measure pulse width)
```

---

## Slide 16: Timer STM32F103

**Prompt:**
```
Buat slide "Timer pada STM32F103" dengan tabel:

┌─────────┬──────────────┬──────────┬─────────┬─────────────────┐
│ Timer   │ Tipe         │ Resolusi │ Channel │ Fitur Khusus    │
├─────────┼──────────────┼──────────┼─────────┼─────────────────┤
│ TIM1    │ Advanced     │ 16-bit   │ 4       │ PWM, Dead-time  │
│ TIM2    │ General      │ 16-bit   │ 4       │ Encoder, IC/OC  │
│ TIM3    │ General      │ 16-bit   │ 4       │ Encoder, IC/OC  │
│ TIM4    │ General      │ 16-bit   │ 4       │ Encoder, IC/OC  │
│ SysTick │ System       │ 24-bit   │ -       │ RTOS tick       │
└─────────┴──────────────┴──────────┴─────────┴─────────────────┘

Clock Tree Diagram:
APB1 (36MHz max) → TIM2, TIM3, TIM4
APB2 (72MHz max) → TIM1, SysTick
```

---

## Slide 17: Perhitungan Timer STM32

**Prompt:**
```
Buat slide "Perhitungan Timer STM32" dengan formula dan contoh:

┌────────────────────────────────────────────────────┐
│ RUMUS DASAR                                        │
├────────────────────────────────────────────────────┤
│                                                    │
│ Timer_Freq = APB_Clock / (PSC + 1)                 │
│                                                    │
│ Period = (ARR + 1) / Timer_Freq                    │
│                                                    │
│ Update_Freq = APB_Clock / ((PSC+1) × (ARR+1))     │
│                                                    │
└────────────────────────────────────────────────────┘

CONTOH: Membuat timer 1 Hz (1 detik) dengan clock 72 MHz

Langkah 1: 72,000,000 = (PSC+1) × (ARR+1)

Langkah 2: Pilih PSC = 7199
           Timer_Freq = 72MHz / 7200 = 10 kHz

Langkah 3: ARR = 10kHz / 1Hz - 1 = 9999

Verifikasi: 72MHz / (7200 × 10000) = 1 Hz ✓

Tampilkan dengan calculator-style box dan step-by-step.
```

---

## Slide 18: Konfigurasi Timer STM32 (HAL)

**Prompt:**
```
Buat slide "Konfigurasi Timer STM32 dengan HAL" dengan code dan diagram:

/* Konfigurasi TIM2 via CubeMX / HAL_TIM_Base_Init */
TIM_HandleTypeDef htim2;

void MX_TIM2_Init(void) {
    htim2.Instance = TIM2;
    htim2.Init.Prescaler = 7200 - 1;      // 72MHz/7200 = 10kHz
    htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim2.Init.Period = 10000 - 1;         // 10kHz/10000 = 1Hz
    htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    HAL_TIM_Base_Init(&htim2);
}

/* Start timer dengan interrupt */
HAL_TIM_Base_Start_IT(&htim2);

/* Callback (user code) */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    if (htim->Instance == TIM2) {
        HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_3);
    }
}

Diagram timing: PSC counting → ARR counting → Update event.
Catatan: HAL menangani NVIC enable dan clear flag secara otomatis.
```

---

## Slide 19: Timer ESP32 (ESP-IDF gptimer)

**Prompt:**
```
Buat slide "Timer pada ESP32 (ESP-IDF gptimer API)" dengan:

Fitur Hardware Timer ESP32:
┌────────────────────────────────────────┐
│ • 4 Hardware Timers (64-bit!)          │
│ • Timer Group 0: Timer 0, Timer 1      │
│ • Timer Group 1: Timer 0, Timer 1      │
│ • Base clock: 80 MHz (APB)             │
│ • Resolusi via gptimer: configurable    │
│ • Auto-reload support                  │
└────────────────────────────────────────┘

Perbandingan dengan STM32:
┌─────────────────┬───────────────┬───────────────┐
│ Aspek           │ STM32 Timer   │ ESP32 Timer   │
├─────────────────┼───────────────┼───────────────┤
│ Resolusi        │ 16-bit        │ 64-bit        │
│ Max Count       │ 65,535        │ 18 quintillion│
│ Jumlah          │ 4             │ 4             │
│ API Framework   │ HAL_TIM_*     │ gptimer_*     │
│ PWM Channels    │ 4 per timer   │ Separate LEDC │
└─────────────────┴───────────────┴───────────────┘
```

---

## Slide 20: Konfigurasi Timer ESP32 (ESP-IDF)

**Prompt:**
```
Buat slide "Konfigurasi Timer ESP32 dengan ESP-IDF gptimer" dengan code:

#include "driver/gptimer.h"

gptimer_handle_t gptimer = NULL;

/* Callback function */
static bool IRAM_ATTR timer_alarm_cb(
    gptimer_handle_t timer,
    const gptimer_alarm_event_data_t *edata,
    void *user_ctx)
{
    // Toggle LED atau set flag
    gpio_set_level(LED_PIN, !gpio_get_level(LED_PIN));
    return true;  // need yield?
}

/* Konfigurasi */
gptimer_config_t timer_config = {
    .clk_src = GPTIMER_CLK_SRC_DEFAULT,
    .direction = GPTIMER_COUNT_UP,
    .resolution_hz = 1000000,  // 1MHz = 1µs per tick
};
gptimer_new_timer(&timer_config, &gptimer);

gptimer_alarm_config_t alarm_config = {
    .alarm_count = 1000000,    // 1 detik
    .reload_count = 0,
    .flags.auto_reload_on_alarm = true,
};
gptimer_set_alarm_action(gptimer, &alarm_config);

gptimer_event_callbacks_t cbs = { .on_alarm = timer_alarm_cb };
gptimer_register_event_callbacks(gptimer, &cbs, NULL);
gptimer_enable(gptimer);
gptimer_start(gptimer);

Catatan: gptimer API menggantikan timer_group API lama
```

---

## Slide 21: Critical Section ESP32

**Prompt:**
```
Buat slide "Critical Section pada ESP32" dengan:

MASALAH - Race Condition:
┌──────────────────────────────────────────────────┐
│ ISR:           counter++;  // Read-Modify-Write  │
│ Main:          value = counter;                  │
│                                                  │
│ ⚡ Interrupt terjadi di tengah operasi = BUG!   │
└──────────────────────────────────────────────────┘

SOLUSI - Critical Section (ESP-IDF):
┌──────────────────────────────────────────────────┐
│ portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED; │
│                                                  │
│ // Di ISR:                                       │
│ portENTER_CRITICAL_ISR(&mux);                    │
│ counter++;                                       │
│ portEXIT_CRITICAL_ISR(&mux);                     │
│                                                  │
│ // Di main loop:                                 │
│ portENTER_CRITICAL(&mux);                        │
│ localValue = counter;                            │
│ portEXIT_CRITICAL(&mux);                         │
└──────────────────────────────────────────────────┘

⚠️ Spinlock mencegah interrupt saat critical section
```

---

## Slide 22: Best Practices ISR

**Prompt:**
```
Buat slide "Best Practices ISR" dengan DO dan DON'T list:

✅ DO:
• Keep ISR short (< 100µs)
• Use volatile for shared variables
• Set flag, process in main loop
• Clear interrupt flags (HAL does it automatically)
• Use IRAM_ATTR on ESP32
• Use critical sections for shared data

❌ DON'T:
• Don't use HAL_Delay() / vTaskDelay() in ISR
• Don't use printf() / ESP_LOGI() in ISR
• Don't do complex calculations
• Don't allocate memory (malloc)
• Don't call non-reentrant functions
• Don't forget volatile keyword

Code Example (ESP-IDF):
volatile bool flag = false;

static void IRAM_ATTR my_isr(void *arg) {
    flag = true;  // ✅ Simple!
}

void app_main(void) {
    while (1) {
        if (flag) {
            flag = false;
            ESP_LOGI(TAG, "Event!");  // ✅ In main
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
```

---

## Slide 23: Debouncing dengan Timer

**Prompt:**
```
Buat slide "Debouncing dengan Timer" dengan:

MASALAH - Button Bounce:
[Waveform showing bouncing signal with multiple transitions in ~20ms]

TANPA DEBOUNCE:
"1 tekan = 5-10 interrupt!"

SOLUSI - Timer-based Debounce (ESP-IDF):
┌────────────────────────────────────────────────────┐
│ static int64_t last_time = 0;                      │
│ #define DEBOUNCE_US 50000  // 50ms                 │
│                                                    │
│ static void IRAM_ATTR button_isr(void *arg) {      │
│     int64_t now = esp_timer_get_time();            │
│     if (now - last_time > DEBOUNCE_US) {           │
│         button_pressed = true;                     │
│         last_time = now;                           │
│     }                                              │
│ }                                                  │
└────────────────────────────────────────────────────┘

Timing diagram: Button press → Debounce window → Valid detection
```

---

## Slide 24: Watchdog Timer

**Prompt:**
```
Buat slide "Watchdog Timer" dengan:

Definisi: "Watchdog = timer khusus yang mereset MCU jika program hang"

Cara Kerja:
┌────────────────────────────────────────────────────┐
│ 1. Watchdog dimulai dengan timeout tertentu         │
│ 2. Program harus "feed/refresh" sebelum timeout    │
│ 3. Jika program hang → WDT timeout → RESET!       │
└────────────────────────────────────────────────────┘

STM32 - Dua Jenis Watchdog:
┌──────────────┬─────────────────────────────────────┐
│ IWDG         │ Independent WDT, clock LSI 40kHz    │
│              │ Selalu running, tidak bisa dihentikan│
├──────────────┼─────────────────────────────────────┤
│ WWDG         │ Window WDT, clock APB1              │
│              │ Refresh hanya di window tertentu     │
└──────────────┴─────────────────────────────────────┘

ESP32 - Task Watchdog:
┌──────────────────────────────────────────────────┐
│ esp_task_wdt: monitor per-task                   │
│ Bisa add/remove task dari watchdog              │
│ esp_task_wdt_reset() untuk refresh              │
└──────────────────────────────────────────────────┘

Diagram: Normal operation vs Hang → Reset
```

---

## Slide 25: Watchdog Timer - Code

**Prompt:**
```
Buat slide "Watchdog Timer - Implementasi" dengan code:

STM32 HAL - IWDG:
┌────────────────────────────────────────────────────┐
│ IWDG_HandleTypeDef hiwdg;                          │
│                                                    │
│ hiwdg.Instance = IWDG;                             │
│ hiwdg.Init.Prescaler = IWDG_PRESCALER_64;         │
│ hiwdg.Init.Reload = 625;  // ~1 detik timeout     │
│ HAL_IWDG_Init(&hiwdg);                            │
│                                                    │
│ while (1) {                                        │
│     // ... proses normal ...                       │
│     HAL_IWDG_Refresh(&hiwdg);  // Feed watchdog   │
│ }                                                  │
└────────────────────────────────────────────────────┘

ESP32 ESP-IDF - Task WDT:
┌────────────────────────────────────────────────────┐
│ #include "esp_task_wdt.h"                          │
│                                                    │
│ esp_task_wdt_config_t wdt_cfg = {                  │
│     .timeout_ms = 5000,                            │
│     .trigger_panic = true,                         │
│ };                                                 │
│ esp_task_wdt_reconfigure(&wdt_cfg);                │
│ esp_task_wdt_add(NULL);  // Add current task       │
│                                                    │
│ while (1) {                                        │
│     // ... proses normal ...                       │
│     esp_task_wdt_reset();  // Feed watchdog        │
│     vTaskDelay(pdMS_TO_TICKS(100));                │
│ }                                                  │
└────────────────────────────────────────────────────┘
```

---

## Slide 26: Timer Cascade (Master-Slave)

**Prompt:**
```
Buat slide "Timer Cascade - Master-Slave" dengan:

Konsep: Menghubungkan 2 timer untuk mendapatkan resolusi lebih tinggi

STM32 - Hardware Timer Cascade:
┌───────────────────────────────────────────────────┐
│ TIM2 (Master)         TIM3 (Slave)                │
│ ┌─────────┐          ┌─────────┐                  │
│ │ PSC=71  │ TRGO     │ ITR1    │                  │
│ │ ARR=999 │─────────►│ Slave   │                  │
│ │ 1kHz    │ Update   │ Mode    │                  │
│ └─────────┘          └─────────┘                  │
│                                                   │
│ Efektif: TIM2 overflow → TIM3 increment           │
│ Total: 16-bit × 16-bit = 32-bit counter!         │
└───────────────────────────────────────────────────┘

HAL Configuration:
• Master: TIM_MasterConfigTypeDef, TriggerOutput = UPDATE
• Slave: TIM_SlaveConfigTypeDef, SlaveMode = EXTERNAL_CLOCK

ESP32 - Software Chaining:
┌───────────────────────────────────────────────────┐
│ gptimer 0 overflow → callback increments counter  │
│ Counter overflow → trigger gptimer 1 action       │
│ Software chaining (ESP32 timer sudah 64-bit)      │
└───────────────────────────────────────────────────┘
```

---

## Slide 27: Perbandingan STM32 vs ESP32

**Prompt:**
```
Buat slide "Perbandingan Interrupt & Timer: STM32 vs ESP32" dengan tabel:

┌─────────────────────┬──────────────────────┬──────────────────────┐
│ Aspek               │ STM32F103 (HAL)      │ ESP32 (ESP-IDF)      │
├─────────────────────┼──────────────────────┼──────────────────────┤
│ Core                │ ARM Cortex-M3        │ Xtensa LX6 Dual      │
│ Interrupt Ctrl      │ NVIC                 │ Interrupt Matrix     │
│ Priority Levels     │ 16 (4-bit)           │ 7 levels             │
│ External Int        │ 16 EXTI lines        │ All GPIO pins        │
│ Timer Count         │ 4 (16-bit)           │ 4 (64-bit)           │
│ Timer API           │ HAL_TIM_*            │ gptimer_*            │
│ ISR Attribute       │ None                 │ IRAM_ATTR required   │
│ GPIO Config         │ HAL_GPIO_Init()      │ gpio_config()        │
│ ISR Install         │ HAL_NVIC_EnableIRQ() │ gpio_install_isr_svc │
│ Watchdog            │ IWDG / WWDG          │ esp_task_wdt         │
│ Timer Cascade       │ Hardware Master-Slave│ Software chaining    │
│ Latency             │ 12 cycles            │ ~20-50 cycles        │
└─────────────────────┴──────────────────────┴──────────────────────┘
```

---

## Slide 28: Latihan Perhitungan

**Prompt:**
```
Buat slide "Latihan Perhitungan Timer" dengan soal:

SOAL 1: STM32 Timer (HAL)
Buatlah timer yang menghasilkan interrupt setiap 250ms
Clock APB1 = 72 MHz

Jawab:
Prescaler = _____, Period (ARR) = _____
(Hint: 250ms = 4 Hz)

SOAL 2: ESP32 Timer (gptimer)
Buatlah gptimer untuk LED blink dengan periode 500ms
Resolution = 1MHz (1µs per tick)

Jawab:
alarm_count = _____ ticks
(Hint: 500ms = 500,000 µs)

SOAL 3: STM32 IWDG
LSI clock = 40kHz, Prescaler = 64
Berapa nilai Reload untuk timeout 2 detik?

Jawab: Reload = _____
(Hint: 40kHz / 64 = 625 Hz)

Format: kotak soal dengan tempat jawaban kosong.
```

---

## Slide 29: Jawaban Latihan

**Prompt:**
```
Buat slide "Jawaban Latihan" dengan solusi:

SOAL 1 - Jawaban:
4 Hz = 72MHz / ((PSC+1) × (ARR+1))
(PSC+1) × (ARR+1) = 18,000,000

Pilihan: PSC = 7199, ARR = 2499
Check: 72MHz / (7200 × 2500) = 4 Hz ✓

SOAL 2 - Jawaban:
Resolution 1MHz → 1µs per tick
500ms = 500,000 µs
alarm_count = 500000

SOAL 3 - Jawaban:
IWDG clock = 40kHz / 64 = 625 Hz
Timeout 2 detik = 2 × 625 = 1250
Reload = 1250

Gunakan checkmark hijau untuk jawaban benar.
```

---

## Slide 30: Summary

**Prompt:**
```
Buat slide "Summary" dengan ringkasan dalam format infographic:

┌────────────────────────────────────────────────────────┐
│                    KEY TAKEAWAYS                        │
├────────────────────────────────────────────────────────┤
│                                                        │
│ 1️⃣ INTERRUPT vs POLLING                               │
│    Interrupt = Efisien, Real-time                     │
│    Polling = Simple, CPU-intensive                    │
│                                                        │
│ 2️⃣ STM32 INTERRUPT (HAL)                              │
│    NVIC + EXTI → HAL_GPIO_EXTI_Callback()            │
│                                                        │
│ 3️⃣ ESP32 INTERRUPT (ESP-IDF)                          │
│    gpio_config() + gpio_isr_handler_add()            │
│    IRAM_ATTR = WAJIB                                 │
│                                                        │
│ 4️⃣ TIMER                                              │
│    STM32: HAL_TIM_Base_Start_IT()                    │
│    ESP32: gptimer_new_timer() + gptimer_start()      │
│                                                        │
│ 5️⃣ WATCHDOG & CASCADE                                 │
│    Safety reset + Extended counting                   │
│                                                        │
│ 6️⃣ BEST PRACTICE                                      │
│    Short ISR + Flag + Process in main loop           │
│                                                        │
└────────────────────────────────────────────────────────┘
```

---

## Slide 31: Referensi

**Prompt:**
```
Buat slide "Referensi" dengan daftar:

📚 Dokumentasi Resmi:
1. STM32F103 Reference Manual (RM0008) - ST
2. ARM Cortex-M3 Technical Reference - ARM
3. ESP32 Technical Reference Manual - Espressif
4. ESP-IDF Programming Guide (gptimer, GPIO ISR)

📖 Buku:
1. "Mastering STM32" - Carmine Noviello (Ch7: Interrupts, Ch11: Timers)
2. "Kolban's Book on ESP32" (p267-268: ISR, p300-302: Timers)

🔗 Online Resources:
1. STM32Cube HAL Documentation
2. ESP-IDF Official Examples (gpio, gptimer, wdt)
3. Espressif GitHub - esp-idf examples

Format dengan ikon buku, dokumen, dan link untuk setiap kategori.
```

---

## Catatan Penggunaan

1. **Setiap prompt dapat digunakan untuk:**
   - AI image generator (DALL-E, Midjourney, Stable Diffusion)
   - Manual design di PowerPoint/Canva
   - Referensi konten untuk presentasi

2. **Framework yang digunakan:**
   - STM32: STM32Cube HAL (BUKAN Arduino/HardwareTimer)
   - ESP32: ESP-IDF native (BUKAN Arduino framework)
   - Semua code menggunakan file .c (bukan .cpp)

3. **Konsistensi Visual:**
   - Maintain color scheme di semua slide
   - Gunakan font yang sama
   - Ukuran diagram proporsional

4. **Interaktivitas:**
   - Slide latihan bisa dijadikan quiz
   - Timing diagram bisa dianimasikan
   - Code snippet highlight satu per satu
