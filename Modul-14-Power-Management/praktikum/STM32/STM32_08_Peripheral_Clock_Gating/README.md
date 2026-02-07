# STM32_08: Peripheral Clock Gating

## Deskripsi
Demonstrasi penghematan daya dengan menonaktifkan clock peripheral yang tidak digunakan. Setiap peripheral menambah ~0.5-2mA.

## Hardware
- STM32F103C8T6 (Blue Pill)
- LED PC13
- UART1 (PA9/PA10)

## Konsep
- `__HAL_RCC_XXX_CLK_ENABLE/DISABLE()` - kontrol clock peripheral
- Monitoring register RCC (APB1ENR, APB2ENR, AHBENR)
- Enable/disable SPI, I2C, USART, TIM, ADC
- Tabel status peripheral ON/OFF

## Cara Build & Upload
```bash
pio run -t upload
pio device monitor
```
