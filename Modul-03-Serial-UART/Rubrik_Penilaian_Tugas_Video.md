# Rubrik Penilaian Tugas Video
## Modul 03: Serial UART Gateway Project

Video demonstrasi adalah salah satu komponen wajib deliverables praktikum ini. Video harus menunjukkan bukti otentik bahwa mahasiswa telah mengerjakan dan memahami project yang dibuat.

## 📹 Spesifikasi Video

- **Platform:** YouTube (Unlisted/Public) atau Google Drive (Open Access)
- **Durasi:** Minimal 5 menit, Maksimal 10 menit
- **Format:** Landscape (16:9), minimal 720p 30fps
- **Audio:** Suara narasi jelas, backsound diperbolehkan (kecil)

---

## 📊 Kriteria Penilaian (Total: 100 Poin)

### 1. Konten & Demo (50 Poin)

| Sub-Kriteria | Deskripsi | Poin |
|--------------|-----------|------|
| **Pembukaan** | Memperkenalkan diri (Nama, NIM) dan judul project dengan jelas. Wajah presenter wajib muncul di awal. | 5 |
| **Setup Hardware** | Memperlihatkan wiring sistem secara close-up. Menjelaskan komponen STM32, ESP32, dan sensor yang digunakan. | 10 |
| **Live Demo** | Mendemonstrasikan sistem bekerja secara real-time: <br> - Mengubah nilai sensor (misal: menutup sensor cahaya) <br> - Melihat perubahan di Web Dashboard <br> - Melakukan kontrol dari Web ke Actuator | 25 |
| **Error Handling** | Mendemonstrasikan skenario error (misal: cabut kabel RX, lalu pasang lagi) untuk membuktikan fitur reliability. | 10 |

### 2. Penjelasan Teknis (30 Poin)

| Sub-Kriteria | Deskripsi | Poin |
|--------------|-----------|------|
| **Code Walkthrough** | Menjelaskan bagian CORE dari kode program (bukan membaca baris per baris). <br> Fokus pada: Struktur paket data, fungsi send/receive, dan parsing logic. | 15 |
| **Penjelasan Protokol** | Menjelaskan format protokol yang didesain (Start byte, payload structure, checksum). | 10 |
| **Pemahaman** | Narasi menunjukkan mahasiswa paham ALUR data dari sensor sampai ke web interface. | 5 |

### 3. Kualitas Produksi (20 Poin)

| Sub-Kriteria | Deskripsi | Poin |
|--------------|-----------|------|
| **Visual** | Gambar stabil, fokus, pencahayaan cukup. Teks/Overlay kode terbaca jelas. | 10 |
| **Audio** | Suara jernih, tidak bising, intonasi tidak monoton. | 5 |
| **Editing** | Alur video runtut, transisi halus, tidak ada bagian yang membuang waktu (loading lama dicut). | 5 |

---

## ⚠️ Notes Penting

1. **Wajah Presenter:** Wajib muncul minimal pada saat pembukaan dan penutup untuk validasi identitas.
2. **Screen Recording:** Gunakan screen recording (OBS/sejenis) untuk menampilkan bagian coding dan web dashboard. Jangan merekam layar monitor dengan HP (text tidak terbaca).
3. **Link Submission:** Kumpulkan Link Video di file TXT atau pada formulir pengumpulan tugas yang disediakan.

## 💡 Tips Membuat Video Bagus

- **Scripting:** Siapkan poin-poin bicara sebelum merekam agar tidak terbata-bata.
- **Lighting:** Gunakan cahaya tambahan jika ruangan gelap.
- **Microphone:** Gunakan headset mic atau mic dedicated untuk suara lebih baik.
- **Zoom-In:** Saat menunjukkan wiring kabel, rekam dari jarak dekat agar koneksi terlihat.
