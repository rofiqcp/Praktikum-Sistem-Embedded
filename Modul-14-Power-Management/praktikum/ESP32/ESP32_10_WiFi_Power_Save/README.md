# ESP32_10: WiFi Power Save Mode

## Deskripsi
Demonstrasi mode penghematan daya WiFi: NONE (~120mA), MIN_MODEM (~20mA), MAX_MODEM (~15mA). Benchmark pada setiap mode.

## Hardware
- ESP32 DevKit V1
- Akses WiFi (ubah SSID/Password di kode)

## Konsep
- `esp_wifi_set_ps()` - set power save mode
- WIFI_PS_NONE, WIFI_PS_MIN_MODEM, WIFI_PS_MAX_MODEM
- Trade-off latency vs power consumption
- Deep sleep sebagai comparison (WiFi OFF = ~10µA)

## PENTING
Ubah `WIFI_SSID` dan `WIFI_PASS` di `src/main.c` sesuai jaringan anda!

## Cara Build & Upload
```bash
pio run -t upload
pio device monitor
```
