# Rubrik Penilaian Project — Modul 14: Power Management

## Informasi Umum

| Item | Keterangan |
|------|-----------|
| **Modul** | 14 — Power Management |
| **Platform** | STM32 (HAL) & ESP32 (ESP-IDF / Arduino) |
| **Tipe Penilaian** | Project Akhir Modul |
| **Total Bobot** | 100% |

---

## Ringkasan Komponen Penilaian

| No | Komponen | Bobot |
|----|----------|-------|
| 1 | Implementasi Power Modes | 25% |
| 2 | Data Retention & State Management | 15% |
| 3 | Power Budget Analysis | 20% |
| 4 | Battery Management | 15% |
| 5 | Python Debug & Analysis | 10% |
| 6 | Kode & Dokumentasi | 10% |
| 7 | Presentasi & Demo | 5% |
| | **Total** | **100%** |

---

## 1. Implementasi Power Modes (25%)

| Kriteria | Sangat Baik (90–100) | Baik (75–89) | Cukup (60–74) | Kurang (<60) |
|----------|----------------------|--------------|----------------|--------------|
| **Sleep Mode** | Mengimplementasikan seluruh mode (Light Sleep, Deep Sleep, dan Hibernation/Standby) dengan benar pada ESP32 maupun STM32; transisi antar mode berjalan mulus tanpa crash | Mengimplementasikan minimal 2 mode sleep dengan benar; transisi stabil pada sebagian besar skenario | Hanya 1 mode sleep yang berhasil diimplementasikan; terdapat minor bug saat transisi | Tidak ada mode sleep yang berfungsi dengan benar; sistem hang atau crash |
| **Wakeup Source** | Mengkonfigurasi ≥3 wakeup source berbeda (timer, GPIO/ext0/ext1, touch pad, ULP untuk ESP32; RTC alarm, WKUP pin, EXTI untuk STM32) dan semuanya berfungsi sempurna | Mengkonfigurasi 2 wakeup source dengan benar dan berfungsi stabil | Hanya 1 wakeup source yang dikonfigurasi dan berfungsi | Tidak ada wakeup source yang berfungsi; sistem tidak dapat bangun dari sleep |
| **Mode Transition** | State machine transisi power mode terdokumentasi dengan diagram; penanganan edge case (wakeup gagal, timeout) diimplementasikan; re-inisialisasi peripheral setelah wakeup berjalan sempurna | Transisi antar mode bekerja dengan baik; peripheral di-reinisialisasi setelah wakeup namun ada minor delay | Transisi bekerja sebagian; beberapa peripheral tidak di-reinisialisasi dengan benar setelah wakeup | Transisi tidak terstruktur; peripheral kehilangan konfigurasi setelah wakeup tanpa recovery |

---

## 2. Data Retention & State Management (15%)

| Kriteria | Sangat Baik (90–100) | Baik (75–89) | Cukup (60–74) | Kurang (<60) |
|----------|----------------------|--------------|----------------|--------------|
| **RTC Memory (ESP32)** | Memanfaatkan `RTC_DATA_ATTR` / RTC slow memory untuk menyimpan variabel kritis (boot count, sensor terakhir, state aplikasi); data konsisten setelah deep sleep | Menggunakan RTC memory untuk menyimpan ≥2 variabel; data bertahan setelah deep sleep dengan benar | Menggunakan RTC memory untuk 1 variabel saja; data kadang tidak konsisten | Tidak menggunakan RTC memory; semua data hilang setelah deep sleep |
| **Backup Register (STM32)** | Memanfaatkan backup register (BKP/RTC) dan backup SRAM untuk menyimpan state kritis; data bertahan melalui Standby mode; VBAT domain dikonfigurasi dengan benar | Menggunakan backup register untuk menyimpan ≥2 data; bertahan melalui Standby mode | Menggunakan backup register minimal; data sebagian hilang setelah Standby | Tidak menggunakan backup register; tidak ada data retention |
| **Boot Detection** | Mendeteksi dan membedakan cold boot vs warm boot vs wakeup dari berbagai sumber; menampilkan informasi reset cause; logic aplikasi berbeda berdasarkan tipe boot | Mendeteksi cold boot vs warm boot dengan benar; reset cause ditampilkan | Deteksi boot sebagian benar; tidak semua skenario ter-handle | Tidak ada mekanisme deteksi tipe boot |

---

## 3. Power Budget Analysis (20%)

