# Jobsheet Modul 03: Menguasai Komunikasi Serial — UART

## Praktikum Sistem Embedded

**Semester:** Genap 2025/2026  
**Durasi:** 3 × 50 menit (2 pertemuan)  
**Platform:** ESP32 DevKit V1 & STM32 Blue Pill (STM32F103C8T6)

---

## 1. Tujuan Praktikum

Setelah menyelesaikan praktikum ini, mahasiswa diharapkan mampu:

1. **Memahami komunikasi serial UART** — Menjelaskan frame data UART (start bit, data bits, parity, stop bit), baud rate, dan parameter konfigurasi 8N1.
2. **Mengkonfigurasi UART pada ESP32 dan STM32** — Menggunakan API ESP-IDF (`uart_driver_install()`, `uart_read_bytes()`) dan HAL STM32 (`HAL_UART_Receive()`, `HAL_UART_Transmit()`) untuk komunikasi serial.
3. **Mengimplementasikan teknik penerimaan data** — Membandingkan metode polling, interrupt-based, dan event queue untuk penerimaan data serial.
4. **Membangun protokol komunikasi** — Merancang command parser, JSON protocol, framing STX/ETX dengan byte stuffing, dan CRC checksum.
5. **Menerapkan teknik buffering dan error handling** — Mengimplementasikan ring buffer, timeout-based parser, multi-UART bridge, dan error statistics monitor.

---

## 2. Peralatan dan Komponen

### 2.1 Perangkat Keras

| No | Komponen | Jumlah | Keterangan |
|----|----------|--------|------------|
| 1 | ESP32 DevKit V1 | 1 | Mikrokontroler utama (3 UART) |
| 2 | STM32 Blue Pill (STM32F103C8T6) | 1 | Mikrokontroler pembanding (3 USART) |
| 3 | ST-Link V2 | 1 | Programmer untuk STM32 |
| 4 | Kabel USB Micro-B | 2 | Untuk ESP32 dan ST-Link |
| 5 | USB-TTL Converter (CH340/CP2102) | 1 | Untuk komunikasi UART eksternal |
| 6 | Breadboard 830 titik | 1 | Papan rangkaian |
| 7 | LED 5mm (merah) | 1 | Indikator status |
| 8 | Resistor 330Ω | 1 | Pembatas arus LED |
| 9 | Active Buzzer 5V | 1 | Untuk percobaan command parser |
| 10 | Kabel jumper male-male | 15 | Koneksi antar komponen |
| 11 | Kabel jumper male-female | 10 | Koneksi ke modul |
| 12 | Logic Analyzer (opsional) | 1 | Untuk analisis sinyal UART |

### 2.2 Perangkat Lunak

| No | Software | Keterangan |
|----|----------|------------|
| 1 | VS Code + PlatformIO | IDE pengembangan |
| 2 | Serial Monitor (115200 baud) | Menampilkan output program |
| 3 | Python 3.x + pyserial (opsional) | Untuk pengujian otomatis |
| 4 | Driver CP210x / CH340 | Driver USB-to-Serial |

---

## 3. Teori Singkat

### 3.1 Konsep UART

UART (Universal Asynchronous Receiver/Transmitter) adalah protokol komunikasi serial asynchronous yang mentransmisikan data secara bit-per-bit melalui dua jalur: TX (transmit) dan RX (receive). Setiap frame data terdiri dari: **start bit** (logika 0), **data bits** (5–9 bit), opsional **parity bit** (even/odd), dan **stop bit** (1–2 bit logika 1). Konfigurasi standar adalah **8N1** (8 data bits, no parity, 1 stop bit) dengan baud rate 115200.

### 3.2 UART pada ESP32 dan STM32

ESP32 memiliki 3 port UART (UART0, UART1, UART2) dengan pin yang dapat di-remap ke GPIO manapun. UART0 (GPIO1/GPIO3) terhubung ke USB-to-Serial onboard. ESP-IDF menyediakan event queue untuk menangani UART secara non-blocking menggunakan FreeRTOS.

STM32F103 memiliki 3 USART (USART1, USART2, USART3). USART1 (PA9/PA10) adalah port utama yang terhubung melalui ST-Link atau USB-TTL converter. HAL menyediakan metode polling (`HAL_UART_Receive()`), interrupt (`HAL_UART_Receive_IT()`), dan DMA untuk penerimaan data.

---

## 4. Langkah Percobaan

