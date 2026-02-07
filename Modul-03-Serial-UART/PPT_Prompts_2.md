# Prompt untuk Pembuatan PPT - Bagian 2: Praktikum Serial UART
## Modul 03: Implementasi Komunikasi Serial UART

---

## 📋 Informasi Presentasi

| Item | Keterangan |
|------|------------|
| **Topik** | Praktikum & Project Serial UART |
| **Jumlah Slide** | 28 Slide |
| **Durasi** | 90-120 menit (Sesi Lab) |
| **Fokus** | Hands-on, Wiring, Coding, Debugging |
| **Platform** | STM32F103 (Arduino framework) & ESP32 |

---

## 🎨 Panduan Desain

### Tema Visual
- **Color Palette:** Sama dengan Part 1 (Deep Blue, Tech Cyan)
- **Style:** Tutorial step-by-step, Diagram Fritzing/Wiring, Screenshots code
- **Emphasis:** Safety warnings (hijau untuk aman, merah untuk bahaya)

### Elemen Visual
- Wiring diagram yang sangat jelas (pin labels)
- Screenshot PlatformIO / VS Code
- Foto hardware real
- Video clips (embedded atau link) untuk hasil output

---

## 📊 Struktur Slide Detail

### Slide 1: Cover Praktikum
**Prompt:**
```
Buat slide cover untuk sesi praktikum.
Judul: "Praktikum Modul 03: Serial UART"
Subtitle: "STM32F103 & ESP32 Communication Implementation"

Visual:
- Split screen image: Kiri STM32 Blue Pill, Kanan ESP32 DevKit
- Kabel jumper menghubungkan keduanya di tengah
- Logo tools: VS Code, PlatformIO, Serial Monitor
```

---

### Slide 2: Review Safety & Rules
**Prompt:**
```
Buat slide aturan keselamatan praktikum.

Judul: "Peraturan Keselamatan Lab"

Aturan Utama:
1. ⚠️ Cek tegangan supply! (STM32 = 3.3V, ESP32 = 5V/3.3V)
2. ⚠️ Jangan hubungkan 5V Logic ke 3.3V Pin tanpa level converter
3. ⚠️ Pastikan ground (GND) terhubung antar device
4. Cabut power saat merakit rangkaian
5. Double check wiring sebelum upload

Visual: Icon peringatan dan checklist safety
```

---

### Slide 3: Persiapan Hardware
**Prompt:**
```
Buat slide checklist alat dan bahan.

Judul: "Tools & Components Checklist"

Hardware:
- 1x STM32F103C8T6 (Blue Pill) + ST-Link
- 1x ESP32 DevKitC V4
- 1x USB-TTL Converter (CP2102/CH340)
- Breadboard & Jumper wires (Male-Male, Male-Female)
- Kabel Micro USB

Software:
- VS Code
- PlatformIO Extension
- Driver CH340 / CP2102
- Serial Terminal (HTerm / CoolTerm recommended)

Gambar: Foto flat-lay semua komponen di atas meja
```

---

### Slide 4: Percobaan 1 - UART Basic (STM32)
**Prompt:**
```
Buat slide judul untuk Percobaan 1.

Judul: "Percobaan 1: Dasar Serial UART pada STM32"

Tujuan:
- Mengirim data string ke PC via USB-TTL
- Menerima command dari PC
- Mengontrol LED via command serial

Konfigurasi:
- UART1 (PA9=TX, PA10=RX)
- Baudrate: 9600
- LED di PC13

Diagram Wiring:
STM32 PA9  (TX) → USB-TTL RX
STM32 PA10 (RX) → USB-TTL TX
STM32 GND       → USB-TTL GND
STM32 3.3V      → USB-TTL 3.3V
```

---

### Slide 5: Kode Percobaan 1 (STM32)
**Prompt:**
```
Buat slide kode program untuk percobaan 1.

Judul: "Kode Percobaan 1: Simple Echo & Control"

Code Snippet (C++):
```cpp
void setup() {
    pinMode(PC13, OUTPUT);
    Serial.begin(9600);
    Serial.println("STM32 Ready!");
}

void loop() {
    if (Serial.available()) {
        char c = Serial.read();
        Serial.print("Received: ");
        Serial.println(c);
        
        if (c == '1') digitalWrite(PC13, LOW); // LED ON
        if (c == '0') digitalWrite(PC13, HIGH); // LED OFF
    }
}
```
Penjelasan singkat:
- `Serial.available()` cek data masuk
- `Serial.read()` ambil 1 byte
- Logic LED PC13 active LOW
```

---

