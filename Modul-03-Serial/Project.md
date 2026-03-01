# Project Modul 03: Sistem Gateway Komunikasi Serial

## Informasi Project

| Item | Keterangan |
|------|------------|
| Modul | 03 — Komunikasi Serial UART |
| Platform | STM32F103C8T6 + ESP32 DevKit V1 |
| Durasi | 2 minggu |
| Tipe | Dual-MCU dengan protokol komunikasi custom |

---

## Deskripsi Umum

Project ini mengembangkan seluruh konsep dari 12 percobaan praktikum serial UART ke dalam sebuah sistem terintegrasi **Gateway Sensor Network** berbasis dual mikrokontroler. Mahasiswa harus menggabungkan teknik polling, interrupt, ring buffer, command parser, JSON protocol, framing, CRC, timeout parsing, multi-UART bridge, dan error monitoring ke dalam satu sistem yang bekerja bersama.

---

## Soal Cerita

### Skenario: Sistem Monitoring Lingkungan Laboratorium Kimia

Sebuah laboratorium kimia di universitas membutuhkan sistem monitoring lingkungan real-time. Laboratorium ini memiliki beberapa ruangan yang masing-masing perlu dimonitor kondisi suhu, kelembaban, dan kualitas udaranya. Karena ruangan-ruangan ini tersebar dan tidak semua memiliki akses WiFi langsung, diperlukan arsitektur gateway.

**STM32F103C8T6 (Sensor Node)** dipasang di setiap ruangan untuk mengumpulkan data dari sensor lokal. Data dikumpulkan secara periodik menggunakan **timer interrupt** dan disimpan sementara di **ring buffer** sebelum dikirimkan. Setiap node memiliki **command parser** untuk menerima perintah konfigurasi (sampling rate, threshold) dari gateway.

**ESP32 DevKit V1 (Gateway Hub)** ditempatkan di ruang server dan terhubung ke beberapa sensor node via kabel UART. Gateway menerima data dari node menggunakan **timeout-based parser** untuk mendeteksi batas paket. Data yang diterima divalidasi menggunakan **CRC-8 checksum** dan di-decode dari format **STX/ETX framing**. Gateway menampilkan data di Serial Monitor dalam format **JSON terstruktur** dan menyediakan **interactive line editor** untuk operator memasukkan perintah.

Jika terjadi anomali (suhu > 40°C atau humidity > 80% atau kualitas udara buruk), gateway mengaktifkan buzzer alarm dan mengirim perintah ke node terkait untuk meningkatkan frekuensi sampling. Sistem juga memiliki **error statistics monitor** yang melacak kesehatan link komunikasi setiap node.

---

## Spesifikasi Teknis

### Mapping Percobaan ke Fitur Sistem

| No | Percobaan Praktikum | Fitur dalam Project |
|----|---------------------|---------------------|
| P01 | UART Echo (Polling) | Handshake awal: node echo-back ACK saat gateway mengirim PING |
| P02 | Interrupt RX | Penerimaan data di gateway dan node berbasis interrupt (non-blocking) |
| P03 | Ring Buffer | Node menyimpan data sensor ke ring buffer saat belum bisa kirim |
| P04 | Printf Redirect | Node menampilkan tabel data sensor ke debug port USART2 |
| P05 | Command Parser | Node menerima perintah dari gateway: `SAMPLE <rate>`, `THRESHOLD <temp> <hum>`, `STATUS`, `RESET` |
| P06 | JSON Protocol | Gateway menampilkan data ke operator dalam format JSON: `{"node":1,"temp":25.3,"hum":60,"air":450}` |
| P07 | Line Editor | Operator gateway mengetik perintah via line editor interaktif dengan history: `node 1 sample 5`, `alert config`, `log show` |
| P08 | STX/ETX Framing | Semua komunikasi node↔gateway menggunakan framing STX/ETX dengan byte stuffing |
| P09 | CRC-8 Checksum | Setiap paket data memiliki CRC-8 untuk validasi integritas |
| P10 | Timeout Parser | Gateway mendeteksi batas paket dari node menggunakan timeout 50ms |
| P11 | Multi-UART Bridge | Gateway bridge antara UART0 (operator terminal) dan UART1/UART2 (ke node) |
| P12 | Error Statistics | Gateway melaporkan error rate setiap link (parity, framing, overrun) dan health status per node |