> **Catatan Umum:**
> - Pastikan Serial Monitor diatur ke **115200 baud, 8N1**
> - Setiap program tersedia dalam dua versi: ESP32 (`praktikum/ESP32/`) dan STM32 (`praktikum/STM32/`)
> - Untuk STM32, gunakan ST-Link V2 dan USB-TTL converter pada PA9/PA10
> - Ketik input melalui Serial Monitor dan tekan Enter untuk mengirim

---

### Percobaan 01: UART Echo — Polling

**Tujuan:** Memahami komunikasi serial dasar dengan metode polling untuk menerima dan mengirim kembali data byte per byte.

#### Rangkaian

| Komponen | ESP32 | STM32 |
|----------|-------|-------|
| UART TX | GPIO1 (USB) | PA9 (USART1) |
| UART RX | GPIO3 (USB) | PA10 (USART1) |
| LED | — | PC13 (built-in) |

> ESP32 menggunakan UART0 via USB (tidak perlu koneksi tambahan). STM32 memerlukan USB-TTL converter pada PA9/PA10.

#### Langkah Kerja

1. Buka project `praktikum/ESP32/ESP32_01` atau `praktikum/STM32/STM32_01` di PlatformIO.
2. Pelajari kode `main.c` — perhatikan konfigurasi UART: baud rate, data bits, stop bits, flow control.
3. Identifikasi fungsi yang digunakan untuk menerima data secara polling:
   - ESP32: `uart_read_bytes()` dengan timeout 20ms
   - STM32: `HAL_UART_Receive()` dengan timeout blocking
4. Perhatikan bagaimana data yang diterima langsung dikirim kembali (echo):
   - ESP32: `uart_write_bytes()`
   - STM32: `HAL_UART_Transmit()`
5. Build dan upload program ke board.
6. Buka Serial Monitor (115200 baud).
7. Ketik beberapa karakter dan amati apakah karakter di-echo kembali.
8. Coba ketik cepat — apakah ada karakter yang hilang?
9. Catat waktu respons (subjektif) dan perilaku program.

#### Tabel Pengamatan

| No | Input Dikirim | Output Diterima | Karakter Hilang? | Catatan |
|----|--------------|-----------------|------------------|---------|
| 1 | `A` | | | |
| 2 | `Hello` | | | |
| 3 | `1234567890` (cepat) | | | |
| 4 | Karakter spesial (`@#$%`) | | | |

#### Pertanyaan Analisa

1. Apa kelebihan dan kekurangan metode polling untuk penerimaan UART?
2. Mengapa bisa terjadi kehilangan karakter saat mengetik cepat dengan metode polling?
3. Berapa timeout yang digunakan pada `uart_read_bytes()` di ESP32? Apa dampaknya jika timeout diubah menjadi 0?
4. Bandingkan kode inisialisasi UART antara ESP32 dan STM32 — apa perbedaan parameter yang dikonfigurasi?

---

### Percobaan 02: UART Interrupt RX — Event Queue

**Tujuan:** Memahami penerimaan data serial berbasis interrupt/event sehingga CPU tidak perlu menunggu (non-blocking).

#### Rangkaian

| Komponen | ESP32 | STM32 |
|----------|-------|-------|
| UART TX | GPIO1 (USB) | PA9 (USART1) |
| UART RX | GPIO3 (USB) | PA10 (USART1) |
| LED | — | PC13 (built-in, toggle per byte) |

#### Langkah Kerja

1. Buka project `ESP32_02` atau `STM32_02`.
2. Pelajari perbedaan pendekatan interrupt pada kedua platform:
   - ESP32: `uart_driver_install()` dengan parameter event queue, task FreeRTOS menunggu `xQueueReceive()`
   - STM32: `HAL_UART_Receive_IT()` untuk menerima 1 byte, callback `HAL_UART_RxCpltCallback()` menangani data
3. Identifikasi jenis event UART yang ditangani di ESP32: `UART_DATA`, `UART_FIFO_OVF`, `UART_BUFFER_FULL`, `UART_BREAK`, `UART_PARITY_ERR`, `UART_FRAME_ERR`.
4. Build dan upload program.
5. Buka Serial Monitor dan ketik beberapa karakter.
6. Amati output — pada ESP32 akan muncul `UART_DATA event: N byte tersedia`.
7. Bandingkan respons dengan Percobaan 01 (polling) — apakah lebih responsif?
8. Pada STM32, perhatikan LED PC13 toggle setiap kali byte diterima.

#### Tabel Pengamatan

| No | Input | Output di Serial Monitor | LED Behavior (STM32) | Catatan |
|----|-------|--------------------------|----------------------|---------|
| 1 | `A` | | | |
| 2 | `Hello World` | | | |
| 3 | 10 karakter cepat | | | |
| 4 | Data 100 byte berturut | | | |

