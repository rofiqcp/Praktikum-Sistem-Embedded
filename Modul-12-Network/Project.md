# Project Modul 12: Network dan IoT

## Praktikum Sistem Embedded

---

## Informasi Umum

| Item | Detail |
|------|--------|
| **Modul** | 12 — Network & IoT |
| **Platform** | ESP32 DevKit V1 *atau* STM32 Blue Pill |
| **Durasi** | 2 minggu sejak modul diberikan |
| **Tipe** | Project Mandiri |
| **Bobot Nilai** | 25% dari total nilai modul |

---

## Deskripsi Umum

Project ini mengintegrasikan **seluruh 12 percobaan** network dan IoT ke dalam satu sistem embedded terpadu. Mahasiswa diminta membangun sistem IoT end-to-end yang menghubungkan dunia fisik (sensor) ke dunia digital (dashboard, cloud, mobile).

---

## Soal Cerita: "Sistem Monitoring Greenhouse Cerdas — FarmConnect"

### Latar Belakang

Bu Ratna adalah pemilik **GreenHaven Farm**, sebuah greenhouse modern seluas 2000m² di Lembang yang menanam tomat cherry premium untuk ekspor ke Jepang. Sayarat kualitas ekspor sangat ketat — tomat harus tumbuh di lingkungan terkontrol:

- Suhu: 22-28°C (siang), 17-20°C (malam)
- Kelembaban: 60-75%
- Intensitas cahaya: 400-600 μmol/m²/s (PAR)
- pH nutrisi: 5.8-6.3
- EC nutrisi: 2.0-3.5 mS/cm
- CO₂: 800-1200 ppm

Bulan lalu, sistem HVAC mengalami malfungsi tengah malam. Suhu naik ke 38°C selama 4 jam tanpa terdeteksi. 200kg tomat cherry (senilai Rp 40 juta) mengalami sunscald dan gagal ekspor.

Bu Ratna ingin membangun **FarmConnect** — sistem monitoring greenhouse yang bisa diakses dari HP kapanpun, memberikan alert real-time, dan mengontrol peralatan secara remote. Ia menghubungi Laboratorium IoT Teknik Elektro untuk bantuan.

### Spesifikasi Greenhouse

- **4 zona tanam**, masing-masing punya sensor dan aktuator sendiri
- **Sensor:** DHT22 (suhu+humidity), LDR (cahaya), pH meter, EC meter (semua boleh disimulasikan)
- **Aktuator:** HVAC (heater+cooler), humidifier, grow light, nutrient pump
- **Konektivitas:** WiFi tersedia di area greenhouse

---

## Tugas dan Spesifikasi

Implementasikan **FarmConnect** dengan fitur-fitur berikut:

---

### Fitur 1: Network Initialization (dari Percobaan 01 — WiFi Scan / AT Init)

**ESP32:** Scan WiFi networks, pilih AP terkuat, tampilkan daftar AP.
**STM32:** Inisialisasi ESP-01 via AT commands, verifikasi komunikasi.

Sistem harus menampilkan informasi jaringan saat startup:
```
[NET] Scanning networks...
[NET] Found 5 APs — Best: GreenHaven_WiFi (RSSI: -42 dBm)
[NET] Connecting...
```

**Penilaian:** Network scan/init sukses, informasi ditampilkan.

---

### Fitur 2: WiFi Connection with Resilience (dari Percobaan 02 — WiFi Station)

Koneksi ke WiFi greenhouse dengan:
- Retry logic: coba ulang 5× jika gagal, delay meningkat (backoff)
- Auto-reconnect jika terputus
- Tampilkan IP address, RSSI, connection status

```
[WIFI] Connected! IP: 192.168.1.50 | RSSI: -45 dBm
[WIFI] Connection lost! Reconnecting... (attempt 2/5)
[WIFI] Reconnected!
```

**Penilaian:** Koneksi stabil, reconnect otomatis berjalan, status termonitor.

---

### Fitur 3: Access Point / TCP Transport (dari Percobaan 03)

**ESP32:** Buat fallback AP mode jika WiFi greenhouse tidak tersedia:
- SSID: `FarmConnect_Setup`
- Akses `192.168.4.1` untuk konfigurasi via web

**STM32:** Implementasikan TCP connection ke server untuk pengiriman data.

