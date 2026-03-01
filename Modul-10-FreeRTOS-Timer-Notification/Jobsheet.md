# Jobsheet Modul 10: FreeRTOS Software Timer dan Task Notification

## Praktikum Sistem Embedded

**Semester:** Genap 2025/2026  
**Durasi:** 3 × 50 menit (2 pertemuan)  
**Platform:** ESP32 DevKit V1 & STM32 Blue Pill (STM32F103C8T6)

---

## 1. Tujuan Praktikum

Setelah menyelesaikan praktikum ini, mahasiswa diharapkan mampu:

1. **Memahami software timer** — Membuat one-shot dan auto-reload timer, mengubah period dinamis, menggunakan timer ID.
2. **Mengimplementasikan debounce dan timeout** — Menggunakan timer untuk filtering noise dan watchdog heartbeat.
3. **Menguasai task notification** — Mengirim sinyal, nilai, dan counting event antar task dengan overhead minimal.
4. **Menggunakan event group** — Sinkronisasi multi-task dengan bit manipulation (AND/OR logic).
5. **Membandingkan mekanisme** — Menganalisis trade-off antara timer, notification, semaphore, dan event group.

---

## 2. Peralatan

| No | Komponen | Jumlah | Keterangan |
|----|----------|--------|------------|
| 1 | ESP32 DevKit V1 | 1 | Dual-core, FreeRTOS built-in |
| 2 | STM32 Blue Pill | 1 | ARM Cortex-M3 + FreeRTOS |
| 3 | ST-Link V2 | 1 | Programmer STM32 |
| 4 | LED 5mm | 3 | Indikator (merah, hijau, kuning) |
| 5 | Resistor 330Ω | 3 | Current limiting |
| 6 | Push Button | 1 | User input / interrupt source |
| 7 | Resistor 10kΩ | 1 | Pull-up button |
| 8 | Breadboard + kabel jumper | 1 set | |

---

## 3. Teori Singkat

**Software Timer** dikelola oleh timer daemon task (bukan hardware timer). Tipe: **one-shot** (fire sekali lalu berhenti) dan **auto-reload** (fire berulang). Callback berjalan di konteks timer task — tidak boleh blocking!

**Task Notification** adalah mekanisme komunikasi ringan bawaan setiap task (tanpa alokasi tambahan). Mendukung: give/take (seperti semaphore), set value (kirim data 32-bit), dan counting. ~45% lebih cepat dari semaphore.

**Event Group** memungkinkan task menunggu kombinasi event menggunakan operasi bit AND/OR. Cocok untuk sinkronisasi barrier (rendezvous) — semua task harus sampai sebelum lanjut.

---

## 4. Langkah Percobaan

> **Catatan:** Serial Monitor 115200 baud. LED pada ESP32: GPIO2, GPIO4, GPIO5. LED pada STM32: PC13, PB0, PB1. Button pada ESP32: GPIO0 (BOOT). Button pada STM32: PA0.

---

### Percobaan 01: Software Timer Basic

**Tujuan:** Memahami pembuatan dan penggunaan one-shot timer dan auto-reload timer.

#### Langkah Kerja

1. Buka project `ESP32_01` atau `STM32_01`.
2. Program membuat 2 timer:
   - **One-shot timer** (3 detik) — menyalakan LED sekali setelah timeout
   - **Auto-reload timer** (1 detik) — toggle LED berulang
3. Build dan upload. Amati perilaku kedua timer.
4. Amati serial: timer callback berjalan di konteks daemon task (bukan task user).
5. Coba stop dan restart timer — amati perubahan perilaku.

#### Tabel Pengamatan

| Timer | Tipe | Period | Callback Count (30s) | LED Behavior |
|-------|------|--------|---------------------|-------------|
| Timer 1 | One-shot | 3s | | |
| Timer 2 | Auto-reload | 1s | | |

#### Pertanyaan Analisa

1. Apa perbedaan one-shot dan auto-reload timer? Kapan gunakan yang mana?
2. Di task mana callback timer dieksekusi? Mengapa tidak boleh ada blocking call di callback?
3. Apa yang terjadi jika callback timer memakan waktu terlalu lama?
4. Berapa prioritas timer daemon task? Bagaimana mempengaruhi responsivitas?

