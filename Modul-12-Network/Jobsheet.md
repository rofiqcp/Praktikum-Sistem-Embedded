# Jobsheet Modul 12: Network dan IoT

## Praktikum Sistem Embedded

**Semester:** Genap 2025/2026  
**Durasi:** 3 × 50 menit (2 pertemuan)  
**Platform:** ESP32 DevKit V1 & STM32 Blue Pill (STM32F103C8T6)

---

## 1. Tujuan Praktikum

Setelah menyelesaikan praktikum ini, mahasiswa diharapkan mampu:

1. **Mengkonfigurasi konektifitas jaringan** — WiFi (ESP32) atau Ethernet/UART bridge (STM32) untuk koneksi internet.
2. **Mengimplementasikan protokol transport** — TCP client/server dan UDP unicast/broadcast.
3. **Menggunakan protokol aplikasi** — HTTP server/client dan MQTT publish/subscribe.
4. **Mengkonfigurasi BLE** — Advertising, GATT server, dan komunikasi BLE.
5. **Membangun sistem IoT end-to-end** — Dashboard sensor real-time dengan multiple protokol.

---

## 2. Peralatan

| No | Komponen | Jumlah | Keterangan |
|----|----------|--------|------------|
| 1 | ESP32 DevKit V1 | 1 | WiFi + BLE built-in |
| 2 | STM32 Blue Pill | 1 | ARM Cortex-M3 |
| 3 | ST-Link V2 | 1 | Programmer STM32 |
| 4 | ESP-01 (ESP8266) | 1 | WiFi module untuk STM32 (UART AT) |
| 5 | W5500 Ethernet Module | 1 | Ethernet untuk STM32 (SPI) |
| 6 | HM-10 BLE Module | 1 | BLE untuk STM32 (UART AT) |
| 7 | LED 5mm | 2 | Indikator |
| 8 | Resistor 330Ω | 2 | Current limiting |
| 9 | Breadboard + kabel jumper | 1 set | |
| 10 | Router WiFi / Hotspot HP | 1 | Akses jaringan |

---

## 3. Teori Singkat

**WiFi (802.11)** menyediakan koneksi nirkabel ke jaringan lokal/internet. ESP32 memiliki WiFi built-in (STA/AP/STA+AP mode). STM32 menggunakan ESP-01 sebagai WiFi coprocessor melalui AT commands via UART.

**TCP** (Transmission Control Protocol) menjamin pengiriman data secara berurutan dan reliable. **UDP** (User Datagram Protocol) lebih cepat tetapi tanpa jaminan — cocok untuk sensor data real-time.

**HTTP** adalah protokol request-response untuk web. Mikrokontroler bisa menjadi HTTP server (menyajikan halaman) atau client (mengambil data dari API).

**MQTT** (Message Queuing Telemetry Transport) adalah protokol publish-subscribe ringan yang ideal untuk IoT. Publisher mengirim data ke topic, subscriber menerima data dari topic yang diminati.

**BLE** (Bluetooth Low Energy) untuk komunikasi jarak dekat hemat daya. GATT (Generic Attribute Profile) mendefinisikan service dan characteristic untuk pertukaran data.

---

## 4. Langkah Percobaan

> **Catatan Penting:** Percobaan ESP32 dan STM32 mencakup **topik yang sama** tetapi dengan **implementasi berbeda** karena perbedaan hardware. ESP32 menggunakan API native, STM32 menggunakan modul eksternal.

---

### Percobaan 01: WiFi Scan / AT Command Init

**Tujuan:** Menginisialisasi koneksi wireless dan melihat jaringan yang tersedia.

#### Langkah Kerja (ESP32)

1. Buka project `ESP32_01`.
2. Program melakukan WiFi scan — menampilkan semua AP yang ditemukan.
3. Amati tabel: SSID, RSSI (kekuatan sinyal), channel, tipe autentikasi.
4. Identifikasi AP yang akan digunakan dan kualitas sinyalnya.

#### Langkah Kerja (STM32)

