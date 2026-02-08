# Modul 01: GPIO dan Digital I/O


## Daftar Isi

## Capaian Pembelajaran

Setelah menyelesaikan bab ini, mahasiswa diharapkan mampu:

1. **Memahami** arsitektur dan fungsi GPIO pada mikrokontroler STM32 dan ESP32
2. **Mengkonfigurasi** pin GPIO sebagai input maupun output digital dengan berbagai mode (push-pull, open-drain, pull-up/pull-down)
3. **Mengimplementasikan** teknik debouncing untuk input dari push button
4. **Menerapkan** akses register langsung (BSRR, GPIO_OUT_REG) untuk operasi GPIO atomik
5. **Menganalisis** perbedaan karakteristik GPIO antara STM32 (HAL) dan ESP32 (ESP-IDF)
6. **Merancang** sistem digital I/O termasuk binary counter, matrix keypad, dan emergency stop

---

## 📚 1. Pendahuluan

### 1.1 Apa itu GPIO?

**GPIO (General Purpose Input/Output)** adalah pin pada mikrokontroler yang dapat dikonfigurasi secara fleksibel sebagai input atau output digital. GPIO merupakan interface paling dasar dan fundamental dalam sistem embedded untuk berinteraksi dengan dunia luar.

```
┌─────────────────────────────────────────────────────────────┐
│                    MIKROKONTROLER                           │
│  ┌─────────────────────────────────────────────────────┐   │
│  │                      CPU                             │   │
│  │   ┌─────────┐    ┌─────────┐    ┌─────────┐        │   │
│  │   │ Program │    │  Data   │    │  ALU    │        │   │
│  │   │ Memory  │    │ Memory  │    │         │        │   │
│  │   └─────────┘    └─────────┘    └─────────┘        │   │
│  └─────────────────────────────────────────────────────┘   │
│                           │                                 │
│                    ┌──────┴──────┐                         │
│                    │ GPIO Block  │                         │
│                    │  Registers  │                         │
│                    └──────┬──────┘                         │
│                           │                                 │
└───────────────────────────┼─────────────────────────────────┘
                            │
            ┌───────────────┼───────────────┐
            │               │               │
         ┌──┴──┐         ┌──┴──┐         ┌──┴──┐
         │PIN 0│         │PIN 1│         │PIN n│
         └──┬──┘         └──┬──┘         └──┬──┘
            │               │               │
         ┌──┴──┐         ┌──┴──┐         ┌──┴──┐
         │ LED │         │Button│        │Sensor│
         └─────┘         └─────┘         └─────┘
```

### 1.2 Pentingnya GPIO dalam Embedded Systems

GPIO adalah **building block** fundamental untuk:

| Aplikasi | Contoh Penggunaan |
|----------|-------------------|
| **Aktuator** | Mengendalikan LED, relay, motor driver |
| **Sensor Digital** | Membaca push button, limit switch, PIR sensor |
| **Komunikasi** | Bit-banging SPI, I2C, atau protokol custom |
| **Debugging** | LED status, logic analyzer interface |
| **User Interface** | Keypad matrix, DIP switch, seven segment |

### 1.3 Perbandingan Singkat GPIO: STM32 vs ESP32

| Fitur | STM32F103C8T6 | ESP32 |
|-------|---------------|-------|
| **Arsitektur** | ARM Cortex-M3 | Xtensa LX6 Dual Core |
| **Tegangan I/O** | 3.3V (5V tolerant pada beberapa pin) | 3.3V (TIDAK 5V tolerant) |
| **Jumlah GPIO** | 37 pin | 34 pin |
| **Drive Current** | Max 25mA per pin | Max 40mA (20mA recommended) |
| **Internal Pull-up/down** | Ya (40kΩ typical) | Ya (45kΩ typical) |
| **Framework** | STM32Cube HAL | ESP-IDF |
| **Built-in LED** | PC13 (Active LOW) | GPIO2 (Active HIGH) |
| **Konfigurasi Mode** | 8 mode per pin | Input/Output/Input-Output/Disable |
| **Atomic Bit Set/Reset** | BSRR register | GPIO_OUT_W1TS / GPIO_OUT_W1TC |

### 1.4 Ruang Lingkup Modul

Modul ini membahas **GPIO digital murni** — yaitu operasi input dan output yang hanya bernilai HIGH (1) atau LOW (0). Tidak membahas sinyal analog (ADC/DAC) maupun modulasi lebar pulsa (yang dicakup di modul terpisah).

---

## 📚 2. Teori Dasar GPIO

### 2.1 Struktur Internal GPIO

#### 2.1.1 STM32F103 GPIO Structure

```
                          VDD (3.3V)
                             │
                         ┌───┴───┐
                         │       │ Pull-up
                         │  Rpu  │ Resistor
                         │       │ (~40kΩ)
                         └───┬───┘
                             │
        ┌────────────────────┼────────────────────┐
        │                    │                    │
        │    ┌───────────────┼───────────────┐   │
        │    │               │               │   │
    ┌───┴────┤          ┌────┴────┐          │   │
    │        │          │         │          │   │
    │  Input │          │  I/O    │          │   │  Output
    │ Driver │◀─────────┤  Pad    ├──────────┤   │  Driver
    │        │          │         │          │   │
    └───┬────┘          └────┬────┘          │   │
        │                    │               │   │
        │    └───────────────┼───────────────┘   │
        │                    │                    │
        │                ┌───┴───┐               │
        │                │       │ Pull-down     │
        │                │  Rpd  │ Resistor      │
        │                │       │ (~40kΩ)       │
        │                └───┬───┘               │
        │                    │                    │
        │                   GND                   │
        │                                         │
        └─────────────────────────────────────────┘
                     ↓
              To Input Register (IDR)
```

#### 2.1.2 Mode GPIO pada STM32F103

STM32F103 menyediakan 8 mode konfigurasi GPIO melalui register CRL/CRH:

| Mode | Kode HAL | Deskripsi |
|------|----------|-----------|
| Input Floating | `GPIO_MODE_INPUT` | Tanpa pull resistor, sensitif noise |
| Input Pull-Up | Dengan `GPIO_PULLUP` | Internal pull-up ~40kΩ aktif |
| Input Pull-Down | Dengan `GPIO_PULLDOWN` | Internal pull-down ~40kΩ aktif |
| Output Push-Pull | `GPIO_MODE_OUTPUT_PP` | Dapat drive HIGH dan LOW |
| Output Open-Drain | `GPIO_MODE_OUTPUT_OD` | Hanya dapat pull LOW |
| AF Push-Pull | `GPIO_MODE_AF_PP` | Alternate function push-pull |
| AF Open-Drain | `GPIO_MODE_AF_OD` | Alternate function open-drain |
| Analog | `GPIO_MODE_ANALOG` | Untuk koneksi ke ADC |

**Konfigurasi Speed** pada STM32:

| Speed Setting | Max Frequency | Use Case |
|---------------|---------------|----------|
| `GPIO_SPEED_FREQ_LOW` | 2 MHz | LED, relay, low-speed I/O |
| `GPIO_SPEED_FREQ_MEDIUM` | 10 MHz | General purpose |
| `GPIO_SPEED_FREQ_HIGH` | 50 MHz | SPI, komunikasi cepat |

> **Tip:** Gunakan speed rendah jika tidak perlu kecepatan tinggi — mengurangi EMI (electromagnetic interference) dan konsumsi daya.

#### 2.1.3 Mode GPIO pada ESP32 (ESP-IDF)

```c
// Mode GPIO pada ESP32 via gpio_config()
typedef enum {
    GPIO_MODE_DISABLE         = 0,    // Pin disabled
    GPIO_MODE_INPUT           = 1,    // Input only
    GPIO_MODE_OUTPUT          = 2,    // Output only (push-pull)
    GPIO_MODE_OUTPUT_OD       = 6,    // Output open-drain
    GPIO_MODE_INPUT_OUTPUT    = 3,    // Bidirectional push-pull
    GPIO_MODE_INPUT_OUTPUT_OD = 7     // Bidirectional open-drain
} gpio_mode_t;

// Pull-up / Pull-down
typedef enum {
    GPIO_PULLUP_DISABLE   = 0,
    GPIO_PULLUP_ENABLE    = 1
} gpio_pullup_t;

typedef enum {
    GPIO_PULLDOWN_DISABLE = 0,
    GPIO_PULLDOWN_ENABLE  = 1
} gpio_pulldown_t;
```

### 2.2 Output Push-Pull vs Open-Drain

```
PUSH-PULL OUTPUT:                    OPEN-DRAIN OUTPUT:

    VDD                                  VDD
     │                                    │
  ┌──┴──┐                              ┌──┴──┐
  │ P-FET│ ◀ ON saat output HIGH       │ Ext │ External
  └──┬──┘                              │Pull-│ Pull-up
     │                                  │ up  │ (Opsional)
     ├───── OUTPUT PIN                 └──┬──┘
     │                                    │
  ┌──┴──┐                                 ├───── OUTPUT PIN
  │ N-FET│ ◀ ON saat output LOW          │
  └──┬──┘                              ┌──┴──┐
     │                                 │ N-FET│ ◀ ON saat LOW
    GND                                └──┬──┘
                                          │
                                         GND

Karakteristik:                     Karakteristik:
- Dapat drive HIGH dan LOW         - Hanya dapat pull LOW
- Sumber arus lebih tinggi         - Butuh external pull-up
- Standar untuk LED drive          - Untuk I2C, level shifting
- Tidak bisa wire-OR               - Mendukung wire-OR (bus sharing)
```

**Kapan menggunakan apa?**

| Fitur | Push-Pull | Open-Drain |
|-------|-----------|------------|
| Drive LED | ✅ Ideal | ⚠️ Perlu ext. pull-up |
| I2C Bus | ❌ | ✅ Wajib |
| Level Shifting | ❌ | ✅ Dengan pull-up ke VDD target |
| Wire-OR / Bus Sharing | ❌ | ✅ Multi-device pada 1 line |

### 2.3 Pull-Up dan Pull-Down Resistor

```
PULL-UP (Default HIGH):              PULL-DOWN (Default LOW):

    VDD (3.3V)                            VDD (3.3V)
     │                                     │
  ┌──┴──┐                              ┌──┴──┐
  │     │ Internal atau                │     │ Button
  │ Rpu │ External Pull-Up            │ BTN │
  │     │ (~40-45kΩ internal)          │     │
  └──┬──┘                              └──┬──┘
     │                                     │
     ├───── GPIO Input ──── MCU            ├───── GPIO Input ──── MCU
     │                                     │
  ┌──┴──┐                              ┌──┴──┐
  │     │ Button                       │     │ Internal atau
  │ BTN │                              │ Rpd │ External Pull-Down
  │     │                              │     │
  └──┬──┘                              └──┬──┘
     │                                     │
    GND                                   GND

  Idle: HIGH (1)                       Idle: LOW (0)
  Pressed: LOW (0)                     Pressed: HIGH (1)
```

> **Penting:** Jangan biarkan pin input **floating** (tidak terhubung ke apa pun). Pin floating akan membaca nilai acak karena noise. Selalu aktifkan pull-up atau pull-down.

### 2.4 GPIO Drive Strength

Drive strength menentukan seberapa besar arus yang dapat disuplai/diterima oleh pin GPIO.

**ESP32 Drive Strength Levels:**

| Level | Arus Approx | Konstanta ESP-IDF |
|-------|-------------|-------------------|
| 0 | ~5 mA | `GPIO_DRIVE_CAP_0` |
| 1 | ~10 mA | `GPIO_DRIVE_CAP_1` |
| 2 | ~20 mA (default) | `GPIO_DRIVE_CAP_2` |
| 3 | ~40 mA | `GPIO_DRIVE_CAP_3` |

**STM32 Drive Strength:**

STM32F103 tidak memiliki drive strength register eksplisit, tetapi speed setting mempengaruhi slew rate dan drive capability. Max current per pin: 25 mA. Total current semua GPIO: max 150 mA.

### 2.5 Konsep Debouncing

Ketika tombol ditekan, terjadi **bouncing** — kontak mekanis yang menghasilkan pulsa tidak stabil selama 5-50ms:

```
TANPA DEBOUNCING:
                 Bouncing period (~5-50ms)
                 ├────────────┤
    HIGH ────────┐   ┌┐  ┌┐  ┌┐
                 │   ││  ││  ││
                 │   ││  ││  ││
    LOW          └───┘└──┘└──┘└────────────────
                 │               │
              Button           Button
              Pressed          Stable

DENGAN SOFTWARE DEBOUNCING:
    HIGH ────────┐
                 │
                 │  ← Delay/timer menunggu stabil
    LOW          └─────────────────────────────
                 │             │
              Button        Debounced
              Pressed       Signal Valid
```

#### Teknik Debouncing:

**1. Hardware Debouncing (RC Filter):**
```
            ┌─────┐
  Button    │     │    MCU
    │       │  R  │     │
    ├───────┤     ├─────┼───── GPIO Input
    │       │10kΩ │     │
  ┌─┴─┐     └─────┘   ┌─┴─┐
  │   │               │ C │ 100nF
  │GND│               │   │
  └───┘               └─┬─┘
                        │
                       GND
```

**2. Software Debouncing (ESP-IDF):**
```c
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"

#define BUTTON_PIN    GPIO_NUM_0
#define DEBOUNCE_US   50000   // 50ms dalam microseconds

static int64_t last_press_time = 0;

// Dalam loop atau task:
void button_task(void *pvParameters)
{
    while (1) {
        int level = gpio_get_level(BUTTON_PIN);
        if (level == 0) {  // Active LOW
            int64_t now = esp_timer_get_time();
            if ((now - last_press_time) > DEBOUNCE_US) {
                last_press_time = now;
                // Button benar-benar ditekan — lakukan aksi
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
```

**3. Software Debouncing (STM32 HAL):**
```c
#include "stm32f1xx_hal.h"

#define BUTTON_PIN    GPIO_PIN_0
#define BUTTON_PORT   GPIOB
#define DEBOUNCE_MS   50

static uint32_t last_press_tick = 0;

// Dalam main loop:
void check_button(void)
{
    if (HAL_GPIO_ReadPin(BUTTON_PORT, BUTTON_PIN) == GPIO_PIN_RESET) {
        uint32_t now = HAL_GetTick();
        if ((now - last_press_tick) > DEBOUNCE_MS) {
            last_press_tick = now;
            // Button benar-benar ditekan
        }
    }
}
```

### 2.6 Current Sourcing dan Sinking

```
SOURCING (LED ke GND):              SINKING (LED ke VDD):

    GPIO Pin (HIGH)                     VDD
        │                                │
        │ →→→ Arus mengalir             │
        │                              ┌─┴─┐
      ┌─┴─┐                            │LED│ Anode
      │   │ Resistor                   │ ▼ │
      │ R │ 220Ω                       └─┬─┘
      │   │                              │
      └─┬─┘                            ┌─┴─┐
        │                              │   │ Resistor
      ┌─┴─┐                            │ R │ 220Ω
      │LED│ Anode                      │   │
      │ ▼ │                            └─┬─┘
      └─┬─┘ Cathode                      │ ◀◀◀ Arus mengalir
        │                                │
       GND                          GPIO Pin (LOW)

Catatan STM32F103:
- PC13 hanya dapat SINK ~3mA (gunakan external LED jika perlu lebih)
- Pin lain dapat source/sink hingga 25mA

Catatan ESP32:
- Semua GPIO dapat source/sink hingga 40mA
- Recommended: 20mA untuk lifetime lebih baik
```

### 2.7 Perhitungan Resistor LED

```
Formula: R = (Vcc - Vf) / If

Dimana:
- Vcc = Tegangan supply (3.3V)
- Vf  = Forward voltage LED (merah~2V, biru/putih~3V)
- If  = Forward current yang diinginkan (10-20mA typical)

Contoh untuk LED merah @ 10mA:
R = (3.3V - 2.0V) / 0.010A = 130Ω
→ Gunakan 150Ω atau 220Ω (nilai standar terdekat)

Contoh untuk LED biru @ 10mA:
R = (3.3V - 3.0V) / 0.010A = 30Ω
→ Gunakan 33Ω atau 47Ω
```

---

## 📚 3. GPIO pada STM32 — STM32Cube HAL API

### 3.1 Pinout STM32F103C8T6 (Blue Pill)

```
                    USB
                   ┌───┐
            ┌──────┤   ├──────┐
            │      └───┘      │
      PB12 ─┤1              40├─ VBat
      PB13 ─┤2              39├─ PC13 ◀── LED (Active LOW)
      PB14 ─┤3              38├─ PC14
      PB15 ─┤4              37├─ PC15
       PA8 ─┤5              36├─ PA0
       PA9 ─┤6  USART1_TX   35├─ PA1
      PA10 ─┤7  USART1_RX   34├─ PA2
      PA11 ─┤8  USB-        33├─ PA3
      PA12 ─┤9  USB+        32├─ PA4
      PA15 ─┤10             31├─ PA5
       PB3 ─┤11             30├─ PA6
       PB4 ─┤12             29├─ PA7
       PB5 ─┤13             28├─ PB0
       PB6 ─┤14 I2C1_SCL    27├─ PB1
       PB7 ─┤15 I2C1_SDA    26├─ PB10
       PB8 ─┤16             25├─ PB11
       PB9 ─┤17             24├─ Reset
      5V   ─┤18             23├─ 3.3V
       GND ─┤19             22├─ GND
       3V3 ─┤20             21├─ GND
            │                  │
            └──────────────────┘
                  ↑    ↑
               SWDIO SWCLK
               (PA13)(PA14)
```

### 3.2 Inisialisasi GPIO dengan HAL

```c
#include "stm32f1xx_hal.h"

// Aktifkan clock GPIO terlebih dahulu!
__HAL_RCC_GPIOA_CLK_ENABLE();
__HAL_RCC_GPIOB_CLK_ENABLE();
__HAL_RCC_GPIOC_CLK_ENABLE();

// Konfigurasi pin output: LED pada PC13
GPIO_InitTypeDef gpio = {0};
gpio.Pin   = GPIO_PIN_13;
gpio.Mode  = GPIO_MODE_OUTPUT_PP;    // Push-Pull output
gpio.Speed = GPIO_SPEED_FREQ_LOW;    // 2 MHz cukup untuk LED
gpio.Pull  = GPIO_NOPULL;
HAL_GPIO_Init(GPIOC, &gpio);

// Konfigurasi pin input: Button pada PB0 dengan pull-up
gpio.Pin   = GPIO_PIN_0;
gpio.Mode  = GPIO_MODE_INPUT;
gpio.Pull  = GPIO_PULLUP;            // Internal pull-up aktif
HAL_GPIO_Init(GPIOB, &gpio);
```

