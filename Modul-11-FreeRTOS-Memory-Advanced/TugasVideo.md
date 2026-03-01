# Tugas Video Modul 11: FreeRTOS Memory Management & Advanced Features

## Praktikum Sistem Embedded

---

## Informasi Umum

| Item | Detail |
|------|--------|
| **Modul** | 11 — FreeRTOS Memory & Advanced |
| **Tipe Tugas** | Video Laporan Praktikum |
| **Durasi Video** | 20–35 menit |
| **Deadline** | 1 minggu setelah praktikum |
| **Format** | MP4, resolusi minimal 720p |
| **Upload** | Google Drive / YouTube (unlisted) — submit link |

---

## Deskripsi

Buat video laporan yang mencakup **penjelasan materi**, **demonstrasi seluruh 12 percobaan**, dan **demo project** SkyWatch-2. Video harus menunjukkan pemahaman memory management, buffer, dan system monitoring.

---

## Ketentuan Teknis

1. **Screen recording** dengan **webcam** terlihat (picture-in-picture, minimal 15% layar).
2. **Narasi suara** menjelaskan setiap bagian — bukan hanya menunjukkan kode/output.
3. Tunjukkan **kode sumber**, **proses build**, **serial monitor output**, dan **hardware** (jika LED).
4. Setiap percobaan harus ditunjukkan **hasilnya berjalan** (bukan hanya kode statis).
5. Wajib ada **analisis** — tidak cukup hanya menunjukkan "berhasil".

---

## Struktur Video

### 1. Pembukaan (1–2 menit)
- Perkenalan: nama, NIM, kelas
- Overview Modul 11: memory management dan advanced features
- Sebutkan platform yang digunakan (ESP32/STM32)

### 2. Penjelasan Materi (3–5 menit)
- Heap management: allocator types (heap_1 to heap_5), fragmentation
- Static vs dynamic allocation: trade-offs
- Memory pool pattern: fixed-size blocks via queue
- Stream buffer vs message buffer vs queue
- Critical section vs mutex: when to use which

### 3. Demonstrasi Percobaan (10–18 menit)

| No | Percobaan | Fokus Demonstrasi |
|----|-----------|-------------------|
| 01 | Heap Monitor | Free, min-ever, largest block, multi-region (ESP32) |
| 02 | Memory Allocation | Fragmentation pattern: sequential vs alternating free |
| 03 | Stack Overflow | HWM monitoring, overflow hook trigger |
| 04 | Static Allocation | xTaskCreateStatic, zero heap impact proof |
| 05 | Memory Pool | Pool alloc/free benchmark vs pvPortMalloc |
| 06 | Stream Buffer | Variable-length byte transfer, trigger level behavior |
| 07 | Message Buffer | Multiple message types, atomic receive, type decoding |
| 08 | Critical Section | No protection → corruption, critical section → fixed |
| 09 | Heap Fragmentation | Hole pattern, failed large alloc, coalescence demo |
| 10 | PSRAM/Memory Map | Region enumeration, speed benchmark (ESP32) / address analysis (STM32) |
| 11 | Memory Leak Detection | Leak rate trending, consecutive drop alert |
| 12 | System Dashboard | Comprehensive output: heap + task + queue + timer stats |

### 4. Demo Project SkyWatch-2 (4–6 menit)
- Arsitektur: task diagram dengan buffer/pool/queue connections
- Heap health dashboard dengan color classification
- Stack guardian warnings
- Memory pool telemetry flow
- Leak detection demo: leaky task → alarm → time-to-exhaustion
- Flight dashboard output
- Tunjukkan **minimal 8 dari 12 fitur** berjalan

### 5. Penutup (1–2 menit)
- Kesimpulan: strategi memory management untuk embedded reliable
- Kapan static vs dynamic vs pool
- Tantangan dan solusi

---

## Penilaian

| Komponen | Bobot | Keterangan |
|----------|-------|------------|
| Penjelasan Materi | 15% | Akurasi konsep heap, pool, buffer, critical section |
| Demonstrasi Percobaan | 40% | Semua 12 percobaan ditunjukkan dan dianalisis |
| Demo Project | 25% | Fitur berjalan, self-diagnostic terlihat |
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
- [ ] Materi: heap management, pool, buffer, critical section
- [ ] Percobaan 01–12: kode + output + analisis masing-masing
- [ ] Project SkyWatch-2: minimal 8 fitur didemonstrasikan
- [ ] Penutup: kesimpulan dan refleksi
- [ ] Format MP4, minimal 720p
- [ ] Link accessible (Google Drive / YouTube Unlisted)

---

*Tugas Video Modul 11 — FreeRTOS Memory & Advanced | Praktikum Sistem Embedded | 2025/2026*
