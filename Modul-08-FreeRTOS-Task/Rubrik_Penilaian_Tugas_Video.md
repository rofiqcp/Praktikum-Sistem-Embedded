# Rubrik Penilaian Tugas Video
## Modul 09: FreeRTOS Task Management

### 📋 Informasi Umum
- **Nama Modul**: FreeRTOS Task Management
- **Jenis Tugas**: Video Demonstrasi Praktikum
- **Total Nilai**: 100 poin
- **Passing Grade**: 70 poin
- **Durasi Video**: 5-10 menit

---

## 🎯 Tujuan Tugas Video

Video demonstrasi bertujuan untuk:
1. Membuktikan pemahaman mahasiswa terhadap materi FreeRTOS Task Management
2. Mendemonstrasikan kemampuan praktis dalam implementasi
3. Melatih kemampuan komunikasi teknis
4. Mendokumentasikan hasil praktikum

---

## 📊 Kriteria Penilaian Detail

### 1. Kualitas Teknis Video (20 poin)

| Kriteria | Excellent (18-20) | Good (14-17) | Satisfactory (10-13) | Needs Work (5-9) | Poor (0-4) |
|----------|-------------------|--------------|----------------------|------------------|------------|
| **Resolusi** | Full HD (1080p) atau lebih | HD (720p) | SD (480p) | Low resolution | Tidak dapat ditonton |
| **Audio** | Jernih, tidak ada noise, volume konsisten | Sedikit noise tapi jelas | Audio kurang jernih | Sulit didengar | Tidak ada audio/rusak |
| **Stabilitas** | Stabil, tidak goyang | Sedikit goyang tapi acceptable | Cukup goyang | Sangat goyang | Unwatchable |
| **Pencahayaan** | Terang, objek terlihat jelas | Cukup terang | Agak gelap | Gelap | Tidak terlihat |

#### Checklist Kualitas Teknis:
- [ ] Video minimal 720p
- [ ] Audio terdengar jelas
- [ ] Tidak ada background noise berlebihan
- [ ] Pencahayaan cukup untuk melihat hardware
- [ ] Frame rate minimal 24fps
- [ ] File format: MP4/MKV/MOV
- [ ] Ukuran file reasonable (<500MB untuk 10 menit)

---

### 2. Struktur dan Organisasi (15 poin)

| Kriteria | Excellent (14-15) | Good (11-13) | Satisfactory (8-10) | Needs Work (4-7) | Poor (0-3) |
|----------|-------------------|--------------|----------------------|------------------|------------|
| **Opening** | Intro profesional, perkenalan lengkap | Intro baik | Intro minimal | Langsung ke demo | Tidak ada intro |
| **Flow** | Alur logis, transisi smooth | Alur baik | Alur cukup | Alur membingungkan | Tidak terstruktur |
| **Closing** | Summary lengkap, kesimpulan jelas | Summary baik | Summary minimal | Tiba-tiba selesai | Tidak ada closing |
| **Timing** | 5-10 menit tepat, pace optimal | Sedikit lebih/kurang | Pace kurang optimal | Terlalu cepat/lambat | Durasi tidak sesuai |

#### Struktur Video yang Diharapkan:
```
0:00 - 0:30  : Opening & Perkenalan
0:30 - 1:30  : Overview sistem dan tujuan
1:30 - 3:00  : Demo hardware setup
3:00 - 6:00  : Demo program berjalan (task by task)
6:00 - 8:00  : Penjelasan kode penting
8:00 - 9:00  : Troubleshooting/challenges
9:00 - 10:00 : Kesimpulan dan penutup
```

---

### 3. Konten Teknis (30 poin)

| Kriteria | Excellent (27-30) | Good (21-26) | Satisfactory (15-20) | Needs Work (8-14) | Poor (0-7) |
|----------|-------------------|--------------|----------------------|------------------|------------|
| **Konsep FreeRTOS** | Penjelasan mendalam, akurat | Penjelasan baik | Penjelasan dasar | Penjelasan kurang | Tidak menjelaskan |
| **Demo Task Creation** | Semua task didemonstrasikan | Mayoritas task | Beberapa task | Minimal task | Tidak demo |
| **Task State Explanation** | Menjelaskan semua state dengan contoh | Menjelaskan dengan baik | Penjelasan dasar | Kurang jelas | Tidak menjelaskan |
| **Priority Demo** | Demo priority scheduling jelas | Demo cukup | Demo minimal | Demo tidak jelas | Tidak demo |
| **Stack Analysis** | Analisis HWM, memory usage | Menunjukkan HWM | Menyebut stack | Tidak detail | Tidak ada |