### Slide 6: Result Percobaan 1
**Prompt:**
```
Buat slide hasil yang diharapkan untuk Percobaan 1.

Judul: "Verifikasi Hasil Percobaan 1"

Langkah Pengujian:
1. Upload kode ke STM32
2. Buka Serial Monitor (Baud 9600)
3. Reset STM32 → Muncul "STM32 Ready!"
4. Ketik '1' → LED menyala, muncul "Received: 1"
5. Ketik '0' → LED mati, muncul "Received: 0"

Troubleshooting:
- Tidak muncul text? Cek baudrate dan kabel TX/RX terbalik.
- LED terbalik? Ingat PC13 active LOW.
```

---

### Slide 7: Percobaan 2 - Command Parser (ESP32)
**Prompt:**
```
Buat slide judul untuk Percobaan 2.

Judul: "Percobaan 2: Command Parser pada ESP32"

Tujuan:
- Menerima string command lengkap (akhiran newline)
- Parsing perintah "LED:ON" dan "LED:OFF"
- Parsing perintah dengan parameter "PWM:128"

Wiring:
ESP32 terhubung ke PC via kabel Micro USB (menggunakan Serial0)
LED eksternal di GPIO 2 (Built-in)
```

---

### Slide 8: Kode Percobaan 2 (ESP32)
**Prompt:**
```
Buat slide kode parsing command pada ESP32.

Judul: "Kode Percobaan 2: String Parsing"

Code Snippet:
```cpp
String inString = "";
void loop() {
  while (Serial.available()) {
    char inChar = (char)Serial.read();
    if (inChar == '\n') {
      processCommand(inString);
      inString = "";
    } else {
      inString += inChar;
    }
  }
}

void processCommand(String cmd) {
  cmd.trim(); // Hapus whitespace
  if (cmd == "LED:ON") digitalWrite(2, HIGH);
  else if (cmd == "LED:OFF") digitalWrite(2, LOW);
  else if (cmd.startsWith("PWM:")) {
    int val = cmd.substring(4).toInt();
    // Set PWM logic here
  }
}
```
```

---

### Slide 9: Result Percobaan 2
**Prompt:**
```
Buat slide hasil yang diharapkan untuk Percobaan 2.

Judul: "Verifikasi Hasil Percobaan 2"

Screenshot Serial Monitor:
User Input: "LED:ON"
Output: "Command Received: LED:ON -> Execute Turn ON"

User Input: "PWM:150"
Output: "Set PWM Duty to 150"

Analisis:
- Menggunakan `String` object memudahkan, tapi hati-hati fragmentasi memori.
- `\n` (newline) digunakan sebagai delimiter akhir paket.
```

---

### Slide 10: Percobaan 3 - Komunikasi Dua Arah
**Prompt:**
```
Buat slide inti praktikum: Komunikasi STM32 ke ESP32.

Judul: "Percobaan 3: STM32 to ESP32 Talk"

Skenario:
1. STM32 membaca internal counter (simulasi sensor).
2. STM32 mengirim data format: "DATA:123\n".
3. ESP32 menerima, mem-parse, dan menampilkan di Serial Monitor PC.

Wiring Diagram Penting:
STM32 PA2 (TX2) ──────► ESP32 GPIO16 (RX2)
STM32 PA3 (RX2) ◄────── ESP32 GPIO17 (TX2)
STM32 GND       ─────── ESP32 GND
```

---

### Slide 11: Kode Sender (STM32)
**Prompt:**
```
Buat slide kode sender untuk STM32.

Judul: "Percobaan 3: Kode Sender (STM32)"

Code config:
- Gunakan `Serial2` (HardwareSerial)
- Baudrate 115200

Snippet:
```cpp
int counter = 0;
void setup() {
  Serial2.begin(115200); // PA2, PA3
}
void loop() {
  counter++;
  Serial2.print("DATA:");
  Serial2.println(counter);
  delay(1000);
}
```
Note: Gunakan `Serial2` bukan `Serial` (Serial untuk upload/debug).
```

---

### Slide 12: Kode Receiver (ESP32)
**Prompt:**
```
Buat slide kode receiver untuk ESP32.

Judul: "Percobaan 3: Kode Receiver (ESP32)"

Code config:
- Gunakan `Serial1` atau `Serial2` dengan pin remapping
- ESP32 HardwareSerial: Serial2 default pin 16, 17

Snippet:
```cpp
void setup() {
  Serial.begin(115200);  // Debug ke PC
  Serial2.begin(115200); // RX dari STM32 (Pin 16, 17)
}
void loop() {
  if (Serial2.available()) {
    String data = Serial2.readStringUntil('\n');
    Serial.print("Received from STM32: ");
    Serial.println(data);
  }
}
```

Visual: Alur data
STM32 (Serial2) → [Kabel] → ESP32 (Serial2) → [Internal] → ESP32 (Serial) → PC
```

---

