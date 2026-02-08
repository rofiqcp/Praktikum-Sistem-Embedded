# Rubrik Penilaian Tugas Video — Modul 14: Power Management

## Informasi Umum

| Item | Keterangan |
|------|-----------|
| **Modul** | 14 — Power Management |
| **Platform** | STM32 (HAL) & ESP32 (ESP-IDF / Arduino) |
| **Tipe Penilaian** | Tugas Video Individu |
| **Durasi Video** | 5–8 menit |
| **Format** | MP4 (resolusi minimal 720p) |
| **Total Bobot** | 100% |

---

## Ringkasan Komponen Penilaian

| No | Komponen | Bobot |
|----|----------|-------|
| 1 | Konten Teknis | 30% |
| 2 | Demonstrasi Praktis | 25% |
| 3 | Analisis Data | 20% |
| 4 | Kualitas Video | 15% |
| 5 | Pemahaman Konsep | 10% |
| | **Total** | **100%** |

---

## 1. Konten Teknis (30%)

| Kriteria | Sangat Baik (90–100) | Baik (75–89) | Cukup (60–74) | Kurang (<60) |
|----------|----------------------|--------------|----------------|--------------|
| **Akurasi Penjelasan** | Penjelasan konsep power management (sleep modes, wakeup sources, clock gating, voltage scaling) 100% akurat dan sesuai dengan datasheet/referensi resmi; tidak ada kesalahan fakta | Penjelasan sebagian besar akurat (>85%); ada minor inakurasi yang tidak signifikan | Penjelasan cukup akurat (>70%); terdapat beberapa kesalahan fakta yang perlu dikoreksi | Penjelasan banyak kesalahan; konsep dasar salah atau membingungkan |
| **Terminologi** | Menggunakan terminologi teknis yang benar dan konsisten: deep sleep, light sleep, standby, stop mode, wakeup source, RTC, backup domain, power domain, current consumption, duty cycle | Terminologi sebagian besar benar; sesekali menggunakan istilah kurang tepat namun masih dapat dipahami | Terminologi dasar benar namun sering menggunakan istilah umum/non-teknis; inkonsisten | Terminologi salah atau tidak menggunakan istilah teknis sama sekali |
| **Kelengkapan Materi** | Mencakup semua topik utama: mode sleep (light/deep/hibernation), wakeup source (timer, GPIO, touch), data retention (RTC memory, backup register), power budget, battery management | Mencakup ≥4 topik utama dengan penjelasan yang memadai | Mencakup 2–3 topik utama; beberapa topik hanya disinggung sekilas | Hanya mencakup 1 topik atau penjelasan sangat dangkal |
| **Perbandingan ESP32 vs STM32** | Menjelaskan perbedaan arsitektur power management kedua platform secara detail: power domain, available modes, typical current consumption, wakeup capability; tabel perbandingan disajikan | Menjelaskan perbedaan utama kedua platform; beberapa aspek dibandingkan dengan cukup detail | Menyebutkan perbedaan secara umum tanpa detail teknis yang memadai | Tidak ada perbandingan antar platform |

---

## 2. Demonstrasi Praktis (25%)

| Kriteria | Sangat Baik (90–100) | Baik (75–89) | Cukup (60–74) | Kurang (<60) |
|----------|----------------------|--------------|----------------|--------------|
| **Demo Sleep Modes** | Mendemonstrasikan ≥3 sleep mode secara langsung (light sleep, deep sleep, standby/hibernation) pada hardware asli; transisi antar mode terlihat jelas di serial monitor; LED indikator menunjukkan state | Mendemonstrasikan 2 sleep mode secara langsung dengan hasil yang jelas terlihat | Mendemonstrasikan 1 sleep mode; hasil kurang jelas terlihat di video | Tidak ada demo sleep mode atau hanya simulasi tanpa hardware |
| **Pengukuran Arus** | Menunjukkan pengukuran arus real-time menggunakan multimeter/INA219/power profiler di setiap mode; nilai terukur ditampilkan dan dibandingkan dengan datasheet; setup pengukuran dijelaskan | Pengukuran arus ditunjukkan di ≥2 mode; nilai ditampilkan; setup cukup jelas | Pengukuran arus ditunjukkan di 1 mode; nilai ditampilkan namun tanpa perbandingan datasheet | Tidak ada pengukuran arus yang ditunjukkan |
| **Wakeup Behavior** | Demo wakeup dari berbagai sumber (timer, button/GPIO, touch pad) pada hardware asli; waktu wakeup terukur; behavior setelah wakeup (reinisialisasi, data recovery) ditunjukkan dengan jelas | Demo wakeup dari ≥2 sumber; behavior setelah wakeup ditunjukkan | Demo wakeup dari 1 sumber; behavior setelah wakeup kurang jelas | Tidak ada demo wakeup atau demo gagal |
| **Wiring & Setup** | Hardware setup ditunjukkan dengan jelas (close-up wiring, pin assignment); diagram skematik ditampilkan; komponen yang digunakan disebutkan dan dijelaskan fungsinya | Hardware setup ditunjukkan; pin assignment disebutkan; diagram skematik ada | Hardware terlihat namun wiring kurang jelas; tidak ada diagram skematik | Hardware setup tidak ditunjukkan |