| Kriteria | Sangat Baik (90–100) | Baik (75–89) | Cukup (60–74) | Kurang (<60) |
|----------|----------------------|--------------|----------------|--------------|
| **Pengukuran Arus** | Mengukur konsumsi arus di setiap mode (active, light sleep, deep sleep, hibernation) menggunakan multimeter/INA219/power profiler; hasil akurat dan sesuai datasheet (±10%) | Mengukur arus di ≥3 mode; hasil mendekati datasheet (±20%) | Mengukur arus di ≥2 mode; deviasi cukup besar dari datasheet (±30%) | Tidak melakukan pengukuran arus atau hasil sangat tidak akurat |
| **Duty Cycle Calculation** | Menghitung duty cycle (waktu aktif vs sleep) dengan benar; formula ditunjukkan; analisis dampak duty cycle terhadap rata-rata konsumsi daya | Perhitungan duty cycle benar; formula ditunjukkan namun analisis kurang mendalam | Duty cycle dihitung namun ada kesalahan minor; formula kurang jelas | Tidak ada perhitungan duty cycle |
| **Estimasi Masa Pakai Baterai** | Menghitung estimasi battery life menggunakan formula $I_{avg} = (I_{active} \times t_{active} + I_{sleep} \times t_{sleep}) / (t_{active} + t_{sleep})$ dengan benar; mempertimbangkan kapasitas baterai (mAh), self-discharge, dan efisiensi regulator; hasil realistis | Estimasi battery life dihitung dengan formula dasar; mempertimbangkan kapasitas baterai; hasil cukup realistis | Estimasi battery life ada namun formula kurang tepat atau asumsi tidak realistis | Tidak ada estimasi battery life |
| **Strategi Optimasi** | Mengusulkan dan mengimplementasikan ≥3 strategi optimasi (clock gating, peripheral shutdown, voltage scaling, adaptive sleep); dampak terukur dan didokumentasikan | Mengimplementasikan 2 strategi optimasi dengan dampak terukur | Mengimplementasikan 1 strategi optimasi; dampak tidak terukur jelas | Tidak ada strategi optimasi yang diimplementasikan |

---

## 4. Battery Management (15%)

| Kriteria | Sangat Baik (90–100) | Baik (75–89) | Cukup (60–74) | Kurang (<60) |
|----------|----------------------|--------------|----------------|--------------|
| **Monitoring Level Baterai** | Membaca tegangan baterai via ADC dengan akurat; konversi ke persentase menggunakan discharge curve yang sesuai tipe baterai (Li-Ion/LiPo); kalibrasi dilakukan | Membaca tegangan baterai via ADC; konversi ke persentase dengan formula linear; cukup akurat | Membaca tegangan baterai namun konversi persentase kurang akurat; tidak ada kalibrasi | Tidak ada monitoring level baterai |
| **Low-Power Warning** | Sistem peringatan multi-level (warning, critical, shutdown); LED/buzzer/serial notification berfungsi; threshold dapat dikonfigurasi; graceful shutdown sebelum baterai habis | Warning saat baterai rendah berfungsi; minimal 2 level peringatan; notifikasi via serial/LED | Warning dasar saat baterai rendah; hanya 1 level peringatan | Tidak ada sistem peringatan baterai rendah |
| **Adaptive Duty Cycling** | Sistem otomatis menyesuaikan frekuensi sampling/reporting berdasarkan level baterai; semakin rendah baterai → semakin jarang sampling; transisi halus antar mode | Duty cycle berubah berdasarkan 2 level baterai (normal/low); transisi berfungsi | Implementasi adaptive duty cycle ada namun tidak responsif atau hanya 1 level perubahan | Tidak ada adaptive duty cycling |

---

## 5. Python Debug & Analysis (10%)

| Kriteria | Sangat Baik (90–100) | Baik (75–89) | Cukup (60–74) | Kurang (<60) |
|----------|----------------------|--------------|----------------|--------------|
| **Fungsionalitas `debug_analysis.py`** | Script berfungsi penuh: membaca data serial, mem-parsing log power mode, menyimpan ke CSV/JSON; error handling robust; argumen CLI tersedia | Script membaca dan mem-parsing data serial dengan benar; output tersimpan; minor error handling | Script berjalan namun parsing sebagian gagal; output tidak lengkap | Script tidak berfungsi atau tidak ada |
| **Visualisasi Data** | Menghasilkan ≥3 plot informatif menggunakan matplotlib (current vs time, power mode timeline, pie chart distribusi mode); axis label, title, legend lengkap; plot tersimpan sebagai PNG | Menghasilkan 2 plot dengan matplotlib; label dan title ada; output PNG tersedia | Menghasilkan 1 plot sederhana; label/title kurang lengkap | Tidak ada visualisasi data |
| **Current Profiling Plots** | Grafik current profile menunjukkan transisi antar power mode dengan jelas; anotasi pada titik wakeup/sleep; perbandingan arus antar mode divisualisasikan; statistik (min, max, avg) ditampilkan | Current profile plot ada dan menunjukkan transisi mode; beberapa anotasi tersedia | Current profile plot ada namun kurang detail; tidak ada anotasi | Tidak ada current profiling plot |

