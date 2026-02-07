# ESP32_07: Battery Monitor (ADC + eFuse Calibration)

## Deskripsi
Monitoring tegangan baterai Li-Ion menggunakan ADC dengan kalibrasi eFuse. Menampilkan level baterai dalam persentase dengan visual bar.

## Hardware
- ESP32 DevKit V1
- Voltage divider: VBAT → R1(100k) → GPIO34 → R2(100k) → GND
- LED GPIO2

## Konsep
- `esp_adc_cal_characterize()` - kalibrasi ADC via eFuse
- Lookup table untuk kurva discharge Li-Ion
- Oversampling 16x untuk noise reduction
- Visual bar display di serial monitor

## Cara Build & Upload
```bash
pio run -t upload
pio device monitor
```