---

## 3. Analisis Data (20%)

| Kriteria | Sangat Baik (90–100) | Baik (75–89) | Cukup (60–74) | Kurang (<60) |
|----------|----------------------|--------------|----------------|--------------|
| **Power Budget Calculation** | Perhitungan power budget lengkap ditampilkan: $I_{avg}$, duty cycle, estimasi battery life; formula ditunjukkan step-by-step; asumsi dijelaskan; hasil realistis dan divalidasi | Perhitungan power budget ada dengan formula yang benar; estimasi battery life dihitung; asumsi sebagian dijelaskan | Perhitungan power budget ada namun formula kurang lengkap; estimasi battery life ada tapi kurang akurat | Tidak ada perhitungan power budget |
| **Visualisasi Matplotlib** | Menampilkan ≥3 plot matplotlib yang informatif: current profile vs time, power mode distribution (pie/bar chart), battery discharge curve; plot rapi dengan label, title, legend, grid | Menampilkan 2 plot matplotlib dengan label dan title yang memadai | Menampilkan 1 plot matplotlib sederhana; label/title kurang lengkap | Tidak ada visualisasi matplotlib |
| **Perbandingan Data ESP32 vs STM32** | Data pengukuran kedua platform disajikan dalam tabel/grafik perbandingan; analisis perbedaan konsumsi daya per mode; rekomendasi pemilihan platform berdasarkan use case | Data perbandingan kedua platform ada dalam bentuk tabel; analisis singkat perbedaan | Menyebutkan angka konsumsi daya kedua platform tanpa tabel/grafik perbandingan | Tidak ada perbandingan data antar platform |
| **Interpretasi Hasil** | Interpretasi data mendalam: menjelaskan mengapa arus berbeda antar mode, faktor yang mempengaruhi (peripheral aktif, clock speed), korelasi dengan teori; insight untuk optimasi | Interpretasi data cukup baik; menjelaskan perbedaan antar mode; beberapa insight untuk optimasi | Interpretasi data dasar; mendeskripsikan hasil tanpa analisis mendalam | Tidak ada interpretasi; hanya menampilkan angka tanpa penjelasan |

---

## 4. Kualitas Video (15%)

| Kriteria | Sangat Baik (90–100) | Baik (75–89) | Cukup (60–74) | Kurang (<60) |
|----------|----------------------|--------------|----------------|--------------|
| **Kualitas Audio** | Audio jernih tanpa noise; volume konsisten; narasi mudah didengar dan dipahami; tidak ada echo/distorsi; menggunakan mikrofon yang memadai | Audio cukup jernih; volume stabil; narasi dapat didengar dengan baik; sedikit background noise | Audio dapat didengar namun ada noise yang cukup mengganggu; volume tidak konsisten | Audio sangat buruk; narasi sulit/tidak terdengar; noise dominan |
| **Kualitas Video** | Resolusi ≥1080p; frame rate stabil; pencahayaan baik; teks/kode di layar terbaca jelas; close-up hardware cukup detail; komposisi visual rapi | Resolusi ≥720p; pencahayaan cukup; teks/kode sebagian besar terbaca; hardware terlihat jelas | Resolusi 720p; pencahayaan kurang optimal; teks/kode agak sulit dibaca; hardware kurang jelas | Resolusi <720p; pencahayaan buruk; teks/kode tidak terbaca; video blur/gelap |
| **Editing & Struktur** | Video di-edit dengan baik; ada opening/closing; transisi antar segmen halus; screen recording kode/terminal jelas; timestamps/chapter markers untuk navigasi; tidak ada bagian yang tidak relevan | Video di-edit cukup baik; ada struktur pembukaan dan penutup; transisi antar segmen ada; screen recording cukup jelas | Video di-edit minimal; struktur kurang terorganisir; beberapa bagian terlalu panjang atau tidak relevan | Video tidak di-edit; tidak ada struktur; banyak bagian tidak relevan atau kosong |
| **Durasi** | Durasi 5–8 menit; konten padat tanpa pengulangan; pacing tepat (tidak terlalu cepat/lambat); setiap menit memiliki konten bermakna | Durasi 5–8 menit; konten cukup padat; sedikit pengulangan; pacing cukup baik | Durasi di luar range (4–5 atau 8–10 menit); ada pengulangan/bagian kosong; pacing kurang tepat | Durasi <4 menit atau >10 menit; banyak pengulangan/bagian kosong; pacing buruk |

