# ESP32_02: Light Sleep GPIO Wake-up

## Deskripsi
Demonstrasi light sleep mode ESP32 dengan GPIO wake-up (tombol BOOT). Berbeda dari deep sleep, light sleep mempertahankan state CPU dan RAM.

## Hardware
- ESP32 DevKit V1
- Tombol BOOT (GPIO0)
- LED GPIO2

## Konsep
- `esp_light_sleep_start()` - masuk light sleep
- `esp_sleep_enable_gpio_wakeup()` - GPIO wake-up
- CPU dan RAM tetap utuh setelah wake-up
- Konsumsi: ~0.8mA (vs ~10µA deep sleep)

## Cara Build & Upload
```bash
pio run -t upload
pio device monitor
```
