# Program 01: ESP32 Deep Sleep + Timer Wake-up

## Deskripsi
Demonstrasi mode deep sleep ESP32 dengan timer wake-up menggunakan ESP-IDF API.
Boot count disimpan di RTC memory untuk tracking siklus sleep.

## Hardware
- ESP32 DevKit V1
- LED Built-in (GPIO2)

## Build & Upload
```
pio run -t upload
pio device monitor
```

## Key Concepts
- `esp_deep_sleep_start()` — masuk deep sleep
- `esp_sleep_enable_timer_wakeup()` — konfigurasi timer wake-up
- `RTC_DATA_ATTR` — variabel di RTC memory (bertahan saat deep sleep)
- `esp_sleep_get_wakeup_cause()` — cek alasan bangun
- Konsumsi daya: ~10 µA dalam deep sleep