#### Pertanyaan Analisa

1. Apa perbedaan mendasar antara metode polling dan interrupt untuk penerimaan UART?
2. Pada ESP32, mengapa digunakan FreeRTOS task terpisah untuk menangani UART event queue?
3. Pada STM32, mengapa `HAL_UART_Receive_IT()` dipanggil kembali di dalam callback? Apa yang terjadi jika tidak dipanggil ulang?
4. Event apa saja yang mungkin terjadi selain `UART_DATA`? Kapan event error terjadi?

---

### Percobaan 03: Custom Ring Buffer

**Tujuan:** Mengimplementasikan ring buffer (circular buffer) sebagai mekanisme buffering data antara ISR dan main task.

#### Rangkaian

Sama dengan Percobaan 01–02 (UART via USB/TTL converter saja).

#### Langkah Kerja

1. Buka project `ESP32_03` atau `STM32_03`.
2. Pelajari implementasi ring buffer — perhatikan struktur data:
   - `buffer[]` — array penyimpanan
   - `head` — indeks tulis
   - `tail` — indeks baca
   - `count` — jumlah data saat ini
3. Identifikasi operasi utama: `rb_init()`, `rb_push()`, `rb_pop()`, `rb_is_full()`, `rb_is_empty()`.
4. Perhatikan di ESP32: ada dua FreeRTOS task terpisah — satu untuk mendorong data ke ring buffer, satu untuk membaca dan mengirim.
5. Perhatikan di STM32: ISR (`HAL_UART_RxCpltCallback`) mendorong ke ring buffer, main loop membaca.
6. Build dan upload program.
7. Ketik karakter di Serial Monitor dan amati echo.
8. Pada ESP32, tunggu 5 detik untuk melihat statistik ring buffer (capacity, count, total push/pop, overflow).
9. Coba kirim data melebihi kapasitas buffer — amati apakah terjadi overflow.

#### Tabel Pengamatan

| No | Aksi | Capacity | Count | Total Push | Overflow | Catatan |
|----|------|----------|-------|------------|----------|---------|
| 1 | Kirim 5 byte | | | | | |
| 2 | Kirim 50 byte | | | | | |
| 3 | Kirim 256+ byte sekaligus | | | | | |

#### Pertanyaan Analisa

1. Mengapa ring buffer penting dalam komunikasi serial? Apa masalah yang dipecahkan?
2. Bagaimana cara mendeteksi dan menangani kondisi buffer penuh (overflow)?
3. Apa yang terjadi jika `rb_push()` dipanggil saat buffer penuh? Apakah data lama tertimpa atau data baru dibuang?
4. Mengapa pada STM32 variabel `count` perlu dideklarasikan `volatile`?

---

### Percobaan 04: Printf Redirect dan Formatted Output

**Tujuan:** Mengarahkan output `printf()` ke UART dan menampilkan data sensor simulasi dalam format tabel terstruktur.

#### Rangkaian

| Komponen | ESP32 | STM32 |
|----------|-------|-------|
| UART TX | GPIO1 (USB) | PA9 (USART1) |
| UART RX | GPIO3 (USB) | PA10 (USART1) |
| LED | GPIO2 (built-in) | PC13 (built-in) |

#### Langkah Kerja

1. Buka project `ESP32_04` atau `STM32_04`.
2. Pada ESP32: perhatikan bahwa `printf()` dan `ESP_LOGx()` secara default sudah mengarah ke UART0 (USB).
3. Pada STM32: pelajari implementasi `_write()` yang mengarahkan `printf()` ke `HAL_UART_Transmit()`.
4. Perhatikan format output tabel data sensor (temperatur, humidity, pressure) yang dicetak periodik setiap 2–5 detik.
5. Build dan upload program.
6. Buka Serial Monitor dan amati tabel sensor yang muncul periodik.
7. Pada ESP32, coba ketik `ON` atau `OFF` untuk mengontrol LED.
8. Pada STM32, ketik `t` (toggle LED) atau `r` (force report).
9. Amati format tabel — apakah kolom rata dan terbaca jelas?

#### Tabel Pengamatan

| No | Waktu (s) | Temp | Humidity | Pressure/Light | LED State | Catatan |
|----|-----------|------|----------|----------------|-----------|---------|
| 1 | 0 | | | | | |
| 2 | 5 | | | | | |
| 3 | 10 | | | | | |
| 4 | Setelah cmd LED | | | | | |

#### Pertanyaan Analisa

