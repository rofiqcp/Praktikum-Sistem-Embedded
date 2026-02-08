# Project Modul 02: Sistem Monitoring Keamanan Real-Time

## 📋 Informasi Project

| Item | Keterangan |
|------|------------|
| **Judul** | Real-Time Security Monitoring System |
| **Modul** | 02 - Interrupt dan Timer |
| **Platform** | STM32F103C8T6 + ESP32 DevKitC |
| **Tingkat Kesulitan** | ⭐⭐⭐⭐ (Lanjut) |
| **Durasi Pengerjaan** | 2 minggu |
| **Jenis** | Sistem Dual-MCU Terintegrasi |

---

## 🎯 Deskripsi Project

Mahasiswa akan mengembangkan **Sistem Monitoring Keamanan Real-Time** yang menggunakan dua mikrokontroler bekerja sama:

- **STM32F103C8T6** bertindak sebagai **Edge Controller** yang menangani interrupt dari berbagai sensor keamanan dengan respons waktu-kritis
- **ESP32 DevKitC** bertindak sebagai **Hub Controller** yang mengumpulkan data, melakukan timing analysis, dan menyediakan interface monitoring

Sistem ini mengimplementasikan konsep interrupt-driven architecture untuk deteksi intrusi dengan response time yang terukur dan logging berbasis timer.

---

## 🎯 Tujuan Project

Setelah menyelesaikan project ini, mahasiswa mampu:

1. Merancang sistem multi-sensor berbasis interrupt yang efisien
2. Mengimplementasikan NVIC priority management pada STM32
3. Menggunakan hardware timer untuk precise timing measurement
4. Mengembangkan komunikasi antar-MCU dengan protokol yang reliable
5. Menerapkan interrupt latency measurement dan optimization
6. Membangun sistem logging dengan timestamp akurat
7. Mengintegrasikan dual-MCU dalam satu sistem kohesif

---

## 📐 Arsitektur Sistem

```
┌──────────────────────────────────────────────────────────────────────────────┐
│                    SECURITY MONITORING SYSTEM ARCHITECTURE                    │
├──────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│   SENSOR ZONE                      STM32 EDGE                   ESP32 HUB   │
│   ──────────                       CONTROLLER                   CONTROLLER   │
│                                                                              │
│   ┌─────────┐    EXTI0          ┌───────────────┐            ┌───────────┐  │
│   │PIR      │───────────────────┤PA0            │            │           │  │
│   │Sensor   │    ▲Priority 1    │               │            │           │  │
│   └─────────┘    │              │  NVIC         │   UART     │  Timer    │  │
│                  │              │  Handler      ├────────────┤  Manager  │  │
│   ┌─────────┐    │ EXTI1        │               │  115200    │           │  │
│   │Door     │───────────────────┤PA1            │  baud      │  Logging  │  │
│   │Switch   │    ▲Priority 2    │               │            │  System   │  │
│   └─────────┘    │              │  Timer2:      │            │           │  │
│                  │              │  Timestamp    │   GPIO     │  Alert    │  │
│   ┌─────────┐    │ EXTI2        │               ├────────────┤  Manager  │  │
│   │Window   │───────────────────┤PA2            │   Sync     │           │  │
│   │Sensor   │    ▲Priority 3    │               │            │           │  │
│   └─────────┘    │              │  Timer3:      │            │           │  │
│                  │              │  Debounce     │            │           │  │
│   ┌─────────┐    │ EXTI3        │               │            │           │  │
│   │Panic    │───────────────────┤PA3            │            │           │  │
│   │Button   │    ▲Priority 0    │  Timer4:      │            │           │  │
│   └─────────┘    │(Highest)     │  Watchdog     │            │           │  │
│                                 └───────┬───────┘            └─────┬─────┘  │
│                                         │                          │        │
│                                         │    ┌────────────────┐    │        │
│                                         └────┤ Serial Monitor ├────┘        │
│                                              │ (PC Dashboard) │             │
│                                              └────────────────┘             │
│                                                                              │
└──────────────────────────────────────────────────────────────────────────────┘
```

---

## 🔧 Spesifikasi Hardware

### Komponen Utama

