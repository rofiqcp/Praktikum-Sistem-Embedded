# Project Modul 11: FreeRTOS Timer & Notification (Smart Kitchen Timer)

## 🎯 Tujuan Project
1.  Mahasiswa mampu merancang sistem embedded multitasking menggunakan **FreeRTOS**.
2.  Mahasiswa dapat mengimplementasikan **Software Timer** (One-shot dan Auto-reload) untuk manajemen waktu event.
3.  Mahasiswa mampu menggunakan **Task Notification** sebagai mekanasi *lightweight communication* antar Task dan dari ISR ke Task.
4.  Mahasiswa dapat membedakan penggunaan *Binary Semaphore* dengan *Task Notification* dalam konteks sinkronisasi.

## 📝 Deskripsi Project: "Sistem Pengontrol Mesin Cuci Otomatis"
Dalam project ini, Anda diminta untuk mensimulasikan logika kontrol mesin cuci sederhana menggunakan FreeRTOS. Sistem ini memiliki beberapa tahapan kerja (Washing, Rinsing, Spinning) yang durasinya diatur oleh **Software Timer**. Interaksi pengguna (Tombol) akan ditangani menggunakan interrupt yang mengirimkan sinyal ke Task controller menggunakan **Task Notification**.

### Skenario Kerja Sistem
Sistem memiliki 3 status utama yang berjalan berurutan secara otomatis setelah tombol Start ditekan:
1.  **Idle**: Menunggu input user.
2.  **Washing**: Mensimulasikan proses pencucian (Motor berputar bolak-balik / LED berkedip lambat). Durasi: 5 detik.
3.  **Rinsing**: Mensimulasikan pembilasan (Klep air buka-tutup / LED berkedip cepat). Durasi: 3 detik.
4.  **Spinning**: Mensimulasikan pengeringan (Motor putar kencang / LED menyala *breathing* atau ON terus). Durasi: 4 detik.
5.  **Finish**: Kembali ke Idle.

Tombol **Pause** dapat ditekan sewaktu-waktu untuk menghentikan timer sementara (Stop Timer), dan ditekan lagi untuk melanjutkan (Start Timer).

## 🛠️ Spesifikasi Teknis

### 1. Hardware
- **Mikrokontroller**: STM32F103 (Blue Pill) atau ESP32.
- **Actuators**:
    - 1 LED Utama (Built-in LED PC13 atau GPIO2): Indikator Status Mesin.
    - 1 LED Tambahan (Opsional): Indikator "System Ready".
- **Inputs**:
    - Button 1 (PA0 / GPIO0): **Start / Pause / Resume**.
    - Button 2 (PA1 / GPIO4): **Emergency Stop / Reset** (Langsung ke Idle).

### 2. Software Requirements (FreeRTOS Components)

#### A. Tasks
1.  **Task Controller**:
    - Priority: High.
    - Fungsi: Menerima notifikasi dari Button ISR. Mengatur *State Machine* sistem (Idle -> Wash -> Rinse -> Spin). Mengontrol Timer (Start/Stop/Reset).
2.  **Task Display/Status**:
    - Priority: Medium.
    - Fungsi: Menampilkan status saat ini ke Serial Monitor setiap detik (atau saat perubahan state). Mengontrol pola kedipan LED berdasarkan state.

#### B. Software Timers
1.  **Process Timer** (One-shot):
    - Digunakan untuk menghitung durasi setiap tahapan (Wash 5s, Rinse 3s, Spin 4s).
    - Saat timer *expire*, callback function akan mengirim notifikasi ke Task Controller untuk pindah ke state berikutnya.
2.  **Blink Timer** (Auto-reload) - *Opsional (Bisa diganti vTaskDelay di Task Status)*:
    - Digunakan untuk mengatur frekuensi kedipan LED tanpa blocking delay.

#### C. Interrupts & Notification
- **Button ISR**:
    - Tidak boleh ada logic berat (seperti delay atau print/printf) di dalam ISR.
    - Gunakan `vTaskNotifyGiveFromISR()` atau `xTaskNotifyFromISR()` untuk mengirim sinyal ke **Task Controller**.
- **Timer Callback**:
    - Callback fungsi timer mengirim notifikasi ke Task Controller bahwa waktu tahapan telah habis.

### 3. Logika State Machine & Indikator LED

| State | Durasi | Pola LED | Serial Output (Contoh) |
| :--- | :--- | :--- | :--- |
| **IDLE** | - | Mati (OFF) | `[SISTEM] Siap. Tekan Start.` |
| **WASH** | 5 Detik | Blink Lambat (500ms ON, 500ms OFF) | `[STATUS] Mencuci... Sisa 4s` |
| **RINSE** | 3 Detik | Blink Cepat (100ms ON, 100ms OFF) | `[STATUS] Membilas... Sisa 2s` |
| **SPIN** | 4 Detik | Nyala Terus (ON) | `[STATUS] Mengeringkan... ` |
| **PAUSED** | - | Blink Sangat Pendek (100ms ON, 1000ms OFF) | `[SISTEM] Terjeda.` |

---

## 📋 Deliverables (Yang Dikumpulkan)

1.  **Source Code**:
    - File `main.c` (STM32) atau `main.cpp` (ESP32).
    - Kode harus rapi, diberi komentar penjelas pada bagian konfigurasi Timer dan Notifikasi.
2.  **Video Demo** (Maksimal 2 menit):
    - Tunjukkan hardware setup yang digunakan.
    - Demokan alur normal: Idle -> Start -> Wash -> Rinse -> Spin -> Idle.
    - Demokan fitur Pause dan Resume di tengah proses.
    - Demokan fitur Reset/Emergency Stop.
    - Tampilkan Serial Monitor yang berjalan sinkron dengan hardware.
3.  **Laporan Singkat (README.md / PDF)**:
    - Penjelasan singkat alur program (State Diagram).
    - Penjelasan mengapa menggunakan Task Notification lebih baik daripada Semaphore/Queue untuk kasus tombol ini.

## 💡 Tips Pengerjaan
1.  **Debouncing**: Ingat bahwa tombol mekanik membutuhkan debouncing. Anda bisa menggunakan delay sederhana di Task Controller setelah menerima notifikasi, atau menggunakan *Software Timer* tambahan untuk debouncing yang lebih elegan.
2.  **Struct State**: Gunakan `enum` untuk mendefinisikan State mesin cuci agar kode lebih mudah dibaca (misal: `typedef enum { IDLE, WASH, RINSE, SPIN, PAUSE } MachineState_t;`).
3.  **Timer ID**: Jika menggunakan satu timer handle untuk berbagai durasi, gunakan `xTimerChangePeriod()` untuk mengubah durasi sebelum memulai timer untuk tahap berikutnya. Atau, gunakan *Timer ID* (`pvTimerGetTimerID`) jika Anda membuat banyak timer instance.

## ⛔ Larangan
- Menggunakan `HAL_Delay()` atau `delay()` (Arduino) yang bersifat blocking di dalam Task utama lebih dari 10ms. Gunakan `vTaskDelay()`.
- Melakukan `printf` atau operasi String di dalam **ISR** atau **Timer Callback**. Lakukan operasi berat tersebut di Task biasa.

---
**Selamat Mengerjakan!**