1. Pada STM32, bagaimana cara kerja redirect `printf()` melalui fungsi `_write()`?
2. Apa perbedaan `printf()` dan `ESP_LOGx()` pada ESP32? Kapan sebaiknya masing-masing digunakan?
3. Mengapa data sensor pada program ini menggunakan random/pseudo-random? Dalam aplikasi nyata, dari mana data sensor berasal?
4. Apa kelebihan menampilkan data dalam format tabel dibanding plain text?

---

### Percobaan 05: Command Parser

**Tujuan:** Membangun command-line interface sederhana yang menerima perintah teks melalui serial dan mengeksekusi aksi terkait.

#### Rangkaian

| Komponen | ESP32 | STM32 |
|----------|-------|-------|
| UART TX/RX | USB (GPIO1/3) | PA9/PA10 |
| LED | GPIO2 | PC13 |
| Buzzer | GPIO4 | PB1 |

> Hubungkan active buzzer: pin positif ke GPIO4 (ESP32) atau PB1 (STM32) melalui transistor/langsung, pin negatif ke GND.

#### Langkah Kerja

1. Buka project `ESP32_05` atau `STM32_05`.
2. Pelajari mekanisme command parsing:
   - Karakter dikumpulkan satu per satu hingga newline (`\n` atau `\r`)
   - String di-trim dan diubah ke uppercase
   - Dicocokkan dengan daftar perintah yang dikenal
3. Identifikasi perintah yang tersedia: `LED ON`, `LED OFF`, `BEEP`, `STATUS`, `HELP`.
4. Pada STM32, perhatikan penggunaan command table (struct array) dengan function pointer.
5. Build dan upload program.
6. Ketik `HELP` dan tekan Enter — lihat daftar perintah.
7. Coba semua perintah satu per satu dan amati respons:
   - `LED ON` → LED menyala, serial menampilkan konfirmasi
   - `LED OFF` → LED mati
   - `BEEP` → Buzzer berbunyi sebentar
   - `STATUS` → Menampilkan status LED dan buzzer
8. Ketik perintah yang tidak dikenal — amati pesan error.
9. Coba ketik perintah dengan huruf kecil — apakah tetap dikenali?

#### Tabel Pengamatan

| No | Perintah Diketik | Respons Serial | Aksi Hardware | Catatan |
|----|-----------------|----------------|---------------|---------|
| 1 | `HELP` | | | |
| 2 | `LED ON` | | | |
| 3 | `led off` | | | |
| 4 | `BEEP` | | | |
| 5 | `STATUS` | | | |
| 6 | `xyz` (invalid) | | | |

#### Pertanyaan Analisa

1. Mengapa input di-convert ke uppercase sebelum dicocokkan? Apa keuntungannya?
2. Pada STM32, apa keuntungan menggunakan command table dengan function pointer dibanding `if-else` chain?
3. Bagaimana program menangani backspace? Mengapa ini penting untuk user experience?
4. Apa yang terjadi jika perintah yang diketik melebihi ukuran buffer? Bagaimana mencegahnya?

---

### Percobaan 06: JSON Protocol

**Tujuan:** Mengimplementasikan protokol JSON untuk komunikasi terstruktur tanpa menggunakan library JSON eksternal.

#### Rangkaian

| Komponen | ESP32 | STM32 |
|----------|-------|-------|
| UART TX/RX | USB (GPIO1/3) | PA9/PA10 |
| LED | GPIO2 | PC13 |

> ESP32 mendukung kontrol multi-pin (GPIO 2, 4, 5, 12, 13, 14, 15). STM32 mengontrol PC13 (pin 13).

#### Langkah Kerja

1. Buka project `ESP32_06` atau `STM32_06`.
2. Pelajari parser JSON manual:
   - `json_get_string()` — mengekstrak nilai string berdasarkan key
   - `json_get_int()` — mengekstrak nilai integer berdasarkan key
   - Deteksi batas objek JSON menggunakan penghitungan brace depth `{}`
3. Identifikasi format perintah yang didukung:
   - SET: `{"cmd":"set","pin":2,"val":1}`
   - GET: `{"cmd":"get","pin":2}`
   - TOGGLE (STM32): `{"cmd":"toggle","pin":13}`
4. Build dan upload program.
5. Kirim perintah JSON melalui Serial Monitor:
   - Ketik `{"cmd":"set","pin":2,"val":1}` → LED menyala
   - Ketik `{"cmd":"set","pin":2,"val":0}` → LED mati
   - Ketik `{"cmd":"get","pin":2}` → Mendapatkan status pin
6. Coba kirim JSON tidak valid — amati pesan error.
7. Catat format respons JSON yang dikembalikan.