1. Buka project `STM32_01`.
2. Hubungkan ESP-01 ke UART2 STM32 (TX2→RX ESP, RX2→TX ESP, 3.3V, GND).
3. Program mengirim AT commands: `AT`, `AT+GMR`, `AT+CWMODE?`, `AT+RST`.
4. Amati response dari ESP-01 — pastikan komunikasi UART berhasil.

#### Tabel Pengamatan

| Platform | Test | Result | Status |
|----------|------|--------|--------|
| ESP32 | AP scan count | | |
| ESP32 | Strongest AP (RSSI) | | |
| STM32 | AT response | OK? | |
| STM32 | Firmware version | | |

#### Pertanyaan Analisa

1. Apa arti nilai RSSI? Berapa dBm yang dianggap sinyal baik/buruk?
2. (STM32) Mengapa menggunakan AT commands untuk berkomunikasi dengan ESP-01?
3. Apa perbedaan mode STA, AP, dan STA+AP pada WiFi?
4. Mengapa ESP32 lebih mudah untuk WiFi dibanding STM32?

---

### Percobaan 02: WiFi Station Connect

**Tujuan:** Menghubungkan mikrokontroler ke jaringan WiFi dan mendapatkan IP address.

#### Langkah Kerja (ESP32)

1. Buka project `ESP32_02`. Isi SSID dan password di kode.
2. Program terhubung ke AP menggunakan event-driven handler.
3. Amati proses: CONNECTING → GOT_IP → status connected.
4. Amati RSSI monitoring periodik — kualitas sinyal real-time.

#### Langkah Kerja (STM32)

1. Buka project `STM32_02`. Isi SSID dan password.
2. Program mengirim `AT+CWMODE=1` (STA) dan `AT+CWJAP="SSID","PASS"`.
3. Amati retry logic jika koneksi gagal.
4. `AT+CIFSR` menampilkan IP address yang didapat.

#### Tabel Pengamatan

| Metric | ESP32 | STM32+ESP-01 |
|--------|-------|-------------|
| Connect time | | |
| IP Address | | |
| RSSI | | |
| Retry count | | |

#### Pertanyaan Analisa

1. Apa perbedaan DHCP dan static IP? Kapan gunakan yang mana?
2. Apa yang terjadi jika password salah? Bagaimana error handling-nya?
3. Mengapa penting retry logic untuk koneksi WiFi?
4. Bandingkan kecepatan koneksi ESP32 native vs STM32+ESP-01.

---

### Percobaan 03: WiFi Access Point / TCP Client

**Tujuan ESP32:** Membuat ESP32 sebagai Access Point (AP) — perangkat lain bisa konek. 
**Tujuan STM32:** Mengirim data TCP melalui koneksi WiFi via ESP-01.

#### Langkah Kerja (ESP32)

1. Buka project `ESP32_03`.
2. ESP32 menjadi Soft-AP — terbentuk jaringan WiFi sendiri.
3. Hubungkan HP/laptop ke AP yang dibuat ESP32.
4. Amati tracking koneksi client: MAC address, event connect/disconnect.

#### Langkah Kerja (STM32)

1. Buka project `STM32_03`.
2. Setelah terkoneksi WiFi, buka TCP koneksi: `AT+CIPSTART="TCP","server",port`.
3. Kirim data: `AT+CIPSEND=length` → tunggu `>` → kirim data.
4. Amati response dari server. `AT+CIPCLOSE` untuk menutup.

#### Tabel Pengamatan (ESP32)

| Event | Client MAC | Action | Client Count |
|-------|-----------|--------|-------------|
| Connect | | | |
| Disconnect | | | |

#### Tabel Pengamatan (STM32)

| Step | AT Command | Response | Status |
|------|-----------|----------|--------|
| Open TCP | AT+CIPSTART | | |
| Send data | AT+CIPSEND | | |
| Receive | | | |
| Close | AT+CIPCLOSE | | |

#### Pertanyaan Analisa

