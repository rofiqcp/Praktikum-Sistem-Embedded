# Jobsheet Modul 13: Network & IoT — Konektivitas Jaringan

## A. Tujuan Praktikum

Setelah menyelesaikan praktikum ini, mahasiswa diharapkan mampu:
1. Memahami arsitektur IoT dan protokol jaringan (TCP/IP, HTTP, MQTT, BLE)
2. Mengimplementasikan koneksi WiFi (Station/AP) pada ESP32 menggunakan ESP-IDF
3. Membangun aplikasi TCP/UDP client-server dan HTTP server/client
4. Menggunakan protokol MQTT untuk komunikasi publish/subscribe
5. Mengimplementasikan BLE advertising dan GATT server pada ESP32
6. Menggunakan modul external (ESP-01, W5500, HM-10) pada STM32 untuk konektivitas jaringan
7. Membangun sistem IoT end-to-end dari sensor hingga dashboard

## B. Alat dan Bahan

| No | Komponen | Jumlah | Keterangan |
|----|----------|--------|------------|
| 1 | ESP32 DevKit V1 | 1 | Platform utama (WiFi/BLE native) |
| 2 | STM32F103C8T6 (Blue Pill) | 1 | Platform kedua (tanpa WiFi) |
| 3 | ST-Link V2 | 1 | Programmer STM32 |
| 4 | Kabel USB Micro | 2 | Untuk ESP32 dan ST-Link |
| 5 | ESP-01 (ESP8266) | 1 | Modul WiFi external untuk STM32 |
| 6 | W5500 Ethernet Module | 1 | Modul Ethernet SPI untuk STM32 |
| 7 | HM-10 BLE Module | 1 | Modul BLE untuk STM32 |
| 8 | Router WiFi / Hotspot HP | 1 | Access Point untuk koneksi |
| 9 | Kabel Ethernet (RJ45) | 1 | Untuk W5500 ke switch/router |
| 10 | Breadboard + Jumper | 1 set | Untuk wiring |
| 11 | LED + Resistor 220Ω | 2 set | Indikator status |
| 12 | Push Button | 1 | Untuk kontrol |
| 13 | Sensor DHT22/BMP280 (opsional) | 1 | Untuk percobaan IoT |
| 14 | Komputer/Laptop | 1 | Untuk serial monitor, browser, MQTT tools |
| 15 | Smartphone + nRF Connect App | 1 | Untuk percobaan BLE |

### Software yang Diperlukan:
- PlatformIO IDE (VS Code extension)
- Serial Monitor (PlatformIO built-in)
- Web Browser (Chrome/Firefox)
- MQTT Explorer / Mosquitto client
- nRF Connect App (Android/iOS) untuk BLE
- Python 3.x + pip (untuk debug scripts)
- Wireshark (opsional, untuk packet analysis)

## C. Dasar Teori

Lihat **Materi.md** untuk pembahasan lengkap. Ringkasan:

### Arsitektur IoT
```
Sensor → MCU → Network → Cloud → Dashboard → Control → Actuator
```

### Perbandingan Platform
| Aspek | ESP32 | STM32 |
|-------|-------|-------|
| WiFi | Built-in | Via ESP-01 (AT cmd) |
| BLE | Built-in | Via HM-10 (AT cmd) |
| Ethernet | RMII | Via W5500 (SPI) |
| Framework | ESP-IDF | STM32Cube HAL |

---

## D. Percobaan

### ═══════════════════════════════════════
### BAGIAN 1: ESP32 — WiFi & Network (Native)
### ═══════════════════════════════════════

### Percobaan 1: WiFi Scan (ESP32_01_WiFi_Scan)

**Tujuan:** Melakukan scanning Access Point WiFi di sekitar dan menampilkan informasi SSID, RSSI, channel, dan mode autentikasi.

**Langkah Kerja:**
1. Buka folder `praktikum/ESP32/ESP32_01_WiFi_Scan/`
2. Buka file `src/main.c`, pelajari kode program
3. Compile: `pio run`
4. Upload: `pio run -t upload`
5. Buka Serial Monitor: `pio device monitor`
6. Amati daftar Access Point yang terdeteksi
7. Jalankan `python debug_analysis.py` untuk visualisasi RSSI

**Tabel Pengamatan:**

| No | SSID | RSSI (dBm) | Channel | Auth Mode | Kualitas |
|----|------|-----------|---------|-----------|----------|
| 1 | | | | | |
| 2 | | | | | |
| 3 | | | | | |

**Pertanyaan:**
1. Apa hubungan antara RSSI dan jarak ke Access Point?
2. Mengapa channel yang berbeda digunakan oleh AP yang berbeda?

---

