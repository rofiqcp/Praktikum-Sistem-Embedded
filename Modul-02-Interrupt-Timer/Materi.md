# Modul 02: Interrupt dan Timer


## Daftar Isi

## Capaian Pembelajaran

Setelah menyelesaikan bab ini, mahasiswa diharapkan mampu:

1. Memahami konsep dasar Interrupt pada arsitektur ARM Cortex-M dan Xtensa
2. Menguasai mekanisme NVIC pada STM32 dan sistem interrupt ESP32
3. Memahami dan mengimplementasikan Timer/Counter pada kedua platform
4. Mengimplementasikan Watchdog Timer (IWDG/WWDG dan esp_task_wdt)
5. Menerapkan teknik Timer Cascade untuk chaining multi-timer
6. Mengembangkan aplikasi berbasis interrupt-driven untuk sistem real-time
7. Menerapkan teknik debouncing berbasis interrupt dan timer
8. Mengoptimalkan penggunaan sumber daya dengan interrupt dan timer

---


## 1. Pendahuluan Interrupt

### 1.1 Definisi dan Konsep Dasar

**Interrupt** adalah mekanisme hardware yang memungkinkan processor menghentikan sementara eksekusi program utama untuk menangani event penting yang memerlukan perhatian segera. Setelah interrupt ditangani, processor melanjutkan eksekusi program utama dari titik terakhir.

**Mengapa Interrupt Penting?**
- **Responsivitas**: Sistem dapat merespons event eksternal dengan segera
- **Efisiensi**: Tidak perlu polling terus-menerus (CPU-intensive)
- **Determinisme**: Waktu respons yang dapat diprediksi
- **Real-time**: Mendukung aplikasi yang time-critical

### 1.2 Perbandingan: Polling vs Interrupt

| Aspek | Polling | Interrupt |
|-------|---------|-----------|
| **Mekanisme** | CPU terus memeriksa status | CPU diberitahu saat event terjadi |
| **Efisiensi CPU** | Rendah (CPU sibuk memeriksa) | Tinggi (CPU bebas melakukan hal lain) |
| **Waktu Respons** | Tergantung frekuensi polling | Sangat cepat (mikrodetik) |
| **Kompleksitas** | Sederhana | Lebih kompleks |
| **Power Consumption** | Tinggi | Rendah |
| **Cocok Untuk** | Sistem sederhana | Sistem real-time |

```
Polling (STM32 HAL):
┌─────────────────────────────────────────────────────┐
│ while(1) {                                          │
│   if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == 0) {   │
│     handle_button();                                │
│   }                                                 │
│   // CPU tidak bisa mengerjakan hal lain efektif    │
│ }                                                   │
└─────────────────────────────────────────────────────┘

Interrupt (STM32 HAL):
┌─────────────────────────────────────────────────────┐
│ // Callback dipanggil otomatis saat tombol ditekan  │
│ void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {    │
│   handle_button();                                  │
│ }                                                   │
│                                                     │
│ // Main program bebas melakukan hal lain            │
│ while(1) {                                          │
│   do_other_tasks();  // Efisien!                    │
│ }                                                   │
└─────────────────────────────────────────────────────┘
```

---

## 2. Arsitektur Interrupt STM32F103C8T6

### 2.1 Nested Vectored Interrupt Controller (NVIC)

STM32F103 menggunakan **NVIC** yang merupakan bagian integral dari core ARM Cortex-M3. NVIC mengelola semua interrupt dengan fitur:

- **Nested Interrupts**: Interrupt prioritas tinggi dapat meng-interrupt handler prioritas rendah
- **Vectored**: Setiap interrupt memiliki alamat handler sendiri
- **Programmable Priority**: 16 level prioritas (4 bit)
- **Low Latency**: 12 clock cycles untuk masuk ke handler