1. (ESP32) Berapa client maksimal yang bisa konek ke Soft-AP?
2. (STM32) Apa arti `AT+CIPSTART="TCP","IP",port`? Jelaskan setiap parameter.
3. Apa perbedaan TCP dan UDP dalam konteks koneksi ini?
4. Kapan mikrokontroler lebih cocok menjadi AP vs STA?

---

### Percobaan 04: TCP Socket / HTTP GET

**Tujuan ESP32:** Mengimplementasikan TCP server dan client menggunakan BSD sockets.
**Tujuan STM32:** Mengirim HTTP GET request melalui TCP via ESP-01.

#### Langkah Kerja (ESP32)

1. Buka project `ESP32_04`.
2. TCP server berjalan di port 8080. Setiap client baru ditangani task terpisah.
3. Gunakan `nc (netcat)` atau browser untuk konek ke ESP32.
4. Data yang dikirim akan di-echo kembali. Amati multi-client handling.

#### Langkah Kerja (STM32)

1. Buka project `STM32_04`.
2. Buka TCP koneksi ke httpbin.org port 80.
3. Kirim HTTP GET request secara manual (raw HTTP string).
4. Parse response: status code, Content-Type, body.

#### Tabel Pengamatan

| Platform | Test | Result |
|----------|------|--------|
| ESP32 | Server listening on port | |
| ESP32 | Client connected from | |
| ESP32 | Echo test | |
| STM32 | HTTP status code | |
| STM32 | Response body | |

#### Pertanyaan Analisa

1. (ESP32) Bagaimana server menangani multiple client secara bersamaan? (multi-task)
2. (STM32) Jelaskan format HTTP GET request: method, path, host, headers.
3. Apa fungsi `bind()`, `listen()`, `accept()` pada TCP server?
4. Mengapa perlu parsing response HTTP? Apa perbedaan header dan body?

---

### Percobaan 05: UDP / Ethernet Init

**Tujuan ESP32:** Komunikasi UDP unicast dan broadcast.
**Tujuan STM32:** Inisialisasi modul Ethernet W5500 via SPI.

#### Langkah Kerja (ESP32)

1. Buka project `ESP32_05`.
2. UDP server menunggu data di port tertentu.
3. UDP client mengirim data unicast dan broadcast (255.255.255.255).
4. Amati bahwa UDP tidak menjamin pengiriman — bandingkan dengan TCP.

#### Langkah Kerja (STM32)

1. Buka project `STM32_05`.
2. Hubungkan W5500 ke SPI1 STM32 (SCK, MOSI, MISO, CS, RST).
3. Program menginisialisasi W5500: set MAC address, IP, gateway, subnet.
4. Verifikasi chip version (0x04) dan PHY link status.

#### Tabel Pengamatan

| Platform | Test | Result |
|----------|------|--------|
| ESP32 | UDP send OK | |
| ESP32 | Broadcast received | |
| STM32 | W5500 chip version | |
| STM32 | PHY link status | |
| STM32 | IP configured | |

#### Pertanyaan Analisa

1. (ESP32) Apa perbedaan UDP unicast dan broadcast? Kapan gunakan yang mana?
2. (STM32) Mengapa W5500 menggunakan SPI? Apa keuntungan hardware TCP/IP stack?
3. UDP tidak menjamin pengiriman — mengapa tetap digunakan? Berikan contoh use case.
4. Apa perbedaan WiFi dan Ethernet untuk embedded system?

---

### Percobaan 06: HTTP Server / TCP Server

**Tujuan ESP32:** Membuat HTTP web server dengan REST API dan kontrol LED.
**Tujuan STM32:** Membuat TCP echo server menggunakan W5500.

#### Langkah Kerja (ESP32)

1. Buka project `ESP32_06`.
2. HTTP server berjalan — buka browser, akses `http://<ESP32_IP>`.
3. Halaman web menampilkan status LED dengan tombol kontrol.
4. Test REST API: `GET /api/status` (JSON), `POST /api/led` (toggle LED).

