# Tugas Video — Modul 08: SPI Bus dan Komunikasi STM32-ESP32

## Informasi Tugas

| Item | Detail |
|---|---|
| Modul | 08 — SPI Bus dan Komunikasi STM32-ESP32 |
| Struktur praktikum | 25 eksperimen: 7 STM32 SPI + 3 STM32 SPI RTOS + 7 ESP32 SPI + 3 ESP32 SPI RTOS + 3 Multi SPI non-RTOS + 2 Multi SPI RTOS |
| Platform | ESP32 DevKit + STM32F103/STM32F4 |
| Format | Video presentasi + demonstrasi hardware |
| Durasi | 20–35 menit |
| Upload | YouTube Unlisted atau link e-learning sesuai instruksi dosen |

---

## 1. Deskripsi

Video adalah bukti pemahaman dan demonstrasi praktikum Modul 08. Isi video harus selaras dengan struktur final:

- **7 eksperimen STM32 SPI**.
- **3 eksperimen STM32 SPI RTOS**.
- **7 eksperimen ESP32 SPI**.
- **3 eksperimen ESP32 SPI RTOS**.
- **3 eksperimen Multi STM32-ESP32 SPI non-RTOS**.
- **2 eksperimen Multi STM32-ESP32 SPI RTOS**.
- **Project final Data Acquisition System dual-MCU**.

Bahasa presentasi: Indonesia.

---

## 2. Ketentuan Teknis

- Screen recording wajib: kode, serial monitor, output build/upload.
- Rekaman hardware wajib: ESP32, STM32, SPI devices, display, wiring MOSI/MISO/SCLK/CS.
- Webcam/presenter disarankan terlihat.
- Resolusi minimal 720p; 1080p disarankan.
- Audio jelas.
- Tampilkan hasil nyata, bukan hanya slide.
- Boleh mempercepat bagian upload/compile, tetapi output penting harus terbaca.

---

## 3. Struktur Video Disarankan

| Bagian | Durasi | Isi |
|---|---:|---|
| Pembukaan | 1–2 menit | nama, NIM, Modul 08, daftar hardware |
| Ringkasan teori | 3–5 menit | SPI bus basics, MOSI/MISO/SCLK/CS, CPOL/CPHA, modes, full-duplex, CS management |
| Demo STM32 SPI | 6–8 menit | 7 eksperimen STM32 SPI ringkas |
| Demo STM32 SPI RTOS | 2–3 menit | 3 eksperimen STM32 SPI RTOS |
| Demo ESP32 SPI | 6–8 menit | 7 eksperimen ESP32 SPI ringkas |
| Demo ESP32 SPI RTOS | 2–3 menit | 3 eksperimen ESP32 SPI RTOS |
| Demo Multi SPI | 4–6 menit | 3 non-RTOS + 2 RTOS eksperimen multi |
| Project final | 4–6 menit | Data Acquisition System dual-MCU |
| Penutup | 1–2 menit | kendala, troubleshooting, kesimpulan |

---

## 4. Materi Teori yang Wajib Disebut

1. MOSI, MISO, SCLK, CS/SS sebagai jalur SPI.
2. Clock Polarity (CPOL) dan Clock Phase (CPHA).
3. 4 mode SPI: mode 0, 1, 2, 3.
4. Full-duplex vs half-duplex vs simplex.
5. Chip Select (CS) aktif LOW untuk multi-slave.
6. Daisy-chain vs independent CS.
7. Kecepatan SPI: 10 kHz - 50+ MHz.
8. STM32 SPI HAL, register, DMA, interrupt.
9. ESP32 ESP-IDF SPI, GPIO matrix, SPI master/slave.
10. SPI Flash W25Q64: JEDEC ID, Read/Write/Erase.
11. Micro SD Card: SPI mode init 400 kHz, CMD/ACMD.
12. SSD1306 OLED SPI: CS, DC, RES pins.
13. MCP3008 ADC: 10-bit, 8-channel, SPI read format.
14. MCP4921 DAC: 12-bit, SPI write format.
15. ADXL345 Accelerometer: mode 3, DEVID, axis data.
16. Error recovery dan bus troubleshooting.
17. SPI vs I2C comparison.
18. Integrasi project dan troubleshooting.