```
┌─────────────────────────────────────────────────────────────┐
│                    NVIC Architecture                         │
├─────────────────────────────────────────────────────────────┤
│  ┌──────────┐    ┌──────────────┐    ┌──────────────────┐   │
│  │ External │───▶│   NVIC       │───▶│ Cortex-M3 Core   │   │
│  │ Events   │    │ • Priority   │    │ • Save Context   │   │
│  └──────────┘    │ • Pending    │    │ • Jump to ISR    │   │
│                  │ • Enable/    │    │ • Restore        │   │
│  ┌──────────┐    │   Disable    │    └──────────────────┘   │
│  │Peripheral│───▶│              │                          │
│  │Interrupts│    └──────────────┘                          │
│  └──────────┘                                               │
└─────────────────────────────────────────────────────────────┘
```

### 2.2 External Interrupt (EXTI) pada STM32

STM32F103 memiliki 16 jalur EXTI (EXTI0-EXTI15) yang dapat dikonfigurasi untuk mendeteksi:
- **Rising Edge**: Transisi LOW → HIGH
- **Falling Edge**: Transisi HIGH → LOW
- **Both Edges**: Kedua transisi

**Mapping GPIO ke EXTI:**
```
GPIO Pin      EXTI Line
────────────────────────
PA0, PB0, PC0  → EXTI0
PA1, PB1, PC1  → EXTI1
PA2, PB2, PC2  → EXTI2
...
PA15, PB15, PC15 → EXTI15
```

**Catatan Penting**: Hanya SATU pin dengan nomor yang sama dapat aktif di EXTI. Contoh: PA0 dan PB0 tidak bisa keduanya menggunakan EXTI0 bersamaan.

### 2.3 Konfigurasi EXTI STM32 dengan HAL

```c
// Konfigurasi GPIO sebagai EXTI input menggunakan STM32Cube HAL
static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    __HAL_RCC_GPIOA_CLK_ENABLE();

    // PA0 sebagai EXTI0 input, falling edge, pull-up
    GPIO_InitStruct.Pin = GPIO_PIN_0;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    // Enable dan set prioritas EXTI0 di NVIC
    HAL_NVIC_SetPriority(EXTI0_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(EXTI0_IRQn);
}
```

### 2.4 Interrupt Handler STM32 (HAL)

```c
// IRQ vector — forward ke HAL handler
void EXTI0_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_0);
}

// HAL callback — dipanggil setelah flag di-clear otomatis
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == GPIO_PIN_0) {
        button_pressed_flag = 1;
    }
}
```

### 2.5 Register Level Reference

```c
// Register-level EXTI setup (untuk pemahaman mendalam)
RCC->APB2ENR |= RCC_APB2ENR_AFIOEN | RCC_APB2ENR_IOPAEN;
GPIOA->CRL &= ~(GPIO_CRL_MODE0 | GPIO_CRL_CNF0);
GPIOA->CRL |= GPIO_CRL_CNF0_1;
GPIOA->ODR |= GPIO_ODR_ODR0;
AFIO->EXTICR[0] &= ~AFIO_EXTICR1_EXTI0;
EXTI->FTSR |= EXTI_FTSR_TR0;
EXTI->IMR |= EXTI_IMR_MR0;
NVIC_EnableIRQ(EXTI0_IRQn);
```

---

## 3. Arsitektur Interrupt ESP32

### 3.1 Dual-Core Interrupt System

ESP32 memiliki arsitektur dual-core (PRO_CPU dan APP_CPU) dengan sistem interrupt yang lebih fleksibel:

```
┌─────────────────────────────────────────────────────────────┐
│                 ESP32 Interrupt Matrix                       │
├─────────────────────────────────────────────────────────────┤
│  ┌──────────────┐         ┌──────────────┐                  │
│  │  PRO_CPU     │         │  APP_CPU     │                  │
│  │  (Core 0)    │         │  (Core 1)    │                  │
│  │  32 Slots    │         │  32 Slots    │                  │
│  └──────┬───────┘         └──────┬───────┘                  │
│         └────────┬───────────────┘                          │
│                  │                                          │
│         ┌────────▼────────┐                                 │
│         │ Interrupt Matrix│                                 │
│         │  (Crossbar)     │                                 │
│         └────────┬────────┘                                 │
│    ┌─────────────┼─────────────┐                            │
│  ┌─▼──┐       ┌──▼─┐        ┌──▼─┐                          │
│  │GPIO│       │Timer│       │UART│  ... (71 sources)        │
│  └────┘       └────┘        └────┘                          │
└─────────────────────────────────────────────────────────────┘
```

### 3.2 GPIO Interrupt ESP32 (ESP-IDF)

ESP32 mendukung interrupt pada semua GPIO pin. Konfigurasi via `gpio_config()`:

```c
typedef enum {
    GPIO_INTR_DISABLE = 0,     // Disable
    GPIO_INTR_POSEDGE = 1,     // Rising edge
    GPIO_INTR_NEGEDGE = 2,     // Falling edge
    GPIO_INTR_ANYEDGE = 3,     // Both edges
    GPIO_INTR_LOW_LEVEL = 4,   // Level LOW
    GPIO_INTR_HIGH_LEVEL = 5,  // Level HIGH
} gpio_int_type_t;
```

### 3.3 Implementasi GPIO Interrupt ESP32 (ESP-IDF)

```c
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"

#define BUTTON_GPIO     GPIO_NUM_0
#define LED_GPIO        GPIO_NUM_2

static const char *TAG = "EXTI_DEMO";
static QueueHandle_t gpio_evt_queue = NULL;

// ISR handler — kirim event ke queue (singkat!)
static void IRAM_ATTR gpio_isr_handler(void *arg)
{
    uint32_t gpio_num = (uint32_t)arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

// Task yang memproses event dari ISR
static void gpio_task(void *arg)
{
    uint32_t io_num;
    while (1) {
        if (xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
            ESP_LOGI(TAG, "GPIO[%lu] interrupt, val: %d",
                     io_num, gpio_get_level(io_num));
            static int led_state = 0;
            led_state = !led_state;
            gpio_set_level(LED_GPIO, led_state);
        }
    }
}

void app_main(void)
{
    // Konfigurasi LED output
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << LED_GPIO),
        .mode = GPIO_MODE_OUTPUT,
    };
    gpio_config(&io_conf);

    // Konfigurasi button input dengan interrupt
    io_conf.pin_bit_mask = (1ULL << BUTTON_GPIO);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    io_conf.intr_type = GPIO_INTR_NEGEDGE;
    gpio_config(&io_conf);

    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    gpio_install_isr_service(0);
    gpio_isr_handler_add(BUTTON_GPIO, gpio_isr_handler,
                         (void *)BUTTON_GPIO);

    xTaskCreate(gpio_task, "gpio_task", 2048, NULL, 10, NULL);
}
```

### 3.4 IRAM_ATTR Explained

```c
// IRAM_ATTR sangat PENTING untuk ESP32 ISR!
static void IRAM_ATTR my_isr(void *arg) { /* ... */ }
```

**Mengapa IRAM_ATTR diperlukan?**
1. ESP32 menyimpan kode program di Flash eksternal (SPI Flash)
2. Flash memerlukan waktu akses yang tidak konsisten (cache miss)
3. Saat interrupt terjadi, kode ISR HARUS tersedia segera
4. IRAM_ATTR memaksa kode disimpan di Internal RAM (akses < 1 cycle)
5. Tanpa IRAM_ATTR, sistem bisa crash (guru panic)

---

## 4. Timer pada STM32F103C8T6

### 4.1 Jenis Timer STM32F103