### 3.3 Fungsi HAL GPIO Utama

```c
// ============================================================
// MENULIS OUTPUT
// ============================================================

// Set pin HIGH
HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);

// Set pin LOW
HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);

// Toggle pin
HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);

// ============================================================
// MEMBACA INPUT
// ============================================================

// Baca level pin — return GPIO_PIN_SET atau GPIO_PIN_RESET
GPIO_PinState state = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_0);

if (state == GPIO_PIN_RESET) {
    // Button ditekan (active LOW dengan pull-up)
}

// ============================================================
// DE-INIT (reset pin ke default)
// ============================================================
HAL_GPIO_DeInit(GPIOC, GPIO_PIN_13);
```

### 3.4 Register GPIO STM32F103

```c
// Alamat Base GPIO
#define GPIOA_BASE  0x40010800
#define GPIOB_BASE  0x40010C00
#define GPIOC_BASE  0x40011000

// Struktur Register GPIO
typedef struct {
    volatile uint32_t CRL;   // Configuration Register Low  (pin 0-7)
    volatile uint32_t CRH;   // Configuration Register High (pin 8-15)
    volatile uint32_t IDR;   // Input Data Register  (read-only)
    volatile uint32_t ODR;   // Output Data Register
    volatile uint32_t BSRR;  // Bit Set/Reset Register (write-only)
    volatile uint32_t BRR;   // Bit Reset Register    (write-only)
    volatile uint32_t LCKR;  // Lock Register
} GPIO_TypeDef;
```

### 3.5 Operasi Atomik dengan BSRR

Register **BSRR (Bit Set/Reset Register)** memungkinkan operasi atomik — set atau reset satu pin tanpa mempengaruhi pin lain, tanpa perlu read-modify-write:

```c
// ============================================================
// BSRR: Bit 0-15 = SET, Bit 16-31 = RESET
// ============================================================

// Set PC13 HIGH (LED OFF karena active low)
GPIOC->BSRR = (1U << 13);         // Set bit 13

// Reset PC13 LOW (LED ON)
GPIOC->BSRR = (1U << (13 + 16));  // Reset bit 13
// Alternatif:
GPIOC->BRR  = (1U << 13);         // BRR khusus untuk reset

// ============================================================
// Mengapa BSRR lebih baik dari ODR?
// ============================================================

// ODR (TIDAK atomik — perlu read-modify-write):
GPIOC->ODR |= (1U << 13);   // ⚠️ Bisa terganggu interrupt
GPIOC->ODR &= ~(1U << 13);  // ⚠️ Bisa terganggu interrupt

// BSRR (ATOMIK — satu instruksi write):
GPIOC->BSRR = (1U << 13);   // ✅ Aman dari interrupt

// ============================================================
// Set BANYAK pin sekaligus
// ============================================================
// Set PA0, PA1, PA2, PA3 HIGH sekaligus:
GPIOA->BSRR = (1U << 0) | (1U << 1) | (1U << 2) | (1U << 3);

// Set PA0 HIGH dan PA3 LOW dalam satu operasi:
GPIOA->BSRR = (1U << 0) | (1U << (3 + 16));

// ============================================================
// Toggle menggunakan ODR (XOR) — satu-satunya cara toggle via register
// ============================================================
GPIOC->ODR ^= (1U << 13);
```

### 3.6 Membaca Register Input (IDR)

```c
// Input Data Register — membaca semua 16 pin sekaligus
uint32_t port_value = GPIOB->IDR;

// Cek pin tertentu
if (GPIOB->IDR & (1U << 0)) {
    // PB0 is HIGH
} else {
    // PB0 is LOW
}

// Baca multiple pin (contoh: DIP switch 4-bit pada PB0-PB3)
uint8_t dip_value = (GPIOB->IDR & 0x0F);  // Mask bit 0-3
```

---

## 📚 4. GPIO pada ESP32 — ESP-IDF API

### 4.1 Pinout ESP32 DevKitC

```
                    ┌─────────────────┐
                    │     USB-C       │
                    │    ┌─────┐      │
              EN ───┤    │     │    ├─── GPIO23 (MOSI)
         GPIO36(VP)─┤    │     │    ├─── GPIO22 (SCL)
         GPIO39(VN)─┤    └─────┘    ├─── GPIO1 (TX0)
          GPIO34 ───┤               ├─── GPIO3 (RX0)
          GPIO35 ───┤               ├─── GPIO21 (SDA)
          GPIO32 ───┤               ├─── GND
          GPIO33 ───┤               ├─── GPIO19 (MISO)
          GPIO25 ───┤               ├─── GPIO18 (SCK)
          GPIO26 ───┤     ESP32     ├─── GPIO5  (SS)
          GPIO27 ───┤    DevKitC    ├─── GPIO17 (TX2)
          GPIO14 ───┤               ├─── GPIO16 (RX2)
          GPIO12 ───┤               ├─── GPIO4
             GND ───┤               ├─── GPIO0 (BOOT)
          GPIO13 ───┤               ├─── GPIO2 ◀── LED Built-in
            3V3 ───┤               ├─── GPIO15
          GPIO15 ───┤               ├─── GND
            3V3 ───┤               ├─── 3V3
                    │               │
                    └───────────────┘

Catatan Penting:
- GPIO34-39: INPUT ONLY (tidak bisa output, tidak ada pull-up/down)
- GPIO6-11:  Terhubung ke flash SPI internal (JANGAN GUNAKAN)
- GPIO0:     Boot mode select (hati-hati saat desain)
- GPIO2:     Built-in LED (bisa digunakan untuk output)
```

### 4.2 Inisialisasi GPIO dengan gpio_config()

Fungsi `gpio_config()` adalah cara utama mengkonfigurasi GPIO di ESP-IDF. Satu panggilan dapat mengkonfigurasi banyak pin sekaligus:

```c
#include "driver/gpio.h"

// ============================================================
// Konfigurasi OUTPUT: LED pada GPIO2 dan GPIO4
// ============================================================
gpio_config_t io_conf_out = {
    .pin_bit_mask = (1ULL << GPIO_NUM_2) | (1ULL << GPIO_NUM_4),
    .mode         = GPIO_MODE_OUTPUT,        // Output push-pull
    .pull_up_en   = GPIO_PULLUP_DISABLE,
    .pull_down_en = GPIO_PULLDOWN_DISABLE,
    .intr_type    = GPIO_INTR_DISABLE        // Tanpa interrupt
};
gpio_config(&io_conf_out);

// ============================================================
// Konfigurasi INPUT: Button pada GPIO0 dengan pull-up
// ============================================================
gpio_config_t io_conf_in = {
    .pin_bit_mask = (1ULL << GPIO_NUM_0),
    .mode         = GPIO_MODE_INPUT,
    .pull_up_en   = GPIO_PULLUP_ENABLE,      // Internal pull-up
    .pull_down_en = GPIO_PULLDOWN_DISABLE,
    .intr_type    = GPIO_INTR_DISABLE
};
gpio_config(&io_conf_in);
```

### 4.3 Fungsi GPIO ESP-IDF Utama

```c
#include "driver/gpio.h"

// ============================================================
// MENULIS OUTPUT
// ============================================================

// Set pin HIGH
gpio_set_level(GPIO_NUM_2, 1);

// Set pin LOW
gpio_set_level(GPIO_NUM_2, 0);

// ============================================================
// MEMBACA INPUT
// ============================================================

// Baca level pin — return 0 atau 1
int level = gpio_get_level(GPIO_NUM_0);

if (level == 0) {
    // Button ditekan (active LOW dengan pull-up)
}

// ============================================================
// KONFIGURASI INDIVIDUAL (alternatif gpio_config)
// ============================================================

// Set direction
gpio_set_direction(GPIO_NUM_2, GPIO_MODE_OUTPUT);

// Set pull mode
gpio_set_pull_mode(GPIO_NUM_0, GPIO_PULLUP_ONLY);

// Set drive strength
gpio_set_drive_capability(GPIO_NUM_2, GPIO_DRIVE_CAP_3);  // 40mA

// Baca drive strength
gpio_drive_cap_t cap;
gpio_get_drive_capability(GPIO_NUM_2, &cap);

// Reset pin ke default
gpio_reset_pin(GPIO_NUM_2);
```

### 4.4 GPIO Matrix — Fitur Unik ESP32

ESP32 memiliki **GPIO Matrix** — mekanisme routing fleksibel yang memungkinkan sinyal peripheral diarahkan ke hampir semua pin GPIO:

```
┌─────────────────────────────────────────────────┐
│                  ESP32 SoC                       │
│                                                  │
│  ┌──────────┐     ┌──────────┐     ┌──────────┐│
│  │  UART0   │     │   GPIO   │     │  GPIO    ││
│  │  UART1   │────▶│  Matrix  │────▶│  Pads    ││
│  │  SPI     │     │ (Routing)│     │ (Fisik)  ││
│  │  I2C     │     │          │     │          ││
│  │  etc.    │     └──────────┘     └──────────┘│
│  └──────────┘                                   │
│   Peripheral         Flexible          Physical │
│   Signals            Routing           Pins     │
└─────────────────────────────────────────────────┘

Contoh: SPI bisa di-route ke GPIO manapun (bukan hanya pin default)
```

> **Catatan:** Beberapa sinyal ("dedicated GPIO") memiliki jalur langsung ke pin tanpa melewati GPIO Matrix, memberikan timing yang lebih presisi.

### 4.5 Register-Level Access ESP32

```c
#include "soc/gpio_reg.h"

// ============================================================
// ESP32 GPIO Register — akses langsung untuk performa tinggi
// ============================================================

// GPIO_OUT_REG: Output register (GPIO 0-31)
// Menulis 1 = HIGH, 0 = LOW
REG_WRITE(GPIO_OUT_REG, value);  // Set semua 32 pin sekaligus

// GPIO_OUT_W1TS_REG: Write 1 to Set (atomik)
REG_WRITE(GPIO_OUT_W1TS_REG, (1U << 2));  // Set GPIO2 HIGH

// GPIO_OUT_W1TC_REG: Write 1 to Clear (atomik)
REG_WRITE(GPIO_OUT_W1TC_REG, (1U << 2));  // Set GPIO2 LOW

// GPIO_IN_REG: Input register (GPIO 0-31)
uint32_t input_val = REG_READ(GPIO_IN_REG);

// Cek pin tertentu
if (REG_READ(GPIO_IN_REG) & (1U << 0)) {
    // GPIO0 is HIGH
}

// ============================================================
// Perbandingan performa
// ============================================================
// gpio_set_level():       ~1 μs (dengan validasi parameter)
// REG_WRITE(W1TS/W1TC):  ~0.1 μs (langsung ke register)
```

---

## 📚 5. Perbandingan Lengkap ESP32 vs STM32 GPIO

### 5.1 Tabel Perbandingan API

| Operasi | STM32 HAL | ESP-IDF |
|---------|-----------|---------|
| **Init clock** | `__HAL_RCC_GPIOx_CLK_ENABLE()` | Otomatis |
| **Konfigurasi pin** | `HAL_GPIO_Init(&GPIO_InitStruct)` | `gpio_config(&io_conf)` |
| **Set HIGH** | `HAL_GPIO_WritePin(GPIOx, pin, GPIO_PIN_SET)` | `gpio_set_level(pin, 1)` |
| **Set LOW** | `HAL_GPIO_WritePin(GPIOx, pin, GPIO_PIN_RESET)` | `gpio_set_level(pin, 0)` |
| **Toggle** | `HAL_GPIO_TogglePin(GPIOx, pin)` | Tidak ada — manual `gpio_set_level()` |
| **Baca input** | `HAL_GPIO_ReadPin(GPIOx, pin)` | `gpio_get_level(pin)` |
| **De-init** | `HAL_GPIO_DeInit(GPIOx, pin)` | `gpio_reset_pin(pin)` |
| **Set drive** | Via GPIO_Speed | `gpio_set_drive_capability(pin, cap)` |
| **Atomic set** | `GPIOx->BSRR = (1 << n)` | `REG_WRITE(GPIO_OUT_W1TS_REG, (1 << n))` |
| **Atomic clear** | `GPIOx->BSRR = (1 << (n+16))` | `REG_WRITE(GPIO_OUT_W1TC_REG, (1 << n))` |