---

## 5. Checklist Demo 25 Eksperimen

### 5.1 STM32 SPI — 7 Eksperimen

| Kode | Demo wajib | Bukti output |
|---|---|---|
| STM32_01 | SPI Bus Scanner | device terdeteksi (Flash ID, OLED, ADC, DAC, ADXL) |
| STM32_02 | W25Q64 Flash Read/Write | ID=EF4017, tulis/baca/verify |
| STM32_03 | Micro SD Card File System | SD terbaca, baca/tulis blok |
| STM32_04 | SSD1306 OLED SPI Graphics | teks/grafik tampil |
| STM32_05 | MCP3008 ADC Multi-Channel | 8 channel ADC valid |
| STM32_06 | MCP4921 DAC Signal Generation | waveform terukur |
| STM32_07 | ADXL345 Accelerometer SPI | accel X,Y,Z berubah saat digerakkan |

---

### 5.2 STM32 SPI RTOS — 3 Eksperimen

| Kode | Demo wajib | Bukti output |
|---|---|---|
| STM32_08 | SPI RTOS Multi Device | multiple task akses SPI via RTOS |
| STM32_09 | SPI RTOS DMA Transfer | DMA transfer SPI valid |
| STM32_10 | SPI RTOS Data Logger | logging data SPI via RTOS |

---

### 5.3 ESP32 SPI — 7 Eksperimen

| Kode | Demo wajib | Bukti output |
|---|---|---|
| ESP32_01 | SPI Bus Master Configuration | config ESP-IDF SPI sukses |
| ESP32_02 | W25Q64 Flash ID dan Memory Map | ID Flash, memory map 8 MB |
| ESP32_03 | Micro SD Card SPI Mode | SD terbaca, baca/tulis blok |
| ESP32_04 | SSD1306 OLED SPI Display | OLED tampil via SPI |
| ESP32_05 | MCP3008 ADC Read Potentiometer | ADC berubah saat pot diputar |
| ESP32_06 | MCP4921 DAC Waveform Output | waveform analog terbentuk |
| ESP32_07 | ADXL345 SPI Acceleration | accel berubah saat digerakkan |

---

### 5.4 ESP32 SPI RTOS — 3 Eksperimen

| Kode | Demo wajib | Bukti output |
|---|---|---|
| ESP32_08 | SPI RTOS Multi Task | multi-task SPI berjalan |
| ESP32_09 | SPI RTOS DMA Benchmark | DMA vs polling benchmark |
| ESP32_10 | SPI RTOS Interrupt Driven | interrupt-based SPI transfer |

---

### 5.5 Multi STM32-ESP32 SPI — 3 non-RTOS + 2 RTOS

| Kode | Demo wajib | Bukti output |
|---|---|---|
| MULTI_01 | SPI Master-Slave Basic (STM32 slave) | data transfer master-slave valid |
| MULTI_02 | SPI Multi Slave CS Management | multi-slave CS berjalan |
| MULTI_03 | SPI Shared Bus Multi Device | shared bus tanpa konflik |
| MULTI_04 | SPI RTOS Gateway | gateway SPI RTOS berfungsi |
| MULTI_05 | SPI Data Acquisition System | integrasi final berjalan |

---

## 6. Demo Project Final

Project final yang ditampilkan: **Data Acquisition System Dual-MCU Modul 08**.

Wajib terlihat:

1. ESP32 dan STM32 aktif.
2. Scanner menemukan device SPI (Flash, SD, OLED, ADC, DAC, ADXL).
3. W25Q64 Flash read/write berhasil.
4. Micro SD Card terbaca dengan SPI mode.
5. SSD1306 OLED menampilkan dashboard real-time.
6. MCP3008 ADC membaca minimal 2 channel.
7. MCP4921 DAC menghasilkan waveform terukur.
8. ADXL345 membaca akselerasi 3-axis.
9. ESP32 dan STM32 komunikasi SPI master-slave atau RTOS gateway.
10. Log data ke W25Q64 Flash circular atau SD Card.
11. Error recovery ditunjukkan dengan device disconnect/reconnect.
12. Multi-slave CS management dijelaskan untuk mencegah konflik.

