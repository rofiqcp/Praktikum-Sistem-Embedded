# STM32 GPIO Digital I/O - 10 Praktikum

Dokumentasi lengkap untuk 10 praktikum GPIO pada STM32 dengan dukungan 3 jenis mikrokontroler.

## Supported Microcontrollers (MCU)

Semua project mendukung **3 MCU berbeda**:

| MCU | Part Number | Clock | Environment | Board |
|-----|-------------|-------|----------------|-------|
| **F103** | STM32F103C8T6 | 72 MHz | `bluepill_f103c8` | Blue Pill |
| **F401** | STM32F401CCU6 | 84 MHz | `stm32f401cc` | Generic |
| **F411** | STM32F411CEU6 | 100 MHz | `stm32f411ce` | Generic |

## Cara Menggunakan

### 1. Pilih MCU di `platformio.ini`

Di setiap folder project terdapat `platformio.ini` dengan template berikut:

```ini
; PILIH SALAH SATU MCU DI BAWAH:
;   - bluepill_f103c8   : STM32F103C8T6 (Blue Pill, 72MHz)
;   - stm32f401cc       : STM32F401CCU6 (84MHz)
;   - stm32f411ce       : STM32F411CEU6 (100MHz)

[platformio]
default_envs = stm32f401cc    ; <-- Ubah sesuai MCU Anda
```

**Contoh mengubah ke Blue Pill (F103):**
```ini
[platformio]
default_envs = bluepill_f103c8
```

### 2. Compile & Upload

```bash
# Compile
pio run

# Upload ke board
pio run -t upload

# Serial Monitor
pio device monitor --baud 115200
```

### 3. Debug dengan breakpoint

```bash
# Debug dengan GDB
pio debug
```

## Daftar 10 Project

| # | Project | Fungsi |
|---|---------|--------|
| 1 | **STM32_P01_LED_Output_High** | LED Output Push-Pull Active-HIGH (8 LED, 4 pola) |
| 2 | **STM32_P02_LED_Output_Low** | LED Output Active-LOW & Open-Drain |
| 3 | **STM32_P03_Button_PullUp_Ext** | Tombol Pull-UP Eksternal (220Ω ke 3.3V) |
| 4 | **STM32_P04_Button_PullDown_Ext** | Tombol Pull-DOWN Eksternal (220Ω ke GND) |
| 5 | **STM32_P05_Button_PullUp_Internal** | Pull-UP Internal ~40kΩ tanpa resistor |
| 6 | **STM32_P06_Button_PullDown_Internal** | Pull-DOWN Internal ~40kΩ Active-HIGH |
| 7 | **STM32_P07_Button_Debounce** | Debounce State Machine non-blocking (4 tombol) |
| 8 | **STM32_P08_LED_Patterns** | GPIO Slew Rate & Pola LED Multi-Kecepatan |
| 9 | **STM32_P09_Encoder_5Pin** | Rotary Encoder Kuadratur 5-pin & Counter LCD |
| 10 | **STM32_P10_Keypad_8Pin** | Scanning Keypad Matrix 4×4 (8-pin) & Tampilan LCD |

## Hardware Configuration

### Pin Mapping

```
Port A (PA):
  PA0-PA7   → 8 LED External (Active-HIGH, 220Ω ke GND)

Port B (PB):
  PB0       → Push Button 1  (P03-P08)
  PB1       → Push Button 2  (P04-P08)
  PB3       → Push Button 3  (P07-P08) [PB2 = BOOT1, dihindari]
  PB4       → Push Button 4  (P07-P08)
  PB6       → SCL I2C1 (LCD 16x2) — AF Open-Drain
  PB7       → SDA I2C1 (LCD 16x2) — AF Open-Drain
  PB8-PB11  → Keypad ROW 1-4 (OUTPUT_PP) [P10]
  PB12-PB15 → Keypad COL 1-4 (INPUT PULLUP) [P10]
  PB12      → Encoder CLK (INPUT PULLUP) [P09]
  PB13      → Encoder DT  (INPUT PULLUP) [P09]
  PB14      → Encoder SW  (INPUT PULLUP) [P09]

Port C (PC):
  PC13      → LED Onboard Blue Pill (Active-LOW, built-in)
```

