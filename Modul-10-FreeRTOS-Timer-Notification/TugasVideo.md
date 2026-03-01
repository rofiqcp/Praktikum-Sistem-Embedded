# Tugas Video Modul 10: FreeRTOS Software Timer dan Task Notification

## Praktikum Sistem Embedded

---

## Informasi Umum

| Item | Detail |
|------|--------|
| **Modul** | 10 — FreeRTOS Timer & Notification |
| **Tipe Tugas** | Video Laporan Praktikum |
| **Durasi Video** | 20–35 menit |
| **Deadline** | 1 minggu setelah praktikum |
| **Format** | MP4, resolusi minimal 720p |
| **Upload** | Google Drive / YouTube (unlisted) — submit link |

---

## Deskripsi

Buat video laporan yang mencakup **penjelasan materi**, **demonstrasi seluruh 12 percobaan**, dan **demo project** Mesin Cuci Otomatis. Video harus menunjukkan pemahaman software timer, task notification, dan event group.

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
- Overview Modul 10: timer, notification, event group
- Sebutkan platform yang digunakan (ESP32/STM32)

### 2. Penjelasan Materi (3–5 menit)
- Software Timer: one-shot vs auto-reload, daemon task, callback context
- Task Notification: give/take, set value, counting — keunggulan vs semaphore
- Event Group: bit manipulation, AND/OR logic, barrier synchronization
- Perbandingan overhead: timer vs notification vs semaphore

### 3. Demonstrasi Percobaan (10–18 menit)

| No | Percobaan | Fokus Demonstrasi |
|----|-----------|-------------------|
| 01 | Timer Basic | One-shot vs auto-reload, callback count |
| 02 | Timer Period Change | Button → speed cycling, dynamic period |
| 03 | Timer ID Multiple | 3 timer satu callback, fire count ratio |
| 04 | Timer Debounce | Raw ISR count vs debounced count |
| 05 | Timeout Monitor | Heartbeat reset, simulated hang → timeout |
| 06 | Notification Basic | Latency comparison notification vs semaphore |
| 07 | Notification Value | Command via 32-bit value, overwrite modes |
| 08 | Notification Counting | Backlog tracking, pdTRUE vs pdFALSE |
| 09 | Notification ISR | Quantitative latency benchmark |
| 10 | Event Group Basic | AND logic (all sensors ready), OR logic (alert) |
| 11 | Event Group Sync | Barrier pattern, sync timing |
| 12 | Benchmark | 1000-iteration comparison table |

### 4. Demo Project Mesin Cuci Otomatis (4–6 menit)
- Arsitektur sistem: state machine + timer + notification flow
- Demo wash cycle states: idle → fill → wash → rinse → spin → done
- Timer-based transitions antar fase
- Notification dari sensor (water level, door status)
- Event group synchronization (semua subsystem ready)
- Tunjukkan **minimal 8 dari 12 fitur** berjalan

### 5. Penutup (1–2 menit)
- Kesimpulan: kapan gunakan timer vs notification vs event group
- Perbandingan performa dengan data benchmark
- Tantangan dan solusi

---

## Penilaian

| Komponen | Bobot | Keterangan |
|----------|-------|------------|
| Penjelasan Materi | 15% | Akurasi konsep timer, notification, event group |
| Demonstrasi Percobaan | 40% | Semua 12 percobaan ditunjukkan dan dianalisis |
| Demo Project | 25% | Fitur berjalan, state machine terlihat |
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
- [ ] Materi: timer types, notification modes, event group operations
- [ ] Percobaan 01–12: kode + output + analisis masing-masing
- [ ] Project Mesin Cuci Otomatis: minimal 8 fitur didemonstrasikan
- [ ] Penutup: kesimpulan dan refleksi
- [ ] Format MP4, minimal 720p
- [ ] Link accessible (Google Drive / YouTube Unlisted)

---

*Tugas Video Modul 10 — FreeRTOS Timer & Notification | Praktikum Sistem Embedded | 2025/2026*