#### Langkah Kerja (STM32)

1. Buka project `STM32_06`.
2. W5500 TCP server di port 8080. Menunggu koneksi client.
3. Data yang masuk di-echo kembali ke client.
4. Amati socket state transitions: OPEN → LISTEN → ESTABLISHED → CLOSE.

#### Tabel Pengamatan

| Platform | Test | Result |
|----------|------|--------|
| ESP32 | Web page loads | |
| ESP32 | LED toggle via API | |
| ESP32 | JSON response | |
| STM32 | TCP server accepts client | |
| STM32 | Echo data correct | |

#### Pertanyaan Analisa

1. (ESP32) Apa itu REST API? Apa perbedaan GET dan POST?
2. (STM32) Jelaskan state diagram TCP socket: LISTEN → ESTABLISHED → CLOSE.
3. Bagaimana web server di mikrokontroler berbeda dari server konvensional (Nginx, Apache)?
4. Apa keuntungan HTTP server di embedded vs cloud server + polling?

---

### Percobaan 07: HTTP Client / UDP via W5500

**Tujuan ESP32:** Mengambil data dari API menggunakan HTTP client.
**Tujuan STM32:** Komunikasi UDP menggunakan W5500.

#### Langkah Kerja (ESP32)

1. Buka project `ESP32_07`.
2. Program mengirim HTTP GET ke `httpbin.org/get` secara periodik.
3. Parse response: status code, headers, body.
4. Amati event handler untuk streaming response chunks.

#### Langkah Kerja (STM32)

1. Buka project `STM32_07`.
2. W5500 UDP socket mengirim data ke target IP/port.
3. Terima response dan parse header UDP (source IP, port, length).
4. Amati bahwa UDP header parsing manual diperlukan pada W5500.

#### Tabel Pengamatan

| Platform | Test | Result |
|----------|------|--------|
| ESP32 | HTTP GET status | |
| ESP32 | Response parse | |
| STM32 | UDP send OK | |
| STM32 | UDP receive OK | |

#### Pertanyaan Analisa

1. (ESP32) Apa perbedaan `esp_http_client` library vs raw TCP?
2. (STM32) Mengapa harus parsing UDP header manual di W5500?
3. Apa keuntungan menggunakan library HTTP vs implementasi manual?
4. Kapan embedded system menjadi HTTP client vs server?

---

### Percobaan 08: MQTT Publish/Subscribe / HTTP Server W5500

**Tujuan ESP32:** Menggunakan MQTT untuk publish sensor data dan subscribe commands.
**Tujuan STM32:** Membuat HTTP web server menggunakan W5500.

#### Langkah Kerja (ESP32)

1. Buka project `ESP32_08`.
2. Connect ke MQTT broker: `test.mosquitto.org` port 1883.
3. Publish sensor data (JSON) ke topic, mis. `esp32/sensor`.
4. Subscribe ke command topic, mis. `esp32/cmd`. Kirim perintah dari MQTT client di PC.
5. Amati QoS 0/1/2 behavior.

#### Langkah Kerja (STM32)

1. Buka project `STM32_08`.
2. W5500 HTTP server di port 80. Sajikan halaman HTML dengan status.
3. Implementasikan `GET /` (HTML page), `GET /api/status` (JSON), `POST /api/led`.
4. Parse HTTP request: method, path, body.

#### Tabel Pengamatan (ESP32)

| MQTT Action | Topic | QoS | Payload | Status |
|-------------|-------|-----|---------|--------|
| Publish | esp32/sensor | 0 | | |
| Subscribe | esp32/cmd | 1 | | |
| Receive cmd | | | | |

#### Tabel Pengamatan (STM32)

| HTTP Request | Path | Response Code | Body |
|-------------|------|-------------|------|
| GET | / | | HTML |
| GET | /api/status | | JSON |
| POST | /api/led | | |

#### Pertanyaan Analisa

