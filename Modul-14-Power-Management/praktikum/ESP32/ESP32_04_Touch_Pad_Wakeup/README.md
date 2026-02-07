# ESP32_04: Touch Pad Wakeup

## Deskripsi
Demonstrasi wake-up dari deep sleep menggunakan touch pad capacitive sensor ESP32. Sentuhan pada pad akan membangunkan MCU.

## Hardware
- ESP32 DevKit V1
- Wire/pad pada Touch0 (GPIO4) dan Touch2 (GPIO2)

## Konsep
- `esp_sleep_enable_touchpad_wakeup()` - touch pad wake-up
- `touch_pad_init()` dan `touch_pad_config()` - konfigurasi touch
- `touch_pad_filter_start()` - filter noise
- Threshold otomatis dari pembacaan awal

## Cara Build & Upload
```bash
pio run -t upload
pio device monitor
```