| Timer | Tipe | Resolusi | Channel | Fitur Khusus |
|-------|------|----------|---------|--------------|
| TIM1 | Advanced | 16-bit | 4 | PWM, Dead-time, Break |
| TIM2 | General | 16-bit | 4 | Input Capture, PWM, Encoder |
| TIM3 | General | 16-bit | 4 | Input Capture, PWM, Encoder |
| TIM4 | General | 16-bit | 4 | Input Capture, PWM, Encoder |
| SysTick | System | 24-bit | - | Delay, RTOS tick |

### 4.2 Arsitektur Timer STM32

```
┌─────────────────────────────────────────────────────────────┐
│                    Timer Block Diagram                       │
├─────────────────────────────────────────────────────────────┤
│  APB Clock (72MHz)                                          │
│       │                                                     │
│       ▼                                                     │
│  ┌─────────┐    ┌─────────┐    ┌─────────┐    ┌─────────┐  │
│  │Prescaler│───▶│ Counter │───▶│ Compare │───▶│ Output  │  │
│  │ (PSC)   │    │  (CNT)  │    │ (CCR)   │    │ Channel │  │
│  │ 16-bit  │    │ 16-bit  │    │ 16-bit  │    │         │  │
│  └─────────┘    └────┬────┘    └─────────┘    └─────────┘  │
│                      │                                      │
│                      ▼                                      │
│               ┌─────────────┐                               │
│               │ Auto-Reload │                               │
│               │    (ARR)    │                               │
│               │   16-bit    │                               │
│               └─────────────┘                               │
│                                                             │
│  Timer_Freq = APB_Clock / (PSC + 1)                         │
│  Period = (ARR + 1) / Timer_Freq                            │
│  Overflow_Time = (PSC + 1) × (ARR + 1) / APB_Clock          │
└─────────────────────────────────────────────────────────────┘
```

### 4.3 Perhitungan Timer STM32

**Rumus Dasar:**
```
Timer Clock = APB Clock / (Prescaler + 1)
Update Period = (ARR + 1) / Timer Clock
Update Frequency = Timer Clock / (ARR + 1)
```

**Contoh: Timer 1 Hz (1 detik) dengan clock 72 MHz:**
```
72,000,000 = (PSC+1) × (ARR+1)

Pilihan: PSC = 7199, ARR = 9999
→ Timer = 72MHz / 7200 = 10kHz
→ Period = 10000 / 10kHz = 1 detik ✓
```

### 4.4 Konfigurasi Timer STM32 (HAL)

```c
TIM_HandleTypeDef htim2;

static void MX_TIM2_Init(void)
{
    __HAL_RCC_TIM2_CLK_ENABLE();
    htim2.Instance = TIM2;
    htim2.Init.Prescaler = 7200 - 1;
    htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim2.Init.Period = 1000 - 1;  // 100ms
    htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    HAL_TIM_Base_Init(&htim2);

    HAL_NVIC_SetPriority(TIM2_IRQn, 3, 0);
    HAL_NVIC_EnableIRQ(TIM2_IRQn);
    HAL_TIM_Base_Start_IT(&htim2);
}

void TIM2_IRQHandler(void) { HAL_TIM_IRQHandler(&htim2); }

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM2) {
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
    }
}
```

---

## 5. Timer pada ESP32 (ESP-IDF)

### 5.1 Hardware Timer ESP32

ESP32 memiliki 4 hardware timer 64-bit yang sangat presisi:

| Timer Group | Timer | Resolusi | Counter |
|-------------|-------|----------|---------|
| Group 0 | Timer 0 | 64-bit | Up/Down |
| Group 0 | Timer 1 | 64-bit | Up/Down |
| Group 1 | Timer 0 | 64-bit | Up/Down |
| Group 1 | Timer 1 | 64-bit | Up/Down |

### 5.2 Konfigurasi Timer ESP32 (ESP-IDF GPTimer)