### Protokol Komunikasi Node → Gateway

```
| STX | NODE_ID | MSG_TYPE | LENGTH | DATA... | CRC8 | ETX |
| 0x02 | 1 byte  | 1 byte   | 1 byte | N bytes | 1 byte | 0x03 |
```

**MSG_TYPE:**
- `0x01` — Sensor Data (temp, humidity, air quality)
- `0x02` — Alert (threshold exceeded)
- `0x03` — Status Response
- `0x04` — ACK/NACK

### Protokol Komunikasi Gateway → Node

```
| STX | NODE_ID | CMD_TYPE | LENGTH | PARAMS... | CRC8 | ETX |
```

**CMD_TYPE:**
- `0x10` — Set Sampling Rate
- `0x11` — Set Threshold
- `0x12` — Request Status
- `0x13` — Reset Node
- `0xFF` — Broadcast PING

### State Machine Gateway

```
                    ┌───────────┐
                    │   IDLE    │
                    └─────┬─────┘
                          │ Terima paket / timeout
                    ┌─────▼─────┐
                    │  RECEIVE  │──── CRC Fail ───► INCREMENT_ERROR
                    └─────┬─────┘
                          │ CRC Pass
                    ┌─────▼─────┐
                    │  PROCESS  │
                    └─────┬─────┘
                     ┌────┴────┐
                     ▼         ▼
               ┌──────────┐ ┌──────────┐
               │ DATA_LOG │ │  ALERT   │
               └──────────┘ └──────────┘
                     │         │ Threshold exceeded
                     │    ┌────▼─────┐
                     │    │  ALARM   │──► Buzzer + LED + Increase sampling
                     │    └──────────┘
                     ▼
               ┌──────────┐
               │ DISPLAY  │──► JSON output ke terminal operator
               └──────────┘
```

---

## Ketentuan Pengerjaan

1. **Struktur folder:**
   ```
   Project-03-Lab-Monitor/
   ├── STM32-Sensor-Node/
   │   ├── Src/main.c
   │   ├── Src/ring_buffer.c
   │   ├── Src/command_parser.c
   │   ├── Src/frame_protocol.c
   │   └── Inc/...
   └── ESP32-Gateway/
       ├── main/main.c
       ├── main/frame_protocol.c
       ├── main/json_display.c
       ├── main/line_editor.c
       ├── main/error_monitor.c
       └── main/...
   ```

2. Kedua MCU harus bekerja secara terintegrasi melalui UART.
3. Setiap fitur harus dapat didemonstrasikan dan diuji.
4. Kode harus modular — setiap fitur di file terpisah.
5. Komunikasi harus reliable — CRC validation + ACK/NACK.

---

## Rubrik Penilaian

| No | Komponen | Bobot |
|----|----------|-------|
| 1 | Komunikasi UART STM32 ↔ ESP32 berfungsi (framing + CRC) | 20% |
| 2 | Command parser dan line editor di gateway | 15% |
| 3 | Ring buffer dan interrupt-based reception | 15% |
| 4 | JSON display dan error monitoring | 15% |
| 5 | Multi-UART bridge (operator ↔ node) | 10% |
| 6 | Alarm system (threshold + buzzer) | 10% |
| 7 | Kode modular dan dokumentasi | 10% |
| 8 | Demo dan video penjelasan | 5% |

---

## Referensi

1. STM32F103xx Reference Manual (RM0008) — Chapter 27: USART
2. ESP-IDF Programming Guide — UART Driver API
3. Mastering STM32, Carmine Noviello — Chapter 9: USART
4. Serial Port Complete, Jan Axelson — Framing & Error Detection
