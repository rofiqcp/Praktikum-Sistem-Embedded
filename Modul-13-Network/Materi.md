# Materi Modul 13: Embedded Networking & IoT Protocols

## 1. Pengantar Embedded Networking
Sistem embedded modern jarang berdiri sendiri. Dengan teknologi IoT (Internet of Things), perangkat embedded perlu bertukar data dengan perangkat lain atau server cloud.
Untuk melakukan ini, mikrokontroler menggunakan **TCP/IP Stack** yang merupakan standar komunikasi internet global.

### TCP/IP vs OSI Model
Dalam dunia embedded, kita lebih fokus pada **TCP/IP Model** yang lebih ringkas dibanding OSI 7 Layer:
1.  **Link Layer**: Driver hardware (WiFi Driver, Ethernet MAC/PHY).
2.  **Internet Layer**: IP (Internet Protocol), Routing.
3.  **Transport Layer**: TCP (Reliable) dan UDP (Fast, Unreliable).
4.  **Application Layer**: Protokol data user (HTTP, MQTT, CoAP, NTP, FTP).

### LwIP (Lightweight IP)
Karena keterbatasan RAM dan Flash, mikrokontroler tidak menggunakan stack jaringan full seperti di Linux/Windows.
Mereka menggunakan **LwIP**, sebuah implementasi TCP/IP open-source yang didesain khusus untuk resource rendah.
- Digunakan oleh ESP-IDF (ESP32) dan STM32 Cube (via Middleware).

---

## 2. HTTP (HyperText Transfer Protocol)
HTTP adalah protokol berbasis text yang paling umum digunakan di web (Client-Server Architecture).
- **Client**: Meminta resource (Request).
- **Server**: Memberikan resource (Response).

### Metode Utama:
1.  **GET**: Meminta data dari server (misal: ambil status cuaca).
    - Data dikirim di URL (query params).
2.  **POST**: Mengirim data ke server (misal: kirim data sensor).
    - Data dikirim di Body request (biasanya format JSON).

### JSON (JavaScript Object Notation) format
Format data standar IoT. Ringan dan mudah dibaca manusia & mesin.
Contoh:
```json
{
  "sensor": "DHT22",
  "data": {
    "temp": 25.5,
    "humidity": 60
  }
}
```

---

## 3. MQTT (Message Queuing Telemetry Transport)
MQTT adalah protokol ringan berbasis **Publish-Subscribe** yang sangat populer di IoT karena hemat bandwidth dan low-latency.

### Komponen MQTT:
1.  **Broker**: Server pusat yang mengatur lalu lintas pesan (e.g., Mosquitto, HiveMQ, EMQX).
2.  **Publisher**: Perangkat yang mengirim data (misal: mikrokontroler mengirim suhu).
3.  **Subscriber**: Perangkat yang menerima data (misal: dashboard monitoring).

### Topik (Topic)
Alamat pesan berupa string hierarkis, misal:
- `rumah/ruang_tamu/suhu`
- `pabrik/mesin_1/vibrasi`
- Wildcard `+` (satu level) dan `#` (multi level) untuk subscribe banyak topik.

### QoS (Quality of Service)
1.  **QoS 0 (At most once)**: "Fire and forget". Pesan dikirim sekali, tidak ada garansi sampai. Tercepat.
2.  **QoS 1 (At least once)**: Pesan digaransi sampai minimal sekali (bisa duplikat). Ada handshake `PUBACK`.
3.  **QoS 2 (Exactly once)**: Pesan digaransi sampai tepat satu kali. Terlambat tapi terhandal.

---

## 4. Hardware Implementation
### ESP32 (WiFi Integrasi)
ESP32 memiliki MAC dan PHY WiFi internal. Stack LwIP berjalan di salah satu core (biasanya Core 0) via FreeRTOS.

### STM32 + W5500 (Ethernet SPI)
STM32 sering menggunakan modul eksternal seperti W5500 (Wiznet).
- W5500 unik karena memiliki **Hardware TCP/IP Stack** sendiri (Offloading), sehingga beban CPU STM32 sangat ringan.
- Komunikasi via SPI.