#### Tabel Pengamatan

| No | JSON Dikirim | Respons JSON | Aksi Hardware | Catatan |
|----|-------------|-------------|---------------|---------|
| 1 | `{"cmd":"set","pin":2,"val":1}` | | | |
| 2 | `{"cmd":"set","pin":2,"val":0}` | | | |
| 3 | `{"cmd":"get","pin":2}` | | | |
| 4 | `{"cmd":"invalid"}` | | | |
| 5 | `not json` | | | |

#### Pertanyaan Analisa

1. Mengapa diimplementasikan parser JSON manual tanpa library? Apa pertimbangan resource di embedded system?
2. Bagaimana program mendeteksi batas awal dan akhir objek JSON dari stream byte serial?
3. Apa kelebihan JSON sebagai format komunikasi dibanding plain text command?
4. Bagaimana cara menangani kasus di mana JSON diterima terpotong (partial)?

---

### Percobaan 07: Interactive Line Editor

**Tujuan:** Membangun line editor interaktif dengan fitur command history menggunakan escape sequence ANSI.

#### Rangkaian

Sama dengan Percobaan 01 (UART via USB/TTL converter saja).

#### Langkah Kerja

1. Buka project `ESP32_07` atau `STM32_07`.
2. Pelajari implementasi line editor:
   - Echo karakter satu per satu
   - Backspace handling (mengirim `\b \b` ke terminal)
   - ESC sequence parser: state machine yang mendeteksi `ESC[A` (arrow up) dan `ESC[B` (arrow down)
3. Identifikasi fitur command history:
   - `history_add()` — menyimpan perintah ke history buffer
   - `history_recall()` — mengambil perintah sebelumnya (arrow up/down)
   - Kapasitas history: 5 entry
4. Build dan upload program.
5. Ketik beberapa perintah: `help`, `info`, `history`, `clear`.
6. Tekan **Arrow Up** untuk recall perintah sebelumnya.
7. Tekan **Arrow Down** untuk navigate ke perintah berikutnya.
8. Ketik `history` untuk melihat daftar perintah yang tersimpan.
9. Ketik `info` untuk melihat informasi sistem (heap, uptime).

#### Tabel Pengamatan

| No | Aksi | Respons | Catatan |
|----|------|---------|---------|
| 1 | Ketik `help` + Enter | | |
| 2 | Ketik `info` + Enter | | |
| 3 | Arrow Up | | |
| 4 | Arrow Up lagi | | |
| 5 | Arrow Down | | |
| 6 | Ketik `history` | | |
| 7 | Ketik `clear` | | |

#### Pertanyaan Analisa

1. Bagaimana state machine ESC sequence bekerja? Gambarkan diagram state-nya (`NONE` → `GOT_ESC` → `GOT_BRACKET` → action).
2. Mengapa arrow key mengirim 3 byte (`ESC [ A`) dan bukan satu byte? Apa standar yang mengatur ini?
3. Bagaimana implementasi backspace bekerja di level terminal? Mengapa perlu mengirim `\b \b` (backspace-space-backspace)?
4. Apa keterbatasan command history dengan size tetap? Bagaimana cara mengimplementasikan history tak terbatas?

---

### Percobaan 08: STX/ETX Framing dengan Byte Stuffing

**Tujuan:** Mengimplementasikan protokol framing biner menggunakan delimiter STX/ETX dan teknik byte stuffing (DLE) untuk data transparansi.

#### Rangkaian

Sama dengan Percobaan 01 (UART via USB/TTL converter saja). LED PC13 pada STM32 toggle setiap siklus pengiriman.

#### Langkah Kerja

1. Buka project `ESP32_08` atau `STM32_08`.
2. Pelajari format frame:
   - `STX (0x02)` — Start of Text (awal frame)
   - `[DATA...]` — Payload (escaped)
   - `ETX (0x03)` — End of Text (akhir frame)
   - `DLE (0x10)` — Data Link Escape (byte stuffing)
3. Pahami byte stuffing: jika data mengandung STX, ETX, atau DLE, maka didahului DLE:
   - `0x02` dalam data → `0x10 0x02`
   - `0x03` dalam data → `0x10 0x03`
   - `0x10` dalam data → `0x10 0x10`
4. Identifikasi fungsi: `frame_encode()`, `frame_decode()`, `hex_dump()`.
5. Pada ESP32: sender dan receiver berjalan di FreeRTOS task terpisah.
6. Pada STM32: frame receiver menggunakan state machine di ISR callback (`FRAME_IDLE` → `FRAME_RECEIVING` → `FRAME_ESCAPE`).
7. Build dan upload program.
8. Amati hex dump frame yang dikirim dan di-decode.
9. Verifikasi bahwa `Encode/Decode MATCH` muncul untuk setiap frame.