| No | Komponen | Jumlah | Fungsi |
|----|----------|--------|--------|
| 1 | STM32F103C8T6 | 1 | Edge Controller |
| 2 | ESP32 DevKitC | 1 | Hub Controller |
| 3 | PIR Sensor HC-SR501 | 1 | Motion detection |
| 4 | Magnetic Door Switch | 1 | Door open/close detection |
| 5 | Magnetic Window Sensor | 1 | Window open/close detection |
| 6 | Push Button (Panic) | 1 | Emergency trigger |
| 7 | LED Merah | 2 | Alert indicator |
| 8 | LED Hijau | 2 | Status indicator |
| 9 | LED Kuning | 1 | Warning indicator |
| 10 | Buzzer Active | 1 | Audible alarm |
| 11 | Resistor 330Ω | 5 | LED current limiting |
| 12 | Resistor 10kΩ | 4 | Pull-up resistors |

### Pin Assignment

**STM32F103C8T6:**
| Pin | Fungsi | Mode | Prioritas |
|-----|--------|------|-----------|
| PA0 | PIR Sensor | EXTI0, Rising | 1 (High) |
| PA1 | Door Switch | EXTI1, Change | 2 |
| PA2 | Window Sensor | EXTI2, Change | 3 |
| PA3 | Panic Button | EXTI3, Falling | 0 (Highest) |
| PA9 | TX to ESP32 | UART1 TX | - |
| PA10 | RX from ESP32 | UART1 RX | - |
| PB3 | Alert LED | Output | - |
| PB4 | Status LED | Output | - |
| PB5 | Sync Signal | Output | - |
| PC13 | Built-in LED | Output | - |

**ESP32 DevKitC:**
| Pin | Fungsi | Mode |
|-----|--------|------|
| GPIO16 | RX from STM32 | UART2 RX |
| GPIO17 | TX to STM32 | UART2 TX |
| GPIO4 | Alert LED | Output |
| GPIO5 | Status LED | Output |
| GPIO18 | Warning LED | Output |
| GPIO19 | Buzzer | Output |
| GPIO21 | Sync from STM32 | Input |
| GPIO2 | Built-in LED | Output |

---

## 📝 Spesifikasi Fungsional

### F1: Interrupt-Driven Sensor Detection (STM32)

**Deskripsi:** STM32 mendeteksi semua event sensor melalui external interrupt dengan prioritas berbeda.

**Fitur:**
- PIR motion detection dengan response time < 1ms
- Door/Window sensor dengan debouncing 50ms
- Panic button dengan highest priority (preemptive)
- Nested interrupt support untuk emergency scenarios

**Implementasi:**
```c
// NVIC Priority Configuration
// Priority 0 = Highest, Priority 15 = Lowest

void configureNVIC(void) {
    // Group 4: 4 bits for preemption, 0 bits for subpriority
    NVIC_SetPriorityGrouping(3);
    
    // Panic Button - Highest priority (can interrupt others)
    NVIC_SetPriority(EXTI3_IRQn, NVIC_EncodePriority(3, 0, 0));
    
    // PIR Sensor - High priority
    NVIC_SetPriority(EXTI0_IRQn, NVIC_EncodePriority(3, 1, 0));
    
    // Door Switch - Medium priority
    NVIC_SetPriority(EXTI1_IRQn, NVIC_EncodePriority(3, 2, 0));
    
    // Window Sensor - Low priority
    NVIC_SetPriority(EXTI2_IRQn, NVIC_EncodePriority(3, 3, 0));
    
    // Enable all EXTI interrupts
    NVIC_EnableIRQ(EXTI0_IRQn);
    NVIC_EnableIRQ(EXTI1_IRQn);
    NVIC_EnableIRQ(EXTI2_IRQn);
    NVIC_EnableIRQ(EXTI3_IRQn);
}
```

### F2: Precision Timing System (STM32)

**Deskripsi:** Menggunakan multiple timer untuk berbagai fungsi timing.

**Timer Allocation:**
| Timer | Fungsi | Konfigurasi |
|-------|--------|-------------|
| TIM2 | Timestamp (μs) | 1MHz, Free-running |
| TIM3 | Debounce Timer | 10kHz, One-shot |
| TIM4 | System Heartbeat | 1Hz, Auto-reload |

**Implementasi:**
```c
// Timestamp Timer - Microsecond precision
void TIM2_Init(void) {
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
    TIM2->PSC = 72 - 1;      // 72MHz / 72 = 1MHz (1μs)
    TIM2->ARR = 0xFFFFFFFF;  // Max value
    TIM2->CR1 |= TIM_CR1_CEN;
}

uint32_t getTimestamp_us(void) {
    return TIM2->CNT;
}
```

### F3: Event Logging and Analysis (ESP32)

**Deskripsi:** ESP32 menerima event dari STM32 dan melakukan logging dengan analisis.