```c
#include "driver/gptimer.h"
#include "esp_log.h"

static const char *TAG = "TIMER";
static gptimer_handle_t gptimer = NULL;

static bool IRAM_ATTR timer_alarm_cb(gptimer_handle_t timer,
    const gptimer_alarm_event_data_t *edata, void *user_ctx)
{
    // Toggle LED atau set flag
    return false;
}

void app_main(void)
{
    gptimer_config_t cfg = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = 1000000, // 1MHz
    };
    gptimer_new_timer(&cfg, &gptimer);

    gptimer_event_callbacks_t cbs = { .on_alarm = timer_alarm_cb };
    gptimer_register_event_callbacks(gptimer, &cbs, NULL);

    gptimer_alarm_config_t alarm = {
        .alarm_count = 1000000,  // 1 detik
        .reload_count = 0,
        .flags.auto_reload_on_alarm = true,
    };
    gptimer_set_alarm_action(gptimer, &alarm);
    gptimer_enable(gptimer);
    gptimer_start(gptimer);
}
```

---

## 6. Watchdog Timer

### 6.1 Konsep Watchdog Timer

Watchdog Timer (WDT) adalah mekanisme keamanan untuk mendeteksi dan memulihkan sistem dari kegagalan software. Prinsipnya:

1. WDT berjalan independen sebagai countdown timer
2. Software harus periodik "kick" (reset) WDT sebelum timeout
3. Jika WDT tidak di-reset → sistem di-reset paksa

```
Normal:   ──┬──────┬──────┬──────┬──  (WDT terus di-kick)
             kick   kick   kick   kick

Hang:     ──┬──────┬──────X
             kick   kick   │← timeout →│ RESET!
                           (software hang)
```

### 6.2 STM32: IWDG dan WWDG

| Fitur | IWDG (Independent) | WWDG (Window) |
|-------|-------------------|---------------|
| Clock | LSI (~40kHz) | APB1 |
| Window | Tidak ada | Ya (upper + lower) |
| Reset | Counter reaches 0 | Outside window |
| Kegunaan | Deteksi hang umum | Deteksi timing error |

**IWDG (HAL):**
```c
IWDG_HandleTypeDef hiwdg;

static void MX_IWDG_Init(void)
{
    hiwdg.Instance = IWDG;
    hiwdg.Init.Prescaler = IWDG_PRESCALER_64;
    hiwdg.Init.Reload = 625;  // ~1s timeout
    HAL_IWDG_Init(&hiwdg);
}

// Kick watchdog periodik di main loop
HAL_IWDG_Refresh(&hiwdg);
```

**WWDG (HAL):**
```c
WWDG_HandleTypeDef hwwdg;

static void MX_WWDG_Init(void)
{
    __HAL_RCC_WWDG_CLK_ENABLE();
    hwwdg.Instance = WWDG;
    hwwdg.Init.Prescaler = WWDG_PRESCALER_8;
    hwwdg.Init.Window = 80;
    hwwdg.Init.Counter = 127;
    hwwdg.Init.EWIMode = WWDG_EWI_ENABLE;
    HAL_WWDG_Init(&hwwdg);
}
```

### 6.3 ESP32: esp_task_wdt (ESP-IDF)

```c
#include "esp_task_wdt.h"

#define WDT_TIMEOUT_S  5

void app_main(void)
{
    esp_task_wdt_config_t wdt_config = {
        .timeout_ms = WDT_TIMEOUT_S * 1000,
        .idle_core_mask = (1 << 0) | (1 << 1),
        .trigger_panic = true,
    };
    esp_task_wdt_reconfigure(&wdt_config);
    esp_task_wdt_add(NULL);

    while (1) {
        do_work();
        esp_task_wdt_reset();  // Kick watchdog
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
```

---

## 7. Timer Cascade (Multi-Timer Chaining)

### 7.1 Konsep Timer Cascade

Timer Cascade menghubungkan output satu timer sebagai clock input timer lain, memperluas range timing melebihi kapasitas timer tunggal:

```
Single Timer (16-bit): Max period = 65535 × prescaler / clock

Cascaded (2 × 16-bit → efektif 32-bit):
┌────────┐  TRGO    ┌────────┐
│ Timer A│─────────▶│ Timer B│──▶ Extended period
│ Master │ overflow │ Slave  │
└────────┘          └────────┘
Max: 65536 × 65536 = 4,294,967,296 counts
```

### 7.2 STM32: Hardware Master-Slave (HAL)

```c
TIM_HandleTypeDef htim2, htim3;

// TIM2 Master: TRGO pada update event
static void MX_TIM2_Master_Init(void)
{
    htim2.Instance = TIM2;
    htim2.Init.Prescaler = 7200 - 1;
    htim2.Init.Period = 10000 - 1;  // 1 detik
    HAL_TIM_Base_Init(&htim2);

    TIM_MasterConfigTypeDef mc = {0};
    mc.MasterOutputTrigger = TIM_TRGO_UPDATE;
    mc.MasterSlaveMode = TIM_MASTERSLAVEMODE_ENABLE;
    HAL_TIMEx_MasterConfigSynchronization(&htim2, &mc);
}

// TIM3 Slave: clock dari TIM2
static void MX_TIM3_Slave_Init(void)
{
    htim3.Instance = TIM3;
    htim3.Init.Prescaler = 0;
    htim3.Init.Period = 60 - 1;  // 60 detik
    HAL_TIM_Base_Init(&htim3);

    TIM_SlaveConfigTypeDef sc = {0};
    sc.SlaveMode = TIM_SLAVEMODE_EXTERNAL1;
    sc.InputTrigger = TIM_TS_ITR1;
    HAL_TIM_SlaveConfigSynchro(&htim3, &sc);

    HAL_TIM_Base_Start_IT(&htim3);
}
```

### 7.3 ESP32: Software Chaining (ESP-IDF)

```c
static volatile uint32_t fast_count = 0;
static volatile uint32_t cascade_minutes = 0;

static bool IRAM_ATTR fast_cb(gptimer_handle_t timer,
    const gptimer_alarm_event_data_t *edata, void *ctx)
{
    fast_count++;
    if (fast_count >= 60) {
        fast_count = 0;
        cascade_minutes++;
    }
    return false;
}
```

---

## 8. Clock Tree Management

Pemahaman Clock Tree sangat penting karena semua peripheral (Timer, UART, SPI, I2C, ADC) bergantung pada clock yang benar. Kesalahan konfigurasi clock menyebabkan timer tidak akurat, baud rate salah, atau ADC tidak berfungsi.

### 8.1 Clock Tree STM32F103

STM32F103 memiliki sistem clock yang fleksibel dengan beberapa sumber:

```
Sumber Clock STM32F103:
┌─────────────────────────────────────────────────────────────┐
│                                                             │
│  HSI (8 MHz)  ──┐                                          │
│  HSE (8 MHz)  ──┼── PLL ──→ SYSCLK (max 72 MHz)           │
│                  │              │                            │
│                  │    ┌─────────┼──────────┐                │
│                  │    ▼         ▼          ▼                │
│                  │  AHB      APB1        APB2               │
│                  │ (72MHz)  (36MHz max)  (72MHz max)        │
│                  │    │        │           │                 │
│  LSE (32.768kHz)─┘   │    ┌───┴───┐   ┌──┴───┐            │
│  LSI (40 kHz)  ──────┘    │TIM2-7 │   │TIM1  │            │
│                            │UART2-5│   │SPI1  │            │
│                            │I2C1-2 │   │USART1│            │
│                            │SPI2-3 │   │ADC1-2│            │
│                            └───────┘   └──────┘            │
└─────────────────────────────────────────────────────────────┘
```

**Konfigurasi Clock STM32 (HAL):**