### 5.2 Tabel Perbandingan Hardware

| Aspek | STM32F103C8T6 | ESP32 |
|-------|---------------|-------|
| **Total GPIO** | 37 pin | 34 pin (26 usable) |
| **Input-only pins** | Tidak ada | GPIO34-39 |
| **Max current/pin** | 25 mA | 40 mA (20 mA recommended) |
| **5V Tolerant** | Sebagian besar pin | ❌ Tidak ada |
| **Internal pull-up** | ~40 kΩ | ~45 kΩ |
| **Internal pull-down** | ~40 kΩ | ~45 kΩ |
| **Speed config** | 2/10/50 MHz | Tidak ada (via drive strength) |
| **Drive strength levels** | Tidak eksplisit | 4 level (5/10/20/40 mA) |
| **GPIO routing** | Fixed alternate function | Flexible GPIO Matrix |
| **Port-wide access** | Ya (IDR/ODR 16-bit) | Ya (GPIO_OUT_REG 32-bit) |
| **Atomic bit set/reset** | BSRR register | W1TS / W1TC register |
| **Built-in LED** | PC13 (active LOW, 3mA max) | GPIO2 (active HIGH) |
| **Clock gating** | Manual (`RCC_CLK_ENABLE`) | Otomatis |

### 5.3 Perbedaan Penting dalam Inisialisasi

```c
// ============================================================
// STM32 — Clock harus diaktifkan manual sebelum pakai GPIO
// ============================================================
__HAL_RCC_GPIOA_CLK_ENABLE();  // WAJIB! Tanpa ini GPIO tidak berfungsi
__HAL_RCC_GPIOB_CLK_ENABLE();
__HAL_RCC_GPIOC_CLK_ENABLE();

GPIO_InitTypeDef gpio = {0};
gpio.Pin   = GPIO_PIN_0 | GPIO_PIN_1;  // Bisa multi-pin
gpio.Mode  = GPIO_MODE_OUTPUT_PP;
gpio.Speed = GPIO_SPEED_FREQ_LOW;
HAL_GPIO_Init(GPIOA, &gpio);

// ============================================================
// ESP32 — Clock otomatis, konfigurasi via bitmask
// ============================================================
gpio_config_t conf = {
    .pin_bit_mask = (1ULL << 2) | (1ULL << 4),  // Multi-pin
    .mode         = GPIO_MODE_OUTPUT,
    .pull_up_en   = GPIO_PULLUP_DISABLE,
    .pull_down_en = GPIO_PULLDOWN_DISABLE,
    .intr_type    = GPIO_INTR_DISABLE
};
gpio_config(&conf);
```

---

## 📚 6. Binary Counter dan Bit Manipulation

### 6.1 Konsep Binary Counter

Binary counter menggunakan LED untuk menampilkan nilai biner. Dengan 4 LED, kita bisa menampilkan angka 0–15 (4-bit):

```
Nilai   LED3  LED2  LED1  LED0
  0:     ○     ○     ○     ○     (0000)
  1:     ○     ○     ○     ●     (0001)
  2:     ○     ○     ●     ○     (0010)
  3:     ○     ○     ●     ●     (0011)
  4:     ○     ●     ○     ○     (0100)
  ...
 15:     ●     ●     ●     ●     (1111)

○ = LED OFF, ● = LED ON
```

### 6.2 Operasi Bit yang Penting

```c
// ============================================================
// Operasi bit dasar untuk GPIO
// ============================================================

uint8_t counter = 0;  // Nilai 0–255

// Cek apakah bit ke-n aktif (1)
if (counter & (1U << n)) { /* bit n = 1 */ }

// Set bit ke-n menjadi 1
counter |= (1U << n);

// Clear bit ke-n menjadi 0
counter &= ~(1U << n);

// Toggle bit ke-n
counter ^= (1U << n);

// ============================================================
// Contoh: Tulis 4-bit counter ke 4 LED
// ============================================================

// STM32 — LED pada PA0, PA1, PA2, PA3
void display_binary_stm32(uint8_t value)
{
    for (int i = 0; i < 4; i++) {
        if (value & (1U << i)) {
            HAL_GPIO_WritePin(GPIOA, (1U << i), GPIO_PIN_SET);
        } else {
            HAL_GPIO_WritePin(GPIOA, (1U << i), GPIO_PIN_RESET);
        }
    }
}

// STM32 — Cara efisien dengan BSRR (set & clear dalam satu write)
void display_binary_stm32_fast(uint8_t value)
{
    uint32_t bsrr = 0;
    for (int i = 0; i < 4; i++) {
        if (value & (1U << i)) {
            bsrr |= (1U << i);          // Set bit (lower 16 bits)
        } else {
            bsrr |= (1U << (i + 16));   // Reset bit (upper 16 bits)
        }
    }
    GPIOA->BSRR = bsrr;  // Semua LED update sekaligus, atomik!
}

// ESP32 — LED pada GPIO4, GPIO5, GPIO18, GPIO19
static const gpio_num_t led_pins[] = {
    GPIO_NUM_4, GPIO_NUM_5, GPIO_NUM_18, GPIO_NUM_19
};

void display_binary_esp32(uint8_t value)
{
    for (int i = 0; i < 4; i++) {
        gpio_set_level(led_pins[i], (value >> i) & 1);
    }
}
```

### 6.3 Mengapa Bit Manipulation Penting?

| Kegunaan | Contoh |
|----------|--------|
| **Efisiensi** | Satu operasi BSRR update banyak pin atomik |
| **DIP Switch** | Baca 4-8 switch sebagai satu angka biner |
| **Keypad Matrix** | Scan baris/kolom dengan bit shifting |
| **Status Encoding** | Encode state machine dalam bit flags |
| **Register Access** | Semua register MCU diakses via bit manipulation |

---

## 📚 7. Implementasi Praktikum

### 7.1 Daftar Program Praktikum