**Fitur:**
- Event logging dengan timestamp
- Response time calculation
- Event frequency analysis
- Alert pattern detection

**Data Format:**
```
[TIMESTAMP_ms] EVENT_TYPE | SENSOR_ID | RESPONSE_TIME_us | STATUS
[1234567890] MOTION | PIR_01 | 850 | ALERT
[1234567895] DOOR_OPEN | DOOR_01 | 920 | WARNING
[1234567900] PANIC | BTN_01 | 120 | EMERGENCY
```

### F4: Alert Management System (ESP32)

**Deskripsi:** Sistem alert bertingkat berdasarkan severity.

**Alert Levels:**
| Level | Kondisi | Respons |
|-------|---------|---------|
| 0 - NORMAL | Tidak ada event | LED Hijau steady |
| 1 - INFO | Door/Window dibuka | LED Kuning blink slow |
| 2 - WARNING | Motion detected | LED Kuning blink fast |
| 3 - ALERT | Multiple triggers | LED Merah + Buzzer intermittent |
| 4 - EMERGENCY | Panic button | LED Merah + Buzzer continuous |

### F5: Communication Protocol

**Format Pesan STM32 → ESP32:**
```
$EVT,<sensor_id>,<event_type>,<timestamp>,<data>*<checksum>\r\n

Contoh:
$EVT,PIR01,MOTION,12345678,1*A5\r\n
$EVT,DOOR01,OPEN,12345700,1*B2\r\n
$EVT,PANIC,PRESS,12345800,1*C3\r\n
```

**Format Respons ESP32 → STM32:**
```
$ACK,<message_id>,<status>*<checksum>\r\n
$CMD,<command_id>,<parameter>*<checksum>\r\n
```

---

## 📋 Deliverables

### D1: Source Code (40%)

| Item | Deskripsi | Bobot |
|------|-----------|-------|
| STM32 Firmware | Edge controller dengan interrupt dan timer | 15% |
| ESP32 Firmware | Hub controller dengan logging dan alert | 15% |
| Communication Protocol | Implementasi protokol reliable | 5% |
| Configuration | Konfigurasi STM32CubeMX / CMake yang benar | 5% |

**Struktur Project:**
```
Project-02-Security-Monitoring/
├── STM32-Edge-Controller/
│   ├── STM32-Edge-Controller.ioc
│   ├── Src/
│   │   ├── main.c
│   │   ├── interrupt_handler.c
│   │   ├── timer_manager.c
│   │   └── uart_protocol.c
│   └── Inc/
│       ├── config.h
│       ├── interrupt_handler.h
│       ├── timer_manager.h
│       └── uart_protocol.h
│
├── ESP32-Hub-Controller/
│   ├── CMakeLists.txt
│   ├── main/
│   │   ├── CMakeLists.txt
│   │   ├── main.c
│   │   ├── event_logger.c
│   │   ├── alert_manager.c
│   │   └── uart_handler.c
│   └── main/
│       ├── config.h
│       ├── event_logger.h
│       ├── alert_manager.h
│       └── uart_handler.h
│
└── docs/
    ├── wiring_diagram.png
    ├── timing_diagram.png
    └── protocol_spec.md
```

### D2: Dokumentasi Teknis (25%)

| Item | Deskripsi | Bobot |
|------|-----------|-------|
| System Architecture | Diagram arsitektur lengkap | 5% |
| Interrupt Timing Analysis | Diagram timing dan latency measurement | 10% |
| Protocol Documentation | Spesifikasi protokol komunikasi | 5% |
| API Documentation | Dokumentasi fungsi dan modul | 5% |

**Timing Diagram yang Diharapkan:**
```
PIR Event Detection and Response Timeline:

                    ISR           ISR          Message        ESP32
PIR Trigger     Entry         Exit          Sent         Received
    │              │             │             │             │
    ▼              ▼             ▼             ▼             ▼
────┼──────────────┼─────────────┼─────────────┼─────────────┼────
    │◄────t1──────►│◄────t2─────►│◄────t3─────►│◄────t4─────►│
    │              │             │             │             │
    │◄──────────── Total Latency (t1+t2+t3+t4) ─────────────►│

Target: Total Latency < 5ms for normal events
        Total Latency < 1ms for panic button
```

### D3: Laporan Project (20%)

| Section | Konten | Bobot |
|---------|--------|-------|
| Abstrak | Ringkasan project 200 kata | 2% |
| Pendahuluan | Latar belakang dan tujuan | 3% |
| Metodologi | Design dan implementasi | 5% |
| Hasil | Data pengukuran dan analisis | 5% |
| Kesimpulan | Summary dan saran | 3% |
| Referensi | Min 5 referensi valid | 2% |