### Slide 13: Result Percobaan 3
**Prompt:**
```
Buat slide verifikasi komunikasi dua microcontroller.

Judul: "Verifikasi Komunikasi Antar MCU"

Expected Serial Monitor Output (di ESP32 port):
```
Received from STM32: DATA:1
Received from STM32: DATA:2
Received from STM32: DATA:3
...
```

Tantangan Troubleshooting:
- Output kosong? Cek koneksi RX-TX silang.
- Output karakter aneh ``? Cek kesesuaian baudrate (115200).
- Data terpotong? Cek `readStringUntil` timeout.
```

---

### Slide 14: Percobaan 4 - Protokol Biner
**Prompt:**
```
Buat slide upgrade ke protokol biner yang lebih robust.

Judul: "Percobaan 4: Implementasi Protokol Struktur Data"

Masalah String: Boros bandwidth, parsing lambat.
Solusi: Kirim raw byte struct.

Struct Definition (Shared di kedua MCU):
```cpp
struct Packet {
  uint8_t startByte; // 0xAA
  uint16_t sensor1;
  uint16_t sensor2;
  uint8_t checksum;
};
```
Total size: 6 bytes. Jauh lebih efisien dari string "DATA:1024,2048\n" (15 bytes).
```

---

### Slide 15: Kode Biner Sender (STM32)
**Prompt:**
```
Buat slide kode pengiriman raw bytes.

Judul: "Kode Sender Biner (STM32)"

Snippet:
```cpp
Packet pkt;
pkt.startByte = 0xAA;

void loop() {
  pkt.sensor1 = analogRead(PA0);
  pkt.sensor2 = analogRead(PA1);
  pkt.checksum = (pkt.startByte + pkt.sensor1 + pkt.sensor2) & 0xFF;
  
  Serial2.write((uint8_t*)&pkt, sizeof(pkt));
  delay(100);
}
```
Penjelasan `Serial.write` vs `Serial.print`.
```

---

### Slide 16: Kode Biner Receiver (ESP32)
**Prompt:**
```
Buat slide kode penerimaan raw bytes.

Judul: "Kode Receiver Biner (ESP32)"

Snippet:
```cpp
void loop() {
  if (Serial2.available() >= sizeof(Packet)) {
    Packet received;
    Serial2.readBytes((uint8_t*)&received, sizeof(Packet));
    
    // Validasi start byte dan checksum
    uint8_t calcChecksum = (...);
    
    if (received.startByte == 0xAA && calcChecksum == received.checksum) {
      Serial.printf("S1: %d, S2: %d\n", received.sensor1, received.sensor2);
    } else {
      Serial.println("Packet Corrupted!");
      // Flush buffer cleanup code
    }
  }
}
```
Penting: Validasi data untuk membuang noise.
```

---

### Slide 17: Project Overview
**Prompt:**
```
Buat slide pengantar Project Modul 3.

Judul: "Final Project: Wireless Sensor Network Gateway"

Objective: Membuat sistem monitoring lingkungan terintegrasi.
Role:
- **STM32**: Sensor Node (Baca Suhu, Cahaya, Gerak) → Kirim Data
- **ESP32**: Gateway (Terima Data, Validasi) → Tampilkan Web Dashboard

Features:
- Protokol komunikasi custom dengan Error Checking (CRC)
- Bidirectional communication (Control LED STM32 dari Web ESP32)
- Reconnection handling
```

---

### Slide 18: Project Spec - STM32 Node
**Prompt:**
```
Buat slide spesifikasi tugas STM32 dalam project.

Judul: "Tugas STM32: Sensor Node Controller"

Input:
- Potensio (Simulasi Suhu/Kelembaban) di PA0, PA1
- Push Button (Simulasi Alert) di PA4

Output:
- UART Packet Sending (Interval 500ms)
- Status LED (Blinking saat TX)
- RX Handler (Menerima perintah dari ESP32 untuk nyalakan Buzzer/LED)

Protokol:
START(0x02) | ID | DATA_LEN | PAYLOAD... | CHECKSUM | STOP(0x03)
```

---

### Slide 19: Project Spec - ESP32 Gateway
**Prompt:**
```
Buat slide spesifikasi tugas ESP32 dalam project.

Judul: "Tugas ESP32: IoT Gateway"

Input:
- UART RX dari STM32

Output:
- Web Server Dashboard (IP Address lokal)
- Menampilkan grafik realtime (Chart.js optional)
- Tombol kontrol di Web untuk kirim perintah balik ke STM32

Logic:
- Buffer Circular untuk data masuk
- Parsing protocol frame
- Update web client (AJAX/WebSocket)
```

---