| No | Program | STM32 | ESP32 | Deskripsi |
|----|---------|-------|-------|-----------|
| 01 | LED_Blink | ✅ | ✅ | Dasar GPIO output — toggle LED periodik |
| 02 | Multi_LED_Running | ✅ | ✅ | Pattern sequencing — LED berjalan |
| 03 | LED_Binary_Counter **(BARU)** | ✅ | ✅ | Counter biner 4-bit dengan bit manipulation |
| 04 | Button_Debounce | ✅ | ✅ | Software debouncing — state machine |
| 05 | Long_Short_Press | ✅ | ✅ | Deteksi durasi tekan (short/long press) |
| 06 | Toggle_Latch | ✅ | ✅ | Latch behavior — tekan sekali toggle state |
| 07 | GPIO_Drive_Strength | ✅ | ✅ | Konfigurasi drive strength / speed |
| 08 | DIP_Switch_Reader | ✅ | ✅ | Baca multi-input sebagai nilai biner |
| 09 | GPIO_Port_Register **(BARU)** | ✅ | ✅ | Akses register langsung (BSRR / GPIO_OUT_REG) |
| 10 | GPIO_Matrix_Keypad | ✅ | ✅ | Scan keypad matrix 4×4 via GPIO |
| 11 | Emergency_Stop | ✅ | ✅ | Safety interlock — emergency stop system |
| 12 | LED_Test_Pattern | ✅ | ✅ | Diagnostic pattern — test semua LED |

> **Catatan:** Program 03 (LED_Binary_Counter) dan 09 (GPIO_Port_Register) adalah program baru yang menekankan bit manipulation dan akses register langsung.

### 7.2 Skema Koneksi Standar

```
KONEKSI PRAKTIKUM GPIO:

STM32F103C8T6:                    ESP32 DevKitC:
┌─────────────────┐               ┌─────────────────┐
│                 │               │                 │
│  PC13 ──[LED]── GND (Built-in)  │  GPIO2 ──[LED]── GND (Built-in)
│                 │               │                 │
│  PA0 ──[R]──[LED]── GND         │  GPIO4 ──[R]──[LED]── GND
│  PA1 ──[R]──[LED]── GND         │  GPIO5 ──[R]──[LED]── GND
│  PA2 ──[R]──[LED]── GND         │  GPIO18──[R]──[LED]── GND
│  PA3 ──[R]──[LED]── GND         │  GPIO19──[R]──[LED]── GND
│                 │               │                 │
│  PB0 ──[BTN]── GND              │  GPIO21──[BTN]── GND
│  PB1 ──[BTN]── GND              │  GPIO22──[BTN]── GND
│                 │               │                 │
└─────────────────┘               └─────────────────┘

R = Resistor 220Ω
BTN = Push Button dengan internal pull-up enabled
```

### 7.3 Contoh: LED Blink (ESP-IDF)

```c
#include <stdio.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define LED_PIN  GPIO_NUM_2

void app_main(void)
{
    // Konfigurasi GPIO output
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << LED_PIN),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);

    int led_state = 0;
    while (1) {
        led_state = !led_state;
        gpio_set_level(LED_PIN, led_state);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
```

### 7.4 Contoh: LED Blink (STM32 HAL)

```c
#include "stm32f1xx_hal.h"

// LED on PC13 (active LOW pada Blue Pill)
#define LED_PIN    GPIO_PIN_13
#define LED_PORT   GPIOC

int main(void)
{
    HAL_Init();
    SystemClock_Config();

    // Enable clock
    __HAL_RCC_GPIOC_CLK_ENABLE();

    // Konfigurasi output
    GPIO_InitTypeDef gpio = {0};
    gpio.Pin   = LED_PIN;
    gpio.Mode  = GPIO_MODE_OUTPUT_PP;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_PORT, &gpio);

    while (1) {
        HAL_GPIO_TogglePin(LED_PORT, LED_PIN);
        HAL_Delay(500);
    }
}
```

---

## 📚 8. Best Practices GPIO

### 8.1 Checklist Keamanan

```
✅ LAKUKAN:
□ Gunakan resistor pembatas arus untuk LED (150–330Ω)
□ Aktifkan pull-up/pull-down untuk input — jangan biarkan floating
□ Implementasikan debouncing untuk mechanical switches
□ Periksa voltage level compatibility sebelum koneksi
□ Gunakan level shifter untuk interfacing 5V devices ke ESP32
□ Aktifkan GPIO clock sebelum konfigurasi (STM32 wajib!)

❌ HINDARI:
□ Melebihi maximum current per pin (25mA STM32, 40mA ESP32)
□ Menghubungkan 5V langsung ke GPIO ESP32
□ Menggunakan GPIO6-11 pada ESP32 (flash SPI internal)
□ Membiarkan input pin floating tanpa pull resistor
□ Short circuit pada output pin
□ Menggunakan GPIO0 ESP32 tanpa pertimbangan (boot pin)
```

### 8.2 Pola Pemrograman yang Baik

```c
// ============================================================
// 1. Definisikan semua pin di satu tempat (header file)
// ============================================================

// config.h — ESP-IDF
#define LED_PIN       GPIO_NUM_2
#define BUTTON_PIN    GPIO_NUM_0
#define LED_COUNT     4

// config.h — STM32 HAL
#define LED_PIN       GPIO_PIN_13
#define LED_PORT      GPIOC
#define BUTTON_PIN    GPIO_PIN_0
#define BUTTON_PORT   GPIOB

// ============================================================
// 2. Gunakan fungsi wrapper — abstraksi platform
// ============================================================

// ESP-IDF version
void led_on(void)  { gpio_set_level(LED_PIN, 1); }
void led_off(void) { gpio_set_level(LED_PIN, 0); }

// STM32 HAL version
void led_on(void)  { HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_RESET); }  // Active LOW
void led_off(void) { HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET); }

// ============================================================
// 3. Non-blocking timing dengan tick counter
// ============================================================

// ESP-IDF (FreeRTOS tick)
TickType_t last_update = xTaskGetTickCount();
if ((xTaskGetTickCount() - last_update) >= pdMS_TO_TICKS(1000)) {
    last_update = xTaskGetTickCount();
    // Aksi periodik
}

// STM32 HAL
uint32_t last_update = HAL_GetTick();
if ((HAL_GetTick() - last_update) >= 1000) {
    last_update = HAL_GetTick();
    // Aksi periodik
}
```

