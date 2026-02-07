# Rubrik Penilaian Project (Modul 11)

## Kriteria Utama

| Aspek | Detail Kriteria | Bobot | Perolehan (0-100) |
| :--- | :--- | :---: | :---: |
| **Logic & Functionality (60%)** | | | |
| 1. Software Timer | Berhasil mengimplementasikan One-shot Timer untuk durasi status (Wash/Rinse/Spin). Callback berfungsi memicu perpindahan state. | 20% | |
| 2. Task Notification | Berhasil menggunakan `xTaskNotifyFromISR` dari interrupt tombol ke Task Controller tanpa error/hang. | 20% | |
| 3. State Machine | Transisi antar state (Idle -> Wash -> Rinse -> Spin -> Idle) berjalan mulus sesuai urutan dan durasi. | 10% | |
| 4. Control Features | Fitur Pause, Resume, dan Reset berfungsi dengan benar (Timer berhenti/lanjut/reset). | 10% | |
| **Code Structure (20%)** | | | |
| 1. Best Practice | Tidak ada blocking delay (`HAL_Delay`) >10ms di Task. Tidak ada logic berat di ISR. | 10% | |
| 2. Readability | Kode rapi, indentasi konsisten, penggunaan nama variabel deskriptif, dan komentar pada bagian Timer/Notification. | 10% | |
| **Demonstration (20%)** | | | |
| 1. Demo Hardware | Hardware (LED/Tombol) merespons sesuai spesifikasi. Timing akurat (cek via Serial Monitor). | 20% | |
| **Total** | | **100%** | |

## Catatan Penilaian
- **Nilai 0**: Program tidak bisa compile atau hardware tidak merespons sama sekali.
- **Nilai <50**: Logika utama salah (misal: masih menggunakan delay biasa, bukan timer).
- **Nilai 85+**: Semua fitur wajib berjalan sempurna, kode rapi, dan ada fitur tambahan (misal: blink timer terpisah).
