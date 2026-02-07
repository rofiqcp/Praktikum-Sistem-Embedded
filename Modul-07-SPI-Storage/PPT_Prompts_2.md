# Prompt untuk Pembuatan PPT - Bagian 2
## Modul 07: SPI Storage - Advanced & Implementation

### Slide Aplikasi: Data Logger
**Prompt:**
Buat slide "Aplikasi: Industrial Data Logger".
Visualisasikan sistem embedded yang merekam data sensor suhu/vibrasi mesin setiap detik selama berbulan-bulan.
Tampilkan alur data:
Sensor -> MCU -> SPI Bus -> SD Card (CSV logs).
Highlight pentingnya "Data Persistence" (data tidak hilang saat mati lampu) dan "Capacity" (SD Card GBs vs Internal Flash KBs).

### Slide Aplikasi: Flash External untuk Display Assets
**Prompt:**
Slide "Aplikasi: GUI Assets Storage".
Gambarkan skenario display TFT LCD yang membutuhkan gambar/font besar.
MCU internal flash terbatas -> Solusi: Simpan icon, background, dan font di W25Qxx External Flash.
Proses: MCU baca Flash via SPI -> Buffer -> Kirim ke Display via SPI/Parallel.
Visual: Layar smartwatch yang menampilkan grafik kaya warna.

### Slide Demo 1: W25Q Flash Raw Access
**Prompt:**
Slide "Praktikum A: STM32 Flash Access".
Tampilkan screenshot logic analyzer (PulseView/Saleae) yang menangkap transaksi SPI:
1. Master menarik CS Low.
2. Master kirim Command 90h (Read ID).
3. Slave kirim Response (Manufacturer ID).
4. Master menarik CS High.
Ilustrasikan waveform clock dan data yang sinkron.

### Slide Demo 2: ESP32 File System
**Prompt:**
Slide "Praktikum B: ESP32 LittleFS/SD".
Split screen comparison:
Kiri: Output Serial Monitor yang menampilkan "File written successfully" dan isi file.
Kanan: Direktori file sistem yang terlihat (seperti file explorer).
Tampilkan code snippet singkat `file.print()` dan `file.readString()`.

### Slide Optimasi: DMA Transfer
**Prompt:**
Visualisasikan "Direct Memory Access (DMA) pada SPI".
Grafik perbandingan CPU Load:
- Tanpa DMA: CPU sibuk 100% selama transfer SPI (menunggu loop).
- Dengan DMA: CPU Load < 5%, DMA controller menangani transfer ke RAM secara background.
Analogi: CPU sebagai Manager, DMA sebagai Asisten yang memindahkan "barang" (data) secara mandiri.

### Slide Troubleshooting SPI
**Prompt:**
Buat slide "SPI Troubleshooting Checklist" dengan gaya checklist interaktif.
Poin-poin masalah umum:
1. **Clock Polarity Wrong**: Data terbaca sampah -> Cek CPOL/CPHA.
2. **Missing CS Toggle**: Slave mengabaikan command -> Pastikan CS Low sebelum kirim.
3. **Speed Too High**: Data korup/kabel panjang -> Turunkan clock speed (<10MHz untuk jumper wires).
4. **Power Supply**: SD Card boros arus -> Cek supply 3.3V stabil.
Visual: Teknisi dengan osiloskop sedang mendebug PCB.

### Slide Advanced Topic: Wear Leveling
**Prompt:**
Jelaskan konsep "Flash Wear Leveling".
Masalah: Flash memory punya batas siklus tulis/hapus (e.g., 100k cycles). Jika menulis di alamat yang sama terus, chip rusak.
Solusi: File System menyebarkan penulisan data ke seluruh address space secara merata.
Visual: Heatmap chip memori, yang satu ada 'hotspot' merah (tanpa wear leveling), yang satu merata hijau (dengan wear leveling).

### Slide Project: Mini Black Box
**Prompt:**
Slide deskripsi project "Mini Black Box Recorder".
Tugas: Buat sistem yang merekam 3 parameter (Uptime, Random Sensor, Button Status) ke SD Card.
Fitur:
- Auto-mount saat boot.
- Append data ke file log.
- Indikator LED saat menulis (Activity light).
- Tombol untuk "Dump Data" ke Serial Monitor.
Visual: Ilustrasi kotak hitam pesawat (Flight Recorder) versi mini dengan ESP32.

### Slide Tips Sukses
**Prompt:**
Slide "Best Practices Development".
1. **Wiring Rapi**: I2C/SPI sangat sensitif noise.
2. **Modular Code**: Pisahkan driver storage dari main logic.
3. **Backup Data**: Selalu close/sync file setelah menulis penting.
4. **Datasheet Friend**: Selalu cek opcode command di datasheet W25Q/SD.
Icon: Jempol "Like", Buku Manual, dan Kabel rapi.

### Slide Penutup & Next Steps
**Prompt:**
Slide "Wrap-up & Next Module".
Summary:
- SPI: Cepat, Full-duplex, Simple.
- Storage: Penting untuk logging & aset.
- FatFS/LittleFS: Memudahkan manajemen data.
Next Module: **Modul 08 - Direct Memory Access (DMA)** - Mempelajari cara transfer data tanpa membebani CPU (pasangan sempurna untuk SPI!).
Visual: Jembatan yang menghubungkan topik Storage ke topik DMA.

### Slide Q&A
**Prompt:**
Slide "Sesi Tanya Jawab".
Ilustrasi karakter robot AI assistant yang siap menjawab pertanyaan.
Background dengan elemen tanda tanya stylised yang futuristik.
Teks: "Any Questions on SPI or Storage?"
