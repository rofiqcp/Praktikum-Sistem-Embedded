# Rubrik Penilaian Tugas Video Modul 10
## FreeRTOS Queue dan Semaphore

### 📋 Informasi Umum
- **Durasi Video**: 5-10 menit
- **Total Nilai**: 100 poin
- **Format**: MP4/WebM, resolusi minimal 720p
- **Platform Upload**: YouTube (unlisted) atau Google Drive

---

## 📊 Kriteria Penilaian

### 1. Kualitas Teknis Video (20 poin)

| Aspek | Excellent (18-20) | Good (14-17) | Adequate (10-13) | Poor (0-9) |
|-------|-------------------|--------------|------------------|------------|
| **Resolusi & Clarity** | Video 1080p, jernih, screen recording tajam | 720p, cukup jelas | 480p, agak blur | Resolusi rendah, sulit dibaca |
| **Audio** | Suara jelas, tanpa noise, volume konsisten | Suara jelas dengan sedikit noise | Suara kurang jelas | Audio tidak terdengar/sangat buruk |
| **Editing** | Transisi smooth, highlight penting, zooming tepat | Edit cukup baik | Minimal editing | Tidak ada editing/raw recording |

---

### 2. Struktur Presentasi (15 poin)

| Aspek | Excellent (13-15) | Good (10-12) | Adequate (7-9) | Poor (0-6) |
|-------|-------------------|--------------|----------------|------------|
| **Pembukaan** | Intro jelas: nama, NIM, judul, tujuan | Intro lengkap tapi singkat | Intro minimal | Tidak ada intro |
| **Alur Logis** | Alur jelas: teori → kode → demo → kesimpulan | Alur terstruktur | Alur kurang teratur | Tidak ada struktur |
| **Penutup** | Kesimpulan lengkap, learning points, Q&A preparation | Kesimpulan ada | Kesimpulan singkat | Tidak ada kesimpulan |
| **Timing** | Pas 5-10 menit, pace tepat | Sedikit terlalu cepat/lambat | Terlalu cepat atau lambat | Di luar range waktu |

---

### 3. Konten Teknis Queue (30 poin)

| Aspek | Excellent (27-30) | Good (21-26) | Adequate (15-20) | Poor (0-14) |
|-------|-------------------|--------------|------------------|------------|
| **Penjelasan Queue** | Konsep FIFO jelas, analogi bagus, use case tepat | Penjelasan cukup jelas | Penjelasan basic | Tidak menjelaskan/salah |
| **xQueueCreate** | Demonstrasi parameter, memory allocation, return check | Demonstrasi cukup | Demonstrasi minimal | Tidak ada demonstrasi |
| **Send/Receive** | xQueueSend, xQueueReceive dengan timeout, blocking behavior | Demonstrasi dasar send/receive | Kurang lengkap | Tidak berfungsi |
| **ISR Usage** | xQueueSendFromISR, yield handling, best practices | FromISR dijelaskan | Mention singkat | Tidak dijelaskan |
| **Struct Transfer** | Demonstrasi queue dengan struct, sizing | Struct basic | Type primitif saja | Tidak ada transfer data |

---

### 4. Konten Teknis Semaphore/Mutex (20 poin)

| Aspek | Excellent (18-20) | Good (14-17) | Adequate (10-13) | Poor (0-9) |
|-------|-------------------|--------------|------------------|------------|
| **Binary Semaphore** | ISR-to-Task signaling demo, give/take flow | Demonstrasi berfungsi | Penjelasan saja | Tidak dijelaskan |
| **Counting Semaphore** | Resource pool demo, count tracking | Demonstrasi cukup | Mention singkat | Tidak dijelaskan |
| **Mutex** | Priority inheritance concept, ownership demo | Demonstrasi berfungsi | Penjelasan basic | Tidak dijelaskan |
| **Perbedaan** | Jelas bedanya kapan pakai masing-masing | Penjelasan cukup | Kurang jelas | Tidak ada perbandingan |

---

### 5. Demonstrasi Hardware (15 poin)

| Aspek | Excellent (13-15) | Good (10-12) | Adequate (7-9) | Poor (0-6) |
|-------|-------------------|--------------|----------------|------------|
| **Hardware Setup** | Close-up wiring, komponen terlihat jelas | Setup terlihat | Kurang jelas | Tidak terlihat |
| **Live Demo** | Real-time demo, button press, LED response terlihat | Demo berjalan | Demo dengan masalah | Demo gagal |
| **Serial Output** | Serial monitor terlihat, output dijelaskan | Serial visible | Sesekali terlihat | Tidak ada serial output |
| **Multi-Platform** | STM32 dan ESP32 ditunjukkan | Salah satu platform | Demo minimal | Tidak ada demo hardware |

---

## 📝 Checklist Konten Video

### Wajib Ditunjukkan:
- [ ] Penjelasan konsep Queue (FIFO, blocking, timeout)
- [ ] Demo xQueueCreate() dengan parameter yang tepat
- [ ] Demo xQueueSend() dan xQueueReceive()
- [ ] Penjelasan Binary vs Counting Semaphore
- [ ] Demo Mutex untuk proteksi resource
- [ ] Serial output yang menunjukkan flow data
- [ ] Hardware demo (LED/button response)

### Bonus Points:
- [ ] Demonstrasi xQueueSendFromISR dengan button (+3)
- [ ] Penjelasan deadlock dan pencegahannya (+2)
- [ ] Dual-core ESP32 dengan queue communication (+3)
- [ ] Event Group demonstration (+2)

---

## 📈 Bonus Points (Max +10)

| Bonus | Poin | Kriteria |
|-------|------|----------|
| ISR Queue Demo | +3 | Demonstrasi queue dari interrupt |
| Deadlock Explanation | +2 | Penjelasan dan pencegahan deadlock |
| Dual-Core Queue | +3 | ESP32 inter-core communication |
| Event Group | +2 | Event group demonstration |
| Code Walkthrough | +2 | Line-by-line explanation kode penting |

---

## ⚠️ Pengurangan Nilai

| Pelanggaran | Pengurangan |
|-------------|-------------|
| Video < 5 menit atau > 12 menit | -10 poin |
| Tidak ada hardware demo | -15 poin |
| Audio tidak jelas/tidak ada | -15 poin |
| Plagiarism (copy video lain) | -100% |
| Late submission (per hari) | -5 poin |
| Tidak ada serial output demo | -10 poin |

---

## 📤 Format Submission

### Naming Convention:
```
Modul10_Queue_Semaphore_[NIM]_[Nama].mp4
```

### Upload Options:
1. **YouTube (Unlisted)** - Kirim link
2. **Google Drive** - Set sharing "Anyone with link"

### Include dalam Description:
- Nama lengkap dan NIM
- Mata kuliah dan kelas
- Timestamp untuk setiap section
- Link repository kode (jika ada)

---

## 📊 Contoh Struktur Video yang Baik

```
0:00 - Intro (nama, NIM, topik)
0:30 - Penjelasan Queue concept
1:30 - Demo xQueueCreate()
2:30 - Demo xQueueSend/Receive
3:30 - Penjelasan Semaphore types
4:30 - Demo Binary Semaphore (ISR)
5:30 - Demo Counting Semaphore
6:30 - Demo Mutex
7:30 - Hardware demo
8:30 - Serial output walkthrough
9:30 - Kesimpulan
```

---

*Rubrik Penilaian Tugas Video Modul 10 - FreeRTOS Queue dan Semaphore*
