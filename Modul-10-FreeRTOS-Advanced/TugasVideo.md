# Tugas Video — Modul 10: FreeRTOS Advanced

## Informasi Tugas

| Item | Detail |
|---|---|
| Modul | 10 — FreeRTOS Advanced |
| Struktur praktikum | 20 eksperimen: 10 STM32 + 10 ESP32 |
| Platform | ESP32 DevKit + STM32F103/STM32F4 |
| Format | Video presentasi + demonstrasi RTOS objects |
| Durasi | 20–35 menit |
| Upload | YouTube Unlisted atau link e-learning sesuai instruksi dosen |

---

## 1. Deskripsi

Video adalah bukti pemahaman dan demonstrasi praktikum Modul 10. Isi video harus selaras dengan struktur final:

- **10 eksperimen STM32**.
- **10 eksperimen ESP32**.
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
| Ringkasan teori | 4–6 menit | FreeRTOS Advanced: Event Groups, Software Timers, Task Notifications, Semaphores, Mutex, Memory Management, Static Allocation, Message/Stream Buffers, Queue Sets |
| Demo STM32 | 7–9 menit | 10 eksperimen STM32 ringkas |
| Demo ESP32 | 7–9 menit | 10 eksperimen ESP32 ringkas |
| Project final | 4–6 menit | Advanced RTOS Industrial System |
| Penutup | 1–2 menit | kendala, troubleshooting, kesimpulan |

---

## 4. Materi Teori yang Wajib Disebut

1. Event Groups: bit flags, sinkronisasi multiple event.
2. Software Timers: non-blocking periodik, task daemon timer.
3. Task Notifications: lebih ringan dari semaphore/queue.
4. Semaphore varieties: Binary, Counting, Mutex + Priority Inheritance.
5. Memory Management: heap_1 hingga heap_5, fragmentasi.
6. Static Allocation: task/queue tanpa heap dinamis.
7. Message Buffers: pesan variabel dengan header panjang otomatis.
8. Stream Buffers: byte stream, trigger level.
9. Queue Sets: pantau multiple queue/semaphore dalam satu task.
10. Pemilihan objek RTOS tepat untuk aplikasi industri.

---

## 5. Checklist Demo 20 Eksperimen

### 5.1 STM32 — 10 Eksperimen

| Kode | Demo wajib | Bukti output |
|---|---|---|
| STM32_01 | Event Groups Basic | LED berubah saat button ditekan, sinkronisasi bit |
| STM32_02 | Software Timers | LED toggle periodik 1 detik tanpa blocking |
| STM32_03 | Task Notifications | LED toggle lebih cepat dari semaphore |
| STM32_04 | Semaphore Varieties | Binary (1:1 sync), Counting (max 3 resource) |
| STM32_05 | Mutex and Priority Inheritance | High task tidak tertunda Medium saat Low pegang mutex |
| STM32_06 | Memory Management | Pilihan heap_4, sisa heap ditampilkan |
| STM32_07 | Static Allocation | Task/queue berjalan tanpa alokasi dinamis |
| STM32_08 | Message Buffers | Pesan 1-128 byte diterima utuh |
| STM32_09 | Stream Buffers | Byte stream utuh tanpa overrun |
| STM32_10 | Queue Sets | Pantau 2 queue + 1 semaphore dalam satu task |

---

### 5.2 ESP32 — 10 Eksperimen

| Kode | Demo wajib | Bukti output |
|---|---|---|
| ESP32_01 | Event Groups Basic | LED berubah sesuai event bit |
| ESP32_02 | Software Timers | LED toggle periodik non-blocking |
| ESP32_03 | Task Notifications | Respon lebih cepat dari semaphore |
| ESP32_04 | Semaphore Varieties | Binary dan Counting semaphore bekerja |
| ESP32_05 | Mutex and Priority Inheritance | Priority Inheritance mencegah inversion |
| ESP32_06 | Memory Management | Heap ESP32 ditampilkan sisa heap |
| ESP32_07 | Static Allocation | Task/queue statis berjalan |
| ESP32_08 | Message Buffers | Pesan variabel diterima utuh |
| ESP32_09 | Stream Buffers | Byte stream diterima utuh |
| ESP32_10 | Queue Sets | Multiple objek ditangani dalam satu task |

---

## 6. Demo Project Final

Project final yang ditampilkan: **Advanced RTOS Industrial System Modul 10**.

Wajib terlihat:

1. ESP32 dan STM32 aktif.
2. Event Groups merespons button press.
3. Software Timers berjalan periodik.
4. Task Notifications lebih efisien dari semaphore.
5. Mutex mencegah priority inversion.
6. Memory Management menunjukkan heap tepat.
7. Static Allocation task/queue kritis.
8. Message Buffer menangani pesan variabel.
9. Stream Buffer menangani byte stream.
10. Queue Sets menangani multiple input.
11. Komunikasi antar MCU berjalan.
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
