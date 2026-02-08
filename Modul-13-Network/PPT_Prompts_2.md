# Prompts untuk Pembuatan PPT — Modul 13 (Bagian 2: Praktikum)

## Slide 1: Judul
**MODUL 13: Network & IoT — Panduan Praktikum**

## Slide 2: Daftar Percobaan
- 12 percobaan ESP32 (native WiFi/BLE)
- 12 percobaan STM32 (via modul external)
- Total: 24 percobaan + 3 project

## Slide 3-4: Percobaan 1-3 ESP32 WiFi
- WiFi Scan: screenshot serial output, tabel AP
- WiFi Station: diagram koneksi, event handler flow
- WiFi AP: smartphone connect ke ESP32

## Slide 5-6: Percobaan 4-5 TCP/UDP
- TCP echo server: diagram client-server
- UDP broadcast: diagram komunikasi
- Demo: terminal netcat

## Slide 7-8: Percobaan 6-7 HTTP
- HTTP Server: screenshot dashboard HTML
- HTTP Client: GET request flow
- Code walkthrough: handler registration

## Slide 9-10: Percobaan 8 MQTT
- MQTT architecture diagram
- Screenshot MQTT Explorer
- Topic hierarchy example
- QoS comparison

## Slide 11-12: Percobaan 9-10 BLE
- BLE advertising: screenshot nRF Connect
- GATT server: service/characteristic diagram
- Read/Write/Notify demo

## Slide 13: Percobaan 11 WebSocket
- Real-time dashboard demo
- WebSocket vs HTTP polling comparison

## Slide 14: Percobaan 12 IoT Dashboard
- Full system diagram
- Screenshot dashboard
- MQTT data flow

## Slide 15-16: STM32 + ESP-01
- Wiring diagram ESP-01 ke STM32
- AT command sequence
- WiFi connect → TCP → HTTP → MQTT

## Slide 17-18: STM32 + W5500
- Wiring diagram W5500 SPI
- Ethernet init → TCP server → HTTP server
- SPI register access

## Slide 19: STM32 + HM-10 BLE
- Wiring diagram HM-10
- AT commands untuk BLE

## Slide 20: Python Debug Scripts
- Cara menjalankan debug_analysis.py
- Contoh visualisasi matplotlib
- Demo real-time monitoring

## Slide 21: Project Ideas
- Smart Home Controller
- BLE Sensor Network
- Industrial IoT Gateway

## Slide 22: Tips & Best Practices
- Gunakan event-driven programming
- Handle error dan timeout
- Test dengan Python scripts dulu
- Keamanan: WPA2, TLS

## Slide 23: Tugas & Deadline