### Slide 20: Wiring Diagram Project
**Prompt:**
```
Buat slide referensi wiring lengkap untuk project.

Judul: "Project Hardware Setup"

Gambar skematik yang jelas menghubungkan:
STM32 [PA2, PA3] <---> ESP32 [GPIO16, GPIO17]
Sensor ke STM32
Power distribution (3.3V vs 5V handling)

Checklist koneksi:
[ ] TX-RX Cross Connection
[ ] Common Ground
[ ] Power stable
```

---

### Slide 21: Protokol Detail
**Prompt:**
```
Buat slide definisi protokol yang harus diimplementasikan.

Judul: "Spesifikasi Protokol Project"

Frame Format (Fixed 10 Bytes):
Byte 0: Header (0xFE)
Byte 1: Command ID (0x01=Data, 0x02=Alert, 0x03=Control)
Byte 2-3: Sensor 1 Value (High, Low)
Byte 4-5: Sensor 2 Value (High, Low)
Byte 6: Status Flags
Byte 7: Reserved
Byte 8: Checksum (XOR Byte 0-7)
Byte 9: Footer (0xFF)

Mahasiswa wajib membuat fungsi `buildFrame()` dan `parseFrame()`.
```

---

### Slide 22: Tips Pengerjaan Project (Part 1)
**Prompt:**
```
Buat slide tips sukses untuk project.

Judul: "Strategi Pengerjaan - Fase 1"

1. **Test Hardware dulu**: Pastikan wiring benar dengan kode "Blink" sederhana.
2. **Loopback Test**: Hubungkan TX-RX STM32 sendiri. Kirim data, pastikan diterima kembali dengan benar.
3. **Raw Data Test**: Kirim data dummy dari STM32, lihat di Serial Monitor ESP32 (tanpa parsing).
```

---

### Slide 23: Tips Pengerjaan Project (Part 2)
**Prompt:**
```
Buat slide strategi debugging lanjutan.

Judul: "Strategi Pengerjaan - Fase 2"

4. **Implement Parsing**: Buat fungsi parser di ESP32, print hasil ke Serial Monitor.
5. **Checksum**: Implementasikan validasi checksum. Print "Error" jika checksum salah.
6. **Web Interface**: Terakhir, integrasikan data variable ke HTML page.

"Divide and Conquer: Jangan integrasikan semua sekaligus!"
```

---

### Slide 24: Common Mistakes
**Prompt:**
```
Buat slide kesalahan umum yang sering terjadi.

Judul: "Common Pitfalls & Solusi"

1. **Baudrate Mismatch**: Data terbaca sampah.
2. **Lupa Common GND**: Komunikasi tidak stabil / gagal.
3. **Logic Level issue**: STM32 toleran 5V di pin tertentu (FT), tapi ESP32 strictly 3.3V. Aman karena sama-sama 3.3V logic.
4. **Buffer Overflow**: Mengirim data terlalu cepat tanpa delay (banjir data).
5. **Blocking Code**: Menggunakan `delay()` yang lama sehingga data serial terlewat. Gunakan `millis()`!
```

---

### Slide 25: Rubrik Penilaian
**Prompt:**
```
Buat slide rubrik penilaian project.

Judul: "Kriteria Penilaian Project"

1. **Fungsionalitas (40%)**: Data terkirim, diterima, dan tampil di Web.
2. **Reliabilitas (20%)**: Tidak ada data error/garbage, auto-reconnect.
3. **Protokol (20%)**: Implementasi framing dan checksum yang benar.
4. **Kode (10%)**: Struktur kode, komentar, kerapian.
5. **Video Demo (10%)**: Kejelasan penjelasan dan demonstrasi.
```

---

### Slide 26: Dokumentasi Video
**Prompt:**
```
Buat slide instruksi tugas video.

Judul: "Tugas Video Demonstrasi"

Format:
- Durasi: Max 10 menit
- Intro: Nama & NIM
- Penjelasan Wiring & Konsep
- Demo Fungsional (Tunjukkan dashboard & hardware beraksi)
- Code Walkthrough (Jelaskan bagian inti parsing/sending)
- Upload ke YouTube / G.Drive link
```

---

### Slide 27: Referensi Tambahan
**Prompt:**
```
Buat slide link belajar mandiri.

Judul: "Resources & Reference"

- STM32UART Reference Manual
- ESP32 Serial API Docs
- JSON Serialization Library (ArduinoJson)
- Tutorial Circular Buffer C/C++
```

---

### Slide 28: Closing & Q&A
**Prompt:**
```
Buat slide penutup.

Judul: "Sesi Praktikum Selesai"

- Simpan semua peralatan kembali ke tempatnya.
- Pastikan area kerja bersih.
- Deadline pengumpulan project: [Tanggal]

"Ada Pertanyaan? Silakan diskusi."
```