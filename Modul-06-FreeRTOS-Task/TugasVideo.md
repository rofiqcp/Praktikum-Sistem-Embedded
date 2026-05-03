# Tugas Video Modul 08: FreeRTOS Task Management

## Praktikum Sistem Embedded

---

## Informasi Umum

| Item | Detail |
|------|--------|
| **Modul** | 08 — FreeRTOS Task Management |
| **Tipe Tugas** | Video Laporan Praktikum |
| **Durasi Video** | 20–35 menit |
| **Deadline** | 1 minggu setelah praktikum |
| **Format** | MP4, resolusi minimal 720p |
| **Upload** | Google Drive / YouTube (unlisted) — submit link |

---

## Deskripsi

Buat video laporan yang mencakup **penjelasan materi**, **demonstrasi seluruh 12 percobaan**, dan **demo project** AquaGuard. Video harus menunjukkan pemahaman konsep FreeRTOS task management secara menyeluruh.

---

## Ketentuan Teknis

1. **Screen recording** dengan **webcam** terlihat (picture-in-picture, minimal 15% layar).
2. **Narasi suara** menjelaskan setiap bagian — bukan hanya menunjukkan kode/output.
3. Tunjukkan **kode sumber**, **proses build**, **serial monitor output**, dan **hardware** (jika menggunakan LED/button fisik).
4. Setiap percobaan harus ditunjukkan **hasilnya berjalan** (bukan hanya kode statis).
5. Wajib ada **analisis** — tidak cukup hanya menunjukkan "berhasil".

---

## Struktur Video

### 1. Pembukaan (1–2 menit)
- Perkenalan: nama, NIM, kelas
- Overview Modul 08: apa itu FreeRTOS task management
- Sebutkan platform yang digunakan (ESP32/STM32)

### 2. Penjelasan Materi (3–5 menit)
- Konsep RTOS vs bare-metal (super loop)
- Task state diagram: Running → Ready → Blocked → Suspended
- Scheduler: preemptive vs cooperative, time slicing
- Stack dan heap management
- Perbedaan implementasi ESP32 (dual-core) vs STM32 (single-core)

### 3. Demonstrasi Percobaan (10–18 menit)

Untuk **setiap percobaan**, tunjukkan:
- Tujuan percobaan (1 kalimat)
- Bagian kode penting (highlight, jangan scroll seluruh file)
- Output serial monitor atau perilaku LED
- Analisis singkat hasil pengamatan

| No | Percobaan | Fokus Demonstrasi |
|----|-----------|-------------------|
| 01 | Task Create Basic | xTaskCreate, task list output, stack info |
| 02 | Task Priority | CPU distribution berubah setelah priority swap |
| 03 | Task Delay Periodic | Bandingkan drift: vTaskDelay vs vTaskDelayUntil |
| 04 | Task Suspend Resume | Button → suspend LED → button → resume |
| 05 | Task Delete | Heap sebelum/sesudah create-delete cycle |
| 06 | Task Stack Monitor | HWM warnings, stack overflow hook trigger |
| 07 | Core Affinity / Priority Inversion | Core ID (ESP32) atau timeline inversion (STM32) |
| 08 | Idle Hook | CPU usage bar/percentage di berbagai load level |
| 09 | Watchdog | Task hang → timeout → system reset → recovery |
| 10 | Task Communication Unsafe | Corruption counter meningkat seiring waktu |
| 11 | Scheduler Info | Dashboard tabel task lengkap |
| 12 | Cooperative Scheduling | Fairness comparison: preemptive vs cooperative vs hogging |

### 4. Demo Project AquaGuard (4–6 menit)
- Arsitektur task: gambar/diagram 6 task dan interaksinya
- Demo fitur utama berjalan: sensor reading, heater control, alarm trigger
- Dashboard output di serial monitor
- Maintenance mode via button
- Watchdog recovery demo
- Scheduling mode switch (A/B/C)
- Tunjukkan **minimal 8 dari 12 fitur** berjalan

### 5. Penutup (1–2 menit)
- Kesimpulan: apa yang dipelajari dari modul ini
- Tantangan yang dihadapi dan solusinya
- Kesiapan menuju Modul 09 (Queue & Semaphore sebagai solusi Percobaan 10)

---

## Penilaian

| Komponen | Bobot | Keterangan |
|----------|-------|------------|
| Penjelasan Materi | 15% | Akurasi konsep, kejelasan penjelasan |
| Demonstrasi Percobaan | 40% | Semua 12 percobaan ditunjukkan dan dianalisis |
| Demo Project | 25% | Fitur berjalan, integrasi antar-task terlihat |
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
- [ ] Materi: RTOS concepts, task states, scheduler types
- [ ] Percobaan 01–12: kode + output + analisis masing-masing
- [ ] Project AquaGuard: minimal 8 fitur didemonstrasikan
- [ ] Penutup: kesimpulan dan refleksi
- [ ] Format MP4, minimal 720p
- [ ] Link accessible (Google Drive: "Anyone with link" / YouTube: Unlisted)

---

*Tugas Video Modul 08 — FreeRTOS Task | Praktikum Sistem Embedded | 2025/2026*