#### Tabel Pengamatan

| No | Data Asli (hex) | Frame Encoded (hex) | Frame Decoded (hex) | Match? | Catatan |
|----|----------------|--------------------|--------------------|--------|---------|
| 1 | Text biasa | | | | |
| 2 | Data dgn 0x02 | | | | |
| 3 | Data dgn 0x10 | | | | |
| 4 | Counter data | | | | |

#### Pertanyaan Analisa

1. Mengapa diperlukan byte stuffing? Apa masalah yang muncul jika data mengandung karakter STX/ETX tanpa escape?
2. Berapa overhead (byte tambahan) yang ditimbulkan oleh framing STX/ETX/DLE? Hitung untuk payload 10 byte tanpa karakter khusus dan 10 byte dengan 3 karakter khusus.
3. Bagaimana state machine receiver menangani urutan `DLE ETX` (DLE sebagai escape) vs `ETX` (akhir frame)?
4. Apa kelebihan dan kekurangan framing STX/ETX dibanding length-prefixed framing?

---

### Percobaan 09: CRC-8 Checksum

**Tujuan:** Mengimplementasikan CRC-8 untuk verifikasi integritas data pada komunikasi serial.

#### Rangkaian

Sama dengan Percobaan 01 (UART via USB/TTL converter saja).

#### Langkah Kerja

1. Buka project `ESP32_09` atau `STM32_09`.
2. Pelajari implementasi CRC-8:
   - Polynomial: `0x07` (CRC-8 standar)
   - Proses: XOR setiap byte data dengan register CRC, lalu shift dan XOR dengan polynomial
3. Pahami format paket: `[LENGTH][DATA...][CRC8]`
   - LENGTH: 1 byte, jumlah byte data (tidak termasuk LENGTH dan CRC)
   - DATA: N byte payload
   - CRC8: 1 byte CRC dihitung dari DATA
4. Identifikasi fungsi: `crc8_calc()`, `send_with_crc()`, `verify_crc()`.
5. Build dan upload program.
6. Amati self-test pada startup — program mengirim data dan memverifikasi sendiri.
7. Perhatikan pengujian dengan data korup — CRC harus FAIL.
8. Catat statistik: total packet, CRC pass, CRC fail.

#### Tabel Pengamatan

| No | Data Dikirim | CRC Calculated | Verifikasi | Pass/Fail | Catatan |
|----|-------------|---------------|------------|-----------|---------|
| 1 | Self-test #1 | | | | |
| 2 | Self-test #2 | | | | |
| 3 | Data korup | | | | |
| 4 | Counter data | | | | |

#### Pertanyaan Analisa

1. Apa perbedaan CRC dan checksum sederhana (misalnya XOR)? Mana yang lebih handal mendeteksi error?
2. Berapa panjang CRC-8? Berapa jumlah maksimum error bit yang dapat dideteksi?
3. Mengapa polynomial `0x07` dipilih untuk CRC-8? Apa dampak pemilihan polynomial terhadap kemampuan deteksi error?
4. Bagaimana cara program mendeteksi bahwa data telah korup? Jelaskan proses verifikasi CRC di sisi penerima.

---

### Percobaan 10: Timeout-based Parser (Modbus-style)

**Tujuan:** Mengimplementasikan deteksi batas paket berdasarkan timeout/silence period, mirip dengan protokol Modbus RTU.

#### Rangkaian

Sama dengan Percobaan 01 (UART via USB/TTL converter saja).

#### Langkah Kerja

1. Buka project `ESP32_10` atau `STM32_10`.
2. Pelajari mekanisme timeout parser:
   - Setiap byte yang diterima me-reset timer timeout
   - Jika tidak ada byte dalam `TIMEOUT_MS` (misalnya 50ms), paket dianggap selesai
   - ESP32: menggunakan `esp_timer_start_once()` yang di-restart setiap byte baru
   - STM32: menggunakan `HAL_GetTick()` di main loop untuk mendeteksi timeout
3. Build dan upload program.
4. Ketik beberapa kata terpisah dengan jeda waktu:
   - Ketik `Hello` cepat → terdeteksi sebagai 1 paket
   - Tunggu 1 detik, ketik `World` → terdeteksi sebagai paket terpisah
5. Amati hex dump dan ASCII display setiap paket yang terdeteksi.
6. Perhatikan statistik: total packets, total bytes, average packet size.

