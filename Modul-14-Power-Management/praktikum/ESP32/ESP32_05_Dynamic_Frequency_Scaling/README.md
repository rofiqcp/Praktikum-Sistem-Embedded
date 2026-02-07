# ESP32_05: Dynamic Frequency Scaling

## Deskripsi
Demonstrasi pengubahan frekuensi CPU ESP32 (240/160/80 MHz) dengan benchmark performa dan pengukuran konsumsi daya pada setiap frekuensi.

## Hardware
- ESP32 DevKit V1
- LED GPIO2

## Konsep
- `esp_pm_configure()` - konfigurasi power management
- Benchmark pada 240MHz, 160MHz, 80MHz
- Clock gating peripheral (`periph_module_disable/enable`)
- Trade-off performa vs konsumsi daya

## Cara Build & Upload
```bash
pio run -t upload
pio device monitor
```
