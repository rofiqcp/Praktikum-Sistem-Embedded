# ESP32_06: RTC Memory Persistence

## Deskripsi
Demonstrasi penyimpanan data di RTC memory yang bertahan selama deep sleep. Program menyimpan array sensor data (suhu, kelembaban) dan statistik.

## Hardware
- ESP32 DevKit V1
- LED GPIO2

## Konsep
- `RTC_DATA_ATTR` - variabel di RTC slow memory
- Buffer sirkuler untuk data logging
- Statistik (min/max/avg) dari data tersimpan
- Data hilang hanya saat power-off total

## Cara Build & Upload
```bash
pio run -t upload
pio device monitor
```