---

## 7. Narasi Minimum untuk Device SPI

Saat menjelaskan device SPI, sebutkan:

> W25Q64 Flash 8 MB memiliki page size 256 byte, sector 4 KB, block 32/64 KB. Operasi tulis membutuhkan Write Enable (0x06) sebelum Page Program (0x02) atau Erase. Status register menunjukkan BUSY saat operasi sedang berjalan.

> Micro SD Card dalam SPI mode dimulai dengan clock 400 kHz untuk inisialisasi (CMD0, CMD8, ACMD41), lalu clock dinaikkan ke 10+ MHz setelah card ready. Setiap blok data berukuran 512 byte.

> MCP3008 ADC 10-bit 8-channel menggunakan SPI mode 0. Pembacaan channel tunggal membutuhkan konfigurasi 5 bit (start, single/diff, channel select) dan menghasilkan 10-bit data (2 byte).

> MCP4921 DAC 12-bit mode 0, data 16-bit: bit 15-12 untuk konfigurasi, bit 11-0 untuk nilai DAC. Vout = (DAC_data / 4096) × Vref × Gain.

> ADXL345 Accelerometer menggunakan SPI mode 3 (CPOL=1, CPHA=1), DEVID register 0x00 berisi 0xE5, data X/Y/Z dalam 16-bit two's complement.

---

## 8. Format Pengumpulan

Kumpulkan:

1. Link video.
2. Link repository atau arsip source code.
3. Laporan PDF/MD.
4. Foto wiring final.
5. Tabel hasil 25 eksperimen.

Nama file/link disarankan:

```text
Modul08_SPI_Nama_NIM
```

---

## 9. Rubrik Penilaian Video

| Komponen | Bobot |
|---|---:|
| Pemahaman teori SPI lengkap | 15% |
| Demo 7 eksperimen STM32 SPI | 10% |
| Demo 3 eksperimen STM32 SPI RTOS | 5% |
| Demo 7 eksperimen ESP32 SPI | 10% |
| Demo 3 eksperimen ESP32 SPI RTOS | 5% |
| Demo 3 eksperimen Multi SPI non-RTOS | 10% |
| Demo 2 eksperimen Multi SPI RTOS | 10% |
| Demo project Data Acquisition System dual-MCU | 20% |
| Kualitas hardware demo dan wiring explanation | 10% |
| Kualitas audio/video/struktur presentasi | 5% |

---

## 10. Penalti

| Pelanggaran | Penalti |
|---|---:|
| Tidak ada demo hardware | −25% |
| Tidak menunjukkan 25 eksperimen | proporsional jumlah yang hilang |
| Salah menyebut Modul 07, bukan Modul 08 | −5% |
| Tidak menjelaskan mode SPI (CPOL/CPHA) | −5% |
| Tidak menjelaskan CS management multi-slave | −5% |
| Tidak ada output serial/display | −10% |
| Audio tidak jelas | −10% |
| Video terlalu pendek (<15 menit) | −10% |
| Terlambat | sesuai kebijakan kelas |

---

## 11. Checklist Akhir Sebelum Upload

- [ ] Judul video menyebut **Modul 08**.
- [ ] Bahasa Indonesia.
- [ ] Hardware ESP32 dan STM32 terlihat.
- [ ] MOSI/MISO/SCLK/CS dijelaskan.
- [ ] 7 eksperimen STM32 SPI ditampilkan.
- [ ] 3 eksperimen STM32 SPI RTOS ditampilkan.
- [ ] 7 eksperimen ESP32 SPI ditampilkan.
- [ ] 3 eksperimen ESP32 SPI RTOS ditampilkan.
- [ ] 3 eksperimen Multi SPI non-RTOS ditampilkan.
- [ ] 2 eksperimen Multi SPI RTOS ditampilkan.
- [ ] Project final ditampilkan.
- [ ] W25Q64 Flash dan SD Card berfungsi.
- [ ] ADC, DAC, ADXL345 berfungsi.
- [ ] Error recovery ditunjukkan.
- [ ] Link bisa diakses.
