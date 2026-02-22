# Prompts untuk Pembuatan PPT — Modul 13 (Bagian 1: Teori)

## Slide 1: Judul
**MODUL 13: Network & IoT — Konektivitas Jaringan**
Praktikum Sistem Embedded | [Nama Institusi] | [Semester/Tahun]

## Slide 2: Tujuan Pembelajaran
- Memahami arsitektur IoT dan protokol jaringan
- Mengimplementasikan WiFi, TCP/UDP, HTTP, MQTT, BLE
- Membandingkan kemampuan network ESP32 vs STM32
- Membangun sistem IoT end-to-end

## Slide 3: Arsitektur IoT
- Diagram 4 layer: Device → Network → Cloud → Application
- Contoh: Sensor DHT22 → ESP32 → MQTT → Dashboard

## Slide 4: Model TCP/IP
- 4 layer: Application, Transport, Internet, Network Access
- Mapping dengan protokol: HTTP/MQTT, TCP/UDP, IP, WiFi/Ethernet

## Slide 5-6: WiFi Fundamentals
- IEEE 802.11 b/g/n, frekuensi 2.4GHz
- Mode: Station, AP, STA+AP
- RSSI dan kualitas sinyal
- Code snippet: esp_wifi_scan_start(), esp_wifi_connect()

## Slide 7-8: TCP vs UDP
- TCP: connection-oriented, reliable, 3-way handshake
- UDP: connectionless, fast, low overhead
- Tabel perbandingan
- Code snippet: socket(), bind(), listen(), accept()

## Slide 9-10: HTTP & REST API
- Request-Response model
- Methods: GET, POST, PUT, DELETE
- Status codes: 200, 404, 500
- Code snippet: httpd_start(), esp_http_client

## Slide 11-13: MQTT Protocol
- Publish/Subscribe architecture
- Broker, topic hierarchy, wildcards (+, #)
- QoS 0, 1, 2
- Last Will Testament (LWT)
- Code snippet: esp_mqtt_client_init()

## Slide 14-16: Bluetooth Low Energy (BLE)
- BLE vs Bluetooth Classic
- GAP: Advertising, Scanning, Connecting
- GATT: Services, Characteristics, Descriptors
- Code snippet: esp_ble_gap_start_advertising()

## Slide 17: WebSocket
- Full-duplex vs HTTP request-response
- Upgrade handshake
- Use case: real-time monitoring

## Slide 18-19: ESP32 vs STM32 Network
- Tabel perbandingan lengkap
- ESP32: native WiFi/BLE
- STM32: via ESP-01, W5500, HM-10

## Slide 20: Keamanan IoT
- WPA2, TLS/SSL, certificate
- Best practices

## Slide 21: Ringkasan & Pertanyaan
