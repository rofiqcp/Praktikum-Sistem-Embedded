# Project Modul 08: High-Speed Signal Acquisition System

## 📝 Deskripsi Project
Mahasiswa diminta membuat sistem akuisisi data "Zero-CPU-Overhead" menggunakan STM32. Sistem ini mensimulasikan unit pemrosesan sinyal audio atau sensor getaran industri yang memerlukan sampling rate tinggi tanpa membebani CPU.

Sistem akan membaca data analog (ADC) terus menerus ke dalam Circular Buffer, melakukan pemrosesan sederhana (misal: rata-rata atau thresholding), lalu mengirimkan hasilnya ke Serial Monitor (UART).

## 🎯 Tujuan
1. Mengimplementasikan **Circular Buffer** dengan DMA pada ADC.
2. Menggunakan mekanisme **Double Buffering (Ping-Pong)** untuk pemrosesan data real-time.
3. Memastikan tidak ada data yang hilang (sample loss) selama proses pengiriman data UART.

## ⚙️ Spesifikasi Teknis

### Hardware
- MCU: STM32F103 (Blue Pill)
- Input: 2x Potensiometer (Simulasi sinyal stereo/dual sensor)
- Output: USB Serial (ke PC Serial Plotter)

### Firmware Requirements
1. **ADC Configuration:**
   - Dual Channel (Scan Mode).
   - Sampling Time: Terapkan secepat mungkin (misal 10-100 kHz total).
2. **DMA Configuration:**
   - Circular Mode.
   - Buffer Size: 100-500 sampel per channel.
   - Data alignment: Half Word (16-bit).
3. **Processing Logic:**
   - Gunakan interrupt `HAL_ADC_ConvHalfCpltCallback` (HTC) untuk memproses paruh pertama buffer.
   - Gunakan interrupt `HAL_ADC_ConvCpltCallback` (TC) untuk memproses paruh kedua buffer.
   - Mengubah LED status saat buffer flip terjadi (Ping vs Pong).

## 🧪 Skenario Pengujian
1. Putar potensiometer dengan cepat.
2. Amati waveform di Serial Plotter.
3. Pastikan waveform mulus tanpa ada "glitch" atau garis putus yang menandakan buffer overrun/sample loss.

## 📄 Deliverables
- [ ] Kode program (STM32CubeIDE)
- [ ] Laporan Analisis
- [ ] Video demonstrasi Serial Plotter
