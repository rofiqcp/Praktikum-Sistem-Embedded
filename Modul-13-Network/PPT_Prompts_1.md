# Prompt untuk Slide Presentasi Teori (Minggu 1)
## Topik: Embedded Networking Basics & TCP/IP

Buatkan outline slide presentasi yang mendalam tentang dasar jaringan untuk sistem embedded.

### Slide 1: Judul
- **Judul**: Introduction to Embedded Networking
- **Subjudul**: Menghubungkan Mikrokontroler ke Internet (TCP/IP & LwIP)
- **Visual**: Ilustrasi mikrokontroler terhubung ke router WiFi dan Cloud.

### Slide 2: Mengapa Embedded System Perlu "Online"?
- **Poin Utama**:
  - IoT (Internet of Things).
  - Monitoring Jarak Jauh (Telemetry).
  - Kontrol Jarak Jauh (Remote Control).
  - OTA (Over-The-Air) Updates.
- **Visual**: Diagram blok sistem IoT sederhana (Sensor -> MCU -> Cloud -> User).

### Slide 3: OSI Model vs TCP/IP Model
- **Poin Utama**:
  - Penjelasan singkat 7 Layer OSI.
  - Fokus pada 4 Layer TCP/IP: Link, Internet, Transport, Application.
  - Perbandingan keduanya.
- **Visual**: Diagram piramida perbandingan OSI vs TCP/IP.

### Slide 4: Transport Layer: TCP vs UDP
- **Poin Utama**:
  - **TCP (Transmission Control Protocol)**: Connection-oriented, Reliable, Lambat (ada handshake & ack). Cocok untuk HTTP, MQTT, Email.
  - **UDP (User Datagram Protocol)**: Connectionless, Unreliable, Cepat. Cocok untuk Streaming Video/Audio, VoIP.
- **Visual**: Tabel perbandingan fitur TCP dan UDP.

### Slide 5: IP Addressing & DHCP
- **Poin Utama**:
  - IPv4 Address (e.g., 192.168.1.100).
  - Static IP vs Dynamic IP (DHCP).
  - MAC Address (Hardware Address).
- **Penjelasan**: Bagaimana MCU mendapatkan IP saat pertama kali connect? (DHCP Discover/Offer/Request/Ack).

### Slide 6: LwIP (Lightweight IP)
- **Poin Utama**:
  - Tantangan networking di MCU: RAM & Flash terbatas.
  - Solusi: LwIP Stack.
  - Fitur LwIP: Support TCP/UDP/ICMP/DHCP dengan footprint kecil.
- **Visual**: Arsitektur software MCU (User App -> LwIP -> Driver -> Hardware).

### Slide 7: Hardware Connectivity
- **Poin Utama**:
  - **WiFi (ESP32)**: Radio terintegrasi, mudah digunakan, konsumsi daya cukup tinggi.
  - **Ethernet (STM32 + W5500)**: Stabil, cepat, wired, butuh hardware tambahan.
- **Visual**: Foto modul ESP32 dan modul Ethernet W5500.

### Slide 8: Kesimpulan Minggu Ini
- **Poin Utama**: 
  - Networking membuka potensi IoT.
  - Memahami stack TCP/IP penting sebelum masuk ke protokol aplikasi.
  - Minggu depan: HTTP & MQTT.