### 8.3 Debugging GPIO

```c
// ============================================================
// ESP-IDF: Gunakan ESP_LOG untuk debug
// ============================================================
#include "esp_log.h"
static const char *TAG = "GPIO";

ESP_LOGI(TAG, "GPIO%d state: %d", pin, gpio_get_level(pin));
ESP_LOGW(TAG, "Button pressed!");
ESP_LOGE(TAG, "GPIO config failed");

// ============================================================
// STM32 HAL: Gunakan UART printf (redirect ke USART)
// ============================================================
#include <stdio.h>
// Setelah setup UART retarget:
printf("PB0 state: %d\r\n",
       HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_0));

// ============================================================
// LED indicator untuk state (tanpa serial)
// ============================================================
typedef enum {
    STATE_IDLE,
    STATE_RUNNING,
    STATE_ERROR
} system_state_t;

void indicate_state(system_state_t state)
{
    switch (state) {
        case STATE_IDLE:    led_off(); break;
        case STATE_RUNNING: led_on();  break;
        case STATE_ERROR:   /* fast blink in timer */ break;
    }
}
```

---

## 📚 9. Troubleshooting

### 9.1 Masalah Umum dan Solusi

| Masalah | Kemungkinan Penyebab | Solusi |
|---------|---------------------|--------|
| LED tidak menyala | Polaritas terbalik, resistor terlalu besar | Periksa anode/cathode, ukur dengan multimeter |
| LED redup | Resistor terlalu besar, drive current rendah | Kurangi nilai resistor (min 100Ω), cek drive strength |
| Button bouncing | Tidak ada debouncing | Implementasi software debounce (50ms delay) |
| Input floating / acak | Tidak ada pull-up/down | Aktifkan internal pull-up atau tambah external |
| GPIO tidak responsif (STM32) | Clock belum diaktifkan | Tambahkan `__HAL_RCC_GPIOx_CLK_ENABLE()` |
| GPIO tidak responsif (ESP32) | Pin input-only atau flash pin | Periksa — GPIO34-39 input only, GPIO6-11 jangan dipakai |
| ESP32 boot loop | GPIO0 tertarik LOW | Lepaskan koneksi ke GPIO0 saat upload/boot |
| PC13 LED redup (STM32) | PC13 max ~3mA | Gunakan pin lain (PA0-PA7) untuk LED lebih terang |
| Binary counter salah | Bit order terbalik | Periksa mapping pin ke bit position |

### 9.2 Kode Diagnostik (ESP-IDF)

```c
#include "driver/gpio.h"
#include "esp_log.h"

static const char *TAG = "GPIO_DIAG";

void test_output_pins(void)
{
    gpio_num_t test_pins[] = {GPIO_NUM_2, GPIO_NUM_4, GPIO_NUM_5,
                               GPIO_NUM_18, GPIO_NUM_19};
    int num_pins = sizeof(test_pins) / sizeof(test_pins[0]);

    for (int i = 0; i < num_pins; i++) {
        gpio_set_direction(test_pins[i], GPIO_MODE_OUTPUT);
        gpio_set_level(test_pins[i], 1);
        vTaskDelay(pdMS_TO_TICKS(200));
        gpio_set_level(test_pins[i], 0);
        ESP_LOGI(TAG, "GPIO%d: OUTPUT OK", test_pins[i]);
    }
}

void test_input_pins(void)
{
    gpio_num_t test_pins[] = {GPIO_NUM_0, GPIO_NUM_21, GPIO_NUM_22};
    int num_pins = sizeof(test_pins) / sizeof(test_pins[0]);

    for (int i = 0; i < num_pins; i++) {
        gpio_set_direction(test_pins[i], GPIO_MODE_INPUT);
        gpio_set_pull_mode(test_pins[i], GPIO_PULLUP_ONLY);
        int level = gpio_get_level(test_pins[i]);
        ESP_LOGI(TAG, "GPIO%d: %s", test_pins[i],
                 level ? "HIGH (idle)" : "LOW (pressed?)");
    }
}
```

---

## Referensi

### Dokumentasi Resmi
1. **STM32F103C8T6 Reference Manual** (RM0008) — STMicroelectronics
2. **STM32F103C8T6 Datasheet** — STMicroelectronics
3. **ESP32 Technical Reference Manual** — Espressif Systems
4. **ESP-IDF Programming Guide: GPIO** — [docs.espressif.com](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/gpio.html)

### Buku Referensi
1. Carmine Noviello, *Mastering STM32* 2nd Edition
2. Neil Kolban, *Kolban's Book on ESP32*

### Online Resources
1. [ESP-IDF GPIO Driver API](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/gpio.html)
2. [STM32 HAL GPIO Documentation](https://www.st.com/resource/en/user_manual/um1785-stm32cube-hal-driver.pdf)

---

## 📝 Latihan Soal

### Soal Teori

1. Jelaskan perbedaan antara mode Push-Pull dan Open-Drain! Kapan masing-masing digunakan?
2. Mengapa PC13 pada STM32 Blue Pill disebut "active LOW"? Apa implikasinya terhadap kode?
3. Hitunglah resistor yang diperlukan untuk LED hijau (Vf=2.2V) dengan arus 15mA pada sistem 3.3V!
4. Apa yang terjadi jika GPIO input dibiarkan floating? Bagaimana solusi di STM32 HAL dan ESP-IDF?
5. Jelaskan mengapa register BSRR pada STM32 lebih aman dari ODR untuk operasi multi-pin!
6. Apa itu GPIO Matrix pada ESP32? Apa keuntungannya dibanding STM32?

### Soal Praktik

1. Implementasikan binary counter 4-bit dengan 4 LED, increment saat button short press, reset saat long press!
2. Buatlah program yang membaca DIP switch 4-bit dan menampilkan nilainya pada LED!
3. Bandingkan kecepatan `gpio_set_level()` vs `REG_WRITE(GPIO_OUT_W1TS_REG)` — ukur dengan GPIO toggle dan logic analyzer!

---

*Modul ini adalah bagian dari Praktikum Sistem Embedded*
*Versi 2.0 — Februari 2026*