> ⚠️ P09 (Encoder) dan P10 (Keypad) berbagi PB12-PB15 — tidak bisa dijalankan bersamaan.

### Skema Dasar

#### LED Circuit
```
GPIO Pin ─┬─[220Ω]─[LED]─┐
          │              │
         Anode        Cathode
          │              │
          └──────────────┴─ GND
```

#### Button Circuit (Pull-up Internal)
```
3.3V ─[Pull-up 40kΩ]─┬─ GPIO Pin
                      │
                    [BTN]
                      │
                     GND
```

## File Structure

Setiap project memiliki struktur:

```
STM32_PXX_ProjectName/
├── platformio.ini          ← Config environment & MCU
├── src/
│   └── main.c              ← Program utama
└── schematic.png           ← Diagram rangkaian
```

## Building Instructions

### Dari Terminal

```bash
# Navigate ke project
cd STM32_P01_LED_Output_High

# Build untuk environment default (atau pilih manual)
pio run

# Build untuk MCU spesifik
pio run -e bluepill_f103c8
pio run -e stm32f401cc
pio run -e stm32f411ce
```

### Dari VS Code

1. **Buka Explorer** (Ctrl+Shift+E)
2. **Pilih Project** → klik folder project
3. **PlatformIO: Build** (tombol checkmark)
4. **Lihat build output** di terminal

## Troubleshooting

### Error: "Unsupported STM32 target"

Pastikan di `platformio.ini`, build_flags sudah benar:

```ini
build_flags = 
    -D USE_HAL_DRIVER
    -D STM32F103xC        ← Sesuai dengan pilihan MCU
    -D HSE_VALUE=8000000
```

### Upload gagal

1. Pastikan **ST-Link V2** terhubung ke pin SWDIO, SWCLK, GND, 3.3V
2. Driver ST-Link sudah terinstall
3. Cek koneksi USB

### Serial output kosong

Beberapa project tidak punya UART output. Gunakan:
- **Breakpoint debugging** dengan GDB
- **LED blinking** sebagai indicator

## Konsep yang Dipelajari

### GPIO Output (P01-P02)
- GPIO output Push-Pull (Active-HIGH) dan Open-Drain (Active-LOW)
- Timing dengan HAL_Delay(), register ODR dan BSRR
- Konsep Hi-Z dan efek kecerahan LED

### GPIO Input (P03-P06)
- Pull-UP eksternal (220Ω ke 3.3V) dan Pull-DOWN eksternal (220Ω ke GND)
- Pull-UP internal (~40kΩ) dan Pull-DOWN internal (~40kΩ)
- Falling edge / Rising edge detection

### Debounce & Pola (P07-P08)
- State machine debounce non-blocking dengan HAL_GetTick()
- GPIO Slew Rate (SPEED_FREQ_LOW/MEDIUM/HIGH) dan pola LED multi-kecepatan

### Periferal Lanjut (P09-P10)
- Rotary Encoder kuadratur 5-pin (CLK/DT/SW): dekode CW/CCW
- Keypad Matrix 4×4 (8-pin): row/column scanning algorithm
- LCD I²C 16×2 sebagai tampilan di semua percobaan

## Reference Documentation

- [STM32CubeF1 HAL Reference](https://www.st.com/resource/en/user_manual/um1850-stm32cubef1-hal-driver-user-manual-stmicroelectronics.pdf)
- [STM32F103 Datasheet](https://www.st.com/resource/en/datasheet/stm32f103c8.pdf)
- [STM32F401 Datasheet](https://www.st.com/resource/en/datasheet/stm32f401cc.pdf)
- [STM32F411 Datasheet](https://www.st.com/resource/en/datasheet/stm32f411ce.pdf)

## Tips & Best Practices

1. **Selalu pre-set output** sebelum configure GPIO untuk menghindari glitch
2. **Gunakan internal pull-up** untuk button input
3. **Debounce minimal 50ms** untuk tactile switches
4. **Test dengan LED** sebelum implementasi kompleks
5. **Dokumentasi register** saat akses langsung

---

**Last Updated:** Feb 22, 2026  
**Status:** All 10 projects verified ✓
