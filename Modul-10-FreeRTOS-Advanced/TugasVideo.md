# Tugas Video — Modul 10: FreeRTOS Advanced

## Informasi Tugas

| Item | Detail |
|---|---|
| Modul | 10 — FreeRTOS Advanced |
| Struktur praktikum | 25 eksperimen: 10 STM32 + 10 ESP32 + 5 Multi STM32-ESP32 |
| Platform | ESP32 DevKit + STM32F103/STM32F4 |
| Format | Video presentasi + demonstrasi RTOS objects |
| Durasi | 20–35 menit |
| Upload | YouTube Unlisted atau link e-learning sesuai instruksi dosen |

---

## 1. Deskripsi

Video adalah bukti pemahaman dan demonstrasi praktikum Modul 10. Isi video harus selaras dengan struktur final:

- **10 eksperimen STM32**.
- **10 eksperimen ESP32**.
- **5 eksperimen Multi STM32-ESP32**.
- **Project final Advanced RTOS Industrial System**.

Bahasa presentasi: Indonesia.

---

## 2. Ketentuan Teknis

- Screen recording wajib: kode, serial monitor, output build/upload.
- Rekaman hardware wajib: ESP32, STM32, LED, button, wiring.
- Webcam/presenter disarankan terlihat.
- Resolusi minimal 720p; 1080p disarankan.
- Audio jelas.
- Tampilkan hasil nyata, bukan hanya slide.
- Boleh mempercepat bagian upload/compile, tetapi output penting harus terbaca.

---

## 3. Struktur Video Disarankan

| Bagian | Durasi | Isi |
|---|---:|---|
| Pembukaan | 1–2 menit | nama, NIM, Modul 10, daftar hardware |
| Ringkasan teori | 4–6 menit | FreeRTOS Advanced: Event Groups, Software Timers, Task Notifications, Semaphores, Mutex, Memory Management, Static Allocation, Critical Section, Message/Stream Buffers, Queue Sets |
| Demo STM32 | 7–9 menit | 10 eksperimen STM32 ringkas |
| Demo ESP32 | 7–9 menit | 10 eksperimen ESP32 ringkas |
| Demo Multi | 5–7 menit | 5 eksperimen Multi STM32-ESP32 |
| Project final | 4–6 menit | Advanced RTOS Industrial System |
| Penutup | 1–2 menit | kendala, troubleshooting, kesimpulan |

---

## 4. Materi Teori yang Wajib Disebut

1. Event Groups: bit flags, sinkronisasi multiple event, distributed multi-MCU.
2. Software Timers: non-blocking periodik, task daemon timer, remote timer control.
3. Task Notifications: lebih ringan dari semaphore/queue, pipeline data.
4. Semaphore varieties: Binary, Counting, Mutex + Priority Inheritance.
5. Memory Management: heap_1 hingga heap_5, fragmentasi.
6. Static Allocation: task/queue tanpa heap dinamis, cocok untuk safety-critical.
7. Critical Section: proteksi atomis singkat vs Mutex, `taskENTER_CRITICAL()`.
8. Task Suspension: `vTaskSuspend()`/`vTaskResume()` untuk kontrol task runtime.
9. Message Buffers: pesan variabel dengan header panjang otomatis.
10. Stream Buffers: byte stream kontinu, trigger level.
11. Queue Sets: pantau multiple queue/semaphore dalam satu task.
12. Multi-MCU coordination: protokol UART + RTOS objects terdistribusi.

---

## 5. Checklist Demo 25 Eksperimen

### 5.1 STM32 — 10 Eksperimen

| Kode | Demo wajib | Bukti output |
|---|---|---|
| STM32_01 | Event Groups Basic | LED berubah saat button ditekan, sinkronisasi bit |
| STM32_02 | Software Timers | LED toggle periodik 1 detik tanpa blocking |
| STM32_03 | Task Notifications | LED toggle lebih cepat dari semaphore |
| STM32_04 | Semaphore Varieties | Binary (1:1 sync), Counting (max 3 resource) |
| STM32_05 | Mutex dan Priority Inheritance | High task tidak tertunda Medium saat Low pegang mutex |
| STM32_06 | Memory Management | Pilihan heap_4, sisa heap ditampilkan |
| STM32_07 | Static Allocation | Task/queue berjalan tanpa alokasi dinamis |
| STM32_08 | Critical Section + Task Suspend | Race condition ditunjukkan; critical section perbaiki; suspend/resume task |
| STM32_09 | Message Buffer + Stream Buffer | Pesan variabel utuh; stream byte kontinu terbaca |
| STM32_10 | Queue Sets Multiplexing | Event sumber teridentifikasi; tidak ada polling |

---

### 5.2 ESP32 — 10 Eksperimen

