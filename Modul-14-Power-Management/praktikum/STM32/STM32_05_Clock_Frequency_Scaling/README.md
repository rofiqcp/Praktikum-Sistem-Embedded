# STM32_05: Clock Frequency Scaling

## Deskripsi
Demonstrasi penghematan daya dengan mengubah frekuensi clock: 72MHz (PLL), 8MHz (HSE), 8MHz (HSI). Benchmark performa pada setiap frekuensi.

## Hardware
- STM32F103C8T6 (Blue Pill)
- LED PC13
- UART1 (PA9/PA10)

## Konsep
- Switch SYSCLK: PLL (72MHz) → HSE (8MHz) → HSI (8MHz)
- Matikan PLL saat tidak dipakai
- Benchmark komputasi pada setiap frekuensi
- 72MHz ~30mA, 8MHz HSE ~10mA, 8MHz HSI ~8mA

## Cara Build & Upload
```bash
pio run -t upload
pio device monitor
```
