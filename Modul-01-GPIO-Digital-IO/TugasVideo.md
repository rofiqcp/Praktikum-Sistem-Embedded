# Tugas Video — Modul 01: GPIO dan Digital I/O

## Informasi Tugas

| Item | Keterangan |
|------|------------|
| **Jenis Tugas** | Video Laporan Praktikum & Project |
| **Durasi Video** | 15–25 menit |
| **Format** | MP4 (H.264) atau link YouTube (Unlisted/Public) |
| **Resolusi Minimum** | 720p (1280×720) |
| **Penilaian** | Individu |
| **Deadline** | 1 minggu setelah praktikum |
| **Total Skor** | 100 poin |

---

## Deskripsi Tugas

Mahasiswa membuat **satu video utuh** yang merupakan **laporan lengkap** dari seluruh kegiatan praktikum dan project Modul 01: GPIO dan Digital I/O. Video ini bukan tutorial singkat, melainkan dokumentasi menyeluruh yang mencakup:

1. **Penjelasan materi** dari awal (teori GPIO, mode output, mode input, debounce, encoder, keypad)
2. **Seluruh 10 percobaan praktikum** (P01–P10) — masing-masing ditunjukkan proses dan hasilnya
3. **Project** (Sistem Kontrol Akses Ruang Laboratorium) — demo dan penjelasan

---

## Ketentuan Teknis Video

### Wajib Ada dalam Video

| Elemen | Keterangan |
|--------|------------|
| **Screen recording** | Layar komputer terlihat saat menjelaskan kode, compile, upload, dan serial monitor |
| **Webcam** | Wajah presenter **harus terlihat** (overlay di pojok atau split screen) selama menjelaskan |
| **Hardware demo** | Video kamera terpisah atau close-up menunjukkan rangkaian fisik (LED, tombol, encoder, keypad, LCD) saat setiap percobaan dijalankan |
| **Narasi suara** | Menjelaskan secara lisan setiap bagian, bukan hanya teks di layar |

### Struktur Video yang Wajib Diikuti

```
1. PEMBUKAAN (1–2 menit)
   - Perkenalan: Nama, NIM, Mata Kuliah, Modul
   - Overview singkat apa yang akan dibahas
   - Tujuan praktikum

2. PENJELASAN MATERI (3–5 menit)
   - Konsep GPIO: apa itu GPIO, arsitektur internal
   - Mode output: Push-Pull vs Open-Drain
   - Mode input: Pull-Up vs Pull-Down (eksternal & internal)
   - Konsep debounce, encoder kuadratur, keypad matrix
   - Active-HIGH vs Active-LOW
   - Perbedaan STM32 HAL vs ESP-IDF
   (Gunakan diagram, slide, atau gambar di layar)

3. PERCOBAAN PRAKTIKUM (7–12 menit)
   Untuk SETIAP percobaan (P01–P10):
   a. Sebutkan judul dan tujuan percobaan
   b. Tunjukkan rangkaian hardware (video kamera)
   c. Tunjukkan kode program penting (screen recording)
   d. Upload dan jalankan program
   e. Tunjukkan hasil: LED menyala, LCD display, serial monitor output
   f. Berikan analisa singkat hasil percobaan

   Percobaan:
   - P01: LED Parade — output push-pull, 4 pola LED
   - P02: Shadow & Ghost — open-drain vs push-pull, LED redup
   - P03: Sentinel Gate — tombol pull-up eksternal
   - P04: Ground Guardian — tombol pull-down eksternal
   - P05: Phantom Touch — pull-up internal
   - P06: Force Field — pull-down internal
   - P07: Clean Contact — debounce state machine 4 tombol
   - P08: Speed Racer — GPIO speed/drive, pola LED multi-mode
   - P09: Twist & Count — rotary encoder, binary counter
   - P10: Matrix Commander — keypad 4×4 scanning

4. PROJECT (3–5 menit)
   - Jelaskan skenario project (Sistem Kontrol Akses Lab)
   - Tunjukkan rangkaian hardware lengkap (video kamera)
   - Demo fitur: keypad input kode, encoder brightness, tombol darurat
   - Tunjukkan LCD display dan semua state sistem
   - Jelaskan bagaimana 10 percobaan terintegrasi dalam project

5. PENUTUP (1–2 menit)
   - Kesimpulan: apa yang dipelajari dari seluruh praktikum
   - Kesulitan yang dihadapi dan solusinya
   - Saran/refleksi
```

---

## Komponen Penilaian

