# STM32_04: Wakeup Pin + Standby Mode

## Deskripsi
Demonstrasi wake-up dari Standby menggunakan WKUP pin (PA0). WKUP pin adalah fitur khusus PWR yang berbeda dari EXTI — hanya rising edge.

## Hardware
- STM32F103C8T6 (Blue Pill)
- Button PA0 (pull-down, rising edge ke VCC)
- LED PC13
- UART1 (PA9/PA10)

## Konsep
- `HAL_PWR_EnableWakeUpPin(PWR_WAKEUP_PIN1)` - enable PA0 WKUP
- Rising edge ONLY pada PA0
- Boot counter via Backup Register
- Berbeda dari EXTI: WKUP bisa bangunkan dari Standby

## Cara Build & Upload
```bash
pio run -t upload
pio device monitor
```
