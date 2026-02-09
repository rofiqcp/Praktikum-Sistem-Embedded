# STM32F411CEU6 LED Blink - USB DFU Upload Guide

## Project Struktur
```
f411ceu6/
├── platformio.ini       # PlatformIO config dengan DFU bootloader
├── include/
│   └── config.h         # GPIO dan timing config
└── src/
    └── main.c           # LED blink firmware
```

## Spesifikasi Hardware
- **Board**: STM32F411CEU6 (WeAct BlackPill)
- **LED**: PC13 (Built-in)
- **Blink Interval**: 500ms ON / 500ms OFF
- **Clock**: 84 MHz (HSE 25MHz -> PLL)
- **Upload Protocol**: USB DFU Bootloader

## Instruksi Upload via USB

### 1. Masukkan Board ke Mode DFU
- Tahan tombol **BOOT0** (aktif HIGH)
- Tekan tombol **RESET** (aktif LOW) sambil tetap tahan BOOT0
- Lepaskan kedua tombol
- LED akan mati, board dalam mode bootloader

### 2. Upload Firmware
```bash
cd /root/otomasi/Praktikum-Sistem-Embedded/f411ceu6
pio run -t upload
```

### 3. Verifikasi Upload Sukses
- Kembali ke mode normal (tekan RESET tanpa BOOT0)
- LED pada PC13 akan mulai berkedip dengan interval 500ms

## Build & Upload Sekali Jalan
```bash
cd /root/otomasi/Praktikum-Sistem-Embedded/f411ceu6
pio run
pio run -t upload
```

## File Firmware
- **Binary**: `.pio/build/stm32f411ce/firmware.bin` (3.88 KB)
- **ELF**: `.pio/build/stm32f411ce/firmware.elf` (138 KB)

## Troubleshooting

### Board tidak terdeteksi DFU
1. Pastikan USB cable terhubung dengan baik
2. Coba command: `dfu-util -l`
3. Jika tidak terdeteksi, tekan RESET 2-3x sambil BOOT0 ditekan

### LED tidak menyala
1. Cek polaritas LED (anode ke 3.3V, katoda ke GND via resistor)
2. Verifikasi firmware terupload dengan benar
3. Cek clock configuration di main.c

## Catatan
- Board ini menggunakan STM32CubeMX HAL library
- DFU bootloader sudah built-in di chip
- Tidak perlu STLink/JTAG untuk programming
- USB koneksi sekaligus untuk power supply (5V) dan programming
