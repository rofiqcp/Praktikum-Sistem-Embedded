# PPT Prompts FreeRTOS Modul 12: Advanced Features

## Slide 15: Introduction to Event Groups
- **Judul**: Event Groups
- **Konsep**:
  - Berbeda dengan Queue/Semaphore yang "mengonsumsi" event.
  - Event Group adalah sekumpulan **Event Bits** (Flag) dalam satu variabel (32-bit atau 24-bit).
  - Satu Task bisa menunggu **kombinasi** bit (AND/OR).
  - Banyak Task bisa di-unblock oleh satu event (Broadcasting).

## Slide 16: Event Group vs Binary Semaphore
- **Tabel Perbandingan**:
| Fitur | Binary Semaphore | Event Group |
| :--- | :--- | :--- |
| **Penyimpanan** | 1 Bit (Take/Give) | 24 Bits (Flags) |
| **Logic** | Single Event | AND / OR Logic |
| **Consumer** | 1 Task Only | Multiple Tasks |
| **Usage** | ISR Sync, Mutex | System State, Rendezvous |

## Slide 17: Event Group API
- **Fungsi Utama**:
  - `xEventGroupCreate()`
  - `xEventGroupSetBits()` / `xEventGroupSetBitsFromISR()`
  - `xEventGroupWaitBits()`
  - `xEventGroupClearBits()`
  - `xEventGroupSync()` (Untuk Rendezvous)

## Slide 18: Studi Kasus - System Initialization
- **Scenario**:
  - Task Main App tidak boleh jalan sebelum:
    1.  WiFi Connected (Bit 0)
    2.  Server Connected (Bit 1)
    3.  Sensors Ready (Bit 2)
- **Solusi**:
  - Task Main App memanggil:
    `xEventGroupWaitBits(eg, (BIT0 | BIT1 | BIT2), ..., waitForAll=TRUE, ...)`
  - Task WiFi, Task MQTT, Task Sensor masing-masing set bit.

## Slide 19: Studi Kasus - Task Rendezvous
- **Scenario**:
  - 3 Task bekerja paralel, tapi harus sinkron di satu titik sebelum lanjut ke tahap 2.
- **Solusi**:
  - `xEventGroupSync(eg, MyBit, AllBits, ...)`
  - Task yang sampai duluan akan *Blocked* sampai Task terakhir memanggil fungsi sync.

## Slide 20: Stream Buffer (New in FreeRTOS v10)
- **Apa itu?**:
  - Struktur data primitive untuk mengirim **Continuous Byte Stream**.
  - Dioptimalkan untuk scenario **Single Writer -> Single Reader**.
  - Sangat efisien untuk data dari ISR (Interrupt) ke Task.
- **Visual**: Pipa data bytes. Bytes masuk satu per satu, keluar satu per satu.

## Slide 21: Stream Buffer vs Queue
- **Stream Buffer**:
  - Mengirim byte arbitrary (bisa 1 byte, 10 bytes, 100 bytes).
  - Tidak menyimpan metadata ukuran per item.
  - Lebih cepat dan hemat RAM.
- **Queue**:
  - Mengirim item berukuran tetap (Fixed Size).
  - Menyalin data item per item.
  - Lebih berat (overhead lebih besar).

## Slide 22: Message Buffer
- **Konsep**:
  - Mirip Stream Buffer, tapi unruk **Discrete Messages** dengan panjang bervariasi.
  - Setiap pesan menyimpan panjangnya (2-4 bytes overhead).
  - Cocok untuk mengirim string, paket data jaringan, atau struct berbeda ukuran.

## Slide 23: Low Power Support (Tickless Idle)
- **Masalah**:
  - Tick Interrupt membangunkan CPU setiap 1ms (1000Hz).
  - CPU tidak bisa masuk deep sleep yang lama.
- **Solusi**: **Tickless Idle**.
  - Scheduler mendeteksi jika tidak ada task yang perlu jalan dalam waktu lama.
  - Scheduler mematikan Tick Interrupt.
  - Mengatur Timer Hardware untuk bangun tepat saat Task terdekat butuh jalan.
  - CPU masuk mode **Deep Sleep**.

## Slide 24: Kesimpulan Modul 12
- **Recap**:
  1.  Manajemen memori (Heap & Stack) krusial untuk stabilitas jangka panjang.
  2.  Event Groups sangat powerful untuk manajemen state sistem yang kompleks.
  3.  Gunakan Stream Buffer untuk komunikasi High-Throughput (ISR to Task).
  4.  Pilih Static Allocation untuk sistem safety-critical.

## Slide 25: Referensi
- FreeRTOS.org - Memory Management
- Mastering the FreeRTOS Real Time Kernel (Chapter 2 & 8)
- ESP32 Low Power Documentation