---

### Percobaan 02: Timer Period Change

**Tujuan:** Mengubah period timer secara dinamis saat runtime.

#### Langkah Kerja

1. Buka project `ESP32_02` atau `STM32_02`.
2. Auto-reload timer menggedipkan LED dengan period awal 1000ms.
3. Setiap button press, speed berubah: 100ms → 500ms → 1000ms → 2000ms → cycle.
4. Amati `xTimerChangePeriod()` (dari task) atau `xTimerChangePeriodFromISR()` (dari ISR).
5. Perhatikan bahwa perubahan period langsung efektif tanpa restart.

#### Tabel Pengamatan

| Button Press | Period (ms) | LED Blink Rate | Change Latency |
|-------------|-----------|---------------|----------------|
| 0 (default) | 1000 | | |
| 1 | | | |
| 2 | | | |
| 3 | | | |

#### Pertanyaan Analisa

1. Apakah `xTimerChangePeriod()` mereset timer countdown dari awal?
2. Apa perbedaan `xTimerChangePeriod()` dan `xTimerChangePeriodFromISR()`?
3. Bagaimana timer daemon memproses command change period — secara sinkron atau asinkron?
4. Skenario apa yang memerlukan perubahan period dinamis?

---

### Percobaan 03: Timer ID — Multiple Timer Satu Callback

**Tujuan:** Menggunakan timer ID untuk membedakan multiple timer yang berbagi satu callback function.

#### Langkah Kerja

1. Buka project `ESP32_03` atau `STM32_03`.
2. Tiga auto-reload timer (500ms, 1000ms, 2000ms) menggunakan **satu callback function** yang sama.
3. Callback menggunakan `pvTimerGetTimerID()` untuk mengetahui timer mana yang fire.
4. Setiap timer toggle LED yang berbeda.
5. Amati rasio fire count — Timer 500ms harus fire 4× lebih sering dari Timer 2000ms.

#### Tabel Pengamatan

| Timer | Period (ms) | Fire Count (20s) | Expected Ratio | Actual Ratio |
|-------|-----------|-----------------|----------------|-------------|
| Timer A | 500 | | 4 | |
| Timer B | 1000 | | 2 | |
| Timer C | 2000 | | 1 | |

#### Pertanyaan Analisa

1. Apa keuntungan menggunakan satu callback untuk multiple timer vs callback terpisah?
2. Bagaimana `pvTimerGetTimerID()` bekerja? Apa tipe datanya?
3. Apakah timer ID bisa diubah saat runtime? Jika ya, untuk apa?
4. Berapa maksimal jumlah software timer yang wajar? Apa batasannya?

---

### Percobaan 04: Timer Debounce

**Tujuan:** Mengimplementasikan software debounce menggunakan one-shot timer.

#### Langkah Kerja

1. Buka project `ESP32_04` atau `STM32_04`.
2. Setiap edge ISR dari button memanggil `xTimerResetFromISR()` — mereset countdown 50ms.
3. Selama button masih bouncing (edge terus terjadi), timer selalu di-reset.
4. Callback hanya fire setelah button **stabil** selama 50ms (bouncing selesai).
5. Bandingkan raw ISR count vs debounced count — raw jauh lebih banyak.

#### Tabel Pengamatan

| Press # | Raw ISR Count | Debounced Count | Bounce Ratio |
|---------|-------------|-----------------|-------------|
| 1 | | 1 | |
| 2 | | 2 | |
| 3 | | 3 | |
| Total | | | |

#### Pertanyaan Analisa

1. Mengapa teknik debounce timer lebih baik dari delay blocking di ISR?
2. Berapa waktu debounce optimal? Terlalu pendek vs terlalu panjang — apa efeknya?
3. Apakah teknik ini menangani both press dan release debounce?
4. Bandingkan software debounce (timer) vs hardware debounce (RC filter).

---

### Percobaan 05: Timer Timeout Monitor