#### Checklist Konten Teknis:
- [ ] Menjelaskan apa itu FreeRTOS
- [ ] Menjelaskan konsep task
- [ ] Demo xTaskCreate/xTaskCreatePinnedToCore
- [ ] Demo vTaskDelay vs vTaskDelayUntil
- [ ] Demo task priority
- [ ] Demo task suspend/resume (jika ada)
- [ ] Demo task delete (jika ada)
- [ ] Menunjukkan stack high water mark
- [ ] Menjelaskan scheduler
- [ ] Menunjukkan output serial monitor

---

### 4. Demonstrasi Hardware (20 poin)

| Kriteria | Excellent (18-20) | Good (14-17) | Satisfactory (10-13) | Needs Work (5-9) | Poor (0-4) |
|----------|-------------------|--------------|----------------------|------------------|------------|
| **Setup Visibility** | Hardware terlihat jelas, labeled | Terlihat dengan baik | Cukup terlihat | Kurang jelas | Tidak terlihat |
| **Wiring Explanation** | Menjelaskan setiap koneksi | Mayoritas dijelaskan | Beberapa dijelaskan | Minimal penjelasan | Tidak dijelaskan |
| **Working Demo** | Semua fungsi berjalan sempurna | Mayoritas berfungsi | Beberapa berfungsi | Minimal berfungsi | Tidak berfungsi |
| **LED/Output Demo** | LED response sesuai task | Cukup sesuai | Kurang sinkron | Tidak konsisten | Tidak berfungsi |

#### Checklist Demo Hardware:
- [ ] Menunjukkan board STM32/ESP32
- [ ] Menunjukkan koneksi LED
- [ ] Menunjukkan sensor (jika ada)
- [ ] Demo LED blink dengan timing berbeda
- [ ] Demo response terhadap input
- [ ] Menunjukkan serial monitor output
- [ ] Close-up yang jelas untuk detail

---

### 5. Kemampuan Presentasi (15 poin)

| Kriteria | Excellent (14-15) | Good (11-13) | Satisfactory (8-10) | Needs Work (4-7) | Poor (0-3) |
|----------|-------------------|--------------|----------------------|------------------|------------|
| **Clarity** | Penjelasan sangat jelas | Jelas | Cukup jelas | Kurang jelas | Tidak jelas |
| **Confidence** | Percaya diri, lancar | Cukup percaya diri | Agak nervous | Nervous | Sangat nervous |
| **Technical Language** | Penggunaan istilah tepat | Mayoritas tepat | Beberapa salah | Banyak salah | Istilah tidak tepat |
| **Engagement** | Engaging, interaktif | Cukup menarik | Monoton | Membosankan | Sangat membosankan |

#### Tips Presentasi:
- Berbicara dengan jelas dan tidak terlalu cepat
- Gunakan pointer/highlight untuk menunjukkan bagian penting
- Jangan membaca script secara verbatim
- Tunjukkan antusiasme terhadap materi
- Siapkan apa yang akan dikatakan sebelum merekam

---

## 📝 Form Penilaian

```
Nama Mahasiswa: _______________________
NIM: _____________
Tanggal Submit: ____________
Link Video: _________________________________

PENILAIAN:
1. Kualitas Teknis Video   : ____ / 20
2. Struktur dan Organisasi : ____ / 15
3. Konten Teknis           : ____ / 30
4. Demonstrasi Hardware    : ____ / 20
5. Kemampuan Presentasi    : ____ / 15

TOTAL                      : ____ / 100

Grade:
[ ] A  (90-100) - Excellent
[ ] AB (85-89)  - Very Good
[ ] B  (80-84)  - Good
[ ] BC (75-79)  - Above Average
[ ] C  (70-74)  - Average
[ ] D  (60-69)  - Below Average
[ ] E  (<60)    - Poor

Catatan Penilai:
_________________________________________________
_________________________________________________
_________________________________________________

Kekuatan:
_________________________________________________

Area Perbaikan:
_________________________________________________

Tanda Tangan Penilai: _____________
Tanggal: _____________
```

