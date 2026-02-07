# Project Modul 12: System Resources & Resilience

## 🎯 Tujuan Project
1.  Merancang sistem multitasking yang **Robust** (Tahan banting) dan efisien memori.
2.  Mengimplementasikan **Event Group** untuk manajemen state sistem yang kompleks.
3.  Menggunakan **Stream Buffer** untuk menangani data stream berkecepatan tinggi dari ISR.
4.  Menerapkan mekanisme **System Health Monitoring** (Stack/Heap monitoring & Stack Overflow Detection).

## 📝 Deskripsi Project: "Reliable Sensor Data Logger"
Sistem Data Logger ini mensimulasikan pengambilan data sensor frekuensi tinggi (misal: akselerometer vibrasi mesin) yang harus diproses dan disimpan tanpa kehilangan data (data loss). Sistem juga harus mampu mendeteksi anomali memori (memory leak/overflow) dan melakukan *Self-Recovery* jika diperlukan.

### Skenario Kerja
Sistem terdiri dari 3 fase utama yang diatur menggunakan **Event Group**:
1.  **Booting**: Inisialisasi Sensor, Storage (Simulasi), dan Komunikasi.
2.  **Active Logging**: Mengambil data dari ISR, buffer buffering, dan processing.
3.  **Error/Maintenance**: Jika terjadi error (Stack overflow/Low heap), sistem masuk mode safe.

### Arsitektur Task
1.  **Task Producer (Sensor ISR)**:
    - Mensimulasikan data masuk setiap 10ms (100Hz).
    - Mengirim data ke Stream Buffer.
2.  **Task Consumer (Processor)**:
    - Membaca data dari Stream Buffer (Batch reading).
    - Melakukan perhitungan rata-rata (Processing).
3.  **Task Monitor (Health Check)**:
    - Periodik (1 detik sekali) mengecek:
        - Sisa Heap.
        - High Water Mark dari semua Task.
    - Jika memori kritis, set Event Bit "ERROR".
4.  **Task Gatekeeper (System Control)**:
    - Menunggu bit Event Group (INIT_DONE, ERROR_DETECTED).
    - Mengatur LED indikator sistem.

## 🛠️ Spesifikasi Teknis

### 1. Hardware
- STM32 (Blue Pill) atau ESP32.
- 1 Potensiometer (Simulasi sensor analog).
- 2 LED (Status OK dan Status Error).

### 2. Software Requirements
- **Event Group**: Minimal 3 bit (SENSOR_READY, STORAGE_READY, ERROR_FLAG).
- **Stream Buffer**: Kapasitas minimal 200 bytes. Trigger level 20 bytes.
- **Memory**: Gunakan `Heap_4` (STM32).
- **Hooks**: Implementasikan `vApplicationStackOverflowHook` dan `vApplicationMallocFailedHook` yang menyalakan LED Error.

---

## 📋 Deliverables
1.  **Source Code**: `main.cpp` yang rapi.
2.  **Video Demo**:
    - Tunjukkan fase booting (LED kedip urut).
    - Tunjukkan data log berjalan lancar.
    - **Trigger Error**: Buat mekanisme buatan (misal tombol) yang memicu alokasi memori besar-besaran atau rekursi sampai Stack Overflow, dan tunjukkan sistem merespons (LED Error nyala / restart).
3.  **Laporan**: Jelaskan alokasi memori yang terjadi.

---
