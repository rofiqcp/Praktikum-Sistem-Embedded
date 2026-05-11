# Tugas Video — Modul 09: FreeRTOS Middle - GPIO, Interrupt, Encoder, Serial, DAC, ADC, I2C, SPI

## Informasi Tugas

| Item | Detail |
|---|---|
| Modul | 09 — FreeRTOS Middle - GPIO, Interrupt, Encoder, Serial, DAC, ADC, I2C, SPI |
| Struktur praktikum | 25 eksperimen: 10 STM32 + 10 ESP32 + 5 Multi STM32-ESP32 |
| Platform | ESP32 DevKit + STM32F103/STM32F4 |
| Format | Video presentasi + demonstrasi hardware |
| Durasi | 20–35 menit |
| Upload | YouTube Unlisted atau link e-learning sesuai instruksi dosen |

---

## 1. Deskripsi

Video adalah bukti pemahaman dan demonstrasi praktikum Modul 09. Isi video harus selaras dengan struktur final:

- **10 eksperimen STM32**.
- **10 eksperimen ESP32**.
- **5 eksperimen Multi STM32-ESP32**.
- **Project final RTOS Sensor Hub System**.

Bahasa presentasi: Indonesia.

---

## 2. Ketentuan Teknis

- Screen recording wajib: kode, serial monitor, output build/upload.
- Rekaman hardware wajib: ESP32, STM32, LED, button, encoder, potensiometer, sensor, wiring.
- Webcam/presenter disarankan terlihat.
- Resolusi minimal 720p; 1080p disarankan.
- Audio jelas.
- Tampilkan hasil nyata, bukan hanya slide.
- Boleh mempercepat bagian upload/compile, tetapi output penting harus terbaca.

---

## 3. Struktur Video Disarankan

| Bagian | Durasi | Isi |
|---|---:|---|
| Pembukaan | 1–2 menit | nama, NIM, Modul 09, daftar hardware |
| Ringkasan teori | 3–5 menit | FreeRTOS basics: task, scheduler, ISR, semaphore, mutex, queue, event group |
| Demo STM32 | 6–8 menit | 10 eksperimen STM32 ringkas |
| Demo ESP32 | 6–8 menit | 10 eksperimen ESP32 ringkas |
| Demo Multi | 4–6 menit | 5 eksperimen multi STM32-ESP32 |
| Project final | 4–6 menit | RTOS Sensor Hub System |
| Penutup | 1–2 menit | kendala, troubleshooting, kesimpulan |

---

## 4. Materi Teori yang Wajib Disebut

1. Task sebagai unit eksekusi konkuren dalam RTOS.
2. Scheduler preemptive berdasarkan priority.
3. Context switch dan stack management.
4. ISR pada RTOS dan ISR-safe API (FromISR).
5. Binary semaphore untuk ISR-to-task signaling.
6. Mutex untuk shared resource protection dan priority inheritance.
7. Queue untuk inter-task communication dan ISR-to-task data transfer.
8. Event group untuk multiple event synchronization.
9. GPIO tasks dengan `vTaskDelay()` non-blocking.
10. External interrupt dengan semaphore/queue dari ISR.
11. Encoder 2-pin interrupt dengan queue RTOS.
12. Serial communication UART dengan queue RTOS.
13. DAC dan ADC dengan task RTOS.
14. I2C dan SPI dengan mutex untuk shared bus.
15. Komunikasi antar-MCU dengan RTOS.

---

## 5. Checklist Demo 25 Eksperimen

### 5.1 STM32 — 10 Eksperimen

| Kode | Demo wajib | Bukti output |
|---|---|---|
| STM32_01 | GPIO Task Blink RTOS | LED blink 1 Hz, serial status |
| STM32_02 | External Interrupt RTOS | Button toggle LED via semaphore |
| STM32_03 | Encoder 2-Pin Interrupt RTOS | Counter CW/CCW di serial |
| STM32_04 | Serial Communication RTOS | Echo dan command via queue |
| STM32_05 | DAC dengan RTOS | Sinyal analog di osiloskop |
| STM32_06 | ADC dengan RTOS | Nilai ADC dan tegangan di serial |
| STM32_07 | I2C dengan RTOS | Data sensor I2C dengan mutex |
| STM32_08 | SPI dengan RTOS | Data SPI transfer dengan mutex |
| STM32_09 | Multi Task GPIO-ADC-UART | Sistem multi-task stabil |
| STM32_10 | FreeRTOS Semaphore Mutex | Akses UART tanpa race condition |

---

### 5.2 ESP32 — 10 Eksperimen

