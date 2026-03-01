# Tugas Video Modul 12: Network dan IoT

## Praktikum Sistem Embedded

---

## Informasi Umum

| Item | Detail |
|------|--------|
| **Modul** | 12 — Network & IoT |
| **Tipe Tugas** | Video Laporan Praktikum |
| **Durasi Video** | 20–35 menit |
| **Deadline** | 1 minggu setelah praktikum |
| **Format** | MP4, resolusi minimal 720p |
| **Upload** | Google Drive / YouTube (unlisted) — submit link |

---

## Deskripsi

Buat video laporan yang mencakup **penjelasan materi**, **demonstrasi seluruh 12 percobaan**, dan **demo project** FarmConnect. Video harus menunjukkan pemahaman network protocols dan IoT system integration.

---

## Ketentuan Teknis

1. **Screen recording** dengan **webcam** terlihat (picture-in-picture, minimal 15% layar).
2. **Narasi suara** menjelaskan setiap bagian — bukan hanya menunjukkan kode/output.
3. Tunjukkan **kode sumber**, **proses build**, **serial monitor output**, dan **browser/app** untuk network testing.
4. Setiap percobaan harus ditunjukkan **hasilnya berjalan** — contoh: browser load halaman, MQTT message di client, BLE connect di HP.
5. Wajib ada **analisis** — tidak cukup hanya menunjukkan "berhasil".

---

## Struktur Video

### 1. Pembukaan (1–2 menit)
- Perkenalan: nama, NIM, kelas
- Overview Modul 12: network connectivity dan IoT
- Sebutkan platform yang digunakan (ESP32/STM32 + modul apa saja)

### 2. Penjelasan Materi (3–5 menit)
- WiFi: STA vs AP mode, DHCP, RSSI
- TCP vs UDP: reliable vs fast, use case masing-masing
- HTTP: request/response, REST API, method (GET/POST)
- MQTT: publish/subscribe, broker, topic, QoS
- BLE: advertising, GATT, service/characteristic
- WebSocket: full-duplex, real-time push

### 3. Demonstrasi Percobaan (10–18 menit)

| No | Percobaan | Fokus Demonstrasi |
|----|-----------|-------------------|
| 01 | WiFi Scan / AT Init | AP list (ESP32) atau AT response (STM32) |
| 02 | WiFi Station | Connect, get IP, RSSI monitoring |
| 03 | AP / TCP Client | Soft-AP (ESP32) atau TCP send/receive (STM32) |
| 04 | TCP Socket / HTTP GET | Multi-client echo (ESP32) atau HTTP response (STM32) |
| 05 | UDP / Ethernet Init | Broadcast (ESP32) atau W5500 PHY status (STM32) |
| 06 | HTTP Server / TCP Server | Web page + REST API (ESP32) atau echo server (STM32) |
| 07 | HTTP Client / UDP W5500 | GET response parse (ESP32) atau UDP exchange (STM32) |
| 08 | MQTT / HTTP Server W5500 | Pub/sub + MQTTX tool (ESP32) atau web page (STM32) |
| 09 | BLE Advertising / UART Bridge | nRF Connect scan (ESP32) atau framing protocol (STM32) |
| 10 | BLE GATT / HM-10 | Read/write/notify (ESP32) atau transparent mode (STM32) |
| 11 | WebSocket / MQTT Manual | Real-time push (ESP32) atau packet hex dump (STM32) |
| 12 | IoT Dashboard | Full system: sensor → MQTT + HTTP dashboard |

### 4. Demo Project FarmConnect (4–6 menit)
- Arsitektur: diagram sensor → processing → multi-protocol publishing
- WiFi connection + auto-reconnect demo
- HTTP dashboard di browser — sensor data + kontrol
- MQTT publish/subscribe di MQTTX app
- BLE access dari HP (nRF Connect)
- Alert trigger demo: threshold crossed → multi-channel notification
- Remote control dari 3+ interface berbeda
- Tunjukkan **minimal 8 dari 12 fitur** berjalan

### 5. Penutup (1–2 menit)
- Kesimpulan: perbandingan protokol untuk IoT (HTTP vs MQTT vs BLE vs WebSocket)
- Pertimbangan keamanan dan reliability
- Refleksi keseluruhan mata kuliah (Modul 01-12)

---

## Penilaian

| Komponen | Bobot | Keterangan |
|----------|-------|------------|
| Penjelasan Materi | 15% | Akurasi konsep WiFi, TCP/UDP, HTTP, MQTT, BLE |
| Demonstrasi Percobaan | 40% | Semua 12 percobaan ditunjukkan dan dianalisis |
| Demo Project | 25% | Fitur berjalan, multi-protocol terlihat |
| Kualitas Video | 10% | Audio jelas, visual terbaca, webcam terlihat |
| Analisis & Pemahaman | 10% | Mampu menjelaskan "mengapa", bukan hanya "apa" |

---

## Penalti

| Pelanggaran | Pengurangan |
|-------------|-------------|
| Webcam tidak terlihat | −10% |
| Tidak ada narasi suara | −15% |
| Percobaan tidak dijalankan (hanya tampil kode) | −5% per percobaan |
| Durasi < 15 menit | −10% |
| Durasi > 40 menit | −5% |
| Resolusi < 720p / audio tidak jelas | −10% |
| Terlambat submit | −10% per hari (maks 3 hari) |

---

## Checklist Sebelum Submit

- [ ] Video berdurasi 20–35 menit
- [ ] Webcam terlihat sepanjang video
- [ ] Narasi suara jelas di seluruh video
- [ ] Pembukaan: nama, NIM, kelas, platform
- [ ] Materi: WiFi, TCP/UDP, HTTP, MQTT, BLE, WebSocket
- [ ] Percobaan 01–12: kode + output + analisis masing-masing
- [ ] Project FarmConnect: minimal 8 fitur didemonstrasikan
- [ ] Penutup: kesimpulan dan refleksi
- [ ] Format MP4, minimal 720p
- [ ] Link accessible (Google Drive / YouTube Unlisted)

---

*Tugas Video Modul 12 — Network & IoT | Praktikum Sistem Embedded | 2025/2026*
