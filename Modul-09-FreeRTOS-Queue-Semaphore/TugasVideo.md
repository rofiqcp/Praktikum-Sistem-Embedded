# Tugas Video Modul 09: FreeRTOS Queue dan Semaphore

## Praktikum Sistem Embedded

---

## Informasi Umum

| Item | Detail |
|------|--------|
| **Modul** | 09 — FreeRTOS Queue & Semaphore |
| **Tipe Tugas** | Video Laporan Praktikum |
| **Durasi Video** | 20–35 menit |
| **Deadline** | 1 minggu setelah praktikum |
| **Format** | MP4, resolusi minimal 720p |
| **Upload** | Google Drive / YouTube (unlisted) — submit link |

---

## Deskripsi

Buat video laporan yang mencakup **penjelasan materi**, **demonstrasi seluruh 12 percobaan**, dan **demo project** SmartPark. Video harus menunjukkan pemahaman queue, semaphore, mutex, dan design pattern.

---

## Ketentuan Teknis

1. **Screen recording** dengan **webcam** terlihat (picture-in-picture, minimal 15% layar).
2. **Narasi suara** menjelaskan setiap bagian — bukan hanya menunjukkan kode/output.
3. Tunjukkan **kode sumber**, **proses build**, **serial monitor output**, dan **hardware** (jika LED/button).
4. Setiap percobaan harus ditunjukkan **hasilnya berjalan** (bukan hanya kode statis).
5. Wajib ada **analisis** — tidak cukup hanya menunjukkan "berhasil".

---

## Struktur Video

### 1. Pembukaan (1–2 menit)
- Perkenalan: nama, NIM, kelas
- Overview Modul 09: inter-task communication dan sinkronisasi
- Sebutkan platform yang digunakan (ESP32/STM32)

### 2. Penjelasan Materi (3–5 menit)
- Queue: konsep FIFO, copy-by-value, blocking read/write
- Binary vs Counting Semaphore: signaling vs resource counting
- Mutex vs Semaphore: ownership, priority inheritance
- Recursive Mutex: nested locking
- Design pattern: producer-consumer, reader-writer

### 3. Demonstrasi Percobaan (10–18 menit)

| No | Percobaan | Fokus Demonstrasi |
|----|-----------|-------------------|
| 01 | Queue Basic | Send/receive count, queue fill level |
| 02 | Queue Struct | Multiple sensor → satu processor, struct identification |
| 03 | Queue Multiple | Command-status pipeline dua arah |
| 04 | Queue ISR | Button ISR → queue → LED toggle, latency |
| 05 | Queue Set | Multiplexed receive dari 2 queue |
| 06 | Binary Semaphore | ISR give → task take, missed event analysis |
| 07 | Counting Semaphore | 5 worker, 3 resource, waiting behavior |
| 08 | Mutex Shared Resource | Tanpa mutex → corruption, dengan mutex → correct |
| 09 | Priority Inversion | Timeline inversion, priority inheritance fix |
| 10 | Recursive Mutex | Nesting level naik/turun, multi-task contention |
| 11 | Producer-Consumer | Bounded buffer fill level, throughput stats |
| 12 | Reader-Writer | Concurrent readers, exclusive writer, starvation check |

### 4. Demo Project SmartPark (4–6 menit)
- Arsitektur sistem: diagram task dan queue/semaphore interconnection
- Vehicle entry flow: ISR → queue → controller → slot allocation
- Slot management: counting semaphore occupancy display
- Revenue protection: race condition demo → mutex fix
- Pipeline monitoring: queue fill level, throughput stats
- Tunjukkan **minimal 8 dari 12 fitur** berjalan

### 5. Penutup (1–2 menit)
- Kesimpulan: kapan gunakan queue vs semaphore vs mutex
- Perbandingan dengan shared variable (Modul 08 P10)
- Tantangan dan solusi

---

## Penilaian

| Komponen | Bobot | Keterangan |
|----------|-------|------------|
| Penjelasan Materi | 15% | Akurasi konsep queue/semaphore/mutex |
| Demonstrasi Percobaan | 40% | Semua 12 percobaan ditunjukkan dan dianalisis |
| Demo Project | 25% | Fitur berjalan, integrasi terlihat |
| Kualitas Video | 10% | Audio jelas, visual terbaca, webcam terlihat |
| Analisis & Pemahaman | 10% | Mampu menjelaskan "mengapa", bukan hanya "apa" |

---

## Penalti

| Pelanggaran | Pengurangan |
|-------------|-------------|
| Webcam tidak terlihat | −10% |
| Tidak ada narasi suara | −15% |
| Percobaan tidak dijalankan (hanya tampil kode) | −5% per percobaan |
| Durasi < 15 menit | −10% |
| Durasi > 40 menit | −5% |
| Resolusi < 720p / audio tidak jelas | −10% |
| Terlambat submit | −10% per hari (maks 3 hari) |

---

## Checklist Sebelum Submit

- [ ] Video berdurasi 20–35 menit
- [ ] Webcam terlihat sepanjang video
- [ ] Narasi suara jelas di seluruh video
- [ ] Pembukaan: nama, NIM, kelas, platform
- [ ] Materi: queue, semaphore, mutex, design patterns
- [ ] Percobaan 01–12: kode + output + analisis masing-masing
- [ ] Project SmartPark: minimal 8 fitur didemonstrasikan
- [ ] Penutup: kesimpulan dan refleksi
- [ ] Format MP4, minimal 720p
- [ ] Link accessible (Google Drive / YouTube Unlisted)

---

*Tugas Video Modul 09 — FreeRTOS Queue & Semaphore | Praktikum Sistem Embedded | 2025/2026*