---

## 5. Pemahaman Konsep (10%)

| Kriteria | Sangat Baik (90–100) | Baik (75–89) | Cukup (60–74) | Kurang (<60) |
|----------|----------------------|--------------|----------------|--------------|
| **Penguasaan Materi** | Menjelaskan konsep power management tanpa membaca teks/catatan; dapat mengelaborasi di luar materi dasar; menghubungkan konsep dengan aplikasi nyata (IoT, wearable, remote sensor) | Menjelaskan konsep dengan baik; sesekali merujuk catatan; dapat memberikan contoh aplikasi | Menjelaskan konsep dasar; sering membaca catatan/slide; contoh aplikasi kurang relevan | Membaca seluruh penjelasan; tidak menguasai konsep yang disampaikan |
| **Pemahaman Trade-off** | Menjelaskan trade-off power management dengan mendalam: latensi wakeup vs penghematan daya, data retention vs konsumsi arus, responsivitas vs battery life; keputusan desain dijelaskan dengan reasoning yang kuat | Memahami beberapa trade-off utama; dapat menjelaskan alasan pemilihan konfigurasi power mode | Menyebutkan trade-off secara umum tanpa analisis mendalam; alasan desain kurang kuat | Tidak memahami trade-off; tidak dapat menjelaskan alasan keputusan desain |
| **Jawaban Pertanyaan** | Mampu menjawab pertanyaan mendalam (diajukan dosen/asisten atau self-posed) dengan benar dan elaboratif; menunjukkan pemahaman menyeluruh terhadap materi modul | Mampu menjawab pertanyaan dengan benar; penjelasan cukup detail | Menjawab pertanyaan dasar; penjelasan kurang detail atau sebagian kurang tepat | Tidak dapat menjawab pertanyaan atau jawaban salah |

---

## Perhitungan Nilai Akhir

$$\text{Nilai Akhir} = \sum_{i=1}^{5} \left( \text{Skor Komponen}_i \times \text{Bobot}_i \right)$$

### Contoh Perhitungan

| Komponen | Skor | Bobot | Kontribusi |
|----------|------|-------|------------|
| Konten Teknis | 85 | 30% | 25.50 |
| Demonstrasi Praktis | 80 | 25% | 20.00 |
| Analisis Data | 78 | 20% | 15.60 |
| Kualitas Video | 90 | 15% | 13.50 |
| Pemahaman Konsep | 82 | 10% | 8.20 |
| **Total** | | **100%** | **82.80** |

### Kategori Nilai Akhir

| Rentang Nilai | Huruf | Predikat |
|---------------|-------|----------|
| 90 – 100 | A | Sangat Baik |
| 75 – 89 | B | Baik |
| 60 – 74 | C | Cukup |
| < 60 | D | Kurang |

---

## Ketentuan Pengumpulan

1. **Format**: Video diunggah ke YouTube (Unlisted) atau Google Drive; link disubmit melalui LMS.
2. **Deadline**: Sesuai jadwal yang ditentukan dosen pengampu.
3. **Keterlambatan**: Pengurangan **10 poin per hari** keterlambatan (maksimal 3 hari).
4. **Thumbnail**: Sertakan thumbnail yang menunjukkan judul modul dan nama mahasiswa.
5. **Deskripsi Video**: Cantumkan nama, NIM, kelas, dan daftar timestamp konten video.
6. **Plagiarisme**: Video yang terbukti menjiplak konten orang lain akan mendapat nilai **0**.
7. **Source Code**: Sertakan link repository GitHub berisi source code yang didemonstrasikan dalam deskripsi video.

---

## Checklist Sebelum Pengumpulan

- [ ] Video berdurasi 5–8 menit
- [ ] Resolusi minimal 720p, audio jernih
- [ ] Demo sleep mode pada hardware asli (ESP32 dan/atau STM32)
- [ ] Pengukuran arus ditunjukkan dengan nilai terukur
- [ ] Perhitungan power budget ditampilkan
- [ ] Minimal 1 plot matplotlib ditampilkan
- [ ] Perbandingan ESP32 vs STM32 dibahas
- [ ] Trade-off power management dijelaskan
- [ ] Link YouTube/Google Drive dapat diakses
- [ ] Source code tersedia di GitHub
