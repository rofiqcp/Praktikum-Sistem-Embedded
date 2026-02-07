# STM32_02: Stop Mode + EXTI Wakeup

## Deskripsi
Demonstrasi Stop Mode STM32F103. Semua clock OFF (HSE, HSI, PLL). Bangun oleh EXTI. **PENTING**: Clock kembali ke HSI 8MHz setelah wake-up, harus rekonfigurasi!

## Hardware
- STM32F103C8T6 (Blue Pill)
- LED PC13, Button PA0
- UART1 (PA9/PA10, 115200 baud)

## Konsep
- `HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI)`
- Rekonfigurasi `SystemClock_Config()` setelah wake-up
- Re-init UART karena clock berubah
- Konsumsi: ~20µA

## Cara Build & Upload
```bash
pio run -t upload
pio device monitor
```
