# ESP32_11: Solar Weather Station (Integrasi Lengkap)

## Deskripsi
Program integrasi semua konsep power management: deep sleep, RTC memory batching, adaptive duty cycling, dan battery monitoring. Simulasi weather station bertenaga surya.

## Hardware
- ESP32 DevKit V1
- ADC GPIO34 (battery monitor via voltage divider)
- LED GPIO2

## Konsep
- Deep sleep + timer wake-up
- RTC memory untuk data batching (10 readings per transmit)
- Adaptive sleep berdasarkan level baterai
- Statistik uptime/sleep tracking
- Simulasi sensor read + data transmit

## Alur Kerja
1. Bangun → Baca sensor → Simpan ke RTC buffer
2. Jika buffer penuh → Transmit semua data
3. Sesuaikan sleep duration
4. Masuk deep sleep → Ulangi

## Cara Build & Upload
```bash
pio run -t upload
pio device monitor
```