#### Tabel Pengamatan

| No | Input (dengan timing) | Jumlah Paket Terdeteksi | Ukuran Paket | Catatan |
|----|----------------------|------------------------|-------------|---------|
| 1 | `Hello` (cepat) | | | |
| 2 | `Hello` jeda `World` | | | |
| 3 | `ABCDE` per karakter (lambat) | | | |
| 4 | 20 byte sekaligus (paste) | | | |

#### Pertanyaan Analisa

1. Mengapa protokol Modbus RTU menggunakan silence period (3.5 character time) untuk mendeteksi batas frame?
2. Berapa timeout optimal untuk baud rate 115200? Hitunglah waktu transmisi 1 karakter (10 bit @ 115200 baud).
3. Apa kekurangan pendekatan timeout-based dibanding length-prefixed atau delimiter-based? Dalam kondisi apa timeout parser lebih cocok?
4. Bagaimana perbedaan implementasi timer di ESP32 (`esp_timer`) vs STM32 (`HAL_GetTick()`)? Mana yang lebih presisi?

---

### Percobaan 11: Multi-UART Bridge

**Tujuan:** Membangun jembatan (bridge) antara dua port UART untuk meneruskan data secara bidirectional.

#### Rangkaian

| Komponen | ESP32 | STM32 |
|----------|-------|-------|
| UART0 TX/RX (ke PC) | GPIO1/GPIO3 (USB) | PA9/PA10 (USART1) |
| UART1/2 TX (ke device) | **GPIO17** | **PA2** (USART2) |
| UART1/2 RX (dari device) | **GPIO16** | **PA3** (USART2) |

> Hubungkan UART1 (ESP32) atau USART2 (STM32) ke perangkat eksternal atau USB-TTL converter kedua.
> **Baud rate:** UART0/USART1 = 115200, UART1/USART2 = 9600.

#### Langkah Kerja

1. Buka project `ESP32_11` atau `STM32_11`.
2. Pelajari konfigurasi dual UART:
   - ESP32: UART0 (USB, 115200) dan UART1 (GPIO17/16, 9600)
   - STM32: USART1 (PA9/PA10, 115200) dan USART2 (PA2/PA3, 9600)
3. Perhatikan implementasi bridge:
   - ESP32: dua FreeRTOS task, masing-masing membaca dari satu UART dan menulis ke yang lain
   - STM32: dua interrupt handler, masing-masing meneruskan byte yang diterima ke USART lain
4. Hubungkan perangkat serial kedua (atau USB-TTL converter) ke UART1/USART2.
5. Build dan upload program.
6. Buka dua Serial Monitor — satu untuk UART0 (115200), satu untuk UART1 (9600).
7. Kirim data dari Monitor 1 → amati muncul di Monitor 2 (dan sebaliknya).
8. Amati statistik bridge secara periodik.

#### Tabel Pengamatan

| No | Data Dari | Data Diterima Di | Bytes Forwarded | Latency (subjektif) | Catatan |
|----|-----------|------------------|-----------------|---------------------|---------|
| 1 | UART0 → UART1 | | | | |
| 2 | UART1 → UART0 | | | | |
| 3 | Bidirectional simultan | | | | |
| 4 | Data besar (100 byte) | | | | |

#### Pertanyaan Analisa

1. Mengapa bridge menggunakan baud rate berbeda (115200 vs 9600) pada kedua UART? Apa masalah yang muncul jika kecepatan sangat berbeda?
2. Apa yang terjadi jika data dikirim terus-menerus dari UART cepat ke UART lambat? Bagaimana mengatasi buffer overflow?
3. Mengapa implementasi ESP32 menggunakan dua task terpisah? Bisakah dilakukan dengan satu task?
4. Jelaskan skenario di mana UART bridge diperlukan dalam sistem embedded nyata.

---

### Percobaan 12: Error Statistics Monitor

**Tujuan:** Memonitor dan melaporkan statistik error UART secara real-time termasuk parity error, framing error, overrun error, dan noise error.

#### Rangkaian

Sama dengan Percobaan 01 (UART via USB/TTL converter saja).

> **Catatan:** Program ini mengaktifkan **EVEN parity**. Terminal standar tanpa parity akan menyebabkan parity error terpicu — ini memang sengaja untuk demonstrasi.

#### Langkah Kerja

1. Buka project `ESP32_12` atau `STM32_12`.
2. Pelajari konfigurasi UART dengan parity:
   - ESP32: `uart_parity_t` = `UART_PARITY_EVEN`
   - STM32: `huart1.Init.Parity = UART_PARITY_EVEN`, `WordLength = UART_WORDLENGTH_9B`