**Tujuan:** Menggunakan timer sebagai watchdog heartbeat — deteksi timeout jika tidak ada heartbeat.

#### Langkah Kerja

1. Buka project `ESP32_05` atau `STM32_05`.
2. One-shot timer timeout 5 detik dimulai.
3. Task mengirim heartbeat berkala — setiap heartbeat mereset timer via `xTimerReset()`.
4. Selama heartbeat aktif, timeout tidak pernah fire.
5. Simulasikan task hang (berhenti kirim heartbeat) → timer fire → WARNING di serial, LED blink.
6. Pada STM32, hang disimulasikan setiap 5 heartbeat.

#### Tabel Pengamatan

| Event | Waktu | Timer State | Output |
|-------|-------|-----------|--------|
| Start | 0s | Active (5s timeout) | |
| Heartbeat 1 | ~1s | Reset | |
| Heartbeat 2 | ~2s | Reset | |
| Task hangs | ~5s | Counting down... | |
| Timeout! | ~10s | FIRED | WARNING! |

#### Pertanyaan Analisa

1. Apa perbedaan pendekatan ini dengan hardware watchdog (IWDG di STM32)?
2. Mengapa `xTimerReset()` cocok untuk heartbeat pattern?
3. Apa yang harus dilakukan saat timeout terdeteksi? (restart task? system reset? alert?)
4. Bagaimana menangani multiple task yang masing-masing perlu dimonitor?

---

### Percobaan 06: Task Notification Basic

**Tujuan:** Menggunakan task notification sebagai pengganti binary semaphore — lebih ringan dan cepat.

#### Langkah Kerja

1. Buka project `ESP32_06` atau `STM32_06`.
2. Button ISR memanggil `vTaskNotifyGiveFromISR()` — membangunkan handler task.
3. Handler task menunggu pada `ulTaskNotifyTake()` — toggle LED saat terbangunkan.
4. Pada ESP32, bandingkan latency: notification vs binary semaphore (mode bergantian setiap 5 press).
5. Amati bahwa notification lebih cepat karena tidak perlu membuat objek terpisah.

#### Tabel Pengamatan

| Method | Press Count | Avg Latency (μs) | Min | Max |
|--------|-----------|------------------|-----|-----|
| Notification | 10 | | | |
| Semaphore | 10 | | | |

#### Pertanyaan Analisa

1. Apa keuntungan task notification dibanding binary semaphore?
2. Apa kelemahan task notification? (hanya bisa notify satu task tertentu)
3. Mengapa notification lebih cepat? Apa yang dihemat?
4. Kapan sebaiknya tetap menggunakan semaphore meskipun notification tersedia?

---

### Percobaan 07: Task Notification Value

**Tujuan:** Mengirim data 32-bit melalui task notification — sebagai pengganti queue ringan.

#### Langkah Kerja

1. Buka project `ESP32_07` atau `STM32_07`.
2. Sender task mengirim command (LED_ON, LED_OFF, STATUS, RESET, dll) via `xTaskNotify()`.
3. Receiver task menggunakan `xTaskNotifyWait()` untuk membaca nilai.
4. Pelajari perbedaan `eSetValueWithOverwrite` dan `eSetValueWithoutOverwrite`.
5. ESP32: ketik command di serial. STM32: cycle otomatis.

#### Tabel Pengamatan

| Action | Command Value | Overwrite Mode | Received? | Result |
|--------|-------------|---------------|----------|--------|
| | eSetValueWithOverwrite | | | |
| | eSetValueWithoutOverwrite | | | |
| Rapid send 2× | | | Yang mana diterima? | |

#### Pertanyaan Analisa

1. Apa perbedaan `eSetValueWithOverwrite` dan `eSetValueWithoutOverwrite`?
2. Bagaimana jika receiver belum sempat membaca dan sender kirim lagi (overwrite)?
3. Kapan sebaiknya menggunakan notification value vs queue?
4. Apakah notification value bisa mengirim struct? Jika data > 32-bit, apa solusinya?

---

### Percobaan 08: Task Notification Counting

**Tujuan:** Menggunakan task notification sebagai counting semaphore — menghitung event.

#### Langkah Kerja