**Penilaian:** ESP32: AP mode berfungsi, client bisa konek. STM32: TCP transport OK.

---

### Fitur 4: TCP/UDP Data Channel (dari Percobaan 04 & 05)

Implementasikan **dua channel komunikasi**:
- **TCP channel** (reliable): untuk command & control (HVAC on/off, set-point)
- **UDP channel** (fast): untuk streaming sensor data real-time

ESP32: BSD sockets. STM32: via W5500 atau ESP-01 AT.

**Penilaian:** Kedua channel berfungsi, TCP reliable, UDP fast.

---

### Fitur 5: HTTP Dashboard (dari Percobaan 06 & 07 — HTTP Server/Client)

HTTP web server menyajikan dashboard greenhouse:

**Halaman utama (`GET /`):**
```html
<h1>FarmConnect Dashboard</h1>
<table>
  <tr><td>Zona 1</td><td>Temp: 25.3°C</td><td>Hum: 68%</td><td>Status: OK</td></tr>
  <tr><td>Zona 2</td><td>Temp: 31.2°C</td><td>Hum: 55%</td><td>Status: WARNING</td></tr>
</table>
```

**REST API:**
- `GET /api/sensors` → JSON semua data sensor
- `GET /api/zone/1` → JSON data zona tertentu
- `POST /api/control` → kontrol aktuator (JSON body: `{"zone":1, "hvac":"on"}`)

**STM32:** Web server via W5500 (HTML + JSON endpoints).

**Penilaian:** Dashboard load, sensor data ditampilkan, kontrol via API bising.

---

### Fitur 6: MQTT Telemetry (dari Percobaan 08 — MQTT)

Publish sensor data ke MQTT broker setiap 10 detik:
- **Topic publish:** `farm/zone1/temp`, `farm/zone1/humidity`, `farm/zone2/temp`, dll.
- **Payload:** JSON `{"value": 25.3, "unit": "C", "timestamp": 123456}`
- **Subscribe:** `farm/control/#` untuk menerima command dari remote

**STM32:** Konstruksi MQTT packet manual dan kirim via ESP-01 TCP.

Test dengan MQTT client di PC (mosquitto_sub/pub atau MQTTX app).

**Penilaian:** MQTT publish/subscribe berfungsi, JSON format benar, QoS configureable.

---

### Fitur 7: BLE Local Access (dari Percobaan 09 & 10 — BLE)

**ESP32:**
- BLE advertising: broadcast nama "FarmConnect" dan beacon sensor ringkasan
- GATT server: service dengan characteristic read (sensor), write (control), notify (alert)
- HP bisa konek via nRF Connect untuk monitoring tanpa WiFi

**STM32:**
- HM-10 BLE module: advertising nama "FarmConnect-STM"
- Data sensor dikirim secara transparan saat BLE connected

**Penilaian:** BLE advertising terlihat, data bisa dibaca via BLE app.

---

### Fitur 8: Real-Time Push (dari Percobaan 11 — WebSocket / UART Bridge)

**ESP32:** WebSocket server untuk push update real-time ke browser:
- Browser connect → auto-receive sensor update setiap 2 detik
- Browser kirim command → langsung dieksekusi (LED/actuator toggle)
- Multi-client: semua browser terkoneksi dapat update bersamaan

**STM32:** Custom UART binary protocol untuk komunikasi bilateral dengan ESP32 coprocessor — framing, checksum, state machine.

**Penilaian:** ESP32: push real-time ke browser, multi-client. STM32: protocol bilateral OK.

---

### Fitur 9: Alert System (integrasi multi-percobaan)

Sistem alert berdasarkan threshold sensor:
- **WARNING** (suhu > 28°C atau < 22°C): Kirim MQTT alert + LED kuning
- **CRITICAL** (suhu > 35°C atau < 15°C): Kirim MQTT alert + LED merah + BLE notify
- **EMERGENCY** (suhu > 40°C): Kirim HTTP POST alert ke API endpoint + auto-shutdown HVAC

Alert harus sampai via **minimal 2 channel** (MQTT + BLE, atau HTTP + MQTT).

**Penilaian:** Alert trigger otomatis, multi-channel delivery berfungsi.

---

