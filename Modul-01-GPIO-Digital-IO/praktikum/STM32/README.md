# STM32 GPIO Digital I/O - 10 Percobaan

Dokumentasi lengkap untuk 10 percobaan GPIO pada STM32 dengan dukungan 3 jenis mikrokontroler.

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

## Daftar 10 Percobaan

| # | Project | Nama Percobaan | Fungsi |
|---|---------|----------------|--------|
| 1 | **STM32_P01_LED_Output_High** | LED Parade | GPIO Output Push-Pull & Pola Cahaya Digital |
| 2 | **STM32_P02_LED_Output_Low** | Shadow & Ghost | Active-LOW, Open-Drain & Logika Terbalik |
| 3 | **STM32_P03_Button_PullUp_Ext** | Sentinel Gate | Tombol Pull-UP Eksternal (220Ω ke 3.3V) |
| 4 | **STM32_P04_Button_PullDown_Ext** | Ground Guardian | Tombol Pull-DOWN Eksternal (220Ω ke GND) |
| 5 | **STM32_P05_Button_PullUp_Internal** | Phantom Touch | Pull-UP Internal & Tombol Tanpa Resistor |
| 6 | **STM32_P06_Button_PullDown_Internal** | Force Field | Pull-DOWN Internal & Logika Active-HIGH |
| 7 | **STM32_P07_Button_Debounce** | Clean Contact | Debounce State Machine & Penghitung Akurat |
| 8 | **STM32_P08_LED_Patterns** | Speed Racer | GPIO Slew Rate & Pola LED Multi-Kecepatan |
| 9 | **STM32_P09_Encoder_5Pin** | Twist & Count | Rotary Encoder Kuadratur & Counter LCD |
| 10 | **STM32_P10_Keypad_8Pin** | Matrix Commander | Pemindaian Keypad 4×4 & Tampilan LCD |

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
STM32_Pxx_NamaPercobaan/
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

### Output GPIO (P01–P02)
- GPIO output configuration (Push-Pull & Open-Drain)
- Active-HIGH vs Active-LOW
- Bit manipulation & bitwise operations

### Input & Pull Resistor (P03–P06)
- GPIO input dengan pull-up/pull-down eksternal & internal
- Edge detection

### Debounce & Patterns (P07–P08)
- State machine debouncing
- GPIO Slew Rate (speed configuration)
- LED patterns

### Encoder & Keypad (P09–P10)
- Rotary encoder kuadratur (CW/CCW)
- Register access (IDR, ODR, BSRR)
- Matrix keypad scanning

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
**Status:** All 10 percobaan verified ✓
