# Rubrik Penilaian Tugas Video
## Modul 04: Smart Battery Monitor (ADC)

Video demonstrasi Project Modul 04 fokus pada verifikasi keakuratan pembacaan sensor dan stabilitas sistem monitoring baterai.

## 📹 Spesifikasi Teknis Video

- **Durasi:** 5 - 8 Menit
- **Resolusi:** Min. 720p
- **Platform:** YouTube (Unlisted) / GDrive
- **Wajah:** Wajib tampil saat intro dan closing

---

## 📊 Detail Kriteria (Total: 100 Poin)

### 1. Demonstrasi Hardware & Akurasi (40 Poin)

| Scene Wajib | Deskripsi Aktivitas | Poin |
|-------------|---------------------|------|
| **Validation Test** | - Tunjukkan pembacaan Voltmeter/Multimeter fisik pada kaki Potensiometer. <br> - Tunjukkan nilai yang tampil di Dashboard Web secara bersamaan. <br> - **Goal:** Buktikan nilainya sama/mirip (toleransi < 2%). | 20 |
| **Stability Test** | - Set potensio pada posisi tetap. <br> - Zoom-in ke Serial Monitor/Dashboard. <br> - **Goal:** Buktikan angka stabil (tidak loncat-loncat) berkat digital filtering. | 10 |
| **Response Test** | - Putar potensio dengan cepat. <br> - **Goal:** Tunjukkan sistem merespons perubahan tanpa delay berlebihan. | 10 |

### 2. Penjelasan Teknis (30 Poin)

| Topik | Deskripsi | Poin |
|-------|-----------|------|
| **ADC Architecture** | Jelaskan singkat perbedaan konfigurasi ADC di STM32 (12-bit SAR) vs ESP32 (Attenuation). | 10 |
| **Signal Conditioning** | Jelaskan rumus konversi: Dari `Raw Value` -> `Voltage Divider Formula` -> `Real Voltage`. | 10 |
| **Filter Logic** | Tunjukkan potongan kode *Moving Average* dan jelaskan cara kerjanya. | 10 |

### 3. Fungsionalitas Sistem (20 Poin)

| Fitur | Deskripsi | Poin |
|-------|-----------|------|
| **Alerting** | Demo saat tegangan diturunkan simulasi Low Battery (LED berubah warna / Warning muncul). | 10 |
| **Dashboard** | Tunjukkan UI Web yang menampilkan Volt, Amphere, dan Status Baterai. | 10 |

### 4. Kualitas Video (10 Poin)

| Aspek | Deskripsi | Poin |
|-------|-----------|------|
| **Visual & Audio** | Gambar jelas (tidak blur saat zoom ke angka multimeter), Suara narasi terdengar jelas. | 10 |

---

## 💡 Tips & Trik Video Modul 04

1.  **Split Screen:** Jika memungkinkan, gunakan editing split screen. Kiri: Kamera ke Multimeter fisik. Kanan: Screen Record Dashboard Web. Ini sangat meyakinkan!
2.  **Multimeter Jelas:** Pastikan angka di multimeter tidak tertutup kabel atau silau lampu.
3.  **Tunjukkan Noise:** Boleh juga mendemokan perbandingan "Filter OFF" vs "Filter ON" untuk nilai tambah (menunjukkan urgensi filtering).
