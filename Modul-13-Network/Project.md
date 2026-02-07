# Project Modul 13: IoT Weather Station & Smart Control Dashboard

## 🎯 Deskripsi Proyek
Buatlah sistem IoT sederhana yang mensimulasikan "Weather Station" yang dapat dimonitor dan dikontrol dari jarak jauh menggunakan protokol MQTT.

## 📋 Spesifikasi Sistem

### 1. Hardware
- ESP32 atau STM32 + W5500.
- LED (sebagai aktuator).
- Sensor Dummy (Variable Random) atau Real Sensor (DHT11/Potensiometer).

### 2. Protokol & Koneksi
- **Koneksi**: WiFi (ESP32) atau Ethernet (STM32).
- **Protokol**: MQTT (Wajib).
- **Format Data**: JSON.

### 3. Fitur Utama
1.  **Telemetry (Publish)**:
    - Perangkat mengirim data sensor setiap 5 detik ke topik: `iot/project/nim_anda/telemetry`.
    - Format JSON: 
      ```json
      {
        "status": "online",
        "temperature": 28.5,
        "humidity": 60,
        "led_status": 1
      }
      ```
2.  **Remote Control (Subscribe)**:
    - Perangkat bisa dikontrol via topik: `iot/project/nim_anda/command`.
    - Payload "ON" -> Menyalakan LED.
    - Payload "OFF" -> Mematikan LED.
3.  **Visualisasi**:
    - Gunakan aplikasi Smartphone (MQTT Dash / IoT MQTT Panel) atau Web Client (HiveMQ Website) untuk membuat Dashboard.
    - Tampilkan Gauge suhu dan Switch untuk LED.

## 🛠️ Langkah Pengerjaan
1.  Setup Library (WiFi/Ethernet & PubSubClient/ArduinoJson).
2.  Buat koneksi ke Broker MQTT public (misal: `broker.hivemq.com`).
3.  Implementasi fungsi Publish data sensor dummy secara periodik (non-blocking, gunakan `millis()`).
4.  Implementasi Callback function untuk menangani pesan masuk (Subscribe) dan kontrol LED.
5.  Setup Dashboard di HP/Browser.

## 📝 Format Laporan
1.  **Diagram Blok Sistem**: Alur data dari Sensor -> MCU -> Broker -> Dashboard.
2.  **Flowchart Program**: Logika koneksi WiFi, reconnect MQTT, dan loop utama.
3.  **Source Code**: Full code dengan komentar.
4.  **Dokumentasi**: Foto/Screenshot Dashboard saat menampilkan data real-time dan saat mengontrol LED.
5.  **Analisa**: Jelaskan apa yang terjadi jika koneksi internet terputus? Bagaimana mekanisme *reconnect* bekerja?

## 🌟 Tantangan (Opsional - Nilai Tambah)
- Tambahkan fitur "Last Will and Testament" (LWT) MQTT agar dashboard tahu jika device offline mendadak.
- Implementasikan SSL/TLS untuk koneksi MQTT yang aman (port 8883).