1. Buka project `ESP32_08` atau `STM32_08`.
2. Multiple producer task memanggil `xTaskNotifyGive()` — setiap call increment notification value.
3. Handler task menggunakan `ulTaskNotifyTake(pdTRUE)` — decrement by 1, proses satu event.
4. Amati backlog: jika producer lebih cepat, notification value (pending count) naik.
5. Bandingkan `pdTRUE` (decrement) vs `pdFALSE` (clear to zero) behavior.

#### Tabel Pengamatan

| Waktu | Events Sent | Events Processed | Backlog (pending) |
|-------|-----------|-----------------|------------------|
| 5s | | | |
| 15s | | | |
| 30s | | | |

#### Pertanyaan Analisa

1. Apa perbedaan `ulTaskNotifyTake(pdTRUE)` dan `ulTaskNotifyTake(pdFALSE)`?
2. Apa keuntungan counting notification dibanding counting semaphore?
3. Apakah ada batas maksimal notification count? Apa tipe datanya?
4. Bagaimana menangani overflow jika events terlalu banyak?

---

### Percobaan 09: Task Notification vs Semaphore dari ISR

**Tujuan:** Membandingkan latency wake-up ISR-to-task: notification vs semaphore secara kuantitatif.

#### Langkah Kerja

1. Buka project `ESP32_09` atau `STM32_09`.
2. Button press memicu ISR — bergantian menggunakan notification dan semaphore.
3. Ukur latency dari ISR trigger sampai task wakeup.
4. ESP32: menggunakan `esp_timer_get_time()` (microsecond precision).
5. Amati tabel perbandingan: min, max, average latency.

#### Tabel Pengamatan

| Method | Samples | Min (μs) | Max (μs) | Avg (μs) |
|--------|---------|---------|---------|---------|
| Notification | | | | |
| Semaphore | | | | |
| Difference | | | | |

#### Pertanyaan Analisa

1. Mana yang lebih cepat? Berapa persentase perbedaannya?
2. Mengapa ada perbedaan latency? Apa yang dilakukan FreeRTOS secara internal?
3. Apakah perbedaan ini signifikan untuk aplikasi nyata?
4. Faktor apa saja yang mempengaruhi ISR-to-task latency (selain mekanisme)?

---

### Percobaan 10: Event Group Basic

**Tujuan:** Menggunakan event group untuk sinkronisasi multi-event dengan logika AND/OR.

#### Langkah Kerja

1. Buka project `ESP32_10` atau `STM32_10`.
2. Tiga sensor task simulasi (Temp, Humidity, Pressure) — masing-masing set 1 bit di event group.
3. Collector task menunggu **semua 3 bit** di-set (AND logic) menggunakan `xEventGroupWaitBits()`.
4. Setelah semua sensor ready → LED menyala, data diproses.
5. Pada ESP32, amati juga OR logic — trigger jika *salah satu* sensor alert.

#### Tabel Pengamatan

| Sensor | Bit | Set Time | All Ready? | Collector Wakeup |
|--------|-----|---------|-----------|-----------------|
| Temp | BIT_0 | | | |
| Humidity | BIT_1 | | | |
| Pressure | BIT_2 | | | |
| --- | ALL | --- | YES | Waktu: |

#### Pertanyaan Analisa

1. Apa perbedaan AND logic dan OR logic pada `xEventGroupWaitBits()`?
2. Apa fungsi parameter `xClearOnExit`? Kapan set `pdTRUE` vs `pdFALSE`?
3. Berapa bit maksimal yang tersedia di event group? (24 bit user pada 32-bit system)
4. Kapan gunakan event group vs multiple notification/semaphore?

---

### Percobaan 11: Event Group Sync (Barrier)

**Tujuan:** Mengimplementasikan barrier synchronization — semua task menunggu di titik rendezvous.

#### Langkah Kerja

1. Buka project `ESP32_11` atau `STM32_11`.
2. Tiga worker task melakukan pekerjaan dengan durasi berbeda (1s, 2s, 3s).
3. Setelah selesai, masing-masing memanggil `xEventGroupSync()` — menunggu semua task selesai.
4. Tidak ada task yang lanjut ke fase berikutnya sampai **semua** task di sync point.
5. LED menyala saat synchronization tercapai.

