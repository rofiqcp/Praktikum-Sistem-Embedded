# ESP32_03: Ext0 & Ext1 Wakeup Sources

## Deskripsi
Demonstrasi multiple wake-up sources: EXT0 (single GPIO), EXT1 (multiple GPIO), dan timer. Program mendeteksi sumber wake-up yang aktif.

## Hardware
- ESP32 DevKit V1
- 2 push button pada RTC GPIO (GPIO25, GPIO26)
- LED GPIO2

## Konsep
- `esp_sleep_enable_ext0_wakeup()` - single RTC GPIO
- `esp_sleep_enable_ext1_wakeup()` - multiple RTC GPIO (bitmask)
- `esp_sleep_get_wakeup_cause()` - deteksi sumber wake-up
- Kombinasi timer + GPIO wake-up

## Cara Build & Upload
```bash
pio run -t upload
pio device monitor
```
