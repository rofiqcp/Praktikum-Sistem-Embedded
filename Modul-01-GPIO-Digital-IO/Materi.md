# Modul 01: GPIO dan Digital I/O
## *Dari Push Button hingga Keypad — Menguasai Antarmuka Digital*

**Program Studi:** Teknologi Rekayasa Otomasi  
**Platform:** STM32F103C8T6 Blue Pill | ESP32 DevKit V1  
**Framework:** STM32Cube HAL | ESP-IDF  
**Durasi:** 3 × 50 menit

---

## Daftar Isi

1. [Pendahuluan & Motivasi](#1-pendahuluan)
2. [Arsitektur GPIO Mikrokontroler](#2-arsitektur-gpio)
3. [Mode Output: Push-Pull & Open-Drain](#3-mode-output)
4. [Logika Active-HIGH dan Active-LOW](#4-logika-aktif)
5. [Mode Input: Pull-UP dan Pull-DOWN](#5-mode-input)
6. [Pull-UP & Pull-DOWN Eksternal vs Internal](#6-pullup-pulldown)
7. [Debounce: Menangani Noise Tombol](#7-debounce)
8. [Kecepatan GPIO dan Slew Rate](#8-gpio-speed)
9. [Akses Register Langsung (BSRR, ODR, IDR)](#9-register)
10. [Rotary Encoder Kuadratur](#10-encoder)
11. [Matrix Keypad Scanning](#11-keypad)
12. [LCD I2C 16×2 — Antarmuka Tampilan](#12-lcd-i2c)
13. [GPIO pada ESP32 — Perbandingan & Perbedaan](#13-esp32-gpio)
14. [Best Practices & Tips Debugging](#14-best-practices)
15. [Ringkasan Percobaan 1–10](#15-ringkasan)

---

## 1. Pendahuluan & Motivasi

### 1.1 Apa itu GPIO?

**GPIO (General Purpose Input/Output)** adalah pin pada mikrokontroler yang dapat dikonfigurasi secara fleksibel sebagai **input** (membaca sinyal dari luar) atau **output** (mengeluarkan sinyal ke luar). GPIO adalah antarmuka paling fundamental antara mikrokontroler dan dunia fisik.

```
┌─────────────────────────────────────────────┐
│              SISTEM EMBEDDED                │
│                                             │
│   ┌─────────┐    ┌──────────┐              │
│   │   CPU   │◄──►│  GPIO    │◄────────────►│ Dunia Nyata
│   │  @64MHz │    │  Regs    │  (LED, BTN,  │
│   └─────────┘    └──────────┘   Motor, dsb)│
└─────────────────────────────────────────────┘
```

### 1.2 Relevansi untuk Teknologi Rekayasa Otomasi

Dalam konteks **otomasi industri**, GPIO digunakan untuk:

| Aplikasi Industri | Komponen | GPIO Mode |
|-------------------|----------|-----------|
| Indikator status mesin | LED, Lampu Pilot | OUTPUT |
| Sensor limit switch | Proximity switch | INPUT |
| Tombol emergency stop | E-Stop button | INPUT (NC) |
| Keypad HMI sederhana | Matrix keypad | INPUT + OUTPUT |
| Encoder posisi | Rotary encoder | INPUT quadrature |
| Relay kontrol | MOSFET driver | OUTPUT |

### 1.3 Hardware yang Digunakan

| No | Komponen | Jumlah | Fungsi |
|----|----------|--------|--------|
| 1 | STM32F103C8T6 (Blue Pill) | 1 | MCU utama (64 MHz, Cortex-M3) |
| 2 | ESP32 DevKit V1 | 1 | MCU sekunder (240 MHz, dual-core) |
| 3 | LED 5mm | 8 | Indikator output digital |
| 4 | Resistor 220Ω | 16 | Current limiting LED + pull-resistor |
| 5 | Push Button | 4 | Input diskrit |
| 6 | Rotary Encoder 5-pin | 1 | Input kuadratur + push |
| 7 | Keypad Matrix 4×4 (8-pin) | 1 | Input matriks |
| 8 | LCD I²C 16×2 | 1 | Tampilan status (PCF8574, addr 0x27) |

---

## 2. Arsitektur GPIO Mikrokontroler

### 2.1 Struktur Internal GPIO STM32F103

Setiap pin GPIO pada STM32F103 terhubung ke sebuah sel I/O dengan komponen:

```
                         VDD (3.3V)
                            │
              ┌─────────────┤
              │             │
         ┌────┴────┐   ┌────┴────┐
         │  PMOS   │   │ Pull-up │ ~40kΩ (software)
         │ (output)│   │  Rpup   │
         └────┬────┘   └────┬────┘
              │             │
              ├─────────────┼──────────── I/O PAD
              │             │
         ┌────┴────┐   ┌────┴────┐
         │  NMOS   │   │Pull-dn  │ ~40kΩ (software)
         │ (output)│   │  Rpdn   │
         └────┬────┘   └────┬────┘
              │             │
           VSS (GND)      VSS
```

**Komponen utama GPIO STM32:**
- **PMOS + NMOS transistor:** membentuk output push-pull
- **PMOS saja (NMOS open):** membentuk output open-drain
- **Pull-up/Pull-down resistor:** ~40kΩ, diaktifkan via software
- **Input buffer:** membaca nilai di pad
- **Output latch (ODR):** menyimpan nilai keluaran
- **BSRR register:** bit-set/reset atomik tanpa read-modify-write

### 2.2 Register GPIO STM32F103

```c
typedef struct {
    __IO uint32_t CRL;   // Control register low  (pin 0-7)
    __IO uint32_t CRH;   // Control register high (pin 8-15)
    __IO uint32_t IDR;   // Input Data Register   (read-only hardware)
    __IO uint32_t ODR;   // Output Data Register
    __IO uint32_t BSRR;  // Bit Set/Reset Register (ATOMIK!)
    __IO uint32_t BRR;   // Bit Reset Register
    __IO uint32_t LCKR;  // Lock Register
} GPIO_TypeDef;
```

| Register | Fungsi | Contoh Akses |
|----------|--------|--------------|
| `IDR` | Baca state input semua pin | `val = GPIOB->IDR & (1<<0)` |
| `ODR` | Tulis/baca state output | `GPIOA->ODR = 0xFF` |
| `BSRR` | Set [15:0] atau Reset [31:16] — ATOMIK | `GPIOA->BSRR = (1<<0)` → SET PA0 |
| `CRL/CRH` | Konfigurasi mode (dikelola HAL) | — |

> **Mengapa BSRR lebih aman dari ODR?**  
> `ODR` memerlukan read-modify-write: baca ODR → ubah bit → tulis kembali. Jika ada interrupt di tengah, bit lain bisa berubah. `BSRR` adalah operasi **atomik** — satu instruksi, tanpa risiko race condition.

### 2.3 Konfigurasi GPIO via HAL

```c
GPIO_InitTypeDef GPIO_InitStruct = {0};

// PA0 sebagai output push-pull, speed rendah
GPIO_InitStruct.Pin   = GPIO_PIN_0;
GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
GPIO_InitStruct.Pull  = GPIO_NOPULL;
GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
```

**HAL Functions GPIO:**
| Fungsi | Keterangan |
|--------|-----------|
| `HAL_GPIO_Init(GPIOx, &init)` | Inisialisasi pin |
| `HAL_GPIO_WritePin(GPIOx, Pin, PinState)` | Tulis HIGH/LOW |
| `HAL_GPIO_ReadPin(GPIOx, Pin)` | Baca state pin |
| `HAL_GPIO_TogglePin(GPIOx, Pin)` | Toggle output |
| `HAL_GPIO_DeInit(GPIOx, Pin)` | Reset ke default |

---

## 3. Mode Output: Push-Pull & Open-Drain

### 3.1 Output Push-Pull (PP)

Pada mode **Push-Pull**, driver output memiliki dua transistor aktif:
- **Push:** PMOS aktif → pin ditarik ke VDD → output **HIGH**
- **Pull:** NMOS aktif → pin ditarik ke GND → output **LOW**

```
VDD ─────────────────────
              │
         [PMOS]  ← aktif saat ODR=1
              │
              ├──── GPIO Pin (dapat drive HIGH & LOW dengan kuat)
              │
         [NMOS]  ← aktif saat ODR=0
              │
GND ─────────────────────
```

**Karakteristik PP:**
- Mampu **source** (keluarkan) dan **sink** (tarik) arus
- Drive strength: max 25mA per pin STM32
- **Paling umum digunakan** untuk LED, relay driver, komunikasi UART

```c
// Inisialisasi PA0 sebagai Output Push-Pull
GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;

// LED ON:  PA0 = HIGH
HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_SET);

// LED OFF: PA0 = LOW
HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_RESET);
```

### 3.2 Output Open-Drain (OD)

Pada mode **Open-Drain**, hanya **NMOS** yang aktif; PMOS dinonaktifkan:
- **ODR = 0:** NMOS aktif → pin ditarik ke GND → output **LOW**
- **ODR = 1:** NMOS non-aktif → pin **Hi-Z** (impedansi tinggi, mengapung)

```
         PMOS  DINONAKTIFKAN ← tidak ada hubungan ke VDD
              │
              ├──── GPIO Pin ←─── HANYA bisa LOW atau Hi-Z
              │
         [NMOS]  ← aktif saat ODR=0
              │
GND ─────────────────────
```

> **PIN HI-Z TIDAK BISA MENJADI HIGH DENGAN SENDIRINYA!**  
> Untuk bisa HIGH, diperlukan **resistor pull-up eksternal atau internal** ke VDD.

**Perbandingan arus saat Hi-Z dengan pull-up internal 40kΩ:**

$$I = \frac{V_{DD} - V_f}{R_{pull} + R_{serial}} = \frac{3.3 - 2.0}{40{,}000 + 220} \approx 0.032\ \text{mA}$$

Hasilnya LED *sangat redup* — inilah demonstrasi utama Percobaan 2 (Shadow & Ghost).

```c
// PA1 Output Open-Drain + pull-up internal
GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
GPIO_InitStruct.Pull = GPIO_PULLUP;

HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET); // LOW  → LED ON (terang)
HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_SET);   // Hi-Z → LED redup (∵ 40kΩ)
```

**Kapan pakai Open-Drain?**
- Bus I²C (wired-AND: banyak perangkat berbagi satu jalur)
- Level shifting (pull-up ke 5V dari pin 3.3V OD)
- Relay driver dengan dioda back-EMF

---

## 4. Logika Active-HIGH dan Active-LOW

### 4.1 Definisi

| Jenis | LED ON saat | LED OFF saat | Arah Arus |
|-------|------------|--------------|-----------|
| **Active-HIGH** | Output = HIGH (1) | Output = LOW (0) | Source: pin → 220Ω → LED → GND |
| **Active-LOW** | Output = LOW (0) | Output = HIGH (1) | Sink: VDD → LED → 220Ω → pin |

### 4.2 Contoh: PC13 Blue Pill (Active-LOW)

Blue Pill memiliki LED bawaan di **PC13** dengan koneksi:
```
VDD ──────[LED]──[220Ω]────── PC13
```

- PC13 = **LOW** → ada beda potensial → arus mengalir → LED **ON**
- PC13 = **HIGH** → tidak ada selisih potensial → LED **OFF**

```c
// LED bawaan PC13 ON:
HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET); // LOW = ON

// LED bawaan PC13 OFF:
HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);   // HIGH = OFF
```

> **Tips Desain:** Active-LOW lebih umum dalam industri untuk **fail-safe** — jika kabel putus, sinyal jatuh ke LOW (via pull-up), sistem dapat mendeteksi kerusakan jalur sinyal.

---

## 5. Mode Input: Pull-UP dan Pull-DOWN

### 5.1 Problem: Floating Input

Jika pin dikonfigurasi sebagai INPUT tanpa pull resistor dan tidak ada sinyal terhubung, pin dalam kondisi **floating** (mengapung — nilai tidak pasti):

```
     Pin GPIO (Floating)
          │
          │  ← Nilai tidak pasti! Bisa HIGH, LOW, atau beralih acak
          │     Sangat sensitif terhadap noise RF, interferensi
```

> **Floating pin dapat membaca nilai acak** — sistem tidak bisa diandalkan!

### 5.2 Pull-UP: Default HIGH

**Resistor Pull-UP** menghubungkan pin ke VDD sehingga nilai default adalah HIGH:

```
     VDD ────[R-PULLUP]────┬──── GPIO Pin
                           │
                       [Button]
                           │
                          GND
```

| Kondisi Tombol | Level pada Pin |
|----------------|---------------|
| Lepas | HIGH (ditarik Rpull ke VDD) |
| Ditekan | LOW (terhubung ke GND via tombol) |

Tombol "aktif" saat membaca LOW → disebut **Active-LOW**.

### 5.3 Pull-DOWN: Default LOW

**Resistor Pull-DOWN** menghubungkan pin ke GND sehingga nilai default adalah LOW:

```
     VDD ────[Button]────┬──── GPIO Pin
                         │
                     [R-PULLDOWN]
                         │
                        GND
```

| Kondisi Tombol | Level pada Pin |
|----------------|---------------|
| Lepas | LOW (ditarik Rpull ke GND) |
| Ditekan | HIGH (terhubung ke VDD via tombol) |

Tombol "aktif" saat membaca HIGH → disebut **Active-HIGH**.

---

## 6. Pull-UP & Pull-DOWN Eksternal vs Internal

### 6.1 Pull-UP Eksternal (Percobaan 3 — Sentinel Gate)

```
     3.3V ──[220Ω]──┬── PB0 (INPUT, NOPULL)
                    │
               [BTN1]
                    │
                   GND
```

| Parameter | Nilai |
|-----------|-------|
| Resistansi | 220Ω |
| Tegangan saat Lepas | ~3.3V (HIGH) |
| Arus saat Tekan | 3.3V / 220Ω ≈ **15 mA** (habis ke GND) |
| Noise immunity | ⭐⭐⭐⭐⭐ sangat baik |

```c
// P03: Pin tanpa pull internal — external menentukan level
GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
GPIO_InitStruct.Pull = GPIO_NOPULL;
```

### 6.2 Pull-DOWN Eksternal (Percobaan 4 — Ground Guardian)

```
     3.3V ──[BTN2]──┬── PB1 (INPUT, NOPULL)
                    │
               [220Ω]
                    │
                   GND
```

```c
// P04: Pin tanpa pull internal
GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
GPIO_InitStruct.Pull = GPIO_NOPULL;
```

### 6.3 Pull-UP Internal (Percobaan 5 — Phantom Touch)

MCU mengaktifkan **resistor internal ~40kΩ** ke VDD. **Tidak perlu resistor eksternal!**

```c
GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
GPIO_InitStruct.Pull = GPIO_PULLUP;   // ~40kΩ internal aktif
```

**Rangkaian ekivalen:**
```
     VDD ──[~40kΩ internal]──┬── PB0
                             │
                        [BTN1]   ← langsung ke GND, no resistor!
                             │
                            GND
```

### 6.4 Pull-DOWN Internal (Percobaan 6 — Force Field)

```c
GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
GPIO_InitStruct.Pull = GPIO_PULLDOWN; // ~40kΩ internal ke GND
```

Wiring: **pin → satu kaki tombol → kaki lain → 3.3V. No resistor!**

> ⚠️ **Jangan hubungkan 5V ke GPIO STM32F103** (kecuali pin berlabel FT = 5V tolerant). Tegangan maksimum input adalah VDD+0.3V = 3.6V.

### 6.5 Perbandingan Lengkap

| Aspek | Ext 220Ω | Ext 10kΩ | Internal 40kΩ |
|-------|----------|-----------|----------------|
| **Komponen** | Butuh resistor | Butuh resistor | ✅ Tidak perlu |
| **Kekuatan pull** | ⭐⭐⭐⭐⭐ (kuat) | ⭐⭐⭐ (sedang) | ⭐⭐ (lemah) |
| **Arus terbuang** | ~15mA saat tekan | 0.33mA | 0.082mA |
| **Noise immunity** | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐ | ⭐⭐ |
| **Kemudahan** | Perlu kabel ekstra | Perlu kabel ekstra | ✅ Sangat mudah |
| **Kasus ideal** | Industri ber-EMI tinggi | Embedded umum | Prototyping, lab |

---

## 7. Debounce: Menangani Noise Tombol

### 7.1 Fenomena Bounce

Tombol mekanik tidak langsung stabil saat ditekan. Terjadi **bouncing** — serangkaian transisi HIGH↔LOW selama 1–50 ms:

```
Tanpa debounce:
────────────────┐  ┌──┐  ┌────┐   ┌──────────────
                └──┘  └──┘    └───┘
                ^ MCU bisa membaca 5-10 press dari 1 tekan!

Dengan debounce 50ms:
────────────────┐                    ┌──────────────
                └────────────────────┘
                ^  tunggu 50ms stabil → baru valid
```

### 7.2 Metode Debounce Software

#### Metode 1: Delay Sederhana (tidak dianjurkan)
```c
if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_0) == GPIO_PIN_RESET) {
    HAL_Delay(50);  // ❌ BLOCKING — program tidak bisa lakukan hal lain!
    if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_0) == GPIO_PIN_RESET) {
        /* valid press */
    }
}
```

#### Metode 2: State Machine Non-Blocking (Percobaan 7 — DIANJURKAN)

```c
typedef enum {
    BTN_RELEASED,
    BTN_DEBOUNCING,
    BTN_PRESSED
} BtnState;

BtnState state[4]          = {BTN_RELEASED};
uint32_t debounce_start[4] = {0};
#define DEBOUNCE_MS 50

void update_button(int idx, GPIO_TypeDef *port, uint16_t pin) {
    uint8_t  raw = HAL_GPIO_ReadPin(port, pin);
    uint32_t now = HAL_GetTick();

    switch (state[idx]) {
        case BTN_RELEASED:
            if (raw == GPIO_PIN_RESET) {
                state[idx]          = BTN_DEBOUNCING;
                debounce_start[idx] = now;
            }
            break;

        case BTN_DEBOUNCING:
            if ((now - debounce_start[idx]) >= DEBOUNCE_MS) {
                if (raw == GPIO_PIN_RESET) {
                    state[idx] = BTN_PRESSED;
                    on_button_press(idx);   // aksi valid!
                } else {
                    state[idx] = BTN_RELEASED; // false alarm (bounce)
                }
            }
            break;

        case BTN_PRESSED:
            if (raw == GPIO_PIN_SET)
                state[idx] = BTN_RELEASED;
            break;
    }
}
```

**State Diagram:**
```
   ┌──────────┐  tekan terdeteksi  ┌─────────────┐  50ms stabil  ┌─────────┐
   │ RELEASED │──────────────────►│ DEBOUNCING  │─────────────►│ PRESSED │
   └──────────┘                   └─────────────┘              └─────────┘
        ▲                                │                           │
        │◄──── bounce (lepas sebelum 50ms)                          │
        │◄──── lepas tombol ─────────────────────────────────────────┘
```

### 7.3 Debounce di ESP32

```c
// ESP32: gunakan esp_timer_get_time() (mikro detik, presisi tinggi)
int64_t debounce_start_us = 0;
#define DEBOUNCE_US 50000  // 50ms = 50000µs

int64_t now_us = esp_timer_get_time();
if ((now_us - debounce_start_us) >= DEBOUNCE_US) {
    /* valid press */
}
```

---

## 8. Kecepatan GPIO dan Slew Rate

### 8.1 Apa itu Slew Rate?

**Slew rate** (GPIO Speed) mengacu pada **kecuraman transisi** sinyal dari LOW→HIGH atau HIGH→LOW. Ini bukan frekuensi maksimum toggle, melainkan seberapa tajam tepi sinyal:

```
HIGH ─── ─ ─ ─ ─ ─ ─ ─    ─ ─ ─ ─ ─ ─────────
              ╱               ╱
Slew:  ╱FAST╱            ╱SLOW╱
     ╱    ╱             ╱    ╱
LOW ─ ─ ─ ─ ─ ─ ─ ─   ─ ─ ─ ─ ─ ─ ─ ─ ─
```

### 8.2 Tabel Konfigurasi Speed STM32

| HAL Constant | Max Slew | Konsumsi | Gunakan Untuk |
|-------------|----------|----------|----------------|
| `GPIO_SPEED_FREQ_LOW` | 2 MHz | Rendah | LED, relay, tombol |
| `GPIO_SPEED_FREQ_MEDIUM` | 10 MHz | Sedang | SPI, UART, I²C |
| `GPIO_SPEED_FREQ_HIGH` | 50 MHz | Tinggi | USB, SDIO, EMMC |

> **Tips:** LED tidak perlu slew rate tinggi. Gunakan `FREQ_LOW` untuk hemat daya & kurangi EMI.

### 8.3 Mengubah Speed Runtime (Percobaan 8)

```c
void Set_GPIO_Speed(uint32_t speed) {
    GPIO_InitTypeDef init = {0};
    init.Pin   = 0xFF;  // PA0-PA7
    init.Mode  = GPIO_MODE_OUTPUT_PP;
    init.Pull  = GPIO_NOPULL;
    init.Speed = speed;   // ubah slew rate saat runtime
    HAL_GPIO_Init(GPIOA, &init);
}
```

Pemanggilan: `Set_GPIO_Speed(GPIO_SPEED_FREQ_HIGH);`

### 8.4 Drive Strength pada ESP32

ESP32 menggunakan **drive capability** — berapa mA yang bisa dikuarkan:

```c
gpio_set_drive_capability(GPIO_NUM_4, GPIO_DRIVE_CAP_0); // ~5mA
gpio_set_drive_capability(GPIO_NUM_4, GPIO_DRIVE_CAP_1); // ~10mA
gpio_set_drive_capability(GPIO_NUM_4, GPIO_DRIVE_CAP_2); // ~20mA (default)
gpio_set_drive_capability(GPIO_NUM_4, GPIO_DRIVE_CAP_3); // ~40mA (max)
```

---

## 9. Akses Register Langsung (BSRR, ODR, IDR)

### 9.1 Mengapa Register Langsung?

| Situasi | HAL | Register Langsung |
|---------|-----|-------------------|
| Set 8 LED sekaligus | Butuh 8× `HAL_GPIO_WritePin()` | `GPIOA->ODR = value` — 1 instruksi |
| Baca 4 tombol bersamaan | Butuh 4× `HAL_GPIO_ReadPin()` | `GPIOB->IDR & mask` — 1 instruksi |
| Toggle bit tanpa race | ReadPin → modifikasi → WritePin | `GPIOA->BSRR = bit` — atomik |

### 9.2 BSRR — Bit Set/Reset Register (Atomik)

```
BSRR bit layout:
  [31:16] = BR (Bit Reset) — tulis 1 untuk RESET pin ke LOW
  [15:0]  = BS (Bit Set)   — tulis 1 untuk SET pin ke HIGH

Contoh atomik: set PA0 HIGH, reset PA1 LOW dalam SATU instruksi:
  GPIOA->BSRR = (1 << 0)         // set PA0
              | (1 << (16 + 1)); // reset PA1
```

```c
// Percobaan 9: tampilkan 8-bit count secara atomik
void display_count_leds(uint8_t count) {
    uint32_t set_mask   = (uint32_t)(count);          // bit yang perlu HIGH
    uint32_t reset_mask = (uint32_t)((~count) & 0xFF); // bit yang perlu LOW
    GPIOA->BSRR = (reset_mask << 16) | set_mask;      // atomik!
}
```

### 9.3 ODR dan IDR

```c
// ODR — tulis 8 LED sekaligus (non-atomik, modify bit tertentu)
GPIOA->ODR = (GPIOA->ODR & 0xFFFFFF00) | (led_value & 0xFF);

// IDR — baca 4 tombol sekaligus
uint16_t raw = GPIOB->IDR;
uint8_t btn0 = (raw >> 0) & 1;   // PB0
uint8_t btn1 = (raw >> 1) & 1;   // PB1
uint8_t btn3 = (raw >> 3) & 1;   // PB3 (bukan PB2 = BOOT1!)
uint8_t btn4 = (raw >> 4) & 1;   // PB4
```

---

## 10. Rotary Encoder Kuadratur

### 10.1 Prinsip Kerja Encoder 5-pin

Encoder rotari menghasilkan **dua sinyal kuadratur A (CLK) dan B (DT)** yang bergeser fase 90°:

```
Putar CW (Clockwise):                Putar CCW (Counter-CW):
CLK: ─┐ ┌─┐ ┌─                     CLK: ─┐ ┌─┐ ┌─
      └─┘ └─┘                             └─┘ └─┘
DT:  ──┐ ┌─┐                        DT:  ┐ ┌─┐ ┌─┐
       └─┘ └                              └─┘ └─┘
      CLK↓ & DT=HIGH → CW (+1)           CLK↓ & DT=LOW → CCW (-1)
```

**Pin Encoder 5-pin:**
| Pin | Fungsi | Terhubung ke STM32 |
|-----|--------|--------------------|
| GND | Ground | GND |
| + | VCC | 3.3V |
| SW | Push switch | PB14 (INPUT PULLUP) |
| DT | Data — sinyal B | PB13 (INPUT PULLUP) |
| CLK | Clock — sinyal A | PB12 (INPUT PULLUP) |

### 10.2 Algoritma Decoding

```c
uint8_t  last_clk = 1;
int16_t  count    = 128;  // mulai tengah (range 0-255)

void poll_encoder(void) {
    uint8_t clk = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_12);
    uint8_t dt  = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_13);
    uint8_t sw  = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_14);

    // Deteksi falling edge CLK (1→0)
    if (last_clk == 1 && clk == 0) {
        if (dt == 1) { count++; if (count > 255) count = 255; }  // CW
        else         { count--; if (count < 0)   count = 0;   }  // CCW
    }
    last_clk = clk;

    // SW press: reset ke 128
    if (sw == 0) { HAL_Delay(20); if (sw == 0) count = 128; }
}
```

---

## 11. Matrix Keypad Scanning

### 11.1 Struktur Keypad 4×4

16 tombol disusun dalam matriks 4×4. 8 pin = 4 Row + 4 Column:

```
         COL1  COL2  COL3  COL4
          │     │     │     │
ROW1 ──┬──●──┬──●──┬──●──┬──●──
       [1]   [2]   [3]   [A]
ROW2 ──┼──┬──●──┬──●──┬──●──┬──
       [4]   [5]   [6]   [B]
ROW3 ──┼──┬──●──┬──●──┬──●──┬──
       [7]   [8]   [9]   [C]
ROW4 ──┼──┬──●──┬──●──┬──●──┬──
       [*]   [0]   [#]   [D]
```

**Pin Assignment STM32:**
- ROW 1–4: PB8, PB9, PB10, PB11 → **OUTPUT_PP**
- COL 1–4: PB12, PB13, PB14, PB15 → **INPUT PULLUP**

### 11.2 Algoritma Scanning

```c
const uint16_t ROW_PINS[4] = {GPIO_PIN_8,  GPIO_PIN_9,  GPIO_PIN_10, GPIO_PIN_11};
const uint16_t COL_PINS[4] = {GPIO_PIN_12, GPIO_PIN_13, GPIO_PIN_14, GPIO_PIN_15};

const uint8_t KEYMAP[4][4] = {
    { 1,  2,  3, 10},   // Row 1
    { 4,  5,  6, 11},   // Row 2
    { 7,  8,  9, 12},   // Row 3
    {15,  0, 14, 13}    // Row 4 (* → 15,  # → 14)
};

int8_t scan_keypad(void) {
    for (int r = 0; r < 4; r++) {
        // Set semua baris HIGH (idle)
        for (int j = 0; j < 4; j++)
            HAL_GPIO_WritePin(GPIOB, ROW_PINS[j], GPIO_PIN_SET);

        // Tarik baris ini ke LOW
        HAL_GPIO_WritePin(GPIOB, ROW_PINS[r], GPIO_PIN_RESET);
        HAL_Delay(1);  // settling time

        // Baca kolom (PULLUP → LOW saat ditekan)
        for (int c = 0; c < 4; c++) {
            if (HAL_GPIO_ReadPin(GPIOB, COL_PINS[c]) == GPIO_PIN_RESET)
                return KEYMAP[r][c];  // tombol ditemukan!
        }
    }
    return -1;  // tidak ada tombol
}
```

---

## 12. LCD I²C 16×2 — Antarmuka Tampilan

### 12.1 LCD I²C Module

LCD I²C menggunakan **backpack PCF8574** yang mengkonversi antarmuka LCD paralel 4-bit menjadi **I²C 2-kawat** (SDA + SCL). Menghemat 6 pin GPIO menjadi hanya 2.

```
STM32 PB6 (SCL) ─────── SCL ─── [PCF8574] ─── [LCD 16×2]
STM32 PB7 (SDA) ─────── SDA ─── (I2C Expander)
            GND ──────── GND
           3.3V ──────── VCC
```

| Parameter | Nilai |
|-----------|-------|
| Protokol | I²C |
| Kecepatan | 100 kHz (Standard Mode) |
| Alamat default | 0x27 atau 0x3F |
| STM32: SCL | PB6 (I2C1) |
| STM32: SDA | PB7 (I2C1) |
| ESP32: SCL | GPIO22 |
| ESP32: SDA | GPIO21 |

### 12.2 Informasi yang Ditampilkan per Percobaan

| Percobaan | LCD Baris 1 | LCD Baris 2 |
|-----------|-------------|-------------|
| P01 LED Parade | `Pola: Running    ` | `Delay: 200ms      ` |
| P02 Shadow Ghost | `Mode: PP vs OD   ` | `PA0=ON PA1=dim    ` |
| P03 Sentinel Gate | `BTN: LEPAS       ` | `Press Count: xx   ` |
| P04 Ground Guard | `BTN: LEPAS       ` | `Press Count: xx   ` |
| P05 Phantom Touch | `InternalPullUP   ` | `B1:OK B2:OK       ` |
| P06 Force Field | `InternalPullDN   ` | `B1:OK B3:OK       ` |
| P07 Clean Contact | `C1:xx C2:xx      ` | `C3:xx C4:xx       ` |
| P08 Speed Racer | `Speed: SLOW 2MHz ` | `Pola: Running     ` |
| P09 Twist & Count | `Count= 175  CW   ` | `▓▓▓▓▓▓░░░░░░░░░░  ` |
| P10 Matrix Cmdr | `KEY: [5]         ` | `Total: 023         ` |

---

## 13. GPIO pada ESP32 — Perbandingan & Perbedaan

### 13.1 Konfigurasi Dasar ESP-IDF

```c
#include "driver/gpio.h"

// Metode batch (dianjurkan untuk inisialisasi banyak pin)
gpio_config_t io_conf = {
    .pin_bit_mask = (1ULL << GPIO_NUM_4) | (1ULL << GPIO_NUM_5),
    .mode         = GPIO_MODE_OUTPUT,
    .pull_up_en   = GPIO_PULLUP_DISABLE,
    .pull_down_en = GPIO_PULLDOWN_DISABLE,
    .intr_type    = GPIO_INTR_DISABLE,
};
gpio_config(&io_conf);

// Operasi individual
gpio_set_level(GPIO_NUM_4, 1);            // HIGH
uint32_t val = gpio_get_level(GPIO_NUM_32); // baca
```

### 13.2 Perhatian Khusus ESP32

| Perhatian | Detail |
|-----------|--------|
| **GPIO34–39 INPUT ONLY** | Tidak bisa output, tidak ada internal pull |
| **GPIO6–11** | Flash SPI internal — JANGAN digunakan! |
| **GPIO2** | LED built-in Active-HIGH (bukan Active-LOW seperti STM32!) |
| **Tegangan max** | 3.3V — TIDAK 5V tolerant! |
| **GPIO36, GPIO39** | Hanya input, tidak ada pull sama sekali |

### 13.3 Perbandingan API

| Fungsi | STM32 HAL | ESP32 ESP-IDF |
|--------|-----------|---------------|
| Init | `HAL_GPIO_Init()` | `gpio_config()` |
| Tulis | `HAL_GPIO_WritePin()` | `gpio_set_level()` |
| Baca | `HAL_GPIO_ReadPin()` | `gpio_get_level()` |
| Pull-up | `GPIO_PULLUP` (dalam init) | `gpio_set_pull_mode(pin, GPIO_PULLUP_ONLY)` |
| Pull-dn | `GPIO_PULLDOWN` | `gpio_set_pull_mode(pin, GPIO_PULLDOWN_ONLY)` |
| Floating | `GPIO_NOPULL` | `gpio_set_pull_mode(pin, GPIO_FLOATING)` |
| Drive | `GPIO_SPEED_FREQ_xxx` | `gpio_set_drive_capability()` |
| Atomik SET | `GPIOA->BSRR = bit` | `GPIO.out_w1ts = bit` |
| Atomik CLR | `GPIOA->BSRR = (bit<<16)` | `GPIO.out_w1tc = bit` |

---

## 14. Best Practices & Tips Debugging

### 14.1 Checklist Sebelum Upload

- [ ] Aktifkan clock GPIO: `__HAL_RCC_GPIOx_CLK_ENABLE()` (STM32)
- [ ] Jangan gunakan **PB2** (BOOT1) untuk tombol pada Blue Pill
- [ ] P09 dan P10 berbagi PB12–PB15 — tidak bisa bersamaan
- [ ] GND STM32 dan ESP32 harus terhubung jika dipakai bersamaan
- [ ] LED selalu butuh resistor seri (min. 100Ω, praktis 220Ω)
- [ ] Cek alamat LCD I²C dengan scanner (0x27 atau 0x3F)

### 14.2 Hitung Arus LED

$$I_{LED} = \frac{V_{CC} - V_f}{R_{serial}} = \frac{3.3 - 2.0}{220} \approx 5.9\ \text{mA}$$

Di mana $V_f$ = forward voltage (merah ≈ 2.0V, hijau ≈ 2.2V, biru ≈ 3.0–3.4V)

> Untuk LED biru ($V_f$ ≈ 3.2V): $I = (3.3 - 3.2)/220 = 0.45\ \text{mA}$ → sangat redup!  
> Gunakan LED merah/kuning/hijau untuk demonstrasi visual yang lebih jelas.

### 14.3 Tabel Troubleshooting

| Masalah | Kemungkinan Penyebab | Solusi |
|---------|---------------------|--------|
| LED tidak menyala | Polaritas terbalik / tanpa resistor | Periksa arah LED, ukur tegangan |
| LCD blank | Alamat I²C salah / SDA/SCL terbalik | I²C scanner, cek PB6=SCL PB7=SDA |
| Tombol bouncing | DEBOUNCE_MS terlalu kecil | Naikkan ke 50ms |
| Encoder loncat | CLK/DT tanpa pull-up | Gunakan PULLUP, tambah kapasitor 100nF |
| Keypad tidak respons | ROW/COL terbalik | Cek wiring, test per baris |
| Upload STM32 gagal | BOOT0 salah, ST-Link tidak terdeteksi | Cek jumper BOOT0=0 |

---

## 15. Ringkasan Percobaan 1–10

| # | Judul | Konsep Utama | Hardware Utama |
|---|-------|-------------|----------------|
| **P01** | **LED Parade** | Push-Pull, Active-HIGH, ODR, 4 pola | 8 LED + LCD |
| **P02** | **Shadow & Ghost** | Open-Drain, Active-LOW, Hi-Z, arus OD | 8 LED + LCD |
| **P03** | **Sentinel Gate** | Input NOPULL, Pull-UP eksternal 220Ω | 1 BTN + 4 LED + LCD |
| **P04** | **Ground Guardian** | Input NOPULL, Pull-DOWN eksternal 220Ω | 1 BTN + 4 LED + LCD |
| **P05** | **Phantom Touch** | Pull-UP internal ~40kΩ, no external | 2 BTN + 4 LED + LCD |
| **P06** | **Force Field** | Pull-DOWN internal ~40kΩ, Active-HIGH | 2 BTN + 4 LED + LCD |
| **P07** | **Clean Contact** | State machine debounce, 4 BTN, non-blocking | 4 BTN + 4 LED + LCD |
| **P08** | **Speed Racer** | GPIO Slew Rate, runtime config, 4 pola | 4 BTN + 8 LED + LCD |
| **P09** | **Twist & Count** | Encoder kuadratur, CW/CCW, BSRR atomik | Encoder + 8 LED + LCD |
| **P10** | **Matrix Commander** | Keypad 4×4 scanning, KEYMAP, INPUT PU | Keypad + 8 LED + LCD |

### Pin Assignment Cepat (STM32 Blue Pill)

| GPIO | Fungsi | Dipakai di |
|------|--------|-----------|
| PA0–PA7 | LED 1–8 (Active-HIGH, 220Ω) | P01 – P10 |
| PC13 | LED built-in (Active-LOW) | P01, P02 |
| PB0 | Push Button 1 | P03, P05, P07, P08 |
| PB1 | Push Button 2 | P04, P06, P07, P08 |
| PB3, PB4 | Push Button 3, 4 | P07, P08 |
| PB6, PB7 | SCL, SDA — LCD I²C | P01 – P10 |
| PB8–PB11 | Keypad ROW 1–4 (OUTPUT) | P10 |
| PB12–PB15 | Keypad COL 1–4 (INPUT PU) | P10 |
| PB12, PB13, PB14 | Encoder CLK, DT, SW | P09 |

---

## Daftar Pustaka

1. Ferretti, C. (2020). *Mastering STM32, 2nd Edition*, Chapter 6: GPIO Management. Leanpub.
2. Kolban, N. (2018). *Kolban's Book on ESP32*, Chapter: GPIO. Self-published.
3. STMicroelectronics. (2021). *STM32F103x8/xB Reference Manual (RM0008)*. ST, Rev 21.
4. Espressif Systems. (2024). *ESP32 Technical Reference Manual v5.3*, Chapter 4: GPIO & RTC GPIO.
5. Williams, J. (1990). "Signal Sources, Conditioners, and Power Circuitry." *Linear Technology AN26*.
6. Ganssle, J. (2008). *The Art of Designing Embedded Systems, 2nd Ed.* Newnes, Chapter 3.