---

## 📋 Checklist Sebelum Submit

### Konten Wajib:
- [ ] Perkenalan diri (nama, NIM)
- [ ] Judul praktikum disebutkan
- [ ] Tujuan praktikum dijelaskan
- [ ] Hardware setup ditampilkan
- [ ] Minimal 3 program didemonstrasikan
- [ ] Penjelasan kode penting
- [ ] Output/hasil ditampilkan
- [ ] Kesimpulan di akhir

### Teknis Video:
- [ ] Durasi 5-10 menit
- [ ] Format file sesuai (MP4/MKV/MOV)
- [ ] Audio jelas terdengar
- [ ] Video tidak blur
- [ ] Nama file sesuai format: `NIM_Nama_Modul09.mp4`

### Upload:
- [ ] Upload ke platform yang ditentukan (Google Drive/YouTube)
- [ ] Set permission agar bisa diakses penilai
- [ ] Test link sebelum submit

---

## ⚠️ Ketentuan Khusus

### Pengurangan Nilai
| Pelanggaran | Pengurangan |
|-------------|-------------|
| Durasi < 5 menit | -10 |
| Durasi > 10 menit | -5 |
| Audio tidak jelas | -10 |
| Video blur/tidak terlihat | -15 |
| Tidak ada demo hardware | -20 |
| Tidak menjelaskan konsep | -15 |
| Plagiarisme/copy video orang lain | -100 (nilai 0) |
| Keterlambatan per hari | -10% |
| Format file salah | -5 |
| Link tidak bisa diakses | Tidak dinilai sampai diperbaiki |

### Bonus
| Kriteria | Bonus |
|----------|-------|
| Video editing profesional (intro, transition) | +5 |
| Subtitle/caption | +3 |
| Diagram/animasi penjelasan | +5 |
| Demo troubleshooting real problem | +5 |
| Perbandingan dengan non-RTOS | +3 |

**Maksimal bonus: 10 poin**

---

## 🎥 Contoh Struktur Script

```
[OPENING - 30 detik]
"Assalamualaikum/Halo, saya [Nama], NIM [NIM].
Pada video ini saya akan mendemonstrasikan praktikum
Modul 09: FreeRTOS Task Management."

[OVERVIEW - 1 menit]
"Tujuan praktikum ini adalah memahami konsep task
dalam FreeRTOS, cara membuat task, mengatur prioritas,
dan mengelola lifecycle task."

[HARDWARE SETUP - 1.5 menit]
"Berikut adalah setup hardware yang digunakan:
- Board: [STM32/ESP32]
- LED terhubung ke pin [X]
- [Komponen lain]
Mari kita lihat wiring diagram..."

[DEMO PROGRAM - 3 menit]
"Program pertama: Basic Multi-Task...
[Tunjukkan kode, jelaskan, run, tunjukkan hasil]
Program kedua: Task Priority...
[Repeat]"

[PENJELASAN KODE - 2 menit]
"Bagian penting dari kode ini adalah...
xTaskCreate() berfungsi untuk...
vTaskDelay() berbeda dengan delay biasa karena..."

[KESIMPULAN - 1 menit]
"Dari praktikum ini saya belajar bahwa...
Tantangan yang dihadapi adalah...
Terima kasih telah menonton."

[CLOSING - 30 detik]
"Demikian video demonstrasi praktikum Modul 09.
Wassalamualaikum/Terima kasih."
```

---

## 📞 FAQ

**Q: Apakah harus wajah terlihat di video?**
A: Tidak wajib, yang penting suara jelas dan hardware terlihat. Tapi showing face di intro/closing adalah plus.

**Q: Boleh edit video?**
A: Sangat dianjurkan! Video yang well-edited menunjukkan effort dan profesionalisme.

**Q: Bagaimana jika program error saat recording?**
A: Tidak masalah, tunjukkan proses debugging-nya. Ini bisa jadi nilai plus.

**Q: Bahasa apa yang digunakan?**
A: Bahasa Indonesia. Istilah teknis boleh dalam Bahasa Inggris.

**Q: Platform upload apa?**
A: Google Drive (set sharing: anyone with link) atau YouTube (unlisted).

---

*Rubrik ini berlaku untuk Modul 09: FreeRTOS Task Management*
*Terakhir diupdate: 2024*
