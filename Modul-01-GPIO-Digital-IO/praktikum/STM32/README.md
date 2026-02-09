# STM32 GPIO Digital I/O - 12 Praktikum

Dokumentasi lengkap untuk 12 praktikum GPIO pada STM32 dengan dukungan 3 jenis mikrokontroler.

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

## Daftar 12 Project

| # | Project | Fungsi |
|---|---------|--------|
| 1 | **STM32_01_LED_Blink** | LED berkedip dasar (500ms) |
| 2 | **STM32_02_Multi_LED_Running** | 4 LED bergeser berurutan |
| 3 | **STM32_03_LED_Binary_Counter** | Hitung biner 0-15 pada 4 LED |
| 4 | **STM32_04_Button_Debounce** | Tombol dengan debounce state machine |
| 5 | **STM32_05_Long_Short_Press** | Deteksi tekan panjang vs pendek |
| 6 | **STM32_06_Toggle_Latch** | Toggle LED di tekan tombol |
| 7 | **STM32_07_GPIO_Drive_Strength** | Test kekuatan drive GPIO |
| 8 | **STM32_08_DIP_Switch_Reader** | Baca DIP switch 4-bit |
| 9 | **STM32_09_GPIO_Port_Register** | Akses register GPIO langsung |
| 10 | **STM32_10_GPIO_Matrix_Keypad** | Scanning keypad matrix 4x4 |
| 11 | **STM32_11_Emergency_Stop** | Tombol emergency dengan interrupt |
| 12 | **STM32_12_LED_Test_Pattern** | Pola diagnostik LED |

## Hardware Configuration

### Pin Mapping (PA, PB, PC)

```
Port A (PA):
  PA0-PA3   → LED atau Row (Keypad)
  PA4-PA7   → (Reserved untuk ekspansi)

Port B (PB):
  PB0       → Button/DIP Switch/Col (Keypad)
  PB1       → Button/DIP Switch/Col (Keypad)
  PB3       → DIP Switch/Col (Keypad) [PB2 = BOOT1, dihindari]
  PB4       → DIP Switch/Col (Keypad)

Port C (PC):
  PC13      → Built-in LED (active-low)
```

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
STM32_XX_ProjectName/
├── platformio.ini          ← Config environment & MCU
├── include/
│   └── config.h            ← Hardware definition
├── src/
│   └── main.c              ← Program utama
├── .gitignore
└── README.md               ← Project-specific docs
```

## Building Instructions

### Dari Terminal

```bash
# Navigate ke project
cd STM32_01_LED_Blink

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

### Basic GPIO (01-03)
- GPIO output configuration
- Timing dengan HAL_Delay()
- Bit manipulation & bitwise operations

### Input & Debouncing (04-06)
- GPIO input dengan pull-up
- State machine debouncing
- Edge detection
- Toggle logic

### Advanced GPIO (07-10)
- Drive strength configuration
- Register access (IDR, ODR, BSRR)
- Matrix multiplexing
- Scanning algorithm

### Safety & Testing (11-12)
- Interrupt handling
- Safety interlock
- Diagnostic patterns

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

**Last Updated:** Feb 9, 2026  
**Status:** All 12 projects verified ✓
