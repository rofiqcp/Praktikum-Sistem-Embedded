# Project Modul 04: Smart Battery Monitoring System

## 📋 Informasi Project

| Item | Keterangan |
|------|------------|
| **Judul** | Dual-MCU Smart Battery Management System (BMS) Monitor |
| **Modul** | 04 - Analog to Digital Converter |
| **Platform** | STM32F103C8T6 + ESP32 DevKitC |
| **Tingkat Kesulitan** | ⭐⭐⭐ (Menengah) |
| **Durasi Pengerjaan** | 2 minggu |
| **Fokus Utama** | Akurasi Sampling Data & Signal Conditioning |

---

## 🎯 Deskripsi Project

Project ini bertujuan membuat sistem pemantauan kesehatan baterai (Battery Health Monitoring) real-time. Sistem ini mensimulasikan monitoring baterai UPS atau Solar Panel storage.

**Pembagian Tugas MCU:**
1.  **STM32 (Precision DAQ):** Bertugas membaca parameter fisik (Tegangan, Arus, Suhu) dengan kecepatan sirkular menggunakan ADC Multi-channel. Melakukan filtering digital (Moving Average) untuk menstabilkan data.
2.  **ESP32 (Internet Gateway):** Menerima data matang dari STM32, melakukan kalkulasi estimasi sisa daya (SoC - State of Charge), dan menampilkan data ke Web Dashboard.

---

## 🔧 Spesifikasi Hardware & Wiring

### Komponen
-   STM32F103C8T6 (Blue Pill)
-   ESP32 DevKitC V4
-   2x Potensiometer (Simulasi Tegangan Baterai 0-12V [scaled] & Arus Charge/Discharge)
-   1x LM35 / NTC Thermistor (Sensor Suhu Baterai)
-   LED Indikator (Merah=Low, Hijau=Normal, Biru=Charging)

### Skema Input Analog (STM32)
1.  **Channel 1 (Voltage Sim):** Potensio 1 dihubungkan ke PA0. (Asumsikan ini V_Bat yang diskalakan).
2.  **Channel 2 (Current Sim):** Potensio 2 dihubungkan ke PA1. (Asumsikan titik tengah 1.65V = 0 Ampere).
3.  **Channel 3 (Temp):** LM35/NTC dihubungkan ke PA2.

---

## 📋 Spesifikasi Fungsional

### F1: Data Acquisition (STM32)
-   Sampling Rate: Minimal 10 Hz per channel.
-   Resolution: 12-bit (0-4095).
-   **Filtering:** Wajib mengimplementasikan **Moving Average Filter** (Window size = 10 samples) sebelum data dikirim ke ESP32.
-   Konversi satuan fisik dilakukan di STM32 (kirim data dalam Volt, Ampere, Celcius).

### F2: Inter-MCU Communication
-   STM32 mengirim data paket struct ke ESP32 via UART (Serial 2) setiap 500ms.
-   Format Data: `{Voltage (float), Current (float), Temp (float)}`.

### F3: Processing & Warning (ESP32)
-   **Tegangan:**
    -   < 11.0V : Alert "Battery Critical" (LED Merah Blink).
    -   11.0V - 12.0V : Warning "Low Battery" (LED Merah On).
    -   > 12.0V : Normal.
-   **Arus:**
    -   Positif: "Discharging".
    -   Negatif: "Charging".
-   **State of Charge (SoC):** Estimasi % baterai berdasarkan tegangan (Lookup Table sederhana atau Linear Mapping).

---

## 🧪 Kriteria Pengujian

1.  **Akurasi Tegangan:** Bandingkan pembacaan STM32 dengan Multimeter. Toleransi error maks 2%.
2.  **Stabilitas:** Saat potensio diam, nilai pembacaan tidak boleh *jitter* lebih dari +/- 0.05V (Bukti filter berhasil).
3.  **Real-time:** Delay perubahan potensio sampai tampil di Serial Monitor maksimal 1 detik.

---

## 📝 Deliverables

1.  **Source Code:**
    -   Project STM32 (PlatformIO).
    -   Project ESP32 (PlatformIO).
2.  **Laporan Project:**
    -   Bab 1: Pendahuluan & Dasar Teori ADC.
    -   Bab 2: Desain Hardware & Rangkaian.
    -   Bab 3: Algoritma Filtering & Konversi Data.
    -   Bab 4: Hasil Pengujian Akurasi (Tabel Perbandingan ADC vs Multimeter).
3.  **Video Demo:**
    -   Demo perubahan tegangan/arus.
    -   Demo fitur filtering (noise reduction).
    -   Demo alarm batas tegangan.

---

## ⚠️ Tantangan (Bonus Points)
-   Gunakan **DMA (Direct Memory Access)** pada STM32 untuk pembacaan ADC (Nilai A+).
-   Implementasikan **Kalibrasi 2-Titik** untuk meningkatkan akurasi pembacaan LM35.
-   Tampilkan grafik data (Plotter) pada Web Dashboard ESP32.

