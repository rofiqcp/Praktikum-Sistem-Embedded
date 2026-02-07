# STM32_06: Backup Register Persistence

## Deskripsi
Demonstrasi Backup Register untuk menyimpan data yang bertahan saat Standby Mode. STM32F103 memiliki 10 register 16-bit (BKP_DR1-DR10).

## Hardware
- STM32F103C8T6 (Blue Pill)
- LED PC13
- UART1 (PA9/PA10)
- Optional: VBAT coin cell

## Konsep
- `HAL_RTCEx_BKUPWrite/Read()` - akses Backup Register
- Magic value untuk deteksi fresh boot vs warm boot
- Status flags, boot counter, timestamp, sensor data
- Data hilang hanya jika VBAT terputus

## Cara Build & Upload
```bash
pio run -t upload
pio device monitor
```