### Percobaan 2: WiFi Station Mode (ESP32_02_WiFi_Station)

**Tujuan:** Menghubungkan ESP32 ke Access Point WiFi dan mendapatkan IP address via DHCP.

**Langkah Kerja:**
1. Edit `src/main.c`, ubah SSID dan PASSWORD sesuai jaringan tersedia
2. Compile dan upload
3. Amati proses koneksi di Serial Monitor
4. Catat IP address yang didapat

**Tabel Pengamatan:**

| Parameter | Nilai |
|-----------|-------|
| SSID Target | |
| IP Address | |
| Gateway | |
| RSSI | |
| Waktu Koneksi | |
| Jumlah Retry | |

---

### Percobaan 3: WiFi Access Point (ESP32_03_WiFi_Access_Point)

**Tujuan:** Membuat ESP32 sebagai Soft-AP yang dapat diakses oleh perangkat lain.

**Langkah Kerja:**
1. Upload program ke ESP32
2. Gunakan smartphone/laptop untuk mencari WiFi "ESP32_AP"
3. Hubungkan ke jaringan tersebut
4. Amati informasi client di Serial Monitor

---

### Percobaan 4: TCP Client/Server (ESP32_04_TCP_Client_Server)

**Tujuan:** Membangun komunikasi TCP client-server menggunakan BSD socket API.

**Langkah Kerja:**
1. Upload program (TCP server di port 8080)
2. Hubungkan ESP32 ke WiFi
3. Dari PC, gunakan `python debug_analysis.py` sebagai TCP client
4. Kirim pesan dan amati echo response
5. Atau gunakan: `echo "Hello" | nc <ESP32_IP> 8080`

---

### Percobaan 5: UDP Communication (ESP32_05_UDP_Communication)

**Tujuan:** Mengirim dan menerima datagram UDP termasuk broadcast.

**Langkah Kerja:**
1. Upload program
2. Gunakan `python debug_analysis.py` sebagai UDP client/server
3. Kirim datagram, amati response
4. Test broadcast ke 255.255.255.255

**Pertanyaan:**
- Apa perbedaan TCP dan UDP dalam hal reliability?
- Kapan UDP lebih cocok daripada TCP?

---

### Percobaan 6: HTTP Server (ESP32_06_HTTP_Server)

**Tujuan:** Membuat web server dengan REST API pada ESP32.

**Langkah Kerja:**
1. Upload program
2. Buka browser, akses `http://<ESP32_IP>/` → lihat HTML dashboard
3. Akses `http://<ESP32_IP>/api/status` → lihat JSON response
4. Gunakan `curl -X POST http://<ESP32_IP>/api/led` → kontrol LED

---

### Percobaan 7: HTTP Client (ESP32_07_HTTP_Client)

**Tujuan:** ESP32 sebagai HTTP client, mengirim GET request dan mem-parse response.

---

### Percobaan 8: MQTT Publish/Subscribe (ESP32_08_MQTT_Pub_Sub)

**Tujuan:** Implementasi MQTT pub/sub menggunakan broker test.mosquitto.org.

**Langkah Kerja:**
1. Upload program
2. ESP32 akan publish ke topic `esp32/sensor`
3. Install MQTT Explorer atau gunakan `python debug_analysis.py`
4. Subscribe ke topic `esp32/sensor` untuk melihat data
5. Publish ke topic `esp32/control` untuk mengirim command

**Pertanyaan:**
- Apa perbedaan QoS 0, 1, dan 2?
- Apa fungsi Last Will Testament (LWT)?

---

### Percobaan 9: BLE Advertising (ESP32_09_BLE_Advertising)

**Tujuan:** Mengiklankan perangkat ESP32 via BLE GAP advertising.

**Langkah Kerja:**
1. Upload program
2. Buka nRF Connect App di smartphone
3. Scan perangkat BLE, cari "ESP32_BLE"
4. Amati advertising data (name, TX power, service UUID)

---

### Percobaan 10: BLE GATT Server (ESP32_10_BLE_GATT_Server)

**Tujuan:** Membuat GATT server dengan service dan characteristic.

**Langkah Kerja:**
1. Upload program
2. Gunakan nRF Connect App
3. Connect ke ESP32
4. Read characteristic → lihat sensor data
5. Write characteristic → kontrol LED (0x01=ON, 0x00=OFF)
6. Enable notification → terima update data otomatis

---

### Percobaan 11: WebSocket Server (ESP32_11_WebSocket_Server)

**Tujuan:** Komunikasi real-time bidirectional via WebSocket.

**Langkah Kerja:**
1. Upload program
2. Buka browser, akses `http://<ESP32_IP>/` → HTML dengan WebSocket client
3. Atau gunakan `python debug_analysis.py` sebagai WebSocket client
4. Kirim pesan, amati response real-time