### Fitur 10: Remote Control (integrasi multi-percobaan)

Aktuator bisa dikontrol dari **3 interface berbeda**:
1. **HTTP API:** `POST /api/control` dengan JSON body
2. **MQTT command:** Publish ke `farm/control/zone1`
3. **BLE write:** Write characteristic dari HP app

Semua interface harus mengontrol LED yang sama (simulasi aktuator).

**Penilaian:** LED dikontrol dari 3 interface berbeda, state konsisten.

---

### Fitur 11: Data Logging (integrasi multi-percobaan)

Log sensor data secara periodik:
- Buffer 50 reading terakhir di memory
- `GET /api/history` menampilkan data log dalam JSON array
- MQTT publish summary setiap 5 menit (rata-rata, min, max)

**Penilaian:** Log tersimpan, retrievable via HTTP dan MQTT.

---

### Fitur 12: System Status Dashboard (dari Percobaan 12 — IoT Dashboard Capstone)

Dashboard serial menampilkan status keseluruhan sistem:
```
╔══════════ FARMCONNECT STATUS ══════════╗
║ Uptime: 01:23:45  Mode: ONLINE          ║
╠═══════════════════════════════════════╣
║ WiFi: Connected (RSSI -42) IP 192.168.1.50 ║
║ MQTT: Connected to broker              ║
║ BLE: 1 client connected               ║
║ HTTP: 23 requests served              ║
║ WebSocket: 2 clients live             ║
╠═══════════════════════════════════════╣
║ Zone 1: T=25.3°C H=68% [OK]          ║
║ Zone 2: T=27.1°C H=72% [OK]          ║
║ Zone 3: T=31.2°C H=55% [WARNING]     ║
║ Zone 4: T=24.8°C H=65% [OK]          ║
╠═══════════════════════════════════════╣
║ Alerts: 3 warnings, 0 critical       ║
║ Data points logged: 450              ║
║ Free Heap: 45000 bytes               ║
╚═══════════════════════════════════════╝
```

**Penilaian:** Dashboard lengkap, semua metrik akurat, auto-refresh.

---

## Ketentuan Pengerjaan

1. **Pilih SATU platform** — ESP32 atau STM32 (dengan modul tambahan sesuai).
2. **Semua 12 fitur** harus diimplementasikan dalam satu codebase.
3. Gunakan **PlatformIO + VS Code**.
4. Sensor boleh **disimulasikan** (random + timer).
5. Harus menggunakan **minimal 3 protokol** berbeda (contoh: HTTP + MQTT + BLE).
6. Sistem harus auto-reconnect jika WiFi terputus.
7. **Keamanan:** Setidaknya gunakan password pada WiFi (WPA2). TLS opsional (bonus).

---

## Rubrik Penilaian

| Komponen | Bobot | Kriteria A (90-100) | Kriteria B (75-89) | Kriteria C (60-74) | Kriteria D (<60) |
|----------|-------|--------------------|--------------------|--------------------|--------------------|
| Network Connectivity | 20% | WiFi STA + AP fallback + auto-reconnect | WiFi STA + reconnect | WiFi connect saja | Tidak connect |
| Protocol Implementation | 25% | HTTP + MQTT + BLE + WebSocket (4 protokol) | 3 protokol berfungsi | 2 protokol | ≤ 1 protokol |
| Dashboard & Control | 20% | Web dashboard + REST API + multi-channel control | Dashboard + 2 control channels | Dashboard basic | Tanpa dashboard |
| Alert & Monitoring | 15% | Multi-channel alert + threshold config + logging | Alert + logging | Alert saja | Tanpa alert |
| System Integration | 20% | Semua 12 fitur terintegrasi, demo lancar | 10 fitur | 7-9 fitur | < 7 fitur |

---

## Referensi

1. ESP-IDF WiFi Guide — Espressif Systems
2. ESP-IDF MQTT Client — Espressif Systems
3. ESP-IDF BLE Reference — Espressif Systems
4. W5500 Datasheet — WIZnet
5. ESP8266 AT Instruction Set — Espressif Systems
6. MQTT Specification 3.1.1 — OASIS
7. Kolban's Book on ESP32 — Neil Kolban

---

*Project Modul 12 — Network & IoT | Praktikum Sistem Embedded | 2025/2026*
