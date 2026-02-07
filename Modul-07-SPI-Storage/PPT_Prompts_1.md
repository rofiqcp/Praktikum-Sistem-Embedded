# Prompt untuk Pembuatan PPT - Bagian 1
## Modul 07: SPI Bus & Storage untuk Embedded Solutions

### Slide Judul
**Prompt:**
Buat slide judul yang profesional dan teknikal untuk "Modul 07: SPI Bus & Storage". Subjudul: "High-Speed Communication & Data Persistence". Tampilkan ilustrasi 3D dari jalur data SPI yang menghubungkan mikrokontroler dengan chip memori (SD Card/Flash) dengan efek glowing data flow. Logo universitas di pojok kiri atas. Background gelap dengan aksen biru neon sirkuit.

### Slide Agenda
**Prompt:**
Slide "Agenda Praktikum" dengan layout 4 kolom atau grid.
Poin materi:
1. Teori Protokol SPI (Architecture, Modes)
2. Hardware Interfaces (STM32 & ESP32)
3. Storage Devices (W25Q Flash & SD Card)
4. File Systems (FatFS & LittleFS)
Gunakan ikon flat modern untuk setiap poin (e.g., chip connection, microcontroller, memory card, folder structure).

### Slide Apa itu SPI?
**Prompt:**
Visualisasikan diagram blok "SPI Architecture" yang jelas.
Tunjukkan satu Master (MCU) terhubung ke 3 Slave devices.
Garis koneksi:
- SCLK (Clock): Dari Master ke semua Slave
- MOSI (Data In): Dari Master ke semua Slave
- MISO (Data Out): Dari semua Slave ke Master
- CS/SS (Chip Select): Jalur individu dari Master ke masing-masing Slave
Style: Skematik modern, clean lines, color coded (MISO hijau, MOSI biru, Clock kuning, CS merah).

### Slide Keunggulan SPI
**Prompt:**
Buat slide perbandingan "Kenapa Menggunakan SPI?".
Tampilkan 3 poin utama dengan ikon besar:
1. High Speed (Up to 80+ MHz vs I2C 3.4 MHz) - visualkan speedometer
2. Full Duplex (Kirim & terima bersamaan) - visualkan panah dua arah simultaneous
3. Simple Protocol (Shift register logic) - visualkan bit shifting
Sertakan tabel komparasi kecil di bawah: SPI vs I2C vs UART.

### Slide Mode SPI (CPOL & CPHA)
**Prompt:**
Gambarkan "Timing Diagram SPI Modes" yang menampilkan 4 kombinasi mode (Mode 0-3).
Tunjukkan sinyal Clock (SCLK) dan Data Sampling.
- CPOL (Clock Polarity): Idle Low vs Idle High
- CPHA (Clock Phase): Sample edge (Rising vs Falling)
Highlight "Mode 0 (CPOL=0, CPHA=0)" dan "Mode 3" sebagai yang paling umum digunakan.
Gunakan grafik sinyal digital yang presisi (logic analyzer style).

### Slide STM32 SPI Hardware
**Prompt:**
Tampilkan diagram "STM32F103 SPI Features".
Gambar chip STM32 dengan highlight pada pin SPI1 (PA5-PA7) dan SPI2 (PB13-PB15).
List fitur utama di samping:
- Master/Slave Mode support
- Full-duplex synchronous transfers
- Hardware CRC calculation
- DMA support untuk high-speed transfer
Background: PCB Blueprint STM32 Blue Pill.

### Slide ESP32 SPI Hardware
**Prompt:**
Slide "ESP32 SPI Controllers".
Tampilkan diagram internal ESP32 dengan blok SPI0, SPI1, SPI2 (HSPI), dan SPI3 (VSPI).
Jelaskan pemetaan:
- SPI0/1: Internal Flash (Don't touch!)
- HSPI/VSPI: User available
Tabel pinout default untuk HSPI dan VSPI.
Visual: ESP32 DevKit module dengan pin yang relevan menyala.

### Slide External Flash (W25Qxx)
**Prompt:**
Visualisasikan "Arsitektur W25Qxx Serial Flash".
Gambar chip SOP-8 package dari Winbond W25Q.
Diagram blok internal:
- Pages (256 bytes)
- Sectors (4 KB)
- Blocks (64 KB)
Jelaskan instruksi dasar: Write Enable (06h), Sector Erase (20h), Page Program (02h), Read Data (03h).
Analogi: Buku (Block) -> Halaman (Page) -> Kata (Byte).

### Slide SD Card Interface
**Prompt:**
Slide "SD Card SPI Mode".
Tampilkan pinout SD Card dan Micro SD Card.
Tabel mapping pin Native SD Mode vs SPI Mode.
Tunjukkan skema koneksi ke MCU (level shifting 3.3V wajib jika MCU 5V).
Note penting: "SD Card initialization sequence" (clock rendah < 400kHz di awal).
Visual: Micro SD card adapter module yang terhubung ke breadboard.

### Slide File Systems Concept
**Prompt:**
Ilustrasi "Embedded File Systems".
Bandingkan dua pendekatan:
1. Raw Access: Address-based read/write (Performance tinggi, susah manage data)
2. File System (FatFS/LittleFS): File-based (fopen, fwrite) - User friendly.
Diagram layer: Application -> File System -> Disk I/O Driver -> SPI HW -> Storage Media.

### Slide FatFS pada STM32
**Prompt:**
Gambarkan struktur "STM32 FatFS Middleware".
Tampilkan screenshot konfigurasi di STM32CubeMX:
- Middleware > FATFS > User-defined
- SPI1 Configuration
Code snippet singkat untuk mount dan write file:
`f_mount()`, `f_open()`, `f_puts()`, `f_close()`.
Visual: Folder structure tree di satu sisi, kode C di sisi lain.

### Slide LittleFS pada ESP32
**Prompt:**
Slide "LittleFS: Fail-safe File System".
Jelaskan keunggulan untuk embedded:
- Power-loss resilience (tidak korup saat mati mendadak)
- Wear leveling (memperpanjang umur flash)
Bandingkan dengan SPIFFS (deprecated).
Code snippet Arduino: `LittleFS.begin()`, `File f = LittleFS.open()`.

### Slide Tantangan Praktikal
**Prompt:**
Slide "Common Pitfalls & Solutions".
Infografis masalah umum:
1. Wiring noise (kabel jumper terlalu panjang) -> Solusi: Kabel pendek/PCB
2. CS Timing (lupa toggle chip select) -> Solusi: Cek logika CS
3. Voltage Mismatch (5V MCU ke 3.3V SD) -> Solusi: Level converter
4. Init Failure -> Solusi: Cek formatting kartu (FAT32).
Ikon warning triangle untuk masalah, ikon centang hijau untuk solusi.

### Slide Konfigurasi Hardware Praktikum
**Prompt:**
Diagram "Wiring Praktikum Modul 07".
Split screen:
Kiri: STM32 + W25Q Flash (SPI1)
Kanan: ESP32 + SD Card Module (VSPI)
Tabel koneksi pin yang jelas (Pin MCU -> Pin Modul).
Gunakan gaya wiring diagram Fritzing yang rapi dan mudah dibaca.

### Slide Demo Project Overview
**Prompt:**
Preview project akhir: "ESP32 Data Logger".
Flowchart sistem:
Sensor Data -> ESP32 -> Buffer -> Write to SD Card (CSV Format).
Tampilkan contoh output file .CSV yang dibuka di Excel (Timestamp, Data1, Data2).
Visual: Board ESP32 yang terhubung sensor dan logging data ke SD card.