1. (ESP32) Apa perbedaan MQTT QoS 0, 1, dan 2? Trade-off masing-masing?
2. (STM32) Bagaimana parse HTTP request header di embedded system?
3. Apa keuntungan MQTT vs HTTP untuk IoT sensor data?
4. Apa itu MQTT broker? Mengapa perlu perantara?

---

### Percobaan 09: BLE Advertising / UART Bridge Protocol

**Tujuan ESP32:** Mengkonfigurasi BLE advertising — device discovery.
**Tujuan STM32:** Komunikasi bilateral dengan ESP32 coprocessor via UART protocol.

#### Langkah Kerja (ESP32)

1. Buka project `ESP32_09`.
2. BLE advertising dimulai — device name, TX power, appearance di-broadcast.
3. Scan dari HP (nRF Connect app) — temukan ESP32.
4. Amati advertising data: nama, flag, UUID.

#### Langkah Kerja (STM32)

1. Buka project `STM32_09`.
2. Custom binary protocol: STX + CMD + LEN + DATA + CHECKSUM + ETX.
3. STM32 mengirim frame ke ESP32 via UART2. ESP32 memproses dan respond.
4. Amati state machine receiver: WAIT_STX → GET_CMD → GET_LEN → GET_DATA → VERIFY.

#### Tabel Pengamatan

| Platform | Test | Result |
|----------|------|--------|
| ESP32 | BLE device visible | |
| ESP32 | Device name correct | |
| ESP32 | Adv data content | |
| STM32 | Frame sent OK | |
| STM32 | Checksum verified | |
| STM32 | Response received | |

#### Pertanyaan Analisa

1. (ESP32) Apa itu BLE advertising? Apa data yang bisa di-broadcast?
2. (STM32) Mengapa perlu custom binary protocol vs plain text?
3. Apa keuntungan checksum dalam protokol komunikasi?
4. Bandingkan BLE advertising vs WiFi scan — tujuan berbeda.

---

### Percobaan 10: BLE GATT Server / BLE HM-10

**Tujuan ESP32:** Implementasi BLE GATT server — custom service dan characteristic.
**Tujuan STM32:** BLE communication via HM-10 module menggunakan AT commands.

#### Langkah Kerja (ESP32)

1. Buka project `ESP32_10`.
2. GATT server membuat custom service (UUID 0x00FF) dengan:
   - Read characteristic: sensor data
   - Write characteristic: LED control
   - Notify characteristic: periodic update
3. Koneksikan dari HP (nRF Connect). Baca, tulis, dan subscribe notify.

#### Langkah Kerja (STM32)

1. Buka project `STM32_10`.
2. HM-10 dihubungkan ke UART. Init: `AT+NAME`, `AT+ROLE0`, `AT+UUID`.
3. Module mengiklankan diri — koneksikan dari HP.
4. Saat connected, data bisa dikirim/terima via UART transparan.

#### Tabel Pengamatan

| Platform | Test | Result |
|----------|------|--------|
| ESP32 | GATT read sensor | |
| ESP32 | GATT write LED | |
| ESP32 | GATT notify received | |
| STM32 | HM-10 advertising | |
| STM32 | BLE connected | |
| STM32 | Data exchange | |

#### Pertanyaan Analisa

1. (ESP32) Apa struktur GATT? (Service → Characteristic → Descriptor)
2. (STM32) Apa perbedaan HM-10 transparent mode vs AT mode?
3. Apa perbedaan BLE read, write, dan notify? Kapan gunakan masing-masing?
4. Bandingkan kemampuan BLE ESP32 native vs STM32+HM-10.

---

### Percobaan 11: WebSocket / MQTT Manual

**Tujuan ESP32:** Komunikasi real-time bidirectional menggunakan WebSocket.
**Tujuan STM32:** Konstruksi packet MQTT manual over TCP via ESP-01.

#### Langkah Kerja (ESP32)

1. Buka project `ESP32_11`.
2. WebSocket server berjalan di port 80 dengan HTTP upgrade.
3. Client (browser/tool) konek — data bisa dikirim dua arah secara real-time.
4. Server broadcast sensor data ke semua connected client.