#### Tabel Pengamatan

| Worker | Work Duration | Arrive at Sync | Wait Time | All Synced? |
|--------|-------------|---------------|----------|------------|
| A | 1s | ~1s | ~2s waiting | |
| B | 2s | ~2s | ~1s waiting | |
| C | 3s | ~3s | 0s | |
| --- | --- | --- | Total: ~3s | YES |

#### Pertanyaan Analisa

1. Apa perbedaan `xEventGroupSync()` dan `xEventGroupWaitBits()`?
2. Apa itu barrier / rendezvous pattern? Berikan contoh penggunaan nyata.
3. Apa yang terjadi jika satu task tidak pernah sampai di sync point?
4. Bisakah `xEventGroupSync()` digunakan untuk multi-phase synchronization?

---

### Percobaan 12: Benchmark — Timer vs Notification vs Semaphore

**Tujuan:** Membandingkan overhead dan latency tiga mekanisme secara kuantitatif (1000 iterasi).

#### Langkah Kerja

1. Buka project `ESP32_12` atau `STM32_12`.
2. Program menjalankan benchmark 3 mekanisme masing-masing 1000 iterasi:
   - **Software Timer:** create → start → callback → stop
   - **Task Notification:** give → take cycle
   - **Binary Semaphore:** give → take cycle
3. Ukur latency per iterasi (μs) dan hitung statistik.
4. Amati tabel perbandingan: min, max, average, stddev.
5. Identifikasi mekanisme tercepat dan alasannya.

#### Tabel Pengamatan

| Mechanism | Min (μs) | Max (μs) | Avg (μs) | StdDev (μs) |
|-----------|---------|---------|---------|-------------|
| Software Timer | | | | |
| Task Notification | | | | |
| Binary Semaphore | | | | |

#### Pertanyaan Analisa

1. Urutkan ketiga mekanisme dari tercepat ke terlambat. Mengapa urutan demikian?
2. Apa overhead utama software timer dibanding notification langsung?
3. Kapan memilih timer meskipun lebih lambat? (periodic, non-blocking)
4. Bagaimana menggunakan hasil benchmark ini untuk desain sistem real?

---

## 5. Tabel Komparatif

| Mekanisme | Tipe | Overhead | ISR Safe? | Capacity |
|-----------|------|----------|----------|----------|
| Software Timer | Periodik/one-shot | Sedang (daemon task) | Reset/Change FromISR | Unlimited (command queue) |
| Task Notification | Point-to-point | Rendah (built-in) | Give FromISR | 1 target task |
| Event Group | Multi-point sync | Sedang | SetBits FromISR | 24 bits |
| Binary Semaphore | Signal | Sedang | Give FromISR | 1 value |
| Counting Semaphore | Event counting | Sedang | Give FromISR | configMAX |

| Aspek | STM32 | ESP32 |
|-------|-------|-------|
| Timer Daemon Priority | `configTIMER_TASK_PRIORITY` | Same |
| Notification API | Standard FreeRTOS | Standard FreeRTOS |
| Event Group Bits | 24 user bits | 24 user bits |
| Timing Precision | SysTick (1ms) | esp_timer (1μs) |
| Benchmark Resolution | Tick-based | μs-based |

---

## 6. Referensi

1. FreeRTOS Software Timer API — https://www.freertos.org/FreeRTOS-Software-Timer-API-Functions.html
2. FreeRTOS Task Notification API — https://www.freertos.org/RTOS-task-notifications.html
3. FreeRTOS Event Group API — https://www.freertos.org/FreeRTOS-Event-Groups.html
4. Mastering the FreeRTOS Real Time Kernel — Richard Barry (Ch. 5, 8, 9)
5. AN4631 — Using FreeRTOS on STM32, STMicroelectronics
6. ESP-IDF FreeRTOS Documentation — Espressif Systems

---

*Jobsheet Modul 10 — FreeRTOS Timer & Notification | Praktikum Sistem Embedded | 2025/2026*
