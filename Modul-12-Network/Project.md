# Project Modul 13: Network & IoT

## Project 1: Smart Home Controller

### Deskripsi
Bangun sistem Smart Home Controller menggunakan ESP32 yang menggabungkan WiFi, MQTT, HTTP dashboard, dan kontrol aktuator. Sistem dapat memonitor kondisi ruangan (suhu, cahaya) dan mengontrol perangkat (LED, relay) dari jarak jauh.

### Diagram Blok
```
┌─────────┐     WiFi      ┌──────────┐     MQTT     ┌──────────┐
│ Sensor  │───────────────>│  ESP32   │─────────────>│  Broker  │
│ DHT22   │               │(STA Mode)│              │Mosquitto │
│ LDR     │               │          │<─────────────│          │
└─────────┘               │ HTTP     │   Subscribe  └──────────┘
                          │ Server   │                    │
┌─────────┐               │ REST API │              ┌──────────┐
│ Actuator│<──────────────│          │              │Dashboard │
│ LED/Relay│              └──────────┘              │ Web/App  │
└─────────┘                                         └──────────┘
```

### Fitur Minimum
1. Baca sensor DHT22 (suhu + kelembaban) setiap 5 detik
2. Publish data sensor ke MQTT broker (topic: `home/sensor/...`)
3. HTTP server dengan halaman HTML dashboard interaktif
4. REST API: GET /api/sensor, POST /api/control
5. Kontrol LED/relay via MQTT subscribe atau HTTP POST
6. Alarm otomatis jika suhu > threshold

### Kriteria Penilaian
| Komponen | Bobot | Deskripsi |
|----------|-------|-----------|
| Fungsionalitas | 40% | Semua fitur berjalan |
| Kode Program | 25% | Bersih, terstruktur, terdokumentasi |
| Dokumentasi | 20% | Laporan, diagram, screenshot |
| Presentasi | 15% | Demonstrasi dan penjelasan |

---

## Project 2: BLE Sensor Network

### Deskripsi
Bangun jaringan sensor menggunakan BLE GATT. Satu ESP32 sebagai GATT server (peripheral) yang mengiklankan data sensor, dan smartphone sebagai central yang membaca data dan mengontrol aktuator.

### Fitur Minimum
1. BLE GATT server dengan custom service
2. Characteristic untuk: sensor read (notify), LED write, device info
3. Advertising dengan device name dan service UUID
4. Smartphone app (nRF Connect) dapat read/write/subscribe
5. Data sensor dikirim via BLE notification setiap 2 detik
6. LED dikontrol via BLE write characteristic

---

## Project 3: Industrial IoT Gateway (STM32)

### Deskripsi
Bangun gateway IoT menggunakan STM32 + ESP-01 atau W5500. STM32 membaca sensor, mengemas data, dan mengirim ke cloud melalui modul external.

### Diagram Blok
```
┌─────────┐     ADC      ┌──────────┐    UART/SPI   ┌──────────┐
│ Sensor  │──────────────>│  STM32   │──────────────>│ ESP-01 / │
│ Temp    │               │  F103    │               │ W5500    │
│ Pot     │               │  HAL     │               │          │
└─────────┘               └──────────┘               └──────────┘
                                                          │
                                                     WiFi/Ethernet
                                                          │
                                                     ┌──────────┐
                                                     │  Cloud   │
                                                     │  MQTT    │
                                                     └──────────┘
```

### Fitur Minimum
1. STM32 membaca minimal 2 sensor (ADC channels)
2. Data dikemas dalam format JSON
3. Dikirim via ESP-01 (AT commands + TCP/MQTT) atau W5500 (Ethernet)
4. Retry logic jika koneksi gagal
5. LED indikator status (connected/error/sending)
6. Periodic reporting setiap 10 detik
7. Python dashboard untuk monitoring