| Komponen | Bobot | Poin |
|----------|-------|------|
| A. Penjelasan Materi | 20% | 20 |
| B. Percobaan Praktikum (P01–P10) | 35% | 35 |
| C. Project | 20% | 20 |
| D. Kualitas Teknis Video | 15% | 15 |
| E. Penyampaian & Komunikasi | 10% | 10 |
| **Total** | **100%** | **100** |

---

## Rubrik Detail

### A. Penjelasan Materi (20 poin)

| Poin | Kriteria |
|------|----------|
| 17–20 | Semua konsep dijelaskan dengan akurat, lengkap, dan jelas. Ada diagram/slide pendukung. |
| 13–16 | Sebagian besar konsep benar, ada minor gaps |
| 9–12 | Penjelasan dasar, beberapa konsep kurang mendalam |
| 5–8 | Penjelasan tidak lengkap atau ada kesalahan konsep |
| 0–4 | Tidak ada penjelasan materi |

### B. Percobaan Praktikum (35 poin)

| Poin | Kriteria |
|------|----------|
| 30–35 | Semua 10 percobaan ditunjukkan lengkap: hardware terlihat, kode dijelaskan, hasil demo jelas, analisa diberikan |
| 24–29 | 8–9 percobaan lengkap, sisanya kurang detail |
| 17–23 | 5–7 percobaan ditunjukkan |
| 10–16 | Kurang dari 5 percobaan |
| 0–9 | Sangat minim atau tidak ada demo percobaan |

### C. Project (20 poin)

| Poin | Kriteria |
|------|----------|
| 17–20 | Project berjalan lengkap sesuai spesifikasi, semua fitur di-demo, penjelasan integrasi konsep jelas |
| 13–16 | Project berjalan dengan sebagian besar fitur, penjelasan cukup |
| 9–12 | Project parsial, beberapa fitur belum jalan |
| 5–8 | Project minimal, banyak fitur tidak berfungsi |
| 0–4 | Tidak ada demo project |

### D. Kualitas Teknis Video (15 poin)

| Aspek | Poin | Kriteria |
|-------|------|----------|
| Visual (screen rec + webcam + hardware) | 0–6 | **6:** Semua elemen visual ada dan jelas. **4:** Ada tapi kurang jelas. **2:** Hanya satu elemen. **0:** Tidak bisa dilihat |
| Audio | 0–5 | **5:** Jernih, tidak ada noise. **3:** Dapat didengar. **1:** Sulit didengar |
| Durasi & editing | 0–4 | **4:** Sesuai durasi, editing rapi. **2:** Sedikit di luar range. **0:** Terlalu pendek/panjang |

### E. Penyampaian & Komunikasi (10 poin)

| Poin | Kriteria |
|------|----------|
| 9–10 | Sangat jelas, percaya diri, alur logis, mudah diikuti |
| 7–8 | Jelas dengan minor gaps |
| 5–6 | Cukup jelas |
| 3–4 | Membingungkan |
| 0–2 | Tidak dapat dimengerti |

---

## Penalti

| Pelanggaran | Penalti |
|-------------|---------|
| Tidak ada webcam (wajah tidak terlihat) | −10 poin |
| Tidak ada hardware demo (hanya screen rec) | −15 poin |
| Tidak ada screen recording (hanya hardware) | −10 poin |
| Terlambat submit (per hari) | −5 poin |
| Plagiarisme | Nilai 0 |
| Durasi < 10 menit | −10 poin |
| Durasi > 30 menit | −5 poin |

---

## Checklist Sebelum Submit

```
□ Video dapat diputar di device lain
□ Webcam terlihat jelas selama presentasi
□ Screen recording terbaca (font cukup besar)
□ Hardware demo terlihat jelas (LED, tombol, LCD, encoder, keypad)
□ Audio jernih dan terdengar
□ Durasi 15–25 menit
□ Semua 10 percobaan ditunjukkan
□ Project di-demo
□ Nama dan NIM disebutkan di video
□ Ada pembukaan dan penutup
```

---

## Tips Teknis

| Tool | Fungsi |
|------|--------|
| OBS Studio | Screen recording + webcam overlay |
| DaVinci Resolve / Kdenlive | Editing gratis |
| HP/Kamera | Rekam hardware demo terpisah, gabungkan saat editing |

1. **Screen recording:** Resolusi 1920×1080, font 14pt+, dark theme IDE
2. **Webcam:** Posisi di pojok kanan bawah, tidak menutupi kode penting
3. **Hardware demo:** Close-up pada LED, tombol, LCD agar terlihat jelas
4. **Narasi:** Jelaskan sambil menunjukkan, jangan hanya membaca teks

---

*Tugas Video Modul 01 — Praktikum Sistem Embedded*