#### Langkah Kerja (STM32)

1. Buka project `STM32_11`.
2. Program membangun packet MQTT 3.1.1 secara manual: CONNECT, PUBLISH, SUBSCRIBE, PINGREQ.
3. Packet dikirim sebagai raw TCP data melalui ESP-01.
4. Amati konstruksi byte-level: packet type, remaining length, payload.

#### Tabel Pengamatan

| Platform | Test | Result |
|----------|------|--------|
| ESP32 | WebSocket connected | |
| ESP32 | Server → client push | |
| ESP32 | Client → server msg | |
| STM32 | MQTT CONNECT sent | |
| STM32 | MQTT PUBLISH sent | |
| STM32 | MQTT SUBSCRIBE OK | |

#### Pertanyaan Analisa

1. (ESP32) Apa keuntungan WebSocket vs HTTP polling untuk real-time data?
2. (STM32) Jelaskan struktur MQTT packet: fixed header, variable header, payload.
3. Apa perbedaan WebSocket dan MQTT? Kapan gunakan yang mana?
4. Mengapa STM32 harus membangun MQTT packet manual?

---

### Percobaan 12: IoT Dashboard (Capstone)

**Tujuan:** Membangun sistem IoT end-to-end — sensor → processing → publish → dashboard.

#### Langkah Kerja (ESP32)

1. Buka project `ESP32_12`.
2. Sensor disimulasikan → data dikumpulkan → dikirim via MQTT.
3. Bersamaan, HTTP server menyajikan halaman dashboard HTML/JS.
4. Dashboard menampilkan data sensor real-time dengan auto-refresh.
5. Command dari dashboard mengontrol LED.

#### Langkah Kerja (STM32)

1. Buka project `STM32_12`.
2. ADC membaca sensor (internal temp + PA0 external).
3. Data dikirim ke queue → upload task mengirim HTTP POST (JSON) via ESP-01.
4. Amati JSON format data dan response dari server.

#### Tabel Pengamatan

| Platform | Metric | Value |
|----------|--------|-------|
| ESP32 | MQTT publish rate | /menit |
| ESP32 | Dashboard loads | |
| ESP32 | Remote LED control | |
| STM32 | Sensor data sent | |
| STM32 | HTTP POST status | |
| STM32 | Server response | |

#### Pertanyaan Analisa

1. Apa komponen utama sistem IoT end-to-end? (sensor, gateway, cloud, dashboard)
2. Mengapa menggunakan dual-protocol (MQTT + HTTP) di ESP32?
3. Bagaimana memastikan data integrity dari sensor sampai dashboard?
4. Apa pertimbangan keamanan untuk sistem IoT production? (TLS, authentication)

---

## 5. Tabel Komparatif

| Aspek | ESP32 (Native) | STM32 (External Modules) |
|-------|----------------|------------------------|
| WiFi | Built-in, `esp_wifi` API | ESP-01 via UART AT commands |
| Ethernet | Tidak ada (butuh PHY) | W5500 via SPI |
| BLE | Built-in, `esp_gap/gatts` | HM-10 via UART AT |
| TCP/UDP | BSD sockets (lwIP) | W5500 socket API / AT commands |
| HTTP | `esp_http_server/client` | Manual HTTP string / W5500 |
| MQTT | ESP-MQTT component | Manual packet construction |
| WebSocket | HTTP server upgrade | Tidak tersedia |

---

## 6. Referensi

1. ESP-IDF WiFi Guide — https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/wifi.html
2. ESP-IDF MQTT Client — https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/protocols/mqtt.html
3. ESP-IDF BLE Guide — https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/bluetooth/
4. W5500 Datasheet — WIZnet
5. ESP8266 AT Instruction Set — Espressif Systems
6. Kolban's Book on ESP32 — Neil Kolban
7. MQTT Specification 3.1.1 — OASIS

---

*Jobsheet Modul 12 — Network & IoT | Praktikum Sistem Embedded | 2025/2026*
