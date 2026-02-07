# STM32_01: Sleep Mode WFI (Wait For Interrupt)

## Deskripsi
Demonstrasi Sleep Mode STM32F103. CPU berhenti, semua peripheral tetap aktif. Bangun dengan interrupt (EXTI button PA0).

## Hardware
- STM32F103C8T6 (Blue Pill)
- LED PC13 (active low)
- Button PA0 (pull-up, falling edge)
- UART1 (PA9=TX, PA10=RX, 115200 baud)

## Konsep
- `HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI)`
- `HAL_SuspendTick()` / `HAL_ResumeTick()`
- EXTI0 interrupt untuk wake-up
- Konsumsi: ~2mA (vs ~30mA aktif)

## Cara Build & Upload
```bash
pio run -t upload
pio device monitor
```
