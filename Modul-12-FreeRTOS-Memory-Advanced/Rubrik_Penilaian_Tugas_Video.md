# Rubrik Penilaian Tugas Video — Modul 12: FreeRTOS Memory Management

## Format Video
- **Durasi**: 5-10 menit
- **Resolusi**: Minimum 720p
- **Audio**: Narasi jelas dalam Bahasa Indonesia
- **Konten**: Demo praktikum + penjelasan konsep

---

## Komponen Penilaian

### 1. Pendahuluan (10%)

| Skor | Kriteria |
|------|----------|
| 9-10 | Memperkenalkan diri, menyebutkan modul, menjelaskan tujuan praktikum dengan jelas |
| 6-8 | Perkenalan singkat, tujuan disebutkan |
| 3-5 | Langsung demo tanpa pendahuluan |
| 0-2 | Tidak ada pendahuluan |

### 2. Demo Hardware & Setup (15%)

| Skor | Kriteria |
|------|----------|
| 13-15 | Menunjukkan board (ESP32/STM32), koneksi serial, IDE/PlatformIO, compile sukses |
| 9-12 | Menunjukkan board dan serial monitor |
| 4-8 | Hanya menunjukkan serial output |
| 0-3 | Tidak menunjukkan setup |

### 3. Demo Program Berjalan (30%)

| Skor | Kriteria |
|------|----------|
| 25-30 | Minimal 4 percobaan didemonstrasikan, output serial jelas terlihat, LED berfungsi |
| 18-24 | 3 percobaan, output terlihat |
| 10-17 | 2 percobaan |
| 0-9 | 1 atau tidak ada demo |

**Percobaan yang WAJIB didemonstrasikan** (minimal 2 dari 4):
- Heap Monitor (01) — showing heap stats
- Stack Overflow Detect (03) — triggering overflow
- Critical Section (08) — corruption vs protected
- System Dashboard (12) — capstone

### 4. Penjelasan Konsep (25%)

| Skor | Kriteria |
|------|----------|
| 21-25 | Menjelaskan: heap schemes, stack overflow, fragmentasi, critical section dengan benar dan mendalam |
| 15-20 | Penjelasan benar tapi kurang mendalam |
| 8-14 | Penjelasan sebagian benar |
| 0-7 | Penjelasan salah atau tidak ada |

**Konsep yang harus dijelaskan**:
- Perbedaan heap_1 vs heap_4
- Mengapa stack overflow berbahaya
- Apa itu fragmentasi dan cara mitigasi
- Perbedaan critical section vs mutex

### 5. Demo Python Script (10%)

| Skor | Kriteria |
|------|----------|
| 9-10 | Menjalankan Python script, menunjukkan plot real-time, menjelaskan data |
| 6-8 | Menjalankan script, output terlihat |
| 3-5 | Hanya menunjukkan script tanpa run |
| 0-2 | Tidak ada demo Python |

### 6. Kualitas Produksi (10%)

| Skor | Kriteria |
|------|----------|
| 9-10 | Audio jelas, video stabil, editing rapi, transisi smooth |
| 6-8 | Audio cukup jelas, video stabil |
| 3-5 | Audio kurang jelas atau video goyang |
| 0-2 | Sulit dilihat/didengar |

---

## Bonus (+5 per item, maks +10)
- Demo di kedua platform (ESP32 + STM32)
- Menunjukkan debugging proses (menjelaskan error dan cara fix)
- Membuat diagram/animasi konsep memory

## Penalti
- Video > 15 menit: **-5**
- Video < 3 menit: **-10**
- Tidak ada narasi/penjelasan: **-15**
- Plagiarisme: **-50**

---

## Deadline & Submission
- Upload ke YouTube (unlisted) atau Google Drive
- Submit link di LMS sebelum deadline
- File pendukung: source code + Python script