```c
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    /* Konfigurasi HSE + PLL → 72 MHz */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState       = RCC_HSE_ON;
    RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    RCC_OscInitStruct.PLL.PLLState   = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource  = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL     = RCC_PLL_MUL9;  // 8 MHz × 9 = 72 MHz
    HAL_RCC_OscConfig(&RCC_OscInitStruct);

    /* Konfigurasi Bus Clocks */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;   // AHB  = 72 MHz
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;     // APB1 = 36 MHz
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;     // APB2 = 72 MHz
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2);
}
```

> **Penting:** Timer clock APB1 di-multiply 2× jika APB1 prescaler > 1. Jadi TIM2-TIM7 berjalan di **72 MHz** (bukan 36 MHz). Ini sering menjadi sumber kesalahan perhitungan timer!

### 8.2 Clock Tree ESP32

ESP32 menggunakan sistem clock yang berbeda:

```
Sumber Clock ESP32:
┌──────────────────────────────────────────────────────┐
│                                                      │
│  XTAL (40 MHz)  ──→ PLL (320/480 MHz)               │
│  RC Osc (8 MHz) ──→ APB Clock (80 MHz default)      │
│  XTAL32K (32.768 kHz) ──→ RTC Clock                 │
│  RC150K (150 kHz)      ──→ RTC Slow Clock            │
│                                                      │
│  CPU Clock: 80 / 160 / 240 MHz (configurable)       │
│  APB Clock: 80 MHz (default)                         │
│  REF_TICK:  1 MHz                                    │
│                                                      │
│  Semua peripheral timer menggunakan APB Clock        │
│  WiFi/BT membutuhkan minimal 80 MHz APB             │
└──────────────────────────────────────────────────────┘
```

**Konfigurasi CPU Frequency ESP32 (ESP-IDF):**

```c
#include "esp_pm.h"

void configure_clock(void)
{
    /* Set CPU ke 240 MHz (maksimal) */
    esp_pm_config_t pm_config = {
        .max_freq_mhz = 240,
        .min_freq_mhz = 80,
        .light_sleep_enable = false
    };
    esp_pm_configure(&pm_config);

    /* Baca frekuensi aktual */
    uint32_t freq = esp_clk_cpu_freq();
    printf("CPU Frequency: %lu Hz\n", freq);
    
    /* APB frequency (untuk perhitungan timer) */
    uint32_t apb = esp_clk_apb_freq();
    printf("APB Frequency: %lu Hz\n", apb);
}
```

### 8.3 Hubungan Clock dengan Timer

Pemahaman clock tree sangat krusial untuk perhitungan timer yang akurat:

| Parameter | STM32F103 | ESP32 |
|-----------|-----------|-------|
| **Timer Source Clock** | APB1×2 = 72 MHz (TIM2-7) | APB = 80 MHz |
| **Prescaler Range** | 0 – 65535 | 2 – 65536 |
| **Auto-Reload Range** | 0 – 65535 (16-bit) | 0 – 2⁶⁴ (64-bit) |
| **Formula** | $f_{timer} = \frac{f_{clk}}{(PSC+1)(ARR+1)}$ | $f_{timer} = \frac{f_{APB}}{divider}$ |
| **1 Hz Example** | PSC=7199, ARR=9999 | divider=80000000 |

> **Tip:** Selalu verifikasi clock aktual dengan oscilloscope atau `SystemCoreClock` variable sebelum menghitung prescaler timer.

---

## 9. Perbandingan STM32 vs ESP32

| Aspek | STM32F103 (HAL) | ESP32 (ESP-IDF) |
|-------|-----------------|-----------------|
| **Interrupt Controller** | NVIC | Interrupt Matrix |
| **Priority Levels** | 16 (4-bit) | 7 levels |
| **External Interrupt** | 16 EXTI lines | All GPIOs |
| **Timer Count** | 4 × 16-bit | 4 × 64-bit |
| **ISR Attribute** | None needed | IRAM_ATTR required |
| **Critical Section** | `__disable_irq()` | `portENTER_CRITICAL()` |
| **Watchdog** | IWDG + WWDG | esp_task_wdt |
| **Timer Cascade** | Hardware master-slave | Software chaining |
| **Framework** | STM32Cube HAL | ESP-IDF |