### D4: Video Demonstrasi (15%)

| Aspek | Deskripsi | Durasi |
|-------|-----------|--------|
| System Overview | Penjelasan arsitektur | 1-2 menit |
| Hardware Demo | Demonstrasi rangkaian | 2-3 menit |
| Interrupt Demo | Demonstrasi prioritas interrupt | 2-3 menit |
| Timer Demo | Demonstrasi timing accuracy | 1-2 menit |
| Integration | Demonstrasi sistem lengkap | 2-3 menit |

**Total durasi video: 8-13 menit**

---

## 🧪 Kriteria Pengujian

### Test Case 1: Interrupt Priority (25%)

| Test | Prosedur | Expected Result |
|------|----------|-----------------|
| TC1.1 | Trigger PIR saat idle | LED + Log dalam < 5ms |
| TC1.2 | Trigger door saat PIR active | PIR selesai dulu, lalu door |
| TC1.3 | Panic saat PIR active | Panic interrupt PIR (preemptive) |
| TC1.4 | Semua sensor simultaneous | Urutan sesuai prioritas |

### Test Case 2: Timer Accuracy (25%)

| Test | Prosedur | Expected Result |
|------|----------|-----------------|
| TC2.1 | Measure timestamp accuracy | Error < 1% |
| TC2.2 | Debounce timing | 50ms ± 2ms |
| TC2.3 | Heartbeat interval | 1000ms ± 10ms |
| TC2.4 | Response time measurement | Consistent reading |

### Test Case 3: Communication (25%)

| Test | Prosedur | Expected Result |
|------|----------|-----------------|
| TC3.1 | Single event transmission | Received correctly |
| TC3.2 | Burst events (5 rapid) | All received in order |
| TC3.3 | Checksum validation | Invalid packets rejected |
| TC3.4 | Acknowledgment system | ACK received for each event |

### Test Case 4: Integration (25%)

| Test | Prosedur | Expected Result |
|------|----------|-----------------|
| TC4.1 | Complete scenario: normal | System operates correctly |
| TC4.2 | Complete scenario: intrusion | Alert escalation correct |
| TC4.3 | Complete scenario: panic | Immediate emergency response |
| TC4.4 | 1 hour stability test | No crashes, memory stable |

---

## ⏰ Timeline Pengerjaan

### Minggu 1

| Hari | Kegiatan |
|------|----------|
| 1-2 | Desain arsitektur dan protocol specification |
| 3-4 | Implementasi STM32 interrupt system |
| 5-6 | Implementasi STM32 timer system |
| 7 | Testing STM32 standalone |

### Minggu 2

| Hari | Kegiatan |
|------|----------|
| 1-2 | Implementasi ESP32 event logger |
| 3-4 | Implementasi komunikasi UART |
| 5 | Integrasi dan debugging |
| 6 | Testing dan dokumentasi |
| 7 | Video recording dan finalisasi |

---

## 📚 Referensi

### Dokumentasi Resmi
1. STM32F103 Reference Manual - Chapter 10: NVIC, Chapter 13-15: Timers
2. ESP32 Technical Reference Manual - Chapter 5: Interrupts, Chapter 17: Timers
3. ARM Cortex-M3 Technical Reference - Exception Handling

### Application Notes
1. AN4228 - STM32 EXTI Application Note
2. AN4013 - Timer cookbook for STM32 microcontrollers

### Tutorial
1. [STM32 Interrupt Tutorial](https://deepbluembedded.com/stm32-external-interrupt-example-code/)
2. [ESP32 Timer Tutorial](https://randomnerdtutorials.com/esp32-timer-wake-up-deep-sleep/)

---

## ⚠️ Catatan Penting

1. **Response Time Priority:** 
   - Panic button HARUS memiliki response time < 1ms
   - Sistem harus dapat meng-interrupt handler yang sedang berjalan

2. **Interrupt Safety:**
   - Semua ISR harus singkat (< 100μs)
   - Gunakan flag-based communication ke main loop
   - Shared variable HARUS menggunakan volatile

3. **Testing:**
   - Lakukan stress test dengan rapid button press
   - Verifikasi tidak ada race condition
   - Ukur actual latency dengan oscilloscope jika tersedia

4. **Code Quality:**
   - Modular design (separate files untuk setiap modul)
   - Meaningful comments
   - Consistent naming convention
