# Prompt untuk Slide Presentasi Teori (Minggu 2)
## Topik: Application Protocols (HTTP & MQTT)

Buatkan outline slide presentasi tentang protokol aplikasi IoT.

### Slide 1: Judul
- **Judul**: IoT Application Protocols
- **Subjudul**: Memahami HTTP, MQTT, dan REST API
- **Visual**: Logo HTTP dan MQTT berdampingan.

### Slide 2: HTTP (HyperText Transfer Protocol)
- **Poin Utama**:
  - Protokol dasar Web.
  - Model: Request (Client) - Response (Server).
  - Stateless (Tidak menyimpan state koneksi).
- **Visual**: Diagram panah Request/Response antara Client dan Server.

### Slide 3: HTTP Methods & Status Codes
- **Poin Utama**:
  - **GET**: Meminta data.
  - **POST**: Mengirim data.
  - **PUT/DELETE**: Update/Hapus data.
  - **Status Code**: 200 OK, 404 Not Found, 500 Server Error.
- **Visual**: Tabel method dan contoh penggunaannya.

### Slide 4: Mengenal MQTT (Message Queuing Telemetry Transport)
- **Poin Utama**:
  - Didesain khusus untuk IoT (Machine-to-Machine).
  - Sangat ringan (header kecil, hemat data).
  - Model: **Publish-Subscribe**.
- **Visual**: Ilustrasi Publisher, Broker, dan Subscriber.

### Slide 5: Arsitektur Pub-Sub
- **Poin Utama**:
  - Tidak ada koneksi langsung antara pengirim dan penerima.
  - **Broker**: Perantara (Post Office).
  - **Topic**: Alamat pengiriman (e.g., "polinema/lab1/suhu").
- **Analogi**: Sama seperti subscribe channel YouTube atau Follow IG (dapat update otomatis).

### Slide 6: HTTP vs MQTT
- **Poin Utama**:
  - **HTTP**: Berat, overhead besar, satu arah (client pull). Cocok untuk kirim file gambar/dokumen.
  - **MQTT**: Ringan, real-time, dua arah (push). Cocok untuk data sensor serial & kontrol.
- **Visual**: Grafik perbandingan Overhead data & Latency.

### Slide 7: Data Serialization (JSON)
- **Poin Utama**:
  - Bagaimana data dikemas agar bisa dibaca di berbagai bahasa pemrograman (C++, Python, JS).
  - Struktur Key-Value Pair.
  - Library: ArduinoJson.
- **Visual**: Contoh snippet JSON data sensor.

### Slide 8: Kesimpulan & Tips Project
- **Poin Utama**:
  - Gunakan HTTP untuk konfigurasi awal / update firmware (OTA).
  - Gunakan MQTT untuk telemetri data real-time & kontrol.
  - Pastikan menggunakan QoS yang tepat.
