# ESP32_08: Hibernation Mode (~5µA)

## Deskripsi
Demonstrasi hibernation mode, mode daya terendah ESP32. Semua power domain dimatikan kecuali RTC timer. RTC memory tidak dipertahankan.

## Hardware
- ESP32 DevKit V1
- LED GPIO2

## Konsep
- `esp_sleep_pd_config()` - matikan power domain
- RTC_SLOW_MEM, RTC_FAST_MEM, RTC_PERIPH = OFF
- Hanya RTC timer wake-up yang tersedia
- Konsumsi: ~5µA (vs ~10µA deep sleep biasa)

## Cara Build & Upload
```bash
pio run -t upload
pio device monitor
```