3. Identifikasi jenis error yang dimonitor:
   - **Parity Error (PE):** Data diterima dengan parity salah
   - **Framing Error (FE):** Stop bit tidak terdeteksi
   - **Overrun Error (ORE):** Data baru datang sebelum data lama dibaca
   - **Noise Error (NE):** Noise pada jalur data (STM32)
   - **FIFO Overflow:** Buffer internal penuh (ESP32)
4. Build dan upload program.
5. Kirim data dari Serial Monitor (yang tidak pakai parity) — amati parity error terpicu.
6. Tunggu laporan periodik yang menampilkan:
   - Total bytes received
   - Jumlah setiap jenis error
   - Error rate (%)
   - Health status: GOOD (< 1%), WARNING (< 5%), CRITICAL (≥ 5%)
7. Amati format laporan yang terstruktur.

#### Tabel Pengamatan

| No | Total RX | Parity Error | Framing Error | Overrun | Error Rate (%) | Health |
|----|----------|-------------|--------------|---------|---------------|--------|
| 1 | Setelah 10 byte | | | | | |
| 2 | Setelah 50 byte | | | | | |
| 3 | Setelah 100 byte | | | | | |
| 4 | Idle 1 menit | | | | | |

#### Pertanyaan Analisa

1. Mengapa parity error terjadi saat mengirim dari terminal tanpa parity ke MCU dengan even parity? Jelaskan mekanismenya.
2. Apa perbedaan antara framing error dan parity error? Kondisi apa yang menyebabkan masing-masing?
3. Bagaimana overrun error terjadi? Apa hubungannya dengan kecepatan baud rate dan kecepatan pembacaan buffer?
4. Dalam sistem industri, batas error rate berapa persen yang biasanya dianggap unacceptable? Mengapa monitoring error penting?

---

## 5. Tabel Komparatif STM32 vs ESP32

| Aspek | STM32F103 (HAL) | ESP32 (ESP-IDF) |
|-------|-----------------|-----------------|
| Jumlah UART | 3 USART | 3 UART |
| Pin Default | PA9/PA10 (USART1) | GPIO1/GPIO3 (UART0 via USB) |
| Pin Remap | Terbatas (alternate function) | Bebas ke GPIO manapun |
| Metode RX | Polling / Interrupt / DMA | Polling / Event Queue / DMA |
| Buffer | Manual (ring buffer di ISR) | Built-in driver buffer |
| Printf Redirect | Override `_write()` | Otomatis ke UART0 |
| Parity Support | Even/Odd (9-bit word) | Even/Odd |
| Error Callback | `HAL_UART_ErrorCallback()` | Event queue (`UART_PARITY_ERR`, dll) |
| Multi-UART | USART1 + USART2 + USART3 | UART0 + UART1 + UART2 |
| Kecepatan Max | Hingga 4.5 Mbps | Hingga 5 Mbps |

---

## 6. Tugas Tambahan

1. **Modifikasi Ring Buffer:** Ubah ring buffer pada Percobaan 03 agar saat buffer penuh, data paling lama ditimpa (overwrite mode). Bandingkan hasilnya dengan mode drop.
2. **Command Parser Extension:** Tambahkan perintah `BAUD <rate>` pada Percobaan 05 yang dapat mengubah baud rate secara runtime. Implementasikan juga perintah `ECHO ON/OFF`.
3. **CRC-16:** Modifikasi Percobaan 09 dari CRC-8 ke CRC-16 (polynomial 0x8005). Bandingkan overhead dan kemampuan deteksi error.

---

## 7. Format Laporan

Laporan praktikum harus mencakup:
1. **Tujuan** — Tujuan dari setiap percobaan
2. **Dasar Teori** — Konsep UART yang digunakan
3. **Langkah Kerja** — Screenshot hasil setiap percobaan
4. **Analisa** — Jawaban pertanyaan analisa setiap percobaan
5. **Kesimpulan** — Rangkuman pembelajaran dari seluruh percobaan

---

## 8. Referensi

1. STM32F103xx Reference Manual (RM0008) — Chapter 27: USART
2. ESP32 Technical Reference Manual — Chapter 13: UART Controller
3. ESP-IDF Programming Guide — UART Driver API
4. Mastering STM32, Carmine Noviello — Chapter 9: USART
5. Serial Port Complete, Jan Axelson — Comprehensive UART reference

---

*Jobsheet Modul 03 — Serial UART | Praktikum Sistem Embedded | 2025/2026*
