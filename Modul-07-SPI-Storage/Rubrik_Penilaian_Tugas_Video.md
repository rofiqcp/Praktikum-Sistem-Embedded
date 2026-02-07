# Rubrik Penilaian Tugas Video
## Modul 07: SPI Bus & Storage Implementation

---

## 📋 Informasi Tugas

| Item | Keterangan |
|------|------------|
| **Topik** | SPI Master-Slave & Data Logging |
| **Platform** | STM32F103 & ESP32 |
| **Durasi Video** | 7-12 menit |
| **Format** | MP4 (H.264), 720p minimum |
| **Bobot Total** | 100 poin |

---

## 📹 Struktur Video yang Diharapkan

### Timeline Rekomendasi

| Segmen | Durasi | Konten |
|--------|--------|--------|
| Opening | 0:30 | Identitas, Judul Project |
| Konsep SPI | 2:00 | Penjelasan teori SPI & pinout yang digunakan |
| Wiring Tour | 1:00 | Close-up rangkaian hardware |
| Demo Log | 2:00 | Proses logging data & validasi file CSV |
| Demo Config | 1:30 | Ubah config di SD Card -> Sistem berubah behavior |
| Code Explain | 2:00 | Highlight bagian critical (SPI ISR, File IO) |
| Closing | 1:00 | Kesimpulan & Kendala |
| **Total** | **~10:00** | |

---

## 📊 Komponen Penilaian

### A. Konten Edukatif (30 poin)

| No | Kriteria | Poin Max | Skor |
|----|----------|----------|------|
| 1 | Penjelasan prinsip kerja Master-Slave SPI | 10 | |
| 2 | Penjelasan timing diagram (CPOL/CPHA) | 5 | |
| 3 | Alasan penggunaan SD Card vs Internal Flash | 5 | |
| 4 | Penjelasan flow data (Sensor->STM32->ESP32->SD) | 10 | |
| | **Subtotal A** | **30** | |

### B. Demonstrasi Fungsional (40 poin)

| No | Kriteria | Poin Max | Skor |
|----|----------|----------|------|
| 1 | **Boot Process**: Serial monitor output jelas saat inisialisasi | 10 | |
| 2 | **Data Acquisition**: Nilai sensor berubah real-time | 10 | |
| 3 | **Storage Verification**: Membuka file CSV di PC dan datanya valid | 15 | |
| 4 | **Stability**: Sistem tidak crash selama demo | 5 | |
| | **Subtotal B** | **40** | |

### C. Kualitas Produksi (20 poin)

| No | Kriteria | Poin Max | Skor |
|----|----------|----------|------|
| 1 | Visual: Fokus, Pencahayaan, Stabil | 5 | |
| 2 | Overlay Text: Menampilkan poin penting/setup | 5 | |
| 3 | Audio: Suara jelas (mic good quality/voiceover) | 5 | |
| 4 | Editing: Cut rapi, transisi smooth, durasi pas | 5 | |
| | **Subtotal C** | **20** | |

### D. Kreativitas & Penyampaian (10 poin)

| No | Kriteria | Poin Max | Skor |
|----|----------|----------|------|
| 1 | Penggunaan Logic Analyzer untuk visualisasi sinyal | 5 | |
| 2 | Gaya bahasa menarik dan mudah dipahami | 5 | |
| | **Subtotal D** | **10** | |

---

## 🌟 Bonus Points

| No | Kriteria Bonus | Poin |
|----|----------------|------|
| 1 | Visualisasi data CSV menjadi Grafik di Excel/Python | +5 |
| 2 | Menampilkan waveform SPI asli dari osiloskop/logic analyzer | +5 |

---

## ⚠️ Penalty Points

| No | Pelanggaran | Pengurangan |
|----|-------------|-------------|
| 1 | Durasi < 5 menit atau > 15 menit | -10 |
| 2 | Video buram/goyang parah | -10 |
| 3 | Suara tidak terdengar | -15 |
| 4 | Plagiarisme total | -100 |

---

## 💬 Feedback Penilai

**General Impression:**
```
[Tulis kesan umum video]
```

**Improvement Points:**
```
[Saran untuk tugas video berikutnya]
```

---

| | |
|----------|------------|
| **Penilai** | _________________ |
| **Tanggal** | _________________ |

