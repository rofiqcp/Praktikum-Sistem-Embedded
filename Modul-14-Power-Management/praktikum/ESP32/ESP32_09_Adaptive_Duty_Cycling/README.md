# ESP32_09: Adaptive Duty Cycling

## Deskripsi
Sleep duration diatur secara dinamis berdasarkan level baterai. Baterai tinggi = sampling sering, baterai rendah = sleep lebih lama.

## Hardware
- ESP32 DevKit V1
- Voltage divider pada GPIO34
- LED GPIO2

## Konsep
- Adaptive sleep: 30s (>80%) / 60s (40-80%) / 120s (20-40%) / 300s (<20%)
- RTC_DATA_ATTR untuk tracking state antar siklus
- ADC battery monitoring + duty cycle adaptation
- Kritis untuk perangkat IoT bertenaga baterai

## Cara Build & Upload
```bash
pio run -t upload
pio device monitor
```