---

## 9. Daftar Program Praktikum

| No | Nama | Topik | Tingkat |
|----|------|-------|---------|
| 01 | EXTI_Interrupt | External Interrupt dasar | Dasar |
| 02 | EXTI_Debounce | Interrupt dengan software debouncing | Dasar |
| 03 | Timer_Periodic | Timer periodik dengan auto-reload | Dasar |
| 04 | Timer_One_Shot | Timer satu kali (one-shot mode) | Menengah |
| 05 | Timer_PWM_Basic | PWM dasar via timer config | Menengah |
| 06 | Watchdog_Timer | IWDG/WWDG (STM32), esp_task_wdt (ESP32) | Menengah |
| 07 | Timer_Cascade | Multi-timer chaining master-slave | Lanjut |
| 08 | Output_Compare_Toggle | Output Compare toggle GPIO | Menengah |
| 09 | Input_Capture | Mengukur lebar pulsa dan frekuensi | Lanjut |
| 10 | Encoder_Interface | Timer encoder mode | Lanjut |
| 11 | Multiple_Timers | Beberapa timer bersamaan | Lanjut |
| 12 | NVIC_Priority | Prioritas dan nesting interrupt | Lanjut |

---

## 10. Best Practices

### 10.1 Aturan ISR

1. **Singkat dan Cepat** — maksimal beberapa mikrodetik
2. **Tidak Ada Blocking** — jangan `vTaskDelay()`, `printf()` di ISR
3. **Gunakan `volatile`** untuk shared variables
4. **IRAM_ATTR** wajib untuk semua ISR di ESP32
5. **Flag-based pattern** — ISR set flag, task proses

```c
// Pattern benar (ESP-IDF)
volatile int event_flag = 0;

static void IRAM_ATTR my_isr(void *arg) {
    event_flag = 1;  // Set flag saja
}

void app_main(void) {
    while (1) {
        if (event_flag) {
            event_flag = 0;
            process_event();  // Proses di main
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
```

---

## 11. Troubleshooting

| Masalah | Penyebab | Solusi |
|---------|----------|--------|
| ISR tidak dipanggil | NVIC/ISR service belum enable | `HAL_NVIC_EnableIRQ()` / `gpio_install_isr_service()` |
| ISR terus-menerus | Flag tidak di-clear | Gunakan HAL handler |
| ESP32 crash | IRAM_ATTR hilang | Tambahkan IRAM_ATTR |
| Timer tidak akurat | PSC/ARR salah | Hitung ulang |
| WDT reset | Lupa kick | `HAL_IWDG_Refresh()` / `esp_task_wdt_reset()` |
| Button bounce | Tidak ada debounce | Timer-based debounce |
| Variabel tidak update | Tidak volatile | Tambahkan volatile |

---

## Referensi

1. *Mastering STM32* - Carmine Noviello — Ch7 (Interrupts), Ch11 (Timers)
2. *Kolban's Book on ESP32* - Neil Kolban — p267-268 (ISR), p300-302 (Timers)
3. *The Definitive Guide to ARM Cortex-M3* - Joseph Yiu
4. STM32F103 Reference Manual (RM0008) — Chapter 10 (EXTI), Chapter 13-15 (Timers)
5. ESP-IDF Programming Guide — GPIO, GPTimer, Task WDT
6. [STM32 Timer Cookbook AN4776](https://www.st.com/resource/en/application_note/an4776.pdf)
7. [ESP-IDF GPTimer API](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/gptimer.html)
8. [ESP-IDF Task Watchdog](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/wdts.html)
