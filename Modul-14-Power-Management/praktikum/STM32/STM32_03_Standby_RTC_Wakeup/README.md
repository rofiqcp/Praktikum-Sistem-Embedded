# STM32_03: Standby Mode + RTC Alarm Wakeup

## Deskripsi
Demonstrasi Standby Mode (daya terendah). 1.8V domain MATI total, SRAM hilang. Hanya RTC + Backup Register bertahan. Bangun oleh RTC Alarm.

## Hardware
- STM32F103C8T6 (Blue Pill)
- LED PC13
- UART1 (PA9/PA10, 115200 baud)
- RTC menggunakan LSI internal

## Konsep
- `HAL_PWR_EnterSTANDBYMode()` - standby, MCU RESET saat bangun
- RTC Alarm sebagai wake-up source
- Backup Register (BKP_DR) untuk menyimpan boot counter
- Konsumsi: ~2µA

## Cara Build & Upload
```bash
pio run -t upload
pio device monitor
```