---

## 6. Kode & Dokumentasi (10%)

| Kriteria | Sangat Baik (90–100) | Baik (75–89) | Cukup (60–74) | Kurang (<60) |
|----------|----------------------|--------------|----------------|--------------|
| **Kualitas Kode** | Kode terstruktur modular (fungsi terpisah per fitur); penamaan variabel/fungsi deskriptif; tidak ada dead code; efisien dalam penggunaan memori | Kode cukup terstruktur; penamaan variabel cukup deskriptif; sedikit dead code | Kode kurang terstruktur; penamaan variabel kurang jelas; ada dead code | Kode tidak terstruktur; sulit dibaca; banyak dead code |
| **Komentar & README** | Setiap fungsi memiliki header comment (deskripsi, parameter, return); komentar inline pada bagian kritis; README lengkap (wiring, cara build, cara run, screenshot hasil) | Komentar ada pada sebagian besar fungsi; README ada dengan instruksi dasar | Komentar minim; README ada namun kurang lengkap | Tidak ada komentar; tidak ada README |
| **Penggunaan HAL/ESP-IDF** | Menggunakan API HAL (STM32) atau ESP-IDF/Arduino (ESP32) dengan benar sesuai best practice; error checking pada setiap panggilan API; penggunaan fitur power management API yang tepat | API digunakan dengan benar; error checking pada sebagian besar panggilan | API digunakan namun ada beberapa panggilan yang kurang tepat; minim error checking | API digunakan secara salah atau tidak konsisten; tidak ada error checking |

---

## 7. Presentasi & Demo (5%)

| Kriteria | Sangat Baik (90–100) | Baik (75–89) | Cukup (60–74) | Kurang (<60) |
|----------|----------------------|--------------|----------------|--------------|
| **Demonstrasi Langsung** | Demo berjalan lancar tanpa kendala; menunjukkan semua fitur (sleep, wakeup, battery monitoring, current measurement); dapat menjelaskan setiap langkah dengan percaya diri | Demo berjalan dengan kendala minor; sebagian besar fitur ditunjukkan; penjelasan cukup baik | Demo berjalan dengan beberapa kendala; hanya sebagian fitur yang berhasil ditunjukkan | Demo gagal atau tidak dilakukan |
| **Tanya Jawab** | Menjawab semua pertanyaan dengan benar dan mendalam; memahami konsep power management secara menyeluruh; dapat menjelaskan trade-off desain | Menjawab sebagian besar pertanyaan dengan benar; pemahaman konsep cukup baik | Menjawab beberapa pertanyaan dengan benar; pemahaman konsep dasar | Tidak dapat menjawab pertanyaan; pemahaman konsep sangat kurang |

---

## Perhitungan Nilai Akhir

$$\text{Nilai Akhir} = \sum_{i=1}^{7} \left( \text{Skor Komponen}_i \times \text{Bobot}_i \right)$$

### Contoh Perhitungan

| Komponen | Skor | Bobot | Kontribusi |
|----------|------|-------|------------|
| Implementasi Power Modes | 85 | 25% | 21.25 |
| Data Retention & State Management | 90 | 15% | 13.50 |
| Power Budget Analysis | 80 | 20% | 16.00 |
| Battery Management | 75 | 15% | 11.25 |
| Python Debug & Analysis | 88 | 10% | 8.80 |
| Kode & Dokumentasi | 82 | 10% | 8.20 |
| Presentasi & Demo | 90 | 5% | 4.50 |
| **Total** | | **100%** | **83.50** |

### Kategori Nilai Akhir

| Rentang Nilai | Huruf | Predikat |
|---------------|-------|----------|
| 90 – 100 | A | Sangat Baik |
| 75 – 89 | B | Baik |
| 60 – 74 | C | Cukup |
| < 60 | D | Kurang |

---

## Catatan Penting

1. **Plagiarisme**: Kode yang terbukti hasil plagiarisme akan mendapat nilai **0** untuk seluruh project.
2. **Keterlambatan**: Pengumpulan terlambat akan dikenakan pengurangan **10 poin per hari** keterlambatan (maksimal 3 hari; lebih dari itu tidak diterima).
3. **Kelengkapan**: Project harus dapat di-build menggunakan PlatformIO tanpa error. Project yang tidak dapat di-compile otomatis mendapat pengurangan **20 poin**.
4. **Kedua Platform**: Idealnya project mencakup implementasi untuk **ESP32 dan STM32**. Hanya mengerjakan satu platform akan mendapat maksimal **80%** dari skor komponen terkait.
5. **Keselamatan**: Pastikan penanganan baterai sesuai prosedur keselamatan laboratorium.