---

### Percobaan 12: IoT Dashboard (ESP32_12_IoT_Dashboard)

**Tujuan:** Membangun sistem IoT lengkap: sensor → MQTT → dashboard.

**Langkah Kerja:**
1. Upload program
2. ESP32 membaca sensor dan publish via MQTT
3. Akses HTTP dashboard di browser
4. Gunakan `python debug_analysis.py` untuk monitoring MQTT

---

### ═══════════════════════════════════════
### BAGIAN 2: STM32 — Network via Modul External
### ═══════════════════════════════════════

### Percobaan 13: UART AT Command (STM32_01_UART_AT_Command)

**Tujuan:** Mengirim AT command ke ESP-01 via UART dan mem-parse response.

**Wiring:**
```
STM32          ESP-01
PA2(TX) -----> RX
PA3(RX) <----- TX
3.3V --------> VCC + CH_PD
GND ---------> GND
```

**Langkah Kerja:**
1. Wiring ESP-01 ke STM32 sesuai diagram
2. Upload program
3. Amati Serial Monitor (UART1 = debug)
4. ESP-01 menerima AT commands via UART2

---

### Percobaan 14: WiFi Connect via ESP-01 (STM32_02_ESP01_WiFi_Connect)

**Tujuan:** Menghubungkan ke WiFi AP menggunakan AT commands ESP-01.

---

### Percobaan 15: TCP Client via ESP-01 (STM32_03_ESP01_TCP_Client)

**Tujuan:** Membuat koneksi TCP melalui ESP-01 AT commands.

---

### Percobaan 16: HTTP GET via ESP-01 (STM32_04_ESP01_HTTP_GET)

**Tujuan:** Mengirim HTTP GET request melalui ESP-01.

---

### Percobaan 17: W5500 Ethernet Init (STM32_05_W5500_Ethernet_Init)

**Tujuan:** Inisialisasi modul W5500 Ethernet via SPI.

**Wiring:**
```
STM32          W5500
PA5(SCK) ----> SCLK
PA6(MISO) <--- MISO
PA7(MOSI) ---> MOSI
PA4(CS) -----> CS
PB0 ---------> RST
3.3V --------> VCC
GND ---------> GND
```

---

### Percobaan 18-20: W5500 TCP/UDP/HTTP Server

**Tujuan:** Implementasi server TCP, UDP, dan HTTP via W5500 Ethernet.

---

### Percobaan 21: UART Bridge ke ESP32 (STM32_09_UART_Bridge_ESP32)

**Tujuan:** Komunikasi custom protocol antara STM32 dan ESP32 via UART.

---

### Percobaan 22: BLE HM-10 (STM32_10_BLE_HM10)

**Tujuan:** Komunikasi BLE menggunakan modul HM-10 via UART AT commands.

---

### Percobaan 23: MQTT via ESP-01 (STM32_11_ESP01_MQTT)

**Tujuan:** Implementasi protokol MQTT menggunakan raw TCP melalui ESP-01.

---

### Percobaan 24: IoT Sensor Gateway (STM32_12_IoT_Sensor_Gateway)

**Tujuan:** Membangun gateway IoT lengkap: STM32 sensor → ESP-01 → cloud.

---

## E. Tugas

1. **Tugas Individu:** Buat sistem monitoring suhu (DHT22/BMP280) yang mengirim data via MQTT dan menampilkan di dashboard web. Dokumentasikan dalam video 5-10 menit.

2. **Tugas Kelompok:** Bandingkan kinerja TCP vs UDP untuk transmisi data sensor (latency, packet loss, throughput). Buat laporan analisis.

3. **Pertanyaan Analisis:**
   - Jelaskan perbedaan arsitektur komunikasi ESP32 (native WiFi) vs STM32 (via ESP-01)!
   - Mengapa MQTT lebih cocok untuk IoT dibanding HTTP polling?
   - Apa kelebihan dan kekurangan BLE dibanding WiFi untuk IoT?
   - Bagaimana cara mengamankan komunikasi IoT? Sebutkan 3 metode!

## F. Python Debug & Analysis Scripts

Setiap folder percobaan memiliki file `debug_analysis.py` yang berfungsi sebagai:
- Serial monitor dan data parser
- Test client (TCP/UDP/HTTP/MQTT/WebSocket/BLE)
- Visualisasi data dengan matplotlib
- Logging dan analisis performa

Cara penggunaan:
```bash
cd praktikum/ESP32/ESP32_01_WiFi_Scan/
python debug_analysis.py          # Mode default (serial monitor)
python debug_analysis.py --help   # Lihat opsi lainnya
```
