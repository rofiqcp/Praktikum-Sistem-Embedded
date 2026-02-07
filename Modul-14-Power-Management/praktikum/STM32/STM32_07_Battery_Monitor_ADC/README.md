# STM32_07: Battery Monitor ADC

## Deskripsi
Monitoring tegangan baterai Li-Ion dengan ADC + kalibrasi VREFINT internal. Menampilkan level baterai dengan visual bar.

## Hardware
- STM32F103C8T6 (Blue Pill)
- Voltage divider: VBAT → R1(100k) → PA1 → R2(100k) → GND
- LED PC13
- UART1 (PA9/PA10)

## Konsep
- VREFINT (1.2V internal) untuk kalibrasi VDDA
- `HAL_ADCEx_Calibration_Start()` - kalibrasi ADC
- Oversampling 16x untuk noise reduction
- Visual bar + persentase level baterai

## Cara Build & Upload
```bash
pio run -t upload
pio device monitor
```