| Kode | Demo wajib | Bukti output |
|---|---|---|
| ESP32_01 | Event Groups Basic | LED berubah sesuai event bit |
| ESP32_02 | Software Timers | LED toggle periodik non-blocking |
| ESP32_03 | Task Notifications | Respon lebih cepat dari semaphore |
| ESP32_04 | Semaphore Varieties | Binary dan Counting semaphore bekerja |
| ESP32_05 | Mutex dan Priority Inheritance | Priority Inheritance mencegah inversion |
| ESP32_06 | Memory Management | Heap ESP32 ditampilkan sisa heap |
| ESP32_07 | Static Allocation | Task/queue statis berjalan |
| ESP32_08 | Critical Section + Task Suspend | Race condition; perbaikan atomis; suspend/resume |
| ESP32_09 | Message Buffer + Stream Buffer | Pesan variabel; stream kontinu |
| ESP32_10 | Queue Sets Multiplexing | Multiple event ditangani tanpa polling |

---

### 5.3 Multi STM32-ESP32 — 5 Eksperimen

| Kode | Demo wajib | Bukti output |
|---|---|---|
| MULTI_01 | Distributed Event Group Sync | Event STM32 muncul di ESP32 LED <10 ms; ACK protocol berjalan |
| MULTI_02 | Multi-MCU Timer Network | ESP32 start/stop/change timer STM32; laporan event tampil |
| MULTI_03 | Task Notification Pipeline | Pipeline ADC data tanpa semaphore; rata-rata tampil tiap detik |
| MULTI_04 | Priority Inheritance Network | Log priority inheritance STM32 di ESP32; statistik terdokumentasi |
| MULTI_05 | Full Advanced Industrial System | Semua objek RTOS aktif; state machine IDLE/ACTIVE/ALARM; watchdog berfungsi |

---

## 6. Demo Project Final

Project final yang ditampilkan: **Advanced RTOS Industrial System Modul 10**.

Wajib terlihat:

1. ESP32 dan STM32 aktif dan saling berkomunikasi.
2. Event Groups state machine ESP32: IDLE/ACTIVE/ALARM.
3. Software Timer watchdog ESP32 trigger ALARM saat STM32 offline.
4. Task Notifications pipeline data ADC dari STM32 ke ESP32.
5. Mutex mencegah priority inversion pada shared UART.
6. Critical Section update statistik atomis di ESP32.
7. Static Allocation task kritis berjalan tanpa heap dinamis.
8. Message Buffer membawa data variabel antar task dan MCU.
9. Stream Buffer logging data sensor kontinu.
10. Queue Sets multiplexing multiple event source di ESP32 gateway.
11. LED indicators: hijau (ACTIVE), kuning (IDLE), merah (ALARM).
12. Serial monitor menampilkan semua objek RTOS aktif secara bersamaan.
12. Error handling RTOS ditunjukkan.

---

## 7. Format Pengumpulan

Kumpulkan:

1. Link video.
2. Link repository atau arsip source code.
3. Laporan PDF/MD.
4. Foto wiring final.
5. Tabel hasil 20 eksperimen.

Nama file/link disarankan:

```text
Modul10_FreeRTOS_Nama_NIM
```

---

## 8. Rubrik Penilaian Video

| Komponen | Bobot |
|---|---:|
| Pemahaman teori FreeRTOS Advanced lengkap | 15% |
| Demo 10 eksperimen STM32 | 15% |
| Demo 10 eksperimen ESP32 | 15% |
| Demo project Advanced RTOS Industrial System | 20% |
| Kualitas hardware demo dan wiring explanation | 10% |
| Kualitas audio/video/struktur presentasi | 10% |

---

## 9. Penalti

| Pelanggaran | Penalti |
|---|---:|
| Tidak ada demo hardware | −25% |
| Tidak menunjukkan 20 eksperimen | proporsional jumlah yang hilang |
| Salah menyebut Modul 09, bukan Modul 10 | −5% |
| Tidak menjelaskan Priority Inheritance | −5% |
| Tidak ada output serial/LED | −10% |
| Audio tidak jelas | −10% |
| Video terlalu pendek (<15 menit) | −10% |
| Terlambat | sesuai kebijakan kelas |

---

## 10. Checklist Akhir Sebelum Upload

- [ ] Judul video menyebut **Modul 10**.
- [ ] Bahasa Indonesia.
- [ ] Hardware ESP32 dan STM32 terlihat.
- [ ] LED indikator RTOS objects dijelaskan.
- [ ] 10 eksperimen STM32 ditampilkan.
- [ ] 10 eksperimen ESP32 ditampilkan.
- [ ] Project final ditampilkan.
- [ ] Priority Inheritance ditunjukkan.
- [ ] Queue Sets menangani multiple objek.
- [ ] Link bisa diakses.
