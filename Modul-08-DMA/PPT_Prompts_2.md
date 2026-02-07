# Prompt untuk Pembuatan PPT - Bagian 2
## Modul 08: Direct Memory Access (DMA) Implementation

**Instruksi Umum:**
Fokus pada implementasi teknis pada STM32 dan ESP32.

### Slide 6: DMA pada STM32F103
- **Judul:** STM32 DMA Controller
- **Konten:**
  - 2 Controller (DMA1 & DMA2).
  - Total 12 Channels (DMA1: 7 Ch, DMA2: 5 Ch).
  - **Fixed Mapping:** Setiap periferal punya channel spesifik.
    - Contoh: ADC1 ada di Channel 1. SPI1_RX di Channel 2.
- **Visual:** Tabel mapping sederhana DMA1 (Ch1-Ch7).

### Slide 7: Mode Operasi DMA
- **Judul:** Circular vs Normal Mode
- **Split Screen:**
  - **Normal Mode:** Transfer N data -> Berhenti. (Cocok untuk transfer file/buffer sekali jalan).
  - **Circular Mode:** Transfer N data -> Ulangi dari awal buffer otomatis. (Cocok untuk ADC Continuous, Audio Stream).
- **Keyword:** Ring Buffer.

### Slide 8: Langkah Coding STM32 (HAL)
- **Judul:** Flow implementasi di STM32Cube
- **Langkah:**
  1. Enable Clock DMA.
  2. Konfigurasi `DMA_Init` (Direction, Width, Priority).
  3. Hubungkan DMA Handle ke Peripheral Handle (`__HAL_LINKDMA`).
  4. Panggil fungsi `HAL_PPP_Start_DMA()`.
     - Contoh: `HAL_ADC_Start_DMA(&hadc1, buf, 100);`

### Slide 9: DMA pada ESP32
- **Judul:** ESP32 DMA Architecture
- **Konten:**
  - Berbeda dengan STM32F1.
  - Menggunakan konsep **Linked List Descriptors**.
  - Mendukung Scatter-Gather (Data tidak harus kontigu di memori).
  - Terintegrasi di periferal: I2S (untuk ADC/DAC/Parallel), SPI, dan RMT.

### Slide 10: Tantangan & Best Practice
- **Judul:** Tips Sukses DMA
- **Poin:**
  - **Memory Alignment:** Pastikan struct/array align dengan data width (32-bit aligned).
  - **Cache Coherency:** (Untuk MCU advanced M7/Esp32) Pastikan data di cache diskronisasi dengan RAM.
  - **Priority:** Jangan beri prioritas Very High ke semua channel, bisa memblokir CPU bus access.

### Slide 11: Tugas Praktikum
- **Judul:** Misi Hari Ini
- **Daftar Tugas:**
  1. **STM32:** Copy Memory-to-Memory (Benchmarking).
  2. **STM32:** Baca Potensiometer terus menerus tanpa loop CPU (Circular DMA).
  3. **ESP32:** Kirim data SPI Buffer besar loopback.