| Kode | Demo wajib | Bukti output |
|---|---|---|
| ESP32_01 | GPIO Task Blink RTOS | LED blink 1 Hz, serial status |
| ESP32_02 | External Interrupt RTOS | Button toggle LED via semaphore |
| ESP32_03 | Encoder 2-Pin Interrupt RTOS | Counter CW/CCW di serial |
| ESP32_04 | Serial Communication RTOS | Echo dan command via queue |
| ESP32_05 | DAC dengan RTOS | Sinyal analog di osiloskop |
| ESP32_06 | ADC dengan RTOS | Nilai ADC dan tegangan di serial |
| ESP32_07 | I2C dengan RTOS | Data sensor I2C dengan mutex |
| ESP32_08 | SPI dengan RTOS | Data SPI transfer dengan mutex |
| ESP32_09 | Multi Task GPIO-ADC-UART | Sistem multi-task stabil |
| ESP32_10 | FreeRTOS Semaphore Mutex | Akses UART tanpa race condition |

---

### 5.3 Multi STM32-ESP32 — 5 Eksperimen

| Kode | Demo wajib | Bukti output |
|---|---|---|
| MULTI_01 | RTOS GPIO Task Sync | LED STM32 dan ESP32 blink sinkron |
| MULTI_02 | RTOS ADC-DAC Communication | ADC STM32 → DAC ESP32 |
| MULTI_03 | RTOS I2C Sensor Sharing | Sensor I2C dibagi antar MCU |
| MULTI_04 | RTOS SPI Data Exchange | Data SPI dipertukarkan |
| MULTI_05 | RTOS Sensor Hub System | Integrasi final sensor hub |

---

## 6. Demo Project Final

Project final yang ditampilkan: **RTOS Sensor Hub System Modul 09**.

Wajib terlihat:

1. ESP32 dan STM32 aktif menjalankan RTOS.
2. Minimal 3 task berjalan di masing-masing MCU.
3. External interrupt dengan semaphore (button → LED).
4. Encoder 2-pin dengan queue RTOS (counter di serial).
5. ADC reading periodik dengan queue.
6. DAC output sinyal analog (osiloskop).
7. I2C sensor reading dengan mutex (jika tersedia).
8. SPI communication dengan mutex (jika tersedia).
9. Komunikasi UART antar-MCU dengan protocol.
10. Multi-task berjalan stabil tanpa blocking.
11. Error handling pada peripheral (timeout, retry).

---

## 7. Narasi Minimum untuk RTOS Primitives

Saat menjelaskan kode, sebutkan:

> Task adalah fungsi yang dijalankan konkuren oleh scheduler RTOS. Semaphore digunakan untuk signaling dari ISR ke task, sedangkan mutex untuk proteksi shared resource seperti UART, I2C, atau SPI bus. Queue digunakan untuk transfer data antar task atau dari ISR ke task. Event group untuk sinkronisasi multiple events.

---

## 8. Format Pengumpulan

Kumpulkan:

1. Link video.
2. Link repository atau arsip source code.
3. Laporan PDF/MD.
4. Foto wiring final.
5. Tabel hasil 25 eksperimen.

Nama file/link disarankan:

```text
Modul09_FreeRTOS_Nama_NIM
```

---

## 9. Rubrik Penilaian Video

| Komponen | Bobot |
|---|---:|
| Pemahaman teori FreeRTOS lengkap | 15% |
| Demo 10 eksperimen STM32 | 15% |
| Demo 10 eksperimen ESP32 | 15% |
| Demo 5 eksperimen Multi STM32-ESP32 | 15% |
| Demo project RTOS Sensor Hub System | 20% |
| Kualitas hardware demo dan wiring explanation | 10% |
| Kualitas audio/video/struktur presentasi | 10% |

---

## 10. Penalti

| Pelanggaran | Penalti |
|---|---:|
| Tidak ada demo hardware | −25% |
| Tidak menunjukkan 25 eksperimen | proporsional jumlah yang hilang |
| Salah menyebut Modul 06/07/08, bukan Modul 09 | −5% |
| Tidak menjelaskan perbedaan semaphore dan mutex | −5% |
| Tidak menjelaskan ISR-safe API (FromISR) | −5% |
| Tidak ada output serial/display | −10% |
| Audio tidak jelas | −10% |
| Video terlalu pendek (<15 menit) | −10% |
| Terlambat | sesuai kebijakan kelas |

---

## 11. Checklist Akhir Sebelum Upload

- [ ] Judul video menyebut **Modul 09**.
- [ ] Bahasa Indonesia.
- [ ] Hardware ESP32 dan STM32 terlihat.
- [ ] LED blink RTOS task ditampilkan.
- [ ] External interrupt dengan semaphore ditampilkan.
- [ ] Encoder 2-pin RTOS ditampilkan.
- [ ] Serial communication RTOS ditampilkan.
- [ ] DAC/ADC RTOS ditampilkan.
- [ ] I2C/SPI RTOS ditampilkan (jika ada).
- [ ] 10 eksperimen STM32 ditampilkan.
- [ ] 10 eksperimen ESP32 ditampilkan.
- [ ] 5 eksperimen Multi ditampilkan.
- [ ] Project final ditampilkan.
- [ ] Link bisa diakses.
